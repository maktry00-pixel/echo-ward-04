"""Conservation and geometric attachment checks for v11 details."""
import collections,hashlib,json,math
from pathlib import Path
from hospital_clutter_v10 import Space,bounds
from hospital_furnishing_v6 import Index
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'artifacts/unreal_hospital_v11';source=ROOT/'unreal/EchoWard/Content/CampaignData/hospital_world.json'
d=json.loads(source.read_text(encoding='utf-8'));old=json.loads((ROOT/'artifacts/unreal_hospital_v10/hospital_world_source.json').read_text(encoding='utf-8'))
checks={};issues=[];data=d['microdetails_dressing'];items=data['items'];parts=collections.defaultdict(list);wall=Index()
for w in json.loads((ROOT/'artifacts/unreal_hospital_v3/raster_walls.json').read_text(encoding='utf-8')):wall.add(w['z'],w['rect'])
for b in d['environment_dressing']['boxes']:
    if b.get('layer')==11:parts[b['item_id']].append(b)
def check(name,v):checks[name]=bool(v)
check('all_previous_world_and_story_data_unchanged',all(d[k]==old[k] for k in old if k not in ('review_revision','environment_dressing','furnishing_captures')))
check('all_previous_environment_preserved',[b for b in d['environment_dressing']['boxes'] if b.get('layer')!=11]==old['environment_dressing']['boxes'] and all(d['environment_dressing'][k]==old['environment_dressing'][k] for k in old['environment_dressing'] if k not in ('boxes','materials')))
check('previous_materials_preserved',all(d['environment_dressing']['materials'][k]==v for k,v in old['environment_dressing']['materials'].items()))
check('unique_items_and_complete_part_assemblies',len(items)==len({i['id'] for i in items})==len(parts) and all(len(parts[i['id']])==i['parts'] for i in items))
check('all_new_details_are_nonblocking',all(not b['solid'] for group in parts.values() for b in group))
space=Space()
for item in items:
    group=parts[item['id']];bb=item['bounds'];actual=[min(b['at'][a]-b['size'][a]/2 for b in group) for a in range(3)]+[max(b['at'][a]+b['size'][a]/2 for b in group) for a in range(3)]
    if max(abs(a-b) for a,b in zip(bb,actual))>.000002:issues.append([item['id'],'bounds'])
    if item['support']:
        x,y,w,h,z=item['support']
        if not(bb[0]>=x-.0001 and bb[1]>=y-.0001 and bb[3]<=x+w+.0001 and bb[4]<=y+h+.0001 and abs(bb[2]-z)<.0121):issues.append([item['id'],'support'])
    if item['mount']=='wall' and (not item['wall_samples'] or not all(wall.hits(p[2],[p[0]-.008,p[1]-.008,.016,.016]) for p in item['wall_samples'])):issues.append([item['id'],'wall_backing'])
    for b in group:
        a=bounds(b['at'],b['size'])
        if space.hits(a):issues.append([item['id'],'new_item_overlap']);break
    for b in group:space.add(bounds(b['at'],b['size']),item['id'])
check('supports_wall_backing_and_new_part_spacing_valid',not issues)
check('all_materials_dimensions_and_coordinates_valid',all(b['material'] in d['environment_dressing']['materials'] and min(b['size'])>0 and all(math.isfinite(v) for v in b['at']+b['size']) for group in parts.values() for b in group))
check('medical_office_wall_and_outdoor_families_present',len(data['counts'])>=25 and all(data['counts'].get(k,0)>100 for k in ('syringe','gauze_packet','pen','socket','vent_grille','sanitizer','medical_notice','label_barcode','fallen_leaves')))
check('previous_85_views_retained_and_five_detail_views_added',d['furnishing_captures'][:len(old['furnishing_captures'])]==old['furnishing_captures'] and len(d['furnishing_captures'])==len(old['furnishing_captures'])+5)
report=dict(success=all(checks.values()),checks=checks,issues=issues,map_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),counts=json.loads((OUT/'details_inventory.json').read_text(encoding='utf-8')),limits=data['limits'])
(OUT/'details_acceptance.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8');print(json.dumps(report,ensure_ascii=False,indent=2))
if not report['success']:raise SystemExit(1)
