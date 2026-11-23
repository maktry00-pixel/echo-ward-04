"""Independent conservation, support and item-level overlap checks."""
import collections,hashlib,json,math
from pathlib import Path
from hospital_clutter_v10 import Space,bounds
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'artifacts/unreal_hospital_v10'
source=ROOT/'unreal/EchoWard/Content/CampaignData/hospital_world.json'
d=json.loads(source.read_text(encoding='utf-8'));old=json.loads((ROOT/'artifacts/unreal_hospital_v9/hospital_world_source.json').read_text(encoding='utf-8'))
checks={};issues=[];items=d['clutter_dressing']['items'];boxes=[b for b in d['environment_dressing']['boxes'] if b.get('layer')==10];parts=collections.defaultdict(list)
for b in boxes:parts[b['item_id']].append(b)
def check(name,passed):checks[name]=bool(passed)
check('all_world_dimensions_furniture_and_gameplay_preserved',all(d[k]==old[k] for k in old if k not in ('review_revision','environment_dressing','furnishing_captures')))
check('all_original_environment_boxes_and_lights_preserved',[b for b in d['environment_dressing']['boxes'] if b.get('layer')!=10]==old['environment_dressing']['boxes'] and all(d['environment_dressing'][k]==old['environment_dressing'][k] for k in old['environment_dressing'] if k not in ('boxes','materials')))
check('new_details_never_block_player',len(boxes)>30000 and all(not b['solid'] for b in boxes))
check('unique_logical_items_render_all_their_parts',len(items)>10000 and len(items)==len({i['id'] for i in items})==len(parts) and all(len(parts[i['id']])==i['parts'] for i in items))
space=Space()
for i in items:
    r=i['support_rect'];bb=i['bounds'];support=i['support_z']
    actual=[min(b['at'][a]-b['size'][a]/2 for b in parts[i['id']]) for a in range(3)]+[max(b['at'][a]+b['size'][a]/2 for b in parts[i['id']]) for a in range(3)]
    if max(abs(a-b) for a,b in zip(bb,actual))>.00002:issues.append([i['id'],'part_bounds'])
    if not(bb[0]>=r[0]-.001 and bb[1]>=r[1]-.001 and bb[3]<=r[2]+.001 and bb[4]<=r[3]+.001 and abs(bb[2]-support-.008)<.00002):issues.append([i['id'],'support'])
    if space.hits(bb):issues.append([i['id'],'overlap_with_new_item'])
    space.add(bb,i['id'])
check('all_belongings_fit_supports_without_overlapping_other_new_items',not issues)
check('all_parts_have_valid_materials_and_finite_dimensions',all(b['material'] in d['environment_dressing']['materials'] and all(math.isfinite(v) for v in b['at']+b['size']) and min(b['size'])>0 for b in boxes))
check('original_80_inspection_views_retained',d['furnishing_captures'][:len(old['furnishing_captures'])]==old['furnishing_captures'])
check('all_15_detail_families_present',len(d['clutter_dressing']['counts'])==15)
report=dict(success=all(checks.values()),checks=checks,issues=issues,map_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),counts=json.loads((OUT/'clutter_inventory.json').read_text(encoding='utf-8')),limits=d['clutter_dressing']['limits'])
(OUT/'clutter_acceptance.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8');print(json.dumps(report,ensure_ascii=False,indent=2))
if not report['success']:raise SystemExit(1)
