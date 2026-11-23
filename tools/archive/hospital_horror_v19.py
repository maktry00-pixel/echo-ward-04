"""Atmosphere profiles and native horror camera samples; geometry conserved."""
import collections,json,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];P=ROOT/'unreal/EchoWard/Content/CampaignData/hospital_world.json';OUT=ROOT/'artifacts/unreal_hospital_v19'
def profile(s):
    sid=s['id'];identity=s.get('identity',{});name=identity.get('soundscape','')
    if sid in ('ST','RF','gallery'):return dict(white_lumens=2200,floor_mist=.18,room_tone=.13,events=['vent','creak','metal_hit','vent'])
    if sid in ('B1','B2','UT') or sid.startswith('SV'):return dict(white_lumens=1500,floor_mist=.32,room_tone=.17,events=['isolation_pipe','pump','isolation_relay','metal_hit','vent'])
    if sid.startswith('DG'):return dict(white_lumens=1650,floor_mist=.26,room_tone=.13,events=['creak','isolation_pipe','knock','vent','whisper'])
    if sid.startswith('PS'):return dict(white_lumens=1500,floor_mist=.24,room_tone=.12,events=['vent','restraint_drag','knock','creak','vent','whisper'])
    if sid.startswith('RH'):return dict(white_lumens=1800,floor_mist=.30,room_tone=.13,events=['isolation_pipe','vent','creak','pump'])
    if sid.startswith('IF'):return dict(white_lumens=1700,floor_mist=.24,room_tone=.16,events=['vent','isolation_relay','vent','metal_hit'])
    if sid.startswith('HS'):return dict(white_lumens=2000,floor_mist=.18,room_tone=.12,events=['radio','creak','isolation_pipe','vent','knock'])
    return dict(white_lumens=1800,floor_mist=.22,room_tone=.14,events=['vent','creak','isolation_pipe','knock','vent','whisper'])
if __name__=='__main__':
    OUT.mkdir(parents=True,exist_ok=True);base=OUT/'baseline_v18.json'
    if not base.exists():assert json.loads(P.read_text(encoding='utf8'))['review_revision']==18;shutil.copy2(P,base)
    d=json.loads(base.read_text(encoding='utf8'));profiles={s['id']:profile(s) for s in d['locations']};profiles['gallery']=profile(dict(id='gallery'))
    d['horror_atmosphere']=dict(version=19,default_bright=False,migrate_previous_inspection_default_once=True,profiles=profiles,white_light_limit=7,mist_volume_limit=5,score_volume=.025,sound_interval_seconds=[24,42],preserve_torch=True,preserve_bright_toggle=True,limits=['Ambient sounds do not broadcast player noise or spawn enemies.','Light and fog pools are local and reused; game geometry is unchanged.'])
    # Explicitly take the real playable corridor positions, with the normal
    # shoulder camera/torch, in both modes for direct comparison.
    selected=('1F','B1','B2','DG','IF','PS','RH','HS','SV','UT','ST','RF')
    for s in d['locations']:
        if s['id'] not in selected:continue
        x,y,z,pitch,yaw=s['capture'];ox,oy,oz=s['origin'];at=[ox+x,oy+y,oz+z]
        # Native sample eye looks down the same existing aisle. It is a view,
        # not a new corridor or a moved player start.
        import math
        a=math.radians(yaw);p=math.radians(pitch)
        target=[at[0]+math.cos(a)*math.cos(p)*8,at[1]+math.sin(a)*math.cos(p)*8,at[2]+math.sin(p)*8]
        for bright in (False,True):d['furnishing_captures'].append(dict(id='v19_'+('bright_' if bright else 'horror_')+s['id'],at=at,target=target,horror_mode=not bright,bright_mode=bright,gameplay_camera=True,pitch=pitch,yaw=yaw))
    # Existing clear detail viewpoints show the actual materials under the
    # normal dark lighting, rather than replacing the scene with concept art.
    for q in list(d['furnishing_captures']):
        if q['id'] in ('v18_scan_3','v18_scan_25','campus_v18_room_1F','campus_v18_room_PS','v18_v16_spine_supports'):
            q=dict(q);q['id']='v19_horror_'+q['id'];q['horror_mode']=True;q['bright_mode']=False;d['furnishing_captures'].append(q)
    d['review_revision']=19;P.write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
    counts=dict(profiles=len(profiles),new_views=sum('v19' in q['id'] for q in d['furnishing_captures']),white_light_limit=7,mist_volume_limit=5)
    (OUT/'inventory.json').write_text(json.dumps(counts,indent=2),encoding='utf8');print(json.dumps(counts),flush=True)
