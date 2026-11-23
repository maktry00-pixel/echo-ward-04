"""REJECTED EXPERIMENT: requires -WardTestNanite in the full editor command line.

The two-prop Nanite conversion slightly regressed the measured small room.
Not called by Setup. Source meshes were restored after the experiment.
Retained only to reproduce the measurement; save native mesh backups first.
"""
from pathlib import Path
import json
import unreal as u

if "WardTestNanite" not in u.SystemLibrary.get_command_line():
    raise RuntimeError("Rejected experiment: use -WardTestNanite only with saved mesh backups")

project = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
subsystem = u.get_editor_subsystem(u.StaticMeshEditorSubsystem)
manifest = json.loads((project / "RawAssets/manifest.json").read_text(encoding="utf-8"))
rows = []
for record in manifest["meshes"]:
    # Small furniture gains little from Nanite; the held pistol is kept conventional.
    if record["triangles"] < 10000 or record["name"] == "SM_Pistol":
        continue
    mesh = u.load_asset("/Game/Prototype/Meshes/" + record["name"])
    if not mesh:
        raise RuntimeError("Missing mesh " + record["name"])
    settings = subsystem.get_nanite_settings(mesh)
    settings.set_editor_property("enabled", True)
    settings.set_editor_property("keep_percent_triangles", 1.0)
    settings.set_editor_property("trim_relative_error", 0.0)
    subsystem.set_nanite_settings(mesh, settings, True)
    if not u.EditorAssetLibrary.save_loaded_asset(mesh):
        raise RuntimeError("Could not save " + record["name"])
    saved = subsystem.get_nanite_settings(mesh)
    if not saved.get_editor_property("enabled"):
        raise RuntimeError("Nanite did not stay enabled")
    rows.append(dict(mesh=record["name"], source_triangles=record["triangles"],
                     nanite=True, keep_percent_triangles=1.0, trim_relative_error=0.0))
if len(rows) != 2:
    raise RuntimeError("Expected the bed and lamp; review manifest before applying")
report = dict(success=True, meshes=rows, textures_changed=False, lights_changed=False,
              collision_changed=False, map_rebuilt=False)
path = project / "Saved/Prototype/optimization_assets.json"
path.write_text(json.dumps(report, indent=2), encoding="utf-8")
u.log("WARD_OPTIMIZATION_ASSETS_OK " + str(path))
