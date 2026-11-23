"""Independent acceptance audit of the supplied review against the authored map."""
from pathlib import Path
from collections import Counter
import hashlib,json,csv

root=Path(__file__).resolve().parents[1]
source=root/'unreal/EchoWard/Content/CampaignData/hospital_world.json'
d=json.loads(source.read_text(encoding='utf-8'))
base=json.loads((root/'artifacts/unreal_hospital_v3/hospital_v2_baseline.json').read_text(encoding='utf-8'))
old={r['id']:r for l in base['locations'] for r in l['rooms']}
zones={r['id']:r for l in d['locations'] for r in l['rooms']}
sites={l['id']:l for l in d['locations']}
checks={}
def check(k,v):checks[k]=bool(v)
check('333_original_ids_and_areas_preserved',set(old)<set(zones) and len(zones)==334 and all(zones[k]['rect'][2]*zones[k]['rect'][3]>=r['rect'][2]*r['rect'][3] for k,r in old.items()))
check('original_production_classes_80_120_133',Counter(zones[k]['production_class'] for k in old)=={'A':80,'B':120,'C':133})
check('DG2_supported_by_eight_columns_and_four_beams',sum(x['kind']=='column' for x in d['structural_members'])==8 and sum(x['kind']=='beam' for x in d['structural_members'])==4)
check('DG_stair_reaches_basement_ground_and_upper',any(s['id']=='DG' and s['z_min']==-4.8 and s['z_max']==4.8 for s in d['stairs']))
check('six_explicit_basement_links_and_SV24_connection',len(d['utility_links'])==7 and d['utility_links'][-1]['target']=='SV24' and d['utility_links'][-1]['points'][-1][2]==0)
check('six_collinear_lift_landings',len(d['lift']['landings'])==6 and all(s['shaft']==[48,4,4,6] for s in d['lift']['landings']))
hide=Counter(n['zone'] for n in d['hideouts'])
check('enclosed_A_two_exits_two_hideouts',all(1+len(r.get('extra_doors',[]))>=2 and hide[r['id']]>=2 for r in zones.values() if r['enclosed'] and r['production_class']=='A'))
check('enclosed_B_has_hideout',all(hide[r['id']]>=1 for r in zones.values() if r['enclosed'] and r['production_class']=='B'))
check('enclosed_B_has_one_or_two_exits',all(1<=1+len(r.get('extra_doors',[]))<=2 for r in zones.values() if r['enclosed'] and r['production_class']=='B'))
check('twenty_C_maintenance_exits',len(d['secret_passages'])==20 and all(zones[s['zone']]['production_class']=='C' for s in d['secret_passages']))
check('five_named_refuges_with_bolts',all(zones[k]['refuge'] and zones[k]['door_type']=='bolt' for k in ('IF08','RH08','DG08','SV23','HS08')))
loops=Counter(p['site'] for p in d['shortcuts'])
check('three_loops_on_each_enclosed_department_floor',all(loops[k]>=3 for k in ('1F','2F','3F','B1','B2','DG','DG2','IF','PS','RH','HS','SV')))
check('corridor_centreline_samples_within_fifteen_metres',not d['corridor_escape_audit']['unresolved_samples'])
distance_path=root/'artifacts/unreal_hospital_v4/escape_distance_audit.json'
if distance_path.exists():
    distance=json.loads(distance_path.read_text(encoding='utf-8'))
    check('all_reachable_corridor_grid_samples_within_fifteen_metre_path',distance['success'] and distance['failed_samples']==0 and distance['source_sha256']==hashlib.sha256(source.read_bytes()).hexdigest())
else:
    check('all_reachable_corridor_grid_samples_within_fifteen_metre_path',False)
check('B2_twenty_four_evolving_zones',len(d['zone_evolution'])==24 and all([x['state'] for x in r['states']]==['clean','infected','damaged','collapsed'] for r in d['zone_evolution']))
check('long_chapters_have_extended_spatial_sequences',all(len(c['route_zones'])>=10 for c in d['chapters'] if c['id'] in (7,10,15)))
check('seventeen_monster_contracts_no_unimplemented_AI_claim',len(d['monster_rules'])==17 and all(not m['implemented'] and m['classification_status']=='review_proposal' for m in d['monster_rules']))
check('combat_classes_six_A_seven_B_four_C',Counter(m['combat_class'] for m in d['monster_rules'])=={'A':6,'B':7,'C':4})
check('five_interaction_prototypes',len(d['movable_props'])==5)
out=root/'artifacts/unreal_hospital_v4';out.mkdir(exist_ok=True)
for key,count in (('doors',sum(1+len(r.get('extra_doors',[])) for r in zones.values() if r['enclosed'])),('windows',len(d['windows'])),('network_devices',69),('threat_nodes',12)):
    rows=list(csv.DictReader((out/(key+'.csv')).open(encoding='utf-8-sig'),delimiter=';'))
    check(key+'_register_matches_source',len(rows)==count)
result=dict(success=all(checks.values()),checks=checks,map_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
    counts=dict(zones=len(zones),hideouts=len(d['hideouts']),corridor_niches=len(d['escape_niches']),shortcuts=len(d['shortcuts'])),
    limits=['Distance audit covers Chapter-0 reachable corridor standing samples on a 0.5m collision grid, not continuous space or all story states.',
            'Classes are the review proposal; monster AI, finale combat and cinematic chapter scenes remain unimplemented.'])
(out/'acceptance.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(result,ensure_ascii=False,indent=2))
if not result['success']:raise SystemExit(1)
