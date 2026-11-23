"""Prepare licensed local source assets. No downloads, no Unreal installation required.

OBJ coordinates are baked in centimetres: Godot/glTF (-Z, X, Y) -> UE (X,Y,Z).
Node hierarchy, normals, interleaved accessors and common drawer pivot are preserved.
Run from any directory; only writes inside this separate Unreal project.
"""
from __future__ import annotations
import base64
import hashlib
import json
from pathlib import Path
import shutil
import wave
import numpy as np
from PIL import Image

PROJECT = Path(__file__).resolve().parents[1]
ROOT = PROJECT.parents[1]
RAW = PROJECT / "RawAssets"
DTYPES = {5120: "i1", 5121: "u1", 5122: "<i2", 5123: "<u2", 5125: "<u4", 5126: "<f4"}
COUNTS = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4, "MAT4": 16}
AXIS = np.array([[0., 0., -1.], [1., 0., 0.], [0., 1., 0.]])


def write_json(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


class Gltf:
    def __init__(self, path):
        self.path = Path(path)
        self.doc = json.loads(self.path.read_text(encoding="utf-8"))
        self.buffers = []
        for buf in self.doc["buffers"]:
            uri = buf["uri"]
            self.buffers.append(base64.b64decode(uri.split(",", 1)[1]) if uri.startswith("data:") else (self.path.parent / uri).read_bytes())

    def accessor(self, index):
        a = self.doc["accessors"][index]
        if "sparse" in a:
            raise ValueError("Sparse accessor not supported; do not silently corrupt geometry")
        v = self.doc["bufferViews"][a["bufferView"]]
        dt = np.dtype(DTYPES[a["componentType"]])
        n = COUNTS[a["type"]]
        result = np.ndarray((a["count"], n), dtype=dt, buffer=self.buffers[v["buffer"]],
                            offset=v.get("byteOffset", 0) + a.get("byteOffset", 0),
                            strides=(v.get("byteStride", n * dt.itemsize), dt.itemsize)).copy()
        if a.get("normalized") and dt.kind in "iu":
            result = result.astype(float) / np.iinfo(dt).max
            if dt.kind == "i":
                result = np.maximum(result, -1)
        return result

    @staticmethod
    def transform(node):
        if "matrix" in node:
            return np.array(node["matrix"]).reshape(4, 4, order="F")
        x, y, z, w = node.get("rotation", [0, 0, 0, 1])
        r = np.array([[1-2*(y*y+z*z), 2*(x*y-z*w), 2*(x*z+y*w)],
                      [2*(x*y+z*w), 1-2*(x*x+z*z), 2*(y*z-x*w)],
                      [2*(x*z-y*w), 2*(y*z+x*w), 1-2*(x*x+y*y)]])
        m = np.eye(4)
        m[:3, :3] = r @ np.diag(node.get("scale", [1, 1, 1]))
        m[:3, 3] = node.get("translation", [0, 0, 0])
        return m

    def primitives(self):
        out = []
        def visit(i, parent):
            node = self.doc["nodes"][i]
            m = parent @ self.transform(node)
            if "mesh" in node:
                for p in self.doc["meshes"][node["mesh"]]["primitives"]:
                    if p.get("mode", 4) != 4:
                        raise ValueError("Only triangle primitives are supported")
                    pos = self.accessor(p["attributes"]["POSITION"])
                    pos = (pos @ m[:3, :3].T + m[:3, 3]) @ AXIS.T * 100
                    normal = self.accessor(p["attributes"]["NORMAL"]) @ np.linalg.inv(m[:3, :3]) @ AXIS.T
                    normal /= np.maximum(np.linalg.norm(normal, axis=1, keepdims=True), 1e-12)
                    uv = self.accessor(p["attributes"]["TEXCOORD_0"])
                    uv[:, 1] = 1 - uv[:, 1]
                    idx = self.accessor(p["indices"]).reshape(-1, 3).astype(int)
                    idx = idx[:, [0, 2, 1]]  # Reflection from glTF right-handed basis.
                    out.append(dict(node=node.get("name", str(i)), pos=pos, normal=normal, uv=uv, idx=idx,
                                    material=p.get("material", 0)))
            for child in node.get("children", []):
                visit(child, m)
        for i in self.doc["scenes"][self.doc.get("scene", 0)]["nodes"]:
            visit(i, np.eye(4))
        return out


def write_obj(path, parts, offset, scale):
    path.parent.mkdir(parents=True, exist_ok=True)
    count = triangles = 0
    all_pos = []
    with path.open("w", encoding="utf-8", newline="\n") as f:
        f.write("# Echo Ward; baked UE centimetres, common asset pivot\n")
        f.write(f"mtllib {path.stem}.mtl\n")
        for p in parts:
            pos = (p["pos"] - offset) * scale
            all_pos.append(pos)
            f.write(f'o {p["node"]}\nusemtl material_{p["material"]}\n')
            for v in pos:
                f.write("v %.7f %.7f %.7f\n" % tuple(v))
            for v in p["uv"]:
                f.write("vt %.7f %.7f\n" % tuple(v))
            for v in p["normal"]:
                f.write("vn %.7f %.7f %.7f\n" % tuple(v))
            for face in p["idx"] + count + 1:
                f.write("f " + " ".join(f"{i}/{i}/{i}" for i in face) + "\n")
            count += len(pos)
            triangles += len(p["idx"])
    verts = np.concatenate(all_pos)
    path.with_suffix(".mtl").write_text("\n".join(f"newmtl material_{i}\nKd 1 1 1\nNs 20\nd 1\n" for i in sorted({p['material'] for p in parts})), encoding="utf-8")
    return dict(vertices=count, triangles=triangles, minimum=verts.min(0).tolist(), maximum=verts.max(0).tolist())


def copy(src, dst):
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst)
    return dst.relative_to(RAW).as_posix()


def prop(asset, stem, target, axis=2, split=False, choose=None):
    folder = ROOT / "assets/models/polyhaven" / asset
    gltf_path = next(iter(sorted(folder.glob("*_2k.gltf"))), None) or next(folder.glob("*_1k.gltf"))
    model = Gltf(gltf_path)
    parts = model.primitives()
    if choose:
        parts = [p for p in parts if choose(p["node"])]
    points = np.concatenate([p["pos"] for p in parts])
    lo, hi = points.min(0), points.max(0)
    offset = np.array([(lo[0]+hi[0])/2, (lo[1]+hi[1])/2, lo[2]])
    scale = target / (hi[axis]-lo[axis])
    textures = {}
    for image in model.doc.get("images", []):
        uri = image["uri"]
        key = "normal" if "nor_gl" in uri else ("arm" if "_arm_" in uri else "color")
        textures[key] = copy(folder / uri, RAW / "Textures" / asset / Path(uri).name)
    results = []
    groups = [(stem, parts)] if not split else [(stem, [p for p in parts if "drawer" not in p["node"]]),
                                                ("SM_Drawer", [p for p in parts if "drawer" in p["node"]])]
    for name, group in groups:
        stats = write_obj(RAW / "Meshes" / f"{name}.obj", group, offset, scale)
        results.append(dict(name=name, source=f"Meshes/{name}.obj", asset=asset, textures=textures,
                            target_axis=axis, target_cm=target, scale=scale, pivot=offset.tolist(), **stats))
    return results


def audio():
    """Small original procedural placeholders for the comparison room."""
    sr = 44100
    rng = np.random.default_rng(404)
    records = []
    durations = {"Shot": .32, "Step": .22, "Hurt": .55, "Impact": .18, "Reload": 1.6,
                 "Drawer": .9, "Door": 1.2, "Power": 1.5, "Growl": 1.9, "Ambience": 12.}
    for name, duration in durations.items():
        t = np.arange(round(sr * duration)) / sr
        noise = rng.normal(0, 1, len(t))
        if name == "Ambience":
            signal = .18*np.sin(2*np.pi*47*t) + .07*np.sin(2*np.pi*71*t) + .04*noise
            # Integer frequencies over 12 s plus a short edge ramp: seamless enough for a placeholder.
            gain = .24
        elif name == "Growl":
            signal = (np.sin(2*np.pi*(67*t+2*np.sin(2*np.pi*4*t))) + .3*noise) * np.sin(np.pi*t/duration)**2
            gain = .48
        elif name in ("Drawer", "Door"):
            signal = (.55*np.sin(2*np.pi*(240*t+18*np.sin(2*np.pi*1.1*t))) + .4*noise) * np.sin(np.pi*t/duration)**2
            gain = .3
        elif name == "Power":
            signal = (.6*np.sin(2*np.pi*(70*t+85*t*t)) + .12*noise) * np.sin(np.pi*t/duration)
            gain = .4
        elif name == "Reload":
            signal = noise * sum(np.exp(-np.maximum(t-a, 0)*80)*(t>=a) for a in (.04, .65, 1.25))
            gain = .45
        elif name == "Hurt":
            signal = (np.sin(2*np.pi*(140*t-50*t*t)) + .5*noise) * np.exp(-t*9)
            gain = .4
        else:
            rate = 38 if name == "Step" else 28
            signal = (noise + .9*np.sin(2*np.pi*(85 if name == "Shot" else 190)*t)) * np.exp(-t*rate)
            gain = .65 if name == "Shot" else .35
        fade = np.minimum(1, t/.002) * np.minimum(1, (duration-t)/.008)
        signal = signal * fade
        pcm = np.clip(signal / max(1, np.max(np.abs(signal))) * gain * 32767, -32767, 32767).astype("<i2")
        dest = RAW / "Audio" / f"{name}.wav"
        dest.parent.mkdir(parents=True, exist_ok=True)
        with wave.open(str(dest), "wb") as out:
            out.setnchannels(1); out.setsampwidth(2); out.setframerate(sr); out.writeframes(pcm.tobytes())
        records.append(dict(name=name, source=dest.relative_to(RAW).as_posix(), loop=name=="Ambience", duration=duration))
    return records


def main():
    RAW.mkdir(parents=True, exist_ok=True)
    meshes = []
    for args in [("old_bed_frame", "SM_Bed", 215, 0),
                 ("painted_wooden_cabinet_02", "SM_Cabinet", 120, 2),
                 ("SchoolChair_01", "SM_Chair", 88, 2),
                 ("desk_lamp_arm_01", "SM_Lamp", 66, 2),
                 ("steel_frame_shelves_01", "SM_Shelf", 185, 2)]:
        meshes.extend(prop(*args))
    meshes.extend(prop("painted_wooden_nightstand", "SM_Nightstand", 76, split=True))
    meshes.extend(prop("service_pistol", "SM_Pistol", 25, axis=1,
                       choose=lambda n: not n.endswith("_b") and "magazine_empty" not in n and "bullet" not in n))
    surfaces = []
    for asset in ("terrazzo_tiles", "painted_plaster_wall", "blue_plaster_weathered"):
        textures = {kind: copy(ROOT / "assets/textures/polyhaven" / asset / f"{file}_4k.jpg",
                              RAW / "Textures" / asset / f"{file}_4k.jpg")
                    for kind, file in [("color", "color"), ("normal", "normal"), ("roughness", "roughness")]}
        surfaces.append(dict(name=asset, textures=textures))
    rocket = ROOT / "assets/models/rocketbox"
    copy(rocket / "Export/Male_Adult_04.fbx", RAW / "Character/SK_Survivor.fbx")
    for src, dst in [("m_idle_breathe_01.max.fbx", "A_Idle"), ("m_walk_neutral.max.fbx", "A_Walk"), ("m_run_neutral.max.fbx", "A_Run")]:
        # Exact motion filenames are checked; no substitution with a different skeleton.
        copy(rocket / "motion" / src, RAW / "Character" / f"{dst}.fbx")
    for src in (rocket / "textures").glob("*.tga"):
        dst = RAW / "Character" / (src.stem + ".png")
        Image.open(src).save(dst)
    copy(rocket / "LICENSE.md", RAW / "Licenses/Rocketbox-MIT.md")
    for src in (ROOT / "assets/models/polyhaven/SOURCE_MANIFEST.json",
                ROOT / "assets/textures/polyhaven/SOURCE_MANIFEST.json",
                ROOT / "assets/textures/polyhaven/HD_SOURCE_MANIFEST.json"):
        copy(src, RAW / "Licenses" / (src.parent.parent.name + "_" + src.name))
    creature_path = RAW / "Monster/creature.json"
    if not creature_path.exists():
        raise RuntimeError("Export original monster first: Godot --headless --path <game> --script tools/export_unreal_creature.gd")
    manifest = dict(version=1, units="centimetres", axes="X=-sourceZ,Y=sourceX,Z=sourceY", meshes=meshes,
                    surfaces=surfaces, audio=audio(), monster=json.loads(creature_path.read_text(encoding="utf-8")),
                    character=dict(mesh="Character/SK_Survivor.fbx", animations=["A_Idle", "A_Walk", "A_Run"],
                                   license="MIT", source="https://github.com/microsoft/Microsoft-Rocketbox"))
    files = []
    for path in sorted(RAW.rglob("*")):
        if path.is_file() and path.name != "manifest.json":
            files.append(dict(path=path.relative_to(RAW).as_posix(), size=path.stat().st_size,
                              sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
    manifest["files"] = files
    write_json(RAW / "manifest.json", manifest)
    print(json.dumps(dict(files=len(files), megabytes=round(sum(x["size"] for x in files)/1e6, 1),
                          meshes=len(meshes), manifest=str(RAW / "manifest.json"))))


if __name__ == "__main__":
    main()
