"""Convert verified glTF artwork to the proven metric OBJ import route."""
import base64,copy,hashlib,json,math
from pathlib import Path
import numpy as np
R=Path(__file__).resolve().parents[1];EXT=R/'assets/external/quality_v23';OUT=R/'unreal/EchoWard/RawAssets/EchoRealV23';OUT.mkdir(parents=True,exist_ok=True)
sources=json.loads((EXT/'SOURCE_MANIFEST.json').read_text(encoding='utf8'))['assets']+json.loads((EXT/'SAMPLES_MANIFEST.json').read_text(encoding='utf8'))['assets'];alpha=json.loads((EXT/'ALPHA_MANIFEST.json').read_text(encoding='utf8'));materials={};assets=[]
select={'fir_sapling':['fir_sapling_a'],'grass_bermuda_01':['grass_bermuda_01_medium_a'],'chemistry_set':['chemistry_set_beaker_large'],'office_notepads':['office_notepads_a4_stack'],'russian_food_cans_01':['russian_food_cans_01_can_fish'],'long_life_food':['long_life_food_milk'],'bananas':['bananas_bunch'],'hamburger_buns':['hamburger_buns_01'],'mounted_fluorescent_lights':['mounted_fluorescent_lights_a'],'fern_02':['fern_02_a'],'shrub_03':['shrub_03_a'],'shrub_sorrel_01':['shrub_sorrel_01_a'],'nettle_plant':['nettle_plant_medium_a_LOD0'],'weed_plant_02':['weed_plant_02_a_LOD0'],'exterior_aircon_unit':['exterior_aircon_unit'],'stationery_supplies':['stationery_supplies_pencilcup']}
# Additional real objects extracted from the author's collections, not a kit
# miniaturised into the dimensions of a single bottle or sheet.
variants=[(860,'chemistry_set',['chemistry_set_beaker_med']),(861,'chemistry_set',['chemistry_set_beaker_small']),(862,'chemistry_set',['chemistry_set_test_tube_a']),(863,'office_notepads',['office_notepads_note_stack']),(864,'office_notepads',['office_notepads_yellow_pad']),(865,'russian_food_cans_01',['russian_food_cans_01_can_cond']),(866,'russian_food_cans_01',['russian_food_cans_01_salt_box']),(867,'long_life_food',['long_life_food_tomatoes']),(868,'long_life_food',['long_life_food_beans']),(869,'long_life_food',['long_life_food_sardines']),(870,'stationery_supplies',['stationery_supplies_pen_red']),(871,'stationery_supplies',['stationery_supplies_eraser']),(872,'grass_bermuda_01',['grass_bermuda_01_medium_c']),(873,'grass_bermuda_01',['grass_bermuda_01_medium_f']),(874,'fir_sapling',['fir_sapling_c'])]
for id,slug,names in variants:
 a=copy.deepcopy(next(a for a in sources if a['slug']==slug));a.update(id=id,variant_nodes=names);sources.append(a)

def transform(n):
 if 'matrix' in n:return np.array(n['matrix']).reshape(4,4).T
 x,y,z,w=n.get('rotation',[0,0,0,1]);m=np.eye(4);m[:3,:3]=np.array([[1-2*(y*y+z*z),2*(x*y-z*w),2*(x*z+y*w)],[2*(x*y+z*w),1-2*(x*x+z*z),2*(y*z-x*w)],[2*(x*z-y*w),2*(y*z+x*w),1-2*(x*x+y*y)]])@np.diag(n.get('scale',[1,1,1]));m[:3,3]=n.get('translation',[0,0,0]);return m

def export(a):
 path=Path(a['file']);d=json.loads(path.read_text(encoding='utf8'));buffers=[base64.b64decode(b['uri'].split(',',1)[1]) if b['uri'].startswith('data:') else (path.parent/b['uri']).read_bytes() for b in d['buffers']]
 def acc(k):
  q=d['accessors'][k];assert 'sparse' not in q;v=d['bufferViews'][q['bufferView']];dt=np.dtype({5120:'i1',5121:'u1',5122:'<i2',5123:'<u2',5125:'<u4',5126:'<f4'}[q['componentType']]);c={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4,'MAT4':16}[q['type']]
  x=np.ndarray((q['count'],c),dtype=dt,buffer=buffers[v['buffer']],offset=v.get('byteOffset',0)+q.get('byteOffset',0),strides=(v.get('byteStride',c*dt.itemsize),dt.itemsize)).copy()
  if q.get('normalized') and dt.kind in 'iu':x=x.astype(float)/np.iinfo(dt).max;x=np.maximum(x,-1)
  return x
 def tex(q):
  if not q:return None
  im=d['images'][d['textures'][q['index']]['source']];uri=im.get('uri');assert uri,'Embedded image requires explicit extraction'
  return dict(file=str((path.parent/uri).resolve()),uv=q.get('texCoord',0),transform=q.get('extensions',{}).get('KHR_texture_transform',{}))
 keys=[];specs=[]
 for m in d.get('materials',[{}]):
  pbr=m.get('pbrMetallicRoughness',{});ex=m.get('extensions',{});s=dict(name=m.get('name',''),color=pbr.get('baseColorFactor',[1,1,1,1]),roughness=pbr.get('roughnessFactor',1),metallic=pbr.get('metallicFactor',1),alpha=m.get('alphaMode','OPAQUE'),cutoff=m.get('alphaCutoff',.5),two_sided=m.get('doubleSided',False),emission=m.get('emissiveFactor',[0,0,0]),normal_strength=m.get('normalTexture',{}).get('scale',1),clearcoat=ex.get('KHR_materials_clearcoat',{}).get('clearcoatFactor',0),transmission=ex.get('KHR_materials_transmission',{}).get('transmissionFactor',0))
  for k,q in [('albedo',pbr.get('baseColorTexture')),('orm',pbr.get('metallicRoughnessTexture')),('normal',m.get('normalTexture')),('ao',m.get('occlusionTexture')),('emissive',m.get('emissiveTexture'))]:
   if q:s[k]=tex(q)
  # JPG glTF does not carry alpha. Bind the original independent alpha map.
  candidates=alpha.get(a['slug'],[])
  for am in candidates:
   suffix=am['channel'].lower().replace('_alpha','').replace('alpha','')
   if not suffix or suffix in m.get('name','').lower():s['alpha_file']=am['file'];break
  glass=s['transmission']>.1 or 'glass' in s['name'].lower()
  if glass:s['alpha']='BLEND';s['color'][3]=min(s['color'][3],.22)
  elif 'alpha_file' in s:s['alpha']='MASK'
  # Do not carry Khronos logos or a branded plate into the fictional hospital.
  if a['slug']=='CarConcept':
   if 'paint' in s['name'].lower():s['color']=[.028,.038,.052,1];s['metallic']=.8;s['clearcoat']=.8
   if 'albedo' in s and 'khronos' in s['albedo']['file'].lower():s.pop('albedo');s['color']=[.08,.08,.08,1]
  s['primary_uv']=s.get('albedo',{}).get('uv',0)
  key='RP23_'+hashlib.sha256(json.dumps(s,sort_keys=True).encode()).hexdigest()[:16];materials[key]=s;keys.append(key);specs.append(s)
 parts=[]
 if a['slug']=='jacaranda_tree':
  cache=np.load(OUT/'geometry/jacaranda_tree.npz');v=cache['pos'];n=cache['normal'];uv=cache['uv'];mi=cache['material'];names=list(cache['material_names']);ind=np.arange(len(v)).reshape(-1,3)[:,::-1]
  for ix,name in enumerate(names):
   key=keys[next(i for i,m in enumerate(d['materials']) if m['name']==name)];chosen=np.flatnonzero(mi==ix);flat=(chosen[:,None]*3+np.arange(3)).reshape(-1);parts.append((key,v[flat],n[flat],uv[flat],np.arange(len(flat)).reshape(-1,3)[:,::-1]))
 else:
  chosen=a.get('variant_nodes',select.get(a['slug']));parents={ch:k for k,node in enumerate(d['nodes']) for ch in node.get('children',[])}
  def visit(k,M):
   node=d['nodes'][k];M=M@transform(node)
   if 'mesh' in node and (not chosen or node.get('name') in chosen) and not (a['slug']=='metal_trash_can' and 'rust' in node.get('name','')):
    for p in d['meshes'][node['mesh']]['primitives']:
     assert p.get('mode',4)==4 and 'KHR_draco_mesh_compression' not in p.get('extensions',{})
     at=p['attributes'];v=acc(at['POSITION']);v=(np.c_[v,np.ones(len(v))]@M.T)[:,:3]
     normal=acc(at['NORMAL'])@np.linalg.inv(M[:3,:3]);normal/=np.maximum(np.linalg.norm(normal,axis=1)[:,None],1e-9)
     ma=p.get('material',0);uvid=specs[ma]['primary_uv'];uv=acc(at['TEXCOORD_'+str(uvid)]) if 'TEXCOORD_'+str(uvid) in at else np.zeros((len(v),2));uv[:,1]=1-uv[:,1]
     ind=acc(p['indices']).astype(np.int64).reshape(-1,3) if 'indices' in p else np.arange(len(v)).reshape(-1,3)
     if np.linalg.det(M[:3,:3])>0:ind=ind[:,::-1]
     parts.append((keys[ma],v[:,[0,2,1]]*100,normal[:,[0,2,1]],uv,ind))
   for ch in node.get('children',[]):visit(ch,M)
  for root in d.get('scenes',[{'nodes':list(range(len(d['nodes'])))}])[d.get('scene',0)]['nodes']:visit(root,np.eye(4))
 assert parts,a['slug'];points=np.concatenate([q[1] for q in parts]);low=points.min(0);high=points.max(0);shift=np.array([(low[0]+high[0])/2,(low[1]+high[1])/2,low[2]]);name=f'EH_{a["id"]:03d}';file=OUT/(name+'.obj');offset=1;slotkeys=list(dict.fromkeys(q[0] for q in parts))
 with file.open('w',encoding='utf8') as f:
  f.write('mtllib '+name+'.mtl\n')
  for key,v,n,uv,ind in parts:
   f.write('g '+name+'\nusemtl '+key+'\n')
   for row in v-shift:f.write('v %.6f %.6f %.6f\n'%tuple(row))
   for row in uv:f.write('vt %.7f %.7f\n'%tuple(row))
   for row in n:f.write('vn %.7f %.7f %.7f\n'%tuple(row))
   for tri in ind+offset:f.write('f '+' '.join(f'{i}/{i}/{i}' for i in tri)+'\n')
   offset+=len(v)
 (OUT/(name+'.mtl')).write_text(''.join(f'newmtl {key}\nKd .5 .5 .5\n\n' for key in slotkeys),encoding='utf8')
 lo=low-shift;hi=high-shift;entry=dict(id=a['id'],name=name,slug=a['slug'],title=a['title'],file=str(file),bounds_cm=[[lo[0],-hi[1],lo[2]],[hi[0],-lo[1],hi[2]]],materials=slotkeys,triangles=sum(len(q[4]) for q in parts),source_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),source=a['source'],authors=a['authors'],license=a['license'],variant_nodes=chosen if a['slug']!='jacaranda_tree' else ['reduced UV copy'],door_leaf=False)
 assets.append(entry);print('PREPARED',entry['id'],a['slug'],entry['triangles'],flush=True)

for source in sources:export(source)
(OUT/'manifest.json').write_text(json.dumps(dict(version=23,units='cm',assets=assets,materials=materials),ensure_ascii=False,indent=2,default=lambda x:x.item()),encoding='utf8');print('PREPARE_COMPLETE',len(assets),len(materials),flush=True)
