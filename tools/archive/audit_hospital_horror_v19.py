"""Conservation, atmosphere bounds and real source-audio checks."""
import hashlib,json,wave,array,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'artifacts/unreal_hospital_v19';P=ROOT/'unreal/EchoWard/Content/CampaignData/hospital_world.json'
d=json.loads(P.read_text(encoding='utf8'));old=json.loads((OUT/'baseline_v18.json').read_text(encoding='utf8'));u=d['horror_atmosphere'];checks={}
def check(k,v):checks[k]=bool(v)
check('all_previous_geometry_furniture_story_and_routes_preserved',all(d[k]==v for k,v in old.items() if k not in ('review_revision','furnishing_captures')))
check('previous_camera_positions_retained',d['furnishing_captures'][:len(old['furnishing_captures'])]==old['furnishing_captures'])
check('horror_default_has_reversible_bright_inspection',u['default_bright'] is False and u['preserve_bright_toggle'] and u['preserve_torch'] and u['migrate_previous_inspection_default_once'])
check('all_35_locations_and_galleries_have_sound_and_light_profiles',set(u['profiles'])=={s['id'] for s in d['locations']}|{'gallery'})
check('bounded_local_lights_fog_and_slow_event_spacing',u['white_light_limit']==7 and u['mist_volume_limit']==5 and u['sound_interval_seconds']==[24,42] and all(1200<=p['white_lumens']<=2400 and .1<=p['floor_mist']<=.4 and .05<=p['room_tone']<=.2 for p in u['profiles'].values()))
campaign=json.loads((P.parent/'campaign.json').read_text(encoding='utf8'));audio={q['id']:q for q in campaign['audio']};used={a for p in u['profiles'].values() for a in p['events']}|{'ambience','score'};rows=[]
for ident in sorted(used):
    q=audio[ident];path=ROOT/'unreal/EchoWard/RawAssets/Campaign'/q['file']
    with wave.open(str(path)) as f:
        channels=f.getnchannels();rate=f.getframerate();frames=f.getnframes();width=f.getsampwidth();pcm=f.readframes(frames)
    assert width==2
    samples=array.array('h',pcm);peak=max(abs(s) for s in samples)/32768;energy=math.sqrt(sum(float(s)*s for s in samples)/len(samples))/32768
    rows.append(dict(id=ident,seconds=frames/rate,channels=channels,rate=rate,peak=peak,rms=energy,source_sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
check('all_used_cues_and_room_tone_have_nonempty_real_pcm',len(rows)==len(used) and all(q['seconds']>.05 and q['rate']>=16000 and q['peak']>0 and q['rms']>0 for q in rows))
views=[q for q in d['furnishing_captures'] if 'v19' in q['id']]
check('29_views_include_12_exact_bright_dark_gameplay_pairs',len(views)==29 and sum(q.get('gameplay_camera',False) for q in views)==24 and sum(q.get('horror_mode',False) for q in views)==17)
for sid in ('1F','B1','B2','DG','IF','PS','RH','HS','SV','UT','ST','RF'):
    a=next(q for q in views if q['id']=='v19_horror_'+sid);b=next(q for q in views if q['id']=='v19_bright_'+sid)
    assert all(a[k]==b[k] for k in ('at','target','pitch','yaw'))
check('horror_and_bright_comparisons_use_identical_positions',True)
report=dict(success=all(checks.values()),map_sha256=hashlib.sha256(P.read_bytes()).hexdigest(),checks=checks,audio_sources=rows,limits=['PCM data and native scheduler checks do not constitute a listening test through the users speakers.','Runtime lighting and mist appearance must be reviewed in real captured frames.'])
(OUT/'atmosphere_acceptance.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8');print(json.dumps(report,ensure_ascii=False,indent=2))
if not report['success']:raise SystemExit(1)
