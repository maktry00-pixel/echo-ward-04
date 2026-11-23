"""Compare imported bounds/material slots with exported OBJ, not just manifest counts."""
from pathlib import Path
import json
import unreal as u
root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
raw=root/'RawAssets/Campaign'
data=json.loads((raw/'campaign.json').read_text(encoding='utf-8'))
issues=[]
for row in data['meshes']:
    mesh=u.load_asset('/Game/Campaign/Meshes/SM_'+row['id'])
    if not mesh:
        issues.append({'mesh':row['id'],'error':'missing'});continue
    lo=[float('inf')]*3;hi=[float('-inf')]*3
    for line in (raw/row['file']).read_text().splitlines():
        if line.startswith('v '):
            p=list(map(float,line.split()[1:4]))
            lo=[min(a,b) for a,b in zip(lo,p)];hi=[max(a,b) for a,b in zip(hi,p)]
    bounds=mesh.get_bounds();center=bounds.origin;extent=bounds.box_extent
    actual=[center.x,center.y,center.z,extent.x,extent.y,extent.z]
    expected=[(a+b)/2 for a,b in zip(lo,hi)]+[(b-a)/2 for a,b in zip(lo,hi)]
    error=max(abs(a-b) for a,b in zip(actual,expected))
    if error>.1:issues.append({'mesh':row['id'],'error':'bounds mismatch cm','max_error':error,'actual':actual,'expected':expected})
    slots=len(mesh.get_editor_property('static_materials'))
    if slots!=row.get('surfaces',1):issues.append({'mesh':row['id'],'error':'material slots','actual':slots,'expected':row.get('surfaces',1)})
materials_ok=0
for row in data['materials']:
    mat=u.load_asset('/Game/Campaign/Materials/MI_'+row['id'])
    if not mat or not u.MaterialEditingLibrary.has_material_usage(mat,u.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES):issues.append({'material':row['id'],'error':'missing HISM shader'})
    else:materials_ok+=1
report={'success':not issues,'meshes_checked':len(data['meshes']),'materials_checked':materials_ok,'issues':issues}
(root/'Saved/Prototype/campaign_asset_audit.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
u.log('CAMPAIGN_ASSET_AUDIT '+json.dumps(report))
