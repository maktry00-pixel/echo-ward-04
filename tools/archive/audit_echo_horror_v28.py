"""Independent check of decay placement, original data and presentation assets."""
from pathlib import Path
import collections,hashlib,json,math
from hospital_clutter_v10 import Space,bounds
R=Path(__file__).resolve().parents[1];P=R/'unreal/EchoWard';O=R/'artifacts/echo_horror_v28'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
old=read(O/'before_identity.json');d=read(P/'Content/CampaignData/hospital_world.json');a=read(P/'Content/CampaignData/echo_horror_v28.json');checks={}
checks['canonical_map_and_all_previous_data_layers_are_unchanged']=all(sha(P/'Content/CampaignData'/n)==h for n,h in old['unchanged_data_hashes'].items()) and sha(P/'Content/CampaignData/echo_detail_v27.json')==old['detail27_data_sha256'] and sha(P/'Content/CampaignData/echo_detail_v26.json')==old['detail_data_sha256'] and a['map_sha256']==old['map_sha256']
zones={r['id']:(s,r) for s in d['locations'] for r in s['rooms']};reserved=Space();partition=Space();placed=Space();overlaps=[]
for s,r in zones.values():
 for door in [r.get('door')]+[q['at'] for q in r.get('extra_doors',[])]:
  if door:reserved.add(bounds([s['origin'][0]+door[0],s['origin'][1]+door[1],s['origin'][2]+1.3],[2.8,2.8,2.7]),r['id'])
for h in d['hideouts']:
 x,y,z=h['at'];w,dep,height=h['size'];reserved.add([x-.8,y-.8,z-.05,x+w+.8,y+dep+1.3,z+height+.2],h['id'])
for i,p in enumerate(d['partitions']):
 h=p.get('thickness',.12)/2;lo=[min(p['a'][j],p['b'][j]) for j in range(3)];hi=[max(p['a'][j],p['b'][j]) for j in range(3)];partition.add([lo[0]-h,lo[1]-h,lo[2],hi[0]+h,hi[1]+h,hi[2]+p.get('height',3.8)],str(i))
for t in a['traces']:
 if placed.hits(t['world_bounds']):overlaps.append(t['id'])
 placed.add(t['world_bounds'],t['id'])
checks['trace_ids_bounds_and_kinds_are_valid']=len({t['id'] for t in a['traces']})==len(a['traces']) and all(t['kind'] in ['Blood','Damp','Drag','Hand','Water'] and t['zone'] in zones and all(math.isfinite(x) for x in t['at']+t['size']) and min(t['size'])>0 for t in a['traces'])
checks['all_new_traces_avoid_doors_hideouts_and_partitions']=all(not reserved.hits(t['world_bounds']) and not partition.hits(t['world_bounds']) for t in a['traces'])
checks['all_traces_are_non_blocking_and_do_not_overlap']=all(not t['solid'] for t in a['traces']) and not overlaps
checks['floor_traces_rest_on_the_same_storey']=all(abs(t['at'][2]-zones[t['zone']][0]['origin'][2]-.026)<.001 for t in a['traces'] if not t['wall'])
checks['wall_traces_fit_the_room_height']=all(zones[t['zone']][0]['origin'][2]-.01<t['world_bounds'][2] and t['world_bounds'][5]<zones[t['zone']][0]['origin'][2]+3.8 for t in a['traces'] if t['wall'])
counts=collections.Counter(t['kind'] for t in a['traces']);checks['all_category_counts_match']=a['counts']==dict(traces=len(a['traces']),markers=len(a['emergency_markers']),**counts)
checks['all_locations_and_gallery_preserve_audio_events']={s['id'] for s in d['locations']}<=set(a['profiles']) and all(a['profiles'][k]['events']==v['events'] and a['profiles'][k]['room_tone']==v['room_tone'] and 900<=a['profiles'][k]['white_lumens']<=2200 for k,v in d['horror_atmosphere']['profiles'].items())
checks['original_bright_inspection_and_flashlight_bindings_remain']='SettingBool(TEXT("inspection_bright"),false)' in (P/'Source/EchoWard/WardBookWorld.cpp').read_text(encoding='utf-8-sig') and 'Enabled?600.f:8.f' in (P/'Source/EchoWard/WardHospitalDetails.cpp').read_text(encoding='utf-8-sig')
checks['new_materials_and_trace_plane_exist']=all((P/'Content/Campaign/HorrorV28'/('MI_'+k+'.uasset')).exists() for k in ['Plaster','Paint','Floor','Facade','Blood','Damp','Drag','Hand','Water']) and (P/'Content/Campaign/HorrorV28/SM_TracePlane.uasset').exists() and read(P/'Saved/Prototype/horror_v28_materials.json')['success']
checks['local_fill_is_small_and_has_no_dynamic_shadow_cost']='SetAttenuationRadius(330)' in (P/'Source/EchoWard/WardHospitalPresentation28.cpp').read_text() and 'HorrorBounce28->SetCastShadows(false)' in (P/'Source/EchoWard/WardHospitalPresentation28.cpp').read_text()
report=dict(success=all(checks.values()),checks=checks,counts=a['counts'],map_sha256=a['map_sha256'],data_sha256=sha(P/'Content/CampaignData/echo_horror_v28.json'),overlaps=overlaps)
(O/'acceptance.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8');print(json.dumps(report,ensure_ascii=False));assert report['success'],[k for k,v in checks.items() if not v]
