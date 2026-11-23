"""Change ONLY inspection-camera metadata for a bounded continuation pass."""
import hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];P=ROOT/'unreal/EchoWard';OUT=ROOT/'artifacts/unreal_hospital_v16'
source=P/'Content/CampaignData/hospital_world.json';stage=P/'Saved/PackagedWindows/Windows/EchoWard/Content/CampaignData/hospital_world.json';shots=P/'Saved/PackageTests/WorldVisualV16/Saved/Prototype/BookWorldCaptures'
d=json.loads(source.read_text(encoding='utf8'));before=dict(d);missing=[c for c in d['furnishing_captures'] if not (shots/('world_'+c['id']+'.png')).exists()]
d['furnishing_captures']=missing
assert {k:v for k,v in before.items() if k!='furnishing_captures'}=={k:v for k,v in d.items() if k!='furnishing_captures'}
stage.write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
r=dict(canonical_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),capture_sha256=hashlib.sha256(stage.read_bytes()).hexdigest(),only_camera_list_changed=True,remaining_furnishing_views=len(missing),full_count=len(before['locations'])+len(before['furnishing_captures']))
(OUT/'capture_resume_manifest.json').write_text(json.dumps(r,indent=2),encoding='utf8');print(json.dumps(r))
