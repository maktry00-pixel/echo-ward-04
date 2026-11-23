"""Remove authored decoration that overlaps real doors and obsolete oval stains."""
from pathlib import Path
import json
import shutil
import copy
import math
import unreal as u
ROOT=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
raw=ROOT/'RawAssets/Campaign/campaign.json'
data=json.loads(raw.read_text(encoding='utf-8-sig'))
backup=ROOT/'Saved/VisualRepairBackup';backup.mkdir(parents=True,exist_ok=True)
if not (backup/'campaign_before.json').exists():shutil.copy2(raw,backup/'campaign_before.json')
hidden=[]
def rotate(q,v):
    x,y,z,w=q;a,b,c=v
    tx,ty,tz=2*(y*c-z*b),2*(z*a-x*c),2*(x*b-y*a)
    return [a+w*tx+y*tz-z*ty,b+w*ty+z*tx-x*tz,c+w*tz+x*ty-y*tx]
def dot(a,b):return sum(x*y for x,y in zip(a,b))
doors=[o for o in data['objects'] if o['kind']=='door']
pieces=[]
for row in data['instances']:
    if row.get('owner'):continue
    if row.get('visual_repair')=='clearance_segment':continue
    x,y,z=row['transform']['position']
    window=abs(x+440)<=145 and abs(abs(y)-1448)<40 and 130<=z<=330
    stain='4cf10154cfc5834b' in row['materials'] and row['mesh']=='dce99596c1eb93e7'
    if window or stain:
        row['visible']=False;row['visual_repair']='door_overlap_window' if window else 'obsolete_flat_stain';hidden.append(row['path'])
    if not row.get('visible',True) or row['mesh']!='d1884d819fca686b' or row['materials']!=['86fbd71ebeb15135'] or not 80<z<150:continue
    scale=row['transform']['scale'];axis=0 if abs(scale[0])>abs(scale[1]) else 1
    direction=rotate(row['transform']['rotation'],[1 if i==axis else 0 for i in range(3)])
    length=abs(scale[axis])*100;spans=[(-length/2,length/2)]
    for door in doors:
        t=door['transform'];width_axis=rotate(t['rotation'],[0,1,0]);normal=[-width_axis[1],width_axis[0],0];delta=[v-p for v,p in zip(row['transform']['position'],t['position'])]
        half=door['properties'].get('width',2.4)*50+7
        if abs(dot(delta,normal))>85:continue
        if length<20 and abs(dot(delta,width_axis))<half:
            row['visible']=False;row['visual_repair']='door_overlap_mount';hidden.append(row['path']);break
        if length<150 or abs(dot(direction,width_axis))<.95:continue
        center=-dot(delta,direction);cut=(center-half,center+half);remaining=[]
        for lo,hi in spans:
            if cut[1]<=lo or cut[0]>=hi:remaining.append((lo,hi));continue
            if cut[0]>lo:remaining.append((lo,cut[0]))
            if cut[1]<hi:remaining.append((cut[1],hi))
        spans=remaining
    if spans!=[(-length/2,length/2)]:
        row['visible']=False;row['visual_repair']='door_overlap_rail';hidden.append(row['path'])
        for i,(lo,hi) in enumerate(spans):
            if hi-lo<8:continue
            part=copy.deepcopy(row);part['path']+='_clearance_'+str(i);part['visible']=True;part['visual_repair']='clearance_segment'
            part['transform']['scale'][axis]=(hi-lo)/100
            part['transform']['position']=[p+v*(lo+hi)/2 for p,v in zip(row['transform']['position'],direction)];pieces.append(part)
data['instances'].extend(pieces)
for light in data['lights']:
    p=light.get('transform',{}).get('position',[])
    if len(p)==3 and abs(p[0]+440)<50 and abs(abs(p[1])-1448)<60:
        light['energy']=0
text=json.dumps(data,ensure_ascii=False,indent=2)
raw.write_text(text,encoding='utf-8')
(ROOT/'Content/CampaignData').mkdir(parents=True,exist_ok=True)
(ROOT/'Content/CampaignData/campaign.json').write_text(text,encoding='utf-8')
if globals().get('UPDATE_CURRENT_LEVEL',False):
    u.get_editor_subsystem(u.LevelEditorSubsystem).load_level('/Game/Campaign/Maps/Campaign')
    actors=u.get_editor_subsystem(u.EditorActorSubsystem)
    for actor in actors.get_all_level_actors():
        if hasattr(actor,'rebuild_scene'):actor.rebuild_scene()
        p=actor.get_actor_location()
        if isinstance(actor,u.SpotLight) and abs(p.x+440)<50 and abs(abs(p.y)-1448)<60:actor.spot_light_component.set_intensity(0)
    if not u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level():raise RuntimeError('Could not save repaired level')
hidden=[r['path'] for r in data['instances'] if r.get('visual_repair','').startswith(('door_overlap','obsolete_flat')) and not r.get('visible',True)]
(ROOT/'Saved/Prototype/set_dressing_repair.json').write_text(json.dumps({'success':True,'hidden_instances':len(hidden),'paths':hidden},indent=2),encoding='utf-8')
u.log('SET_DRESSING_REPAIR '+str(len(hidden)))
