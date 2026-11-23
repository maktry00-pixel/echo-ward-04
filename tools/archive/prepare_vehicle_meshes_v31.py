"""Normalise user-provided vehicles; retain hero hinges and source textures."""
import bpy,json,sys,hashlib,math,traceback,os
from pathlib import Path
from mathutils import Vector,Matrix
R=Path(__file__).resolve().parents[1];O=R/'unreal/EchoWard/RawAssets/EchoCarsV31';O.mkdir(parents=True,exist_ok=True);(O/'Textures').mkdir(exist_ok=True)
D=R/'artifacts/echo_cars_v31';inventory=json.loads((D/'inventory.json').read_text());assets=[];materials={};vehicles=[]
priority={'.glb':0,'.gltf':1,'.fbx':2,'.blend':3,'.obj':4,'.usdz':5,'.ply':6}
length_override={'2023-toyota-gr-corolla':4.41,'free-1975-porsche-911-930-turbo':4.29,'mercedes-benz-g-class-w263':4.82,'police-car-suv':5.05,'gls-van':6.8}
def load(p):
 bpy.ops.wm.read_factory_settings(use_empty=True);bpy.context.preferences.filepaths.use_scripts_auto_execute=False;e=p.suffix.lower()
 if e in ['.glb','.gltf']:bpy.ops.import_scene.gltf(filepath=str(p))
 elif e=='.fbx':bpy.ops.import_scene.fbx(filepath=str(p),use_anim=True,ignore_leaf_bones=True)
 elif e=='.blend':bpy.ops.wm.open_mainfile(filepath=str(p),load_ui=False,use_scripts=False)
 elif e=='.obj':bpy.ops.wm.obj_import(filepath=str(p))
 elif e=='.usdz':bpy.ops.wm.usd_import(filepath=str(p))
 elif e=='.ply':bpy.ops.wm.ply_import(filepath=str(p))
 else:raise ValueError(e)
 bpy.context.scene.frame_set(0)
def point_surface(ob):
 # Keep the supplied scan, reconstruct its outer shell, and transfer nearest RGB.
 if len(ob.data.polygons):return ob
 color=ob.data.color_attributes.active_color
 assert color,'Point cloud has no RGB';attribute=color.name
 g=bpy.data.node_groups.new('SourceScanSurface31','GeometryNodeTree');g.interface.new_socket(name='Geometry',in_out='INPUT',socket_type='NodeSocketGeometry');g.interface.new_socket(name='Geometry',in_out='OUTPUT',socket_type='NodeSocketGeometry')
 n=g.nodes;links=g.links;src=n.new('NodeGroupInput');out=n.new('NodeGroupOutput');vol=n.new('GeometryNodePointsToVolume');vol.resolution_mode='VOXEL_SIZE';vol.inputs['Voxel Size'].default_value=.045;vol.inputs['Radius'].default_value=.055;vol.inputs['Density'].default_value=1
 links.new(src.outputs['Geometry'],vol.inputs['Points']);mesh=n.new('GeometryNodeVolumeToMesh');mesh.inputs['Threshold'].default_value=.15;links.new(vol.outputs['Volume'],mesh.inputs['Volume'])
 nearest=n.new('GeometryNodeSampleNearest');nearest.domain='POINT';links.new(src.outputs['Geometry'],nearest.inputs['Geometry']);pos=n.new('GeometryNodeInputPosition');links.new(pos.outputs['Position'],nearest.inputs['Sample Position'])
 named=n.new('GeometryNodeInputNamedAttribute');named.data_type='FLOAT_COLOR';named.inputs['Name'].default_value=attribute
 sample=n.new('GeometryNodeSampleIndex');sample.data_type='FLOAT_COLOR';sample.domain='POINT';links.new(src.outputs['Geometry'],sample.inputs['Geometry']);links.new(named.outputs['Attribute'],sample.inputs['Value']);links.new(nearest.outputs['Index'],sample.inputs['Index'])
 store=n.new('GeometryNodeStoreNamedAttribute');store.data_type='FLOAT_COLOR';store.domain='POINT';store.inputs['Name'].default_value='Col';links.new(mesh.outputs['Mesh'],store.inputs['Geometry']);links.new(sample.outputs['Value'],store.inputs['Value']);links.new(store.outputs['Geometry'],out.inputs['Geometry'])
 mod=ob.modifiers.new('Reconstructed supplied point cloud','NODES');mod.node_group=g;bpy.context.view_layer.objects.active=ob;ob.select_set(True);bpy.ops.object.modifier_apply(modifier=mod.name)
 assert len(ob.data.polygons)>100;return ob

def image_file(im,lookup):
 if not im:return None
 if im.packed_file:
  raw=bytes(im.packed_file.data);ext=Path(im.filepath).suffix.lower()
  if ext not in ['.jpg','.jpeg','.png','.tga','.dds','.bmp','.webp']:ext='.png'
  p=O/'Textures'/('T_'+hashlib.sha256(raw).hexdigest()[:20]+ext)
  if not p.exists():p.write_bytes(raw)
  return str(p)
 p=Path(bpy.path.abspath(im.filepath))
 if not p.exists():p=lookup.get(Path(im.filepath.replace('\\','/')).name.lower())
 if p and p.exists():return str(p)
 return None

def material(m,lookup,vertex_color=False):
 if not m:spec=dict(name='default',color=[.15,.15,.15,1],roughness=.65,metallic=0,alpha='OPAQUE',two_sided=False,emission=[0,0,0])
 else:
  name=m.name.lower();bs=next((n for n in m.node_tree.nodes if n.type=='BSDF_PRINCIPLED'),None) if m.use_nodes else None
  def val(key,default):
   if not bs or key not in bs.inputs:return default
   v=bs.inputs[key].default_value
   try:return list(v)
   except:return float(v)
  glass=any(x in name for x in ['glass','glas','стек','window']) and not any(x in name for x in ['glass_plastic','window_frame','windowframe'])
  paint=any(x in name for x in ['body','carpaint','paint','car_paint'])
  spec=dict(name=m.name,color=val('Base Color',list(m.diffuse_color)),roughness=val('Roughness',m.roughness),metallic=val('Metallic',m.metallic),alpha='BLEND' if glass or val('Alpha',1)<.95 else 'OPAQUE',two_sided=glass,emission=val('Emission Color',[0,0,0,1])[:3],clearcoat=.65 if paint else val('Coat Weight',0),vertex_color=vertex_color)
  spec['color']=spec['color'][:4]
  if glass:spec['color'][3]=.22;spec['roughness']=min(spec['roughness'],.15)
  elif any(x in name for x in ['rubber','tire','tyre','leather','fabric']):spec['roughness']=max(.55,spec['roughness']);spec['metallic']=0
  elif any(x in name for x in ['chrome','metal','alloy']):spec['metallic']=max(.65,spec['metallic'])
  if paint:spec['metallic']=max(.4,spec['metallic']);spec['roughness']=min(.32,max(.16,spec['roughness']))
  if m.use_nodes:
   def find_image(sock,seen=None):
    seen=set() if seen is None else seen
    if not sock or not sock.is_linked:return None
    for lk in sock.links:
     n=lk.from_node
     if n in seen:continue
     seen.add(n)
     if n.type=='TEX_IMAGE':return n.image
     for i in n.inputs:
      im=find_image(i,seen)
      if im:return im
    return None
   for key,pin in [('albedo','Base Color'),('normal','Normal'),('roughness_map','Roughness'),('metallic_map','Metallic')]:
    f=image_file(find_image(bs.inputs.get(pin)) if bs else None,lookup)
    if f:spec[key]=f
  if 'albedo' in spec:
   # FBX diffuse factors often duplicate the texture tint; preserve the actual map.
   spec['color'][:3]=[1,1,1]
  # Original FBX includes albedo; Sketchfab also supplied the independent normal maps.
  for suffix in ['_normal.png','_Normal.png','_normal.jpg','_n.png']:
   q=lookup.get((m.name+suffix).lower())
   if q and 'normal' not in spec:spec['normal']=str(q)
  for key,suffixes in [('roughness_map',['_roughness.png','_roughness.jpg','_rough.png']),('metallic_map',['_metallic.png','_metalness.png']),('alpha_file',['_opacity.png','_alpha.png'])]:
   for suffix in suffixes:
    q=lookup.get((m.name+suffix).lower())
    if q:spec[key]=str(q);break
  if 'alpha_file' in spec and not glass:spec['alpha']='MASK';spec['two_sided']=True
  if 'gauges' in name:spec['emission']=[.22,.25,.22]
 spec.setdefault('clearcoat',0);spec.setdefault('vertex_color',vertex_color)
 key='VC31_'+hashlib.sha256(json.dumps({k:v for k,v in spec.items() if k!='name'},sort_keys=True).encode()).hexdigest()[:16];materials[key]=spec
 stub=bpy.data.materials.get(key) or bpy.data.materials.new(key);stub.diffuse_color=tuple(spec['color']);return stub,key

def is_transparent(m):
 if not m:return False
 name=m.name.lower()
 if any(t in name for t in ['glass','glas','window']) and 'frame' not in name:return True
 bs=next((n for n in m.node_tree.nodes if n.type=='BSDF_PRINCIPLED'),None) if m.use_nodes else None
 return bool(bs and bs.inputs.get('Alpha') and bs.inputs['Alpha'].default_value<.95)

def export_asset(mid,slug,objects,N,lookup,pivot=None,kind='body',nanite=False,max_tri=240000,glass_only=None):
 pivot=Vector((0,0,0)) if pivot is None else pivot
 bpy.ops.object.select_all(action='DESELECT');created=[];deps=bpy.context.evaluated_depsgraph_get()
 for ob in objects:
  ev=ob.evaluated_get(deps);me=bpy.data.meshes.new_from_object(ev,preserve_all_data_layers=True,depsgraph=deps)
  if not me.polygons:bpy.data.meshes.remove(me);continue
  if glass_only is not None:
   import bmesh
   bm=bmesh.new();bm.from_mesh(me);delete=[]
   for f in bm.faces:
    ma=me.materials[f.material_index] if f.material_index<len(me.materials) else None
    glass=is_transparent(ma)
    if glass!=glass_only:delete.append(f)
   bmesh.ops.delete(bm,geom=delete,context='FACES');bm.to_mesh(me);bm.free()
   if not me.polygons:bpy.data.meshes.remove(me);continue
  me.transform(Matrix.Translation(-pivot)@N@ob.matrix_world)
  part=bpy.data.objects.new('vehicle_piece',me);bpy.context.collection.objects.link(part)
  for i in range(len(me.materials)):
   mat=me.materials[i];stub,key=material(mat,lookup,bool(me.color_attributes));me.materials[i]=stub
  if not me.materials:
   stub,key=material(None,lookup,bool(me.color_attributes));me.materials.append(stub)
  part.select_set(True);created.append(part)
 assert created,(slug,kind)
 bpy.context.view_layer.objects.active=created[0];bpy.ops.object.join();joined=bpy.context.object;joined.name='EH_'+str(mid)
 # Consolidate identical material slots before import; never lose the interior/UVs.
 keys=[];indices={};used=sorted({p.material_index for p in joined.data.polygons})
 for i in used:
  mat=joined.data.materials[i]
  if mat.name not in keys:keys.append(mat.name)
  indices[i]=keys.index(mat.name)
 face_slots=[indices[poly.material_index] for poly in joined.data.polygons]
 joined.data.materials.clear()
 for k in keys:joined.data.materials.append(bpy.data.materials[k])
 for poly,slot in zip(joined.data.polygons,face_slots):poly.material_index=slot
 joined.data.calc_loop_triangles();original=len(joined.data.loop_triangles)
 original_mesh=joined.data.copy();before_bounds=[(min(v.co[i] for v in joined.data.vertices),max(v.co[i] for v in joined.data.vertices)) for i in range(3)]
 if False and original>max_tri:
  dec=joined.modifiers.new('Budgeted source geometry','DECIMATE');dec.ratio=max_tri/original;dec.use_collapse_triangulate=True;bpy.ops.object.modifier_apply(modifier=dec.name)
 after_bounds=[(min(v.co[i] for v in joined.data.vertices),max(v.co[i] for v in joined.data.vertices)) for i in range(3)]
 if max(abs(x-y) for a,b in zip(before_bounds,after_bounds) for x,y in zip(a,b))>2:
  print('DECIMATION_BOUNDS_REJECTED',slug,kind,before_bounds,after_bounds,flush=True);bad=joined.data;joined.data=original_mesh;bpy.data.meshes.remove(bad)
 else:bpy.data.meshes.remove(original_mesh)
 joined.data.calc_loop_triangles();tri=len(joined.data.loop_triangles);used_indices={i for f in joined.data.polygons for i in f.vertices};pts=[joined.data.vertices[i] for i in used_indices];lo=[min(v.co[i] for v in pts) for i in range(3)];hi=[max(v.co[i] for v in pts) for i in range(3)]
 filename=O/('EH_'+str(mid)+'.fbx');bpy.context.scene.unit_settings.system='NONE';bpy.context.scene.unit_settings.scale_length=1
 bpy.ops.export_scene.fbx(filepath=str(filename),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',global_scale=1,apply_unit_scale=False,apply_scale_options='FBX_SCALE_NONE',use_space_transform=False,bake_space_transform=False,use_mesh_modifiers=False,mesh_smooth_type='FACE',add_leaf_bones=False,bake_anim=False,path_mode='ABSOLUTE',embed_textures=False)
 row=dict(id=mid,name='EH_'+str(mid),slug=slug,kind=kind,file=str(filename),bounds_cm=[[lo[0],-hi[1],lo[2]],[hi[0],-lo[1],hi[2]]],materials=keys,triangles=tri,original_triangles=original,nanite=nanite,pivot_cm=[pivot.x,-pivot.y,pivot.z],source_sha256=hashlib.sha256(filename.read_bytes()).hexdigest(),door_leaf=False,source='User-supplied archive assets/models/cars/'+slug+'.zip',license='User-supplied; publisher license not included')
 assets.append(row);bpy.data.objects.remove(joined,do_unlink=True);return row

for ix,src in enumerate(inventory['vehicles']):
 slug=Path(src['archive']).stem;done=O/(slug+'_prepared.json')
 if done.exists() and json.loads(done.read_text()).get('revision')==6:
  old=json.loads(done.read_text());assets+=old['assets'];materials.update(old['materials']);vehicles.append(old['vehicle']);continue
 try:
  scene=json.loads((D/(slug+'_scene.json')).read_text());p=Path(scene['file']);load(p);root=R/'assets/models/cars/extracted_v31'/slug
  lookup={}
  for f in root.rglob('*'):
   if f.is_file() and f.suffix.lower() in ['.png','.jpg','.jpeg','.tga','.dds','.bmp','.webp']:lookup.setdefault(f.name.lower(),f)
  meshes=[o for o in bpy.context.scene.objects if o.type=='MESH' and not o.hide_render and not any(x in o.name.lower() for x in ['cutter','collision','collider'])]
  if slug=='court-porsche-test':meshes=[o for o in meshes if all((o.matrix_world@Vector(v)).x>-6 and (o.matrix_world@Vector(v)).x<1 and (o.matrix_world@Vector(v)).y>9 and (o.matrix_world@Vector(v)).y<18 and (o.matrix_world@Vector(v)).z<3 for v in o.bound_box)]
  if slug=='gls-van':meshes=[point_surface(meshes[0])]
  # This legacy source duplicates paint with a coplanar renderer-specific coat shell.
  # UE uses the paint material's clear-coat lobe, so retain only the real paint surface.
  if slug=='free-1975-porsche-911-930-turbo':meshes=[o for o in meshes if o.name!='Circle.000']
  if slug=='auto-mezzanine-deck-trailer':meshes=[o for o in meshes if o.name!='Cylinder005']
  if slug=='2025-bentley-continental-gt-speed':meshes=[o for o in meshes if not o.name.startswith('Icosphere')]
  if slug=='mercedes-benz-maybach-2022':meshes=[o for o in meshes if o.name!='Cube_Cube.001']
  if slug=='court-porsche-test':meshes=[o for o in meshes if o.name!='Plane_0']
  # Exclude render-stage floors and helper planes, not real doors/glass.
  meshes=[o for o in meshes if not (len(o.data.polygons)<20 and max(o.dimensions.x,o.dimensions.y)>4 and o.dimensions.z<.02)]
  assert meshes,slug
  import numpy as np
  deps=bpy.context.evaluated_depsgraph_get();cloud=[]
  for ob in meshes:
   ev=ob.evaluated_get(deps);me=bpy.data.meshes.new_from_object(ev,preserve_all_data_layers=True,depsgraph=deps)
   co=np.empty(len(me.vertices)*3,dtype=np.float32);me.vertices.foreach_get('co',co);co=co.reshape(-1,3);M=np.array(ob.matrix_world);cloud.append(co@M[:3,:3].T+M[:3,3]);bpy.data.meshes.remove(me)
  ar=np.concatenate(cloud);sample=ar[::max(1,len(ar)//50000)];eig,V=np.linalg.eigh(np.cov(sample[:,:2].T));long=V[:,np.argmax(eig)]
  if long[np.argmax(abs(long))]<0:long=-long
  if slug=='mercedes-e-class-w212':long=np.array([0.,1.])
  yaw=math.atan2(long[1],long[0]);rot=Matrix.Rotation(-yaw,4,'Z');rotated=ar@np.array(rot)[:3,:3].T;low=Vector(rotated.min(axis=0));high=Vector(rotated.max(axis=0));size=high-low
  scale=100 if size.x<.1 else 1
  if slug in length_override:scale=length_override[slug]/size.x
  shift=Vector(((low.x+high.x)*.5,(low.y+high.y)*.5,low.z));N=Matrix.Diagonal(Vector((100*scale,100*scale,100*scale,1)))@Matrix.Translation(-shift)@rot
  del cloud,ar,rotated
  mid=1000+ix;before=len(assets);oldkeys=set(materials)
  has_glass=any(is_transparent(m) for ob in meshes for m in ob.data.materials)
  body=export_asset(mid,slug,meshes,N,lookup,nanite=True,glass_only=False);gl=export_asset(1100+ix,slug,meshes,N,lookup,kind='glass',max_tri=65000,glass_only=True) if has_glass else None
  full_lo=list(body['bounds_cm'][0]);full_hi=list(body['bounds_cm'][1])
  if gl:
   for i in range(3):full_lo[i]=min(full_lo[i],gl['bounds_cm'][0][i]);full_hi[i]=max(full_hi[i],gl['bounds_cm'][1][i])
  vehicle=dict(slug=slug,title=slug.replace('-',' '),archive=src['archive'],archive_sha256=src['sha256'],body=mid,glass=gl['id'] if gl else 0,bounds_cm=[full_lo,full_hi],scale_correction=scale,source_yaw_degrees=math.degrees(yaw),point_cloud_reconstruction=slug=='gls-van')
  if slug=='mercedes-e-class-w212':
   movable=['DoorFL','DoorFR','DoorRL','DoorRR','Hood','Trunk','WheelFL','WheelFR','WheelRL','WheelRR','SteeringWheel'];parts=[]
   def root_part(ob):
    par=ob.parent
    while par:
     if par.name in movable:return par.name
     par=par.parent
    return 'Body'
   fixed=[o for o in meshes if root_part(o)=='Body'];export_asset(2000,slug,fixed,N,lookup,kind='hero_body',max_tri=200000)
   parts.append(dict(name='Body',model=2000,pivot_cm=[0,0,0],axis='none',open_degrees=0))
   for n,name in enumerate(movable):
    part_objects=[o for o in meshes if root_part(o)==name];parent=bpy.data.objects[name];pivot=N@parent.matrix_world.translation;entry=export_asset(2001+n,slug,part_objects,N,lookup,pivot=pivot,kind='hero_'+name,max_tri=45000)
    angle=65 if name.endswith('L') else -65
    if name=='Hood':angle=55
    if name=='Trunk':angle=-55
    parts.append(dict(name=name,model=entry['id'],pivot_cm=entry['pivot_cm'],axis='pitch' if name in ['Hood','Trunk'] or name.startswith('Wheel') else 'yaw',open_degrees=angle if name.startswith('Door') or name in ['Hood','Trunk'] else 0))
   vehicle['hero_parts']=parts;seat=N@Vector((-.36,-.20,1.12));vehicle['driver_eye_cm']=[seat.x,-seat.y,seat.z]
  vehicles.append(vehicle);batch_assets=assets[before:];used={k for a in batch_assets for k in a['materials']};payload=dict(revision=6,vehicle=vehicle,assets=batch_assets,materials={k:materials[k] for k in used});done.write_text(json.dumps(payload,ensure_ascii=False,indent=2))
  print('VEHICLE_PREPARED',slug,'meshes',len(batch_assets),'size_cm',[round(full_hi[i]-full_lo[i],1) for i in range(3)],flush=True)
 except Exception:
  (O/(slug+'_failed.txt')).write_text(traceback.format_exc());print('VEHICLE_PREP_FAILED',slug,traceback.format_exc(),flush=True)
(O/'manifest.json').write_text(json.dumps(dict(version=31,units='cm',assets=assets,materials=materials,vehicles=vehicles),ensure_ascii=False,indent=2))
print('CAR_PREPARATION_COMPLETE',len(vehicles),len(assets),len(materials),flush=True)
