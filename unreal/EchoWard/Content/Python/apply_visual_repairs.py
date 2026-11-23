from pathlib import Path
import runpy
import unreal as u
root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
runpy.run_path(str(root/'Content/Python/import_stalker.py'))
runpy.run_path(str(root/'Content/Python/repair_set_dressing.py'),init_globals={'UPDATE_CURRENT_LEVEL':True})
