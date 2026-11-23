"""Add purposeful, metric PBR dressing without modifying the canonical map."""
import collections,csv,hashlib,json,math,random
from pathlib import Path
import numpy as np
from hospital_clutter_v10 import Space,bounds,rotate
from echo_support_coverage_v23 import covers
R=Path(__file__).resolve().parents[1];P=R/'unreal/EchoWard';raw=json.loads((P/'RawAssets/EchoRealV23/manifest.json').read_text(encoding='utf8'));d=json.loads((P/'Content/CampaignData/hospital_world.json').read_text(encoding='utf8'));q22=json.loads((P/'Content/CampaignData/echo_quality_v22.json').read_text(encoding='utf8'));binding=json.loads((P/'Content/CampaignData/echo_custom_bindings.json').read_text(encoding='utf8'));planes=json.loads((P/'RawAssets/EchoRealV23/support_planes.json').read_text(encoding='utf8'));byid={a['id']:a for a in raw['assets']};slug={}
for a in raw['assets']:slug.setdefault(a['slug'],a['id'])
alias_names={175:'old_bed_frame',181:'painted_wooden_nightstand',239:'painted_wooden_cabinet_02',240:'steel_frame_shelves_01',242:'painted_wooden_cabinet',244:'painted_wooden_cabinet_02',419:'steel_frame_shelves_03',234:'metal_office_desk',433:'dining_table',310:'coffee_table_round_01',274:'metal_office_desk',345:'metal_office_desk',444:'dining_table',236:'SchoolChair_01',309:'modern_arm_chair_01',600:'sofa_03',237:'painted_wooden_bench',184:'wheelchair_01',211:'industrial_storage_cart',192:'industrial_storage_cart',280:'CommercialRefrigerator',443:'CommercialRefrigerator',420:'CommercialRefrigerator',278:'industrial_microscope',254:'classic_laptop',259:'clipboard',256:'office_notepads',261:'office_notepads',264:'stationery_supplies',266:'vintage_stapler',267:'DiffuseTransmissionTeacup',269:'modified_thermos',270:'metal_trash_can',326:'korean_fire_extinguisher_01',327:'fire_alarm',334:'plastic_broom',347:'metal_toolbox',348:'cross_pein_hammer',350:'combination_wrench',344:'plastic_crate_01',356:'cardboard_box_01',358:'cardboard_box_01',362:'hand_truck',416:'CashRegister_01',425:'plastic_crate_02',426:'long_life_food',134:'street_lamp_02',131:'desk_lamp_arm_01',121:'mounted_fluorescent_lights',466:'painted_wooden_bench',493:'fir_sapling',498:'shrub_03',501:'grass_bermuda_01',603:'potted_plant_04',618:'plastic_broom',483:'water_manhole_cover',640:'CarConcept',641:'fir_sapling',642:'jacaranda_tree',643:'industrial_wall_lamp',644:'street_lamp_02',648:'planter_box_01',555:'service_pistol',561:'fish_knife'}
# 254 is a wall clock in this library; do not replace it with a laptop.
alias_names.pop(254)
alias_names.pop(334)
alias_names.pop(327)
alias_names.pop(269)
# The sampled antique lantern is not a suitable replacement for the modern
# hospital pole or the tiny lamp hood. Retain those authored fixtures.
for key in (134,643,644):alias_names.pop(key)
# A bottle-filled shop display is unsuitable for the laboratory; likewise a
# cloth dining table must not replace the stainless kitchen preparation top.
for key in (280,444,618):alias_names.pop(key)
alias_names[358]='wooden_crate_01'
aliases={str(k):slug[v] for k,v in alias_names.items() if v in slug};aliases.update({'261':864,'264':870})
furniture=d['prototype_props']+d['environment_props'];parents={p['id']:p for p in furniture};catalog=d['prototype_catalog'];rooms={r['id']:(s,r) for s in d['locations'] for r in s['rooms']};rng=random.Random(23073)
def bb_world(p):
 b=catalog[str(p['model'])]['bounds'];pts=[rotate([x,y,z],p['turn']) for x in [b[0],b[3]] for y in [b[1],b[4]] for z in [b[2],b[5]]];return [min(v[i] for v in pts)+p['at'][i] for i in range(3)]+[max(v[i] for v in pts)+p['at'][i] for i in range(3)]
def support(p):
 old=binding['prototype_models'].get(str(p['model']));new=aliases.get(str(old));b=np.array(catalog[str(p['model'])]['bounds'],float);out=[]
 if new and str(new) in planes:
  bb=np.array(byid[new]['bounds_cm'],float)/100;size=bb[1]-bb[0];target=b[3:]-b[:3];align=(size[0]>size[1])!=(target[0]>target[1]) and max(size[:2])>min(size[:2])*1.2 and max(target[:2])>min(target[:2])*1.2;angle=(90 if align else 0)+180;theta=math.radians(angle);rot=np.array([[math.cos(theta),-math.sin(theta),0],[math.sin(theta),math.cos(theta),0],[0,0,1]])
  rotated=np.array([rot@np.array([x,y,z]) for x in bb[:,0] for y in bb[:,1] for z in bb[:,2]]);scale=target/(rotated.max(0)-rotated.min(0))
  if align:scale[0],scale[1]=scale[1],scale[0]
  shift=(b[:3]+b[3:])*.5-rot@((bb[0]+bb[1])*.5*scale)
  room_angle=math.radians(p['turn']*90);room_rot=np.array([[math.cos(room_angle),-math.sin(room_angle),0],[math.sin(room_angle),math.cos(room_angle),0],[0,0,1]])
  matrix=np.column_stack([room_rot@rot@np.diag(scale),room_rot@shift+np.array(p['at'])])
  for index,plane in enumerate(planes[str(new)]):
   x0,y0,x1,y1=plane['rect'];z=plane['z'];pts=[]
   for x in [x0,x1]:
    for y in [y0,y1]:pts.append(np.array(rotate(rot@([x,y,z]*scale)+shift,p['turn']))+p['at'])
   pts=np.array(pts);lo=pts.min(0);hi=pts.max(0)
   xy=np.array(plane['triangles']);vertices=np.concatenate([xy,np.full((*xy.shape[:2],1),z)],axis=2);triangles=(vertices@matrix[:,:3].T+matrix[:,3])[:,:,:2]
   out.append(dict(rect=[float(lo[0]+.025),float(lo[1]+.025),float(hi[0]-.025),float(hi[1]-.025)],z=float(lo[2]),source_mesh=new,source_plane=index,to_world=matrix.tolist(),triangles=triangles))
 else:
  top=catalog[str(p['model'])].get('top',0)
  if top and p['model'] in (16,17,18,19,20,21,22,23,24,37,38,89,108,132):
   bb=bb_world(p);out.append(dict(rect=[bb[0]+.035,bb[1]+.035,bb[3]-.035,bb[4]-.035],z=p['at'][2]+top,source_mesh=old))
 return out

space=Space();itemspace=Space();world=Space()
for p in furniture:
 b=bb_world(p);world.add(b,p['id']);space.add(b,p['id'])
for p in q22['props']:itemspace.add(p['bounds'],p['id']);space.add(p['bounds'],p['id'])
for layer in ['item_types_dressing','clutter_dressing','microdetails_dressing','campus_plan']:
 for p in d[layer]['items']:
  itemspace.add(p['bounds'],p['id']);space.add(p['bounds'],p['id'])
  if p.get('solid'):world.add(p['bounds'],p['id'])
for i,b in enumerate(d['environment_dressing']['boxes']):
 if b.get('solid') and b['at'][2]+b['size'][2]/2>.18:world.add(bounds(b['at'],b['size']),'env_'+str(i))
# Reserve the actual hide volume plus the player's approach, not merely the
# visible wardrobe/bed outline. These corners are gameplay space.
for h in d['hideouts']:
 x,y,z=h['at'];w,depth,height=h['size'];world.add([x-.8,y-.8,z-.05,x+w+.8,y+depth+1.3,z+height+.2],'reserved_hide_'+h['id'])
for index,p in enumerate(d['partitions']):
 a=np.array(p['a']);b=np.array(p['b']);half=p.get('thickness',.12)*.5;world.add(list(np.minimum(a,b)-[half,half,0])+list(np.maximum(a,b)+[half,half,p.get('height',3.8)]),'partition_'+str(index))
for s in d['locations']:
 for room in s['rooms']:
  if not room.get('enclosed',True):continue
  for door in [room.get('door')]+[x['at'] for x in room.get('extra_doors',[])]:
   if not door:continue
   at=[s['origin'][0]+door[0],s['origin'][1]+door[1],s['origin'][2]+1.3];world.add(bounds(at,[2.8,2.8,2.7]),'door_'+room['id'])

overrides={};old_by_id={};unresolved_support=[];support_assignments={}
for layer in ['clutter_dressing','microdetails_dressing']:old_by_id.update({p['id']:p for p in d[layer]['items']})
support_cache={p['id']:support(p) for p in furniture if p['mount']=='floor'}
for p in q22['props']:
 ref=old_by_id.get(p['id'],p);parent=parents.get(ref.get('parent'))
 if not parent:continue
 candidates=support_cache.get(parent['id'],[]);b=p['bounds'];w,dep,h=[b[i+3]-b[i] for i in range(3)];cx=(b[0]+b[3])/2;cy=(b[1]+b[4])/2;choices=[]
 for plane in candidates:
  x0,y0,x1,y1=plane['rect']
  if w+.02>x1-x0 or dep+.02>y1-y0:continue
  positions=[(min(x1-w*.5,max(x0+w*.5,cx)),min(y1-dep*.5,max(y0+dep*.5,cy)))]+[(x,y) for x in np.linspace(x0+w*.5,x1-w*.5,5) for y in np.linspace(y0+dep*.5,y1-dep*.5,5)]
  for x,y in positions:
   z=plane['z']+.009;candidate=[float(x-w*.5),float(y-dep*.5),z,float(x+w*.5),float(y+dep*.5),z+h]
   if covers(plane,candidate) and not itemspace.hits(candidate,p['id']):choices.append((abs(z-b[2])+math.hypot(x-cx,y-cy)*2,candidate,plane))
 if choices:
  score,best,selected=min(choices,key=lambda q:q[0]);support_assignments[p['id']]={k:v for k,v in selected.items() if k!='triangles'}
  if max(abs(x-y) for x,y in zip(best,b))>.003:overrides[p['id']]=best;itemspace.add(best,p['id']);space.add(best,p['id'])
 elif candidates:unresolved_support.append(p['id'])

def ids(names):return [slug[s] if isinstance(s,str) else s for s in names if not isinstance(s,str) or s in slug]
office=ids(['clipboard','office_notepads','stationery_supplies','vintage_stapler','modified_thermos','DiffuseTransmissionTeacup',863,864,870,871])
clinical=ids(['medical_box','medical_tape','clipboard','chemistry_set','plastic_container','metal_jug','modified_thermos',860,861,862,863])
technical=ids(['adjustable_wrench','combination_wrench','flathead_screwdriver','screwdriver','pliers','tongue_groove_pliers','cross_pein_hammer','crowbar_01','Drill_01','circuit_board','retro_multimeter','lubricant_spray','measuring_tape_01','metal_toolbox'])
food=ids(['long_life_food','russian_food_cans_01','food_apple_01','bananas','hamburger_buns','lemon','DiffuseTransmissionTeacup',865,866,867,868,869])
cleaning=ids(['all_purpose_cleaner','bleach_bottle','multi_cleaner_bottle','drain_cleaner','medical_tape','plastic_container'])
props=[];counts=collections.Counter()
def room_role(zone):
 s,r=rooms.get(zone,({},{}));name=(r.get('name','')+' '+r.get('furniture_template','')).lower()
 if any(x in name for x in ['кух','буфет','кафе','магаз','супермар','пищ','kitchen','retail']):return 'food',food
 if any(x in name for x in ['мастер','генерат','электр','технич','инженер','гараж','ремонт','engineer','workshop']):return 'technical',technical
 if any(x in name for x in ['убор','прачеч','санита','сануз','бельев','laundry']):return 'cleaning',cleaning
 if any(x in name for x in ['лаборат','анализ','процедур','реаним','операц','диагнос','лаборант','гелиос','laboratory']):return 'clinical',clinical
 if any(x in name for x in ['игров','семейн','сенсор','арт-терап']):return 'recreation',office+[853]
 return 'office',office
def add(mesh,at,turn,category,**extra):
 a=byid[mesh];b=np.array(a['bounds_cm'],float).reshape(2,3)/100
 # Newly added items retain real proportions and source metric dimensions.
 if mesh==853:b*=2.5
 local=list(b[0])+list(b[1]);pts=np.array([rotate([x,y,z],turn) for x in b[:,0] for y in b[:,1] for z in b[:,2]])+at;wb=list(pts.min(0))+list(pts.max(0));ident='REAL23_'+str(len(props)).zfill(5)
 if 'support' in extra:extra['support']={k:v for k,v in extra['support'].items() if k!='triangles'}
 entry=dict(id=ident,mesh=mesh,at=at,bounds=local,turn=turn,world_bounds=wb,category=category,**extra);props.append(entry);counts[category]+=1;space.add(wb,ident);itemspace.add(wb,ident)
 if extra.get('solid'):world.add(wb,ident)
 return entry

for p in furniture:
 if p['mount']!='floor' or p['model'] not in (4,13,14,15,16,17,18,19,20,21,22,23,24,37,38,89,132):continue
 role,pool=room_role(p['zone']);surfaces=support_cache.get(p['id'],[])
 for plane in surfaces:
  if len(props)>=12500:break
  x0,y0,x1,y1=plane['rect'];w=x1-x0;dep=y1-y0
  if w<.14 or dep<.12:continue
  candidates=[(x0+w*fx,y0+dep*fy) for fx in [.13,.37,.63,.87] for fy in [.2,.52,.82]];rng.shuffle(candidates);placed=0
  for x,y in candidates:
   if counts['supported_detail']>=12500:break
   for mesh in rng.sample(pool,min(len(pool),5)):
    b=np.array(byid[mesh]['bounds_cm'],float).reshape(2,3)/100
    if mesh==853:b*=2.5
    size=b[1]-b[0];turn=rng.randrange(4);sw,sd=(size[1],size[0]) if turn%2 else (size[0],size[1])
    if size[2]>.55 or sw+.04>w or sd+.04>dep:continue
    if not (x0+sw/2<=x<=x1-sw/2 and y0+sd/2<=y<=y1-sd/2):continue
    z=plane['z']+.009;wb=[x-sw/2,y-sd/2,z,x+sw/2,y+sd/2,z+size[2]]
    if not covers(plane,wb) or space.hits(wb,p['id']):continue
    add(mesh,[x,y,z],turn,'supported_detail',parent=p['id'],zone=p['zone'],role=role,support=plane);placed+=1;break
   if placed>=6:break

# Select small floor belongings near room edges, preserving the central aisle
# and generous door swing/access envelopes. New floor pieces have simple
# matching collisions; the existing map remains unmodified.
for zone,(site,room) in rooms.items():
 if not room.get('enclosed',True) or zone=='RH00':continue
 name=room['name'].lower();role,_=room_role(zone);rx,ry,w,h=room['rect'];ox,oy,z=site['origin'];rect=[rx+ox,ry+oy,rx+ox+w,ry+oy+h]
 if any(t in name for t in ['лест','шахт','лифт','тоннел','туннел','лестнич','коллектор']):continue
 corridor=any(t in name for t in ['коридор','холл','вестибюль','ожидания'])
 pool=ids(['metal_trash_can','plastic_crate_02']) if corridor else ids(['metal_toolbox','plastic_crate_01','industrial_pastic_container','cardboard_box_01']) if role=='technical' else ids(['multi_cleaner_5_litre','plastic_crate_02']) if role=='cleaning' else ids(['plastic_crate_02','cardboard_box_01']) if role=='food' else ids(['medical_box','cardboard_box_01'])
 candidates=[]
 for fx,fy in [(.06,.10),(.94,.10),(.06,.90),(.94,.90),(.06,.45),(.94,.55)]:candidates.append([rect[0]+w*fx,rect[1]+h*fy])
 placed=0
 for x,y in candidates:
  mesh=rng.choice(pool);b=np.array(byid[mesh]['bounds_cm']).reshape(2,3)/100;size=b[1]-b[0]
  if max(size[:2])>1 or size[2]>1.2:continue
  x=min(rect[2]-.18-size[0]/2,max(rect[0]+.18+size[0]/2,x));y=min(rect[3]-.18-size[1]/2,max(rect[1]+.18+size[1]/2,y));wb=[x-size[0]/2,y-size[1]/2,z+.018,x+size[0]/2,y+size[1]/2,z+.018+size[2]]
  if world.hits(wb) or itemspace.hits(wb):continue
  if corridor:
   cross=x-(rect[0]+rect[2])/2 if w<h else y-(rect[1]+rect[3])/2;extent=size[0]/2 if w<h else size[1]/2
   if abs(cross)-extent<.9:continue
  add(mesh,[x,y,z+.018],0,'floor_belonging',zone=zone,role=role,support_z=z,room_rect=rect,solid=True);placed+=1
  if placed>=(3 if corridor else 1):break

# Unobtrusive wall equipment in appropriate spaces, clear of all door routes.
for index,(zone,(site,room)) in enumerate(rooms.items()):
 if not room.get('enclosed',True) or index%3:continue
 role,_=room_role(zone);rx,ry,w,h=room['rect'];ox,oy,z=site['origin'];rect=[rx+ox,ry+oy,rx+ox+w,ry+oy+h]
 mesh=slug['power_box_01'] if role=='technical' else slug['fire_alarm'] if index%2 else slug['security_camera_01'];b=np.array(byid[mesh]['bounds_cm']).reshape(2,3)/100;size=b[1]-b[0];height=1.5 if mesh==slug['fire_alarm'] else 2.25 if mesh==slug['security_camera_01'] else 1.2
 if size[0]>1 or size[1]>.6:continue
 for frac in [.28,.72]:
  x=rect[0]+w*frac;y=rect[1]+size[1]/2+.15;wb=[x-size[0]/2,y-size[1]/2,z+height,x+size[0]/2,y+size[1]/2,z+height+size[2]]
  if world.hits(wb) or itemspace.hits(wb):continue
  add(mesh,[x,y,z+height],0,'wall_equipment',zone=zone,role=role,room_rect=rect,wall_side='north');break

# Break up the regular lawn tufts with mixed plants and ferns. Keep decorative
# plants on the original lawn surfaces and outside solid/travel boundaries.
for lawn in [x for x in d['environment_dressing']['boxes'] if x['kind']=='lawn']:
 at=lawn['at'];sz=lawn['size'];area=sz[0]*sz[1];n=min(12,max(1,int(area/18)))
 for k in range(n):
  mesh=rng.choice(ids(['fern_02','shrub_sorrel_01','nettle_plant','weed_plant_02','shrub_03']));bb=np.array(byid[mesh]['bounds_cm']).reshape(2,3)/100;size=bb[1]-bb[0]
  if size[0]+.3>sz[0] or size[1]+.3>sz[1]:continue
  x=at[0]+rng.uniform(-sz[0]/2+size[0]/2+.1,sz[0]/2-size[0]/2-.1);y=at[1]+rng.uniform(-sz[1]/2+size[1]/2+.1,sz[1]/2-size[1]/2-.1);z=at[2]+sz[2]/2+.005;wb=[x-size[0]/2,y-size[1]/2,z,x+size[0]/2,y+size[1]/2,z+size[2]]
  if world.hits(wb) or itemspace.hits(wb):continue
  add(mesh,[x,y,z],0,'lawn_plant',zone=lawn.get('zone','ST'),lawn_bounds=bounds(at,sz),support_z=z)

views=[]
for c in q22['captures']:
 if c['id'].startswith('v22_court_'):c=dict(c);c['id']=c['id'].replace('v22_','v23_');views.append(c)
for suffix in [126,28,86,128,81,152]:
 c=next(x for x in binding['review_captures'] if x['id']=='v21_detail_'+str(suffix));c=dict(c);c['id']=c['id'].replace('v21_','v23_');views.append(c)
for p in props:
 if p.get('role')!='clinical':continue
 if p['category']!='supported_detail':continue
 zone=p.get('zone');site,room=rooms[zone];b=p['world_bounds'];target=[(b[0]+b[3])/2,(b[1]+b[4])/2,(b[2]+b[5])/2];rx,ry,w,h=room['rect'];ox,oy,oz=site['origin']
 for dx,dy in [(1.1,-.8),(-1.1,.8),(.8,1.1),(-.8,-1.1)]:
  eye=[target[0]+dx,target[1]+dy,max(target[2]+.5,oz+1.5)]
  if rx+ox+.25<eye[0]<rx+ox+w-.25 and ry+oy+.25<eye[1]<ry+oy+h-.25 and not world.hits(bounds(eye,[.3,.3,.3])):views.append(dict(id='v23_lab_clutter',at=eye,target=target));break
 if len(views)>11:break
result=dict(version=23,map_sha256=hashlib.sha256((P/'Content/CampaignData/hospital_world.json').read_bytes()).hexdigest(),mesh_aliases=aliases,preserve_proportions={str(i):True for i in (256,259,261,264,266,267,270,326,344,347,348,350,356,358,555,561)},props=props,bounds_overrides=overrides,unresolved_support=unresolved_support,captures=views,counts=dict(new_items=len(props),**counts,aliases=len(aliases),corrected_supports=len(overrides),source_models=len(raw['assets'])),license_credit='Powered by Poly Haven; Khronos sample asset authors listed in THIRD_PARTY_ART.md',limits=['Existing architecture and story remain; specialised devices without a suitable verified replacement retain their current models.','glTF sheen/diffuse transmission/clearcoat are translated approximately to the existing Unreal rendering path.'])
(R/'artifacts/echo_real_v23/support_assignments.json').write_text(json.dumps(support_assignments,ensure_ascii=False,separators=(',',':')),encoding='utf8')
(P/'Content/CampaignData/echo_real_v23.json').write_text(json.dumps(result,ensure_ascii=False,indent=2,default=lambda x:x.item()),encoding='utf8')
out=R/'artifacts/echo_real_v23';out.mkdir(exist_ok=True)
with (out/'replacement_coverage.csv').open('w',encoding='utf-8-sig',newline='') as f:
 writer=csv.writer(f);writer.writerow(['prototype_id','name','old_mesh','new_mesh','source','status'])
 for k,v in binding['prototype_models'].items():
  new=aliases.get(str(v));writer.writerow([k,catalog[k]['name'],v,new or '',byid[new]['source'] if new else '', 'external detailed model' if new else 'retained specialised/current model'])
print(json.dumps(result['counts'],ensure_ascii=False),flush=True)
