"""Offline asset checks; native status comes only from separate runtime evidence."""
from __future__ import annotations
import ast
import hashlib
import json
import math
from pathlib import Path
import re
import struct
import unittest
import wave
import numpy as np
from PIL import Image
from prepare_assets import Gltf, PROJECT, RAW, ROOT, AXIS

DATA = json.loads((RAW / "manifest.json").read_text(encoding="utf-8"))


def obj(path):
    pos, normal, uv, faces = [], [], [], []
    for line in path.read_text(encoding="utf-8").splitlines():
        split = line.split()
        if not split:
            continue
        if split[0] == "v": pos.append([float(x) for x in split[1:]])
        elif split[0] == "vn": normal.append([float(x) for x in split[1:]])
        elif split[0] == "vt": uv.append([float(x) for x in split[1:]])
        elif split[0] == "f": faces.append([[int(x)-1 for x in vertex.split("/")] for vertex in split[1:]])
    return np.array(pos), np.array(normal), np.array(uv), np.array(faces)


class Preparation(unittest.TestCase):
    def test_manifest_integrity_and_portability(self):
        for record in DATA["files"]:
            with self.subTest(file=record["path"]):
                path = RAW / record["path"]
                self.assertTrue(path.resolve().is_relative_to(RAW.resolve()))
                self.assertEqual(path.stat().st_size, record["size"])
                self.assertEqual(hashlib.sha256(path.read_bytes()).hexdigest(), record["sha256"])

    def test_triangle_geometry_uv_normal_and_indices(self):
        for record in DATA["meshes"] + DATA["monster"]["parts"]:
            with self.subTest(mesh=record["name"]):
                pos, normal, uv, faces = obj(RAW / record["source"])
                contents = (RAW / record["source"]).read_text(encoding="utf-8")
                library = re.search(r"^mtllib (.+)$", contents, re.MULTILINE)
                self.assertIsNotNone(library)
                mtl = (RAW / record["source"]).parent / library[1]
                definitions = set(re.findall(r"^newmtl (.+)$", mtl.read_text(), re.MULTILINE))
                self.assertTrue(set(re.findall(r"^usemtl (.+)$", contents, re.MULTILINE)).issubset(definitions))
                self.assertEqual(len(pos), record["vertices"])
                self.assertEqual(len(faces), record["triangles"])
                self.assertEqual(len(pos), len(normal))
                self.assertEqual(len(pos), len(uv))
                self.assertTrue(all(np.isfinite(a).all() for a in (pos, normal, uv)))
                self.assertEqual(faces.shape[1:], (3, 3))
                self.assertGreaterEqual(int(faces.min()), 0)
                self.assertLess(int(faces.max()), len(pos))
                self.assertTrue(np.allclose(np.linalg.norm(normal, axis=1), 1, atol=.002))
                points = pos[faces[:, :, 0]]
                cross = np.cross(points[:, 1]-points[:, 0], points[:, 2]-points[:, 0])
                average = normal[faces[:, :, 2]].sum(1)
                meaningful = np.linalg.norm(cross, axis=1) > 1e-5
                orientation = np.einsum("ij,ij->i", cross, average)
                # A very small number of sculpted degenerate/inverted source faces may exist.
                self.assertGreater(float(np.mean(orientation[meaningful] >= -1e-4)), .995)

    def test_metric_dimensions_and_common_drawer_pivot(self):
        meshes = {m["name"]: m for m in DATA["meshes"]}
        for m in meshes.values():
            if m["name"] == "SM_Drawer": continue
            lo, hi = np.array(m["minimum"]), np.array(m["maximum"])
            self.assertGreater(float((hi-lo).min()), 1)
            self.assertAlmostEqual(lo[2], 0, places=4)
            if m["name"] != "SM_Nightstand":
                self.assertAlmostEqual((lo+hi)[0], 0, places=4)
                self.assertAlmostEqual((lo+hi)[1], 0, places=4)
        self.assertAlmostEqual(meshes["SM_Bed"]["maximum"][0]-meshes["SM_Bed"]["minimum"][0], 215, places=3)
        self.assertAlmostEqual(meshes["SM_Nightstand"]["maximum"][2], 76, places=3)
        body, drawer = meshes["SM_Nightstand"], meshes["SM_Drawer"]
        self.assertEqual(body["pivot"], drawer["pivot"])
        self.assertEqual(body["scale"], drawer["scale"])
        combined_lo = np.minimum(body["minimum"], drawer["minimum"])
        combined_hi = np.maximum(body["maximum"], drawer["maximum"])
        np.testing.assert_allclose((combined_lo+combined_hi)[:2], 0, atol=1e-4)
        self.assertGreater(drawer["minimum"][2], 35)
        self.assertLess(drawer["maximum"][2], body["maximum"][2])
        self.assertLess(drawer["maximum"][1]-drawer["minimum"][1], body["maximum"][1]-body["minimum"][1])

    def test_accessor_offsets_stride_and_normalization(self):
        model = Gltf.__new__(Gltf)
        model.buffers = [b"HEAD" + struct.pack("<fffIfffI", 1., 2., 3., 99, 4., 5., 6., 88), b"x\x00\x7f\xff"]
        model.doc = {"bufferViews": [{"buffer": 0, "byteOffset": 4, "byteStride": 16}, {"buffer": 1, "byteOffset": 1}],
                     "accessors": [{"bufferView": 0, "componentType": 5126, "count": 2, "type": "VEC3"},
                                   {"bufferView": 0, "byteOffset": 12, "componentType": 5125, "count": 2, "type": "SCALAR"},
                                   {"bufferView": 1, "componentType": 5121, "count": 3, "type": "SCALAR", "normalized": True}]}
        np.testing.assert_array_equal(model.accessor(0), [[1, 2, 3], [4, 5, 6]])
        np.testing.assert_array_equal(model.accessor(1), [[99], [88]])
        np.testing.assert_allclose(model.accessor(2).ravel(), [0, 127/255, 1])

    def test_trs_and_column_major_matrix(self):
        m = Gltf.transform(dict(translation=[10, 20, 30], scale=[2, 3, 4], rotation=[0, 0, math.sqrt(.5), math.sqrt(.5)]))
        np.testing.assert_allclose((m @ [1, 0, 0, 1]), [10, 22, 30, 1], atol=1e-6)
        np.testing.assert_allclose(Gltf.transform(dict(matrix=m.flatten(order="F").tolist())), m)
        np.testing.assert_array_equal(AXIS @ [1, 2, 3] * 100, [-300, 100, 200])

    def test_articulated_creature_positions_and_materials(self):
        creature = DATA["monster"]
        self.assertEqual(len(creature["leg_pivots"]), 6)
        self.assertEqual(len(creature["parts"]), 8)
        p = np.array(creature["leg_pivots"])
        np.testing.assert_allclose(p[:, 2], 72)
        np.testing.assert_allclose(p[::2, 1], -p[1::2, 1])
        # Reassembled dimensions in UE cm should equal the original ~1.8m-wide crawler.
        points = []
        for record in creature["parts"]:
            v = obj(RAW / record["source"])[0]
            if record["name"] == "SM_Head": v += creature["head_pivot"]
            elif record["name"].startswith("SM_Leg"): v += creature["leg_pivots"][int(record["name"][-1])]
            points.append(v)
        extent = np.ptp(np.concatenate(points), axis=0)
        self.assertGreater(extent[1], 150)
        self.assertLess(extent[1], 210)
        self.assertGreater(extent[2], 90)
        self.assertLess(extent[2], 150)
        for m in creature["materials"]:
            self.assertTrue(all(0 <= c <= 1 for c in m["color"]))
        header = (PROJECT / "Source/EchoWard/CreatureLayout.h").read_text()
        self.assertIn("LegPivots[6]", header)

    def test_image_payloads_and_4k_sources(self):
        for record in DATA["files"]:
            p = RAW / record["path"]
            if p.suffix.lower() not in (".png", ".jpg"):
                continue
            with Image.open(p) as image:
                self.assertGreaterEqual(min(image.size), 512)
                if "_4k" in p.name: self.assertEqual(image.width, 4096)
                image.verify()

    def test_audio_payloads(self):
        for record in DATA["audio"]:
            with wave.open(str(RAW / record["source"]), "rb") as w:
                self.assertEqual((w.getnchannels(), w.getsampwidth(), w.getframerate()), (1, 2, 44100))
                self.assertAlmostEqual(w.getnframes()/w.getframerate(), record["duration"], places=3)
                samples = np.frombuffer(w.readframes(w.getnframes()), dtype="<i2")
                self.assertGreater(int(np.abs(samples).max()), 100)
                self.assertLess(int(np.abs(samples).max()), 32767)
                self.assertEqual(int(samples[0]), 0)

    def test_native_character_sources_and_licenses(self):
        for name in ("SK_Survivor", "A_Idle", "A_Walk", "A_Run"):
            contents = (RAW / f"Character/{name}.fbx").read_bytes()
            self.assertTrue(contents.startswith(b"Kaydara FBX Binary"))
            self.assertIn(b"Bip01", contents)
        self.assertIn("Permission is hereby granted", (RAW / "Licenses/Rocketbox-MIT.md").read_text())
        for file in (RAW / "Licenses").glob("*.json"):
            self.assertTrue(all(m["license"] == "CC0-1.0" for m in json.loads(file.read_text(encoding="utf-8"))))

    def test_scripts_parse_and_config_dependencies(self):
        for path in [PROJECT / "Content/Python/build_prototype.py", PROJECT / "Tools/prepare_assets.py"]:
            ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
        project = json.loads((PROJECT / "EchoWard.uproject").read_text())
        self.assertEqual(project["Modules"][0]["Name"], "EchoWard")
        self.assertEqual(project["TargetPlatforms"], ["Windows"])
        cfg = (PROJECT / "Config/DefaultGame.ini").read_text()
        self.assertIn('DirectoriesToAlwaysCook=(Path="/Game/Prototype")', cfg)
        cpp = (PROJECT / "Source/EchoWard/WardPrototype.cpp").read_text(encoding="utf-8")
        expected = {m["name"] for m in DATA["meshes"] + DATA["monster"]["parts"]}
        for asset in re.findall(r"/Game/Prototype/(?:Meshes|Monster)/(SM_\w+)\.", cpp):
            self.assertIn(asset, expected)
        # Fewer feature claims than the Godot game: no implication that saves/locales are migrated.
        self.assertTrue((ROOT / "project.godot").exists())


if __name__ == "__main__":
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(Preparation)
    run = unittest.TextTestRunner(verbosity=2).run(suite)
    doctor_path = PROJECT / "Saved/Prototype/doctor.json"
    doctor = json.loads(doctor_path.read_text(encoding="utf-8-sig")) if doctor_path.exists() else {}
    blockers = []
    if not doctor.get("engine_found", False): blockers.append("Unreal Engine not installed")
    elif not doctor.get("engine_ready", False): blockers.append("Unreal Engine installation not completed")
    if not doctor.get("cpp_tools_found", False): blockers.append("Visual Studio C++ tools not installed")
    native_log = PROJECT / 'Saved/Prototype/assessment_build.log'
    import_path = PROJECT / 'Saved/Prototype/import_result.json'
    smoke_path = PROJECT / 'Saved/Prototype/smoke_result.json'
    imported = json.loads(import_path.read_text(encoding='utf-8-sig')) if import_path.exists() else {}
    smoke = json.loads(smoke_path.read_text(encoding='utf-8-sig')) if smoke_path.exists() else {}
    compiled = (PROJECT/'Binaries/Win64/UnrealEditor-EchoWard.dll').exists() and native_log.exists() and 'Result: Succeeded' in native_log.read_text(encoding='utf-8-sig')
    played = smoke.get('won',False) and bool(smoke.get('checks')) and all(row.get('passed',False) for row in smoke['checks'])
    measured = all((PROJECT/f'Saved/Prototype/benchmark_{profile}.json').exists() for profile in ['high','epic'])
    result = dict(source_verification_success=run.wasSuccessful(), test_cases=run.testsRun,
                  failures=len(run.failures), errors=len(run.errors), source_files=len(DATA["files"]),
                  static_mesh_parts=len(DATA["meshes"])+len(DATA["monster"]["parts"]),
                  raw_bytes=sum(x["size"] for x in DATA["files"]), engine_found=doctor.get("engine_found", False),
                  engine_ready=doctor.get("engine_ready", False),
                  cpp_tools_found=doctor.get("cpp_tools_found", False), native_compile=compiled,
                  native_import=imported.get('success',False) and (PROJECT/'Content/Prototype/Maps/Ward04.umap').exists(),
                  gameplay_playtested=played, fps_measured=measured,
                  native_evidence_note='Native room route, not human playtest or migrated full campaign; see ENGINE_COMPARISON.md.',
                  blockers=blockers,
                  tests=[dict(test=str(t), traceback=err) for t, err in run.failures+run.errors])
    out = ROOT / "artifacts/unreal_prototype_readiness.json"
    out.write_text(json.dumps(result, ensure_ascii=False, indent=2)+"\n", encoding="utf-8")
    raise SystemExit(0 if run.wasSuccessful() else 1)
