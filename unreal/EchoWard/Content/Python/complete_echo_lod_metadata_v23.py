"""Recover import LOD evidence, querying any overwritten batch entries."""
import json,unreal as u
from pathlib import Path
P=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));path=P/'Content/CampaignData/echo_custom_runtime.json';data=json.loads(path.read_text(encoding='utf8'));raw=json.loads((P/'RawAssets/EchoRealV23/manifest.json').read_text(encoding='utf8'));counts={};S=u.get_editor_subsystem(u.StaticMeshEditorSubsystem)
try:
 for file in (P/'Saved/Prototype').glob('real_import_batch_*.json'):
  for a in json.loads(file.read_text(encoding='utf8'))['details']:counts[a['id']]=a['lods']
 for a in data['meshes']:
  if a['id']<700:continue
  if a['id'] not in counts:
   m=u.load_asset(a['asset']);assert m;counts[a['id']]=S.get_lod_count(m);m=None;u.SystemLibrary.collect_garbage()
  a['lods']=counts[a['id']]
 for a in raw['assets']:
  assert counts[a['id']]>=(4 if a['triangles']>1500 else 1),(a['id'],counts[a['id']])
 path.write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf8');(P/'Saved/Prototype/real_lods_v23.json').write_text(json.dumps(dict(success=True,models=len(raw['assets']),counts=counts)),encoding='utf8');u.log('REAL_LOD_METADATA_COMPLETE')
finally:u.SystemLibrary.quit_editor()
