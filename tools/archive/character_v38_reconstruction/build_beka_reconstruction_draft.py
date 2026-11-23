"""Local manual reconstruction draft. Not a RealityScan scan or final likeness."""
import bpy,bmesh,math,json,random
from pathlib import Path
from mathutils import Vector
ROOT=Path('D:/Projects/ECHO')
OUT=ROOT/'assets/models/hero_custom/beka_reconstruction_draft_v1'
OUT.mkdir(parents=True,exist_ok=True)
REVIEW=ROOT/'artifacts/echo_hero_v38/manual_assembly_v1';REVIEW.mkdir(exist_ok=True)
random.seed(381)
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'assets/models/hero_custom/photo_outfit_v1/beka_photo_outfit_v1.blend'))
bpy.context.preferences.filepaths.save_version=0
for o in list(bpy.data.objects):
 if o.type in ['CAMERA','LIGHT']:bpy.data.objects.remove(o,do_unlink=True)
body=next(o for o in bpy.data.objects if o.type=='MESH' and o.name.startswith('BodyMesh'))
rig=next(o for o in bpy.data.objects if o.type=='ARMATURE')
# Keep a real skin hand surface; remove covered anatomy instead of leaving skin through cloth.
bm=bmesh.new();bm.from_mesh(body.data)
coords={v:body.matrix_world@v.co for v in bm.verts}
drop=[v for v in bm.verts if not (abs(coords[v].x)>.435 and .84<coords[v].z<1.09)]
bmesh.ops.delete(bm,geom=drop,context='VERTS');bm.to_mesh(body.data);bm.free()
body.name='Beka_Hands';body.hide_render=False;body.hide_set(False)
before=set(bpy.data.objects)
bpy.ops.import_scene.fbx(filepath=str(ROOT/'artifacts/echo_hero_v38/photo_fit_v2/face.fbx'),use_anim=False)
face=next(o for o in bpy.data.objects if o not in before and o.type=='MESH')
faceRig=next(o for o in bpy.data.objects if o not in before and o.type=='ARMATURE')
# Remove hidden mouth/occlusion lash placeholder surfaces. Use actual visible LOD0 skin and eyes.
bm=bmesh.new();bm.from_mesh(face.data)
for v in bm.verts:
 p=face.matrix_world@v.co
 if p.z>1.49:
  taper=min(1,max(0,(p.z-1.49)/.10))
  v.co.x*=1-.045*taper
bad=[p for p in bm.faces if p.material_index not in [0,1,3,4] or all((face.matrix_world@v.co).z<1.455 for v in p.verts)]
bmesh.ops.delete(bm,geom=bad,context='FACES')
bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.00002)
bm.to_mesh(face.data);bm.free();face.data.update()
face.name='Beka_Head_Neck'
# Head bound to body head bone for this review draft; no facial animation claim.
for mod in list(face.modifiers):face.modifiers.remove(mod)
mw=face.matrix_world.copy();face.parent=rig;face.matrix_world=mw
face.vertex_groups.clear();g=face.vertex_groups.new(name='head');g.add(list(range(len(face.data.vertices))),1.,'REPLACE')
mod=face.modifiers.new('Body head attachment','ARMATURE');mod.object=rig
for o in list(bpy.data.objects):
 if o not in before and o.type!='MESH' and o!=face:bpy.data.objects.remove(o,do_unlink=True)
def mat(name,col,rough=.65,metal=0):
 m=bpy.data.materials.new(name);m.diffuse_color=(*col,1);m.use_nodes=True
 p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*col,1);p.inputs['Roughness'].default_value=rough;p.inputs['Metallic'].default_value=metal
 return m
skin=mat('Skin hands warm natural',(.42,.23,.15),.62)
body.data.materials.clear();body.data.materials.append(skin)
white=mat('Eye sclera',(.62,.59,.53),.25)
teeth=mat('Teeth warm enamel',(.65,.57,.43),.32)
# 4 camera projections, stored in native source resolution, not falsely called 4K.
images={}
for a in [0,90,180,270]:
 p=ROOT/f'assets/models/hero_custom/turnaround_synthetic_v1/images/hero_{a:03d}.png'
 images[a]=bpy.data.images.load(str(p),check_existing=True);images[a].pack()
def photo_mat(name,a,rough=.75):
 m=mat(name,(.2,.2,.2),rough);n=m.node_tree.nodes;p=n.get('Principled BSDF');im=n.new('ShaderNodeTexImage');im.image=images[a];im.interpolation='Linear'
 m.node_tree.links.new(im.outputs['Color'],p.inputs['Base Color'])
 return m
photo_skin={a:photo_mat('Head photo projection '+str(a),a,.65) for a in images}
photo_cloth={a:photo_mat('Cloth photo projection '+str(a),a,.86) for a in images}
def projection_uv(o,kind):
 o.data.update()
 uv=o.data.uv_layers.new(name='PhotoProjection') if not o.data.uv_layers else o.data.uv_layers.active
 mats=photo_skin if kind=='head' else photo_cloth
 # Preserve eye and teeth materials separately from head photos.
 original={p.index:p.material_index for p in o.data.polygons}
 o.data.materials.clear()
 for a in [0,90,180,270]:o.data.materials.append(mats[a])
 if kind=='head':o.data.materials.append(white);o.data.materials.append(teeth)
 matrix=o.matrix_world
 for p in o.data.polygons:
  n=(matrix.to_3x3()@p.normal).normalized()
  a=0 if -n.y>=abs(n.x) else 180 if n.y>=abs(n.x) else 90 if n.x>0 else 270
  p.material_index=[0,90,180,270].index(a)
  if kind=='head' and original[p.index] in [3,4]:p.material_index=4
  if kind=='head' and original[p.index]==1:p.material_index=5
  for li in p.loop_indices:
   co=matrix@o.data.vertices[o.data.loops[li].vertex_index].co
   horizontal=co.x if a==0 else -co.y if a==90 else -co.x if a==180 else co.y
   px=535+horizontal*750
   py=(1404 if kind=='head' else 1444)-co.z*750
   uv.data[li].uv=(px/1086,1-py/1448)
projection_uv(face,'head')
# Keep geometry trim and hardware. Add photographic woven detail to large cloth surfaces only.
for o in list(bpy.data.objects):
 if o.type!='MESH' or o in [body,face]:continue
 if o.name.startswith(('Full zip hoodie body','Connected olive trousers')):
  projection_uv(o,'cloth')
 if o.name.startswith('V chest seam'):
  for v in o.data.vertices:v.co.z+=.075
# Suppress legacy shin/foot material visibility by keeping only hands in anatomy above.
# Hair: a real volumetric cap and swept guide strands, not a flat photograph.
hairmat=mat('Black side swept hair',(.009,.008,.006),.46)
n=hairmat.node_tree.nodes;p=n.get('Principled BSDF')
noise=n.new('ShaderNodeTexNoise');noise.inputs['Scale'].default_value=170
bump=n.new('ShaderNodeBump');bump.inputs['Strength'].default_value=.22;bump.inputs['Distance'].default_value=.0005
hairmat.node_tree.links.new(noise.outputs['Fac'],bump.inputs['Height']);hairmat.node_tree.links.new(bump.outputs['Normal'],p.inputs['Normal'])
parts=[]
def bindhead(o):
 o.parent=rig;o.matrix_parent_inverse=rig.matrix_world.inverted()
 g=o.vertex_groups.new(name='head');g.add(list(range(len(o.data.vertices))),1,'REPLACE')
 m=o.modifiers.new('Head attachment','ARMATURE');m.object=rig;parts.append(o)
vs=[];fs=[];N=96;M=24
# phi0 front (negativeY), pi back. Hairline lower at back, fringe falls across forehead.
for j in range(M):
 t=j/(M-1)
 for k in range(N):
  ph=2*math.pi*k/N
  front=max(0,math.cos(ph));back=max(0,-math.cos(ph))
  boundary=1.42+.25*back+.07*front*(.5+.5*math.sin(ph))
  th=.025+t*boundary
  wave=.0015*math.sin(ph*10+t*8)*math.sin(th)
  x=(.088+wave)*math.sin(th)*math.sin(ph)-.005*(1-t)
  y=.005-(.096+wave)*math.sin(th)*math.cos(ph)
  z=1.664+(.112+wave)*math.cos(th)+.005*front*(1-t)
  vs.append((x,y,z))
for j in range(M-1):
 for k in range(N):fs.append((j*N+k,j*N+(k+1)%N,(j+1)*N+(k+1)%N,(j+1)*N+k))
me=bpy.data.meshes.new('Swept hair volume');me.from_pydata(vs,[],fs);me.update();cap=bpy.data.objects.new('Beka swept hair volume',me);bpy.context.collection.objects.link(cap);cap.data.materials.append(hairmat)
for p in cap.data.polygons:p.use_smooth=True
bindhead(cap)
def strand(name,pts,r):
 cu=bpy.data.curves.new(name,'CURVE');cu.dimensions='3D';cu.resolution_u=4;cu.bevel_depth=r;cu.bevel_resolution=1
 sp=cu.splines.new('NURBS');sp.points.add(len(pts)-1)
 for p,co in zip(sp.points,pts):p.co=(*co,1)
 sp.order_u=3;sp.use_endpoint_u=True
 o=bpy.data.objects.new(name,cu);bpy.context.collection.objects.link(o);o.data.materials.append(hairmat)
 bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o;bpy.ops.object.convert(target='MESH');bindhead(o)
# Front hair locks sweeping to subject-right (image-left).
for i in range(100):
 s=i/99;x0=-.068+.136*s
 pts=[]
 for j in range(10):
  t=j/9
  x=x0-.020*math.sin(math.pi*t)
  y=-.027-.070*t
  z=1.748+.028*math.sin(math.pi*t)-(.066+.012*(1-s))*t
  pts.append((x,y,z+random.uniform(-.001,.001)))
 strand('Swept fringe lock',pts,random.uniform(.0006,.00115))
for i in range(95):
 ph=random.uniform(-math.pi,math.pi);th0=random.uniform(.1,.8)
 pts=[]
 for j in range(8):
  t=j/7;th=th0+t*.52;ang=ph-.16*t
  pts.append((.090*math.sin(th)*math.sin(ang)-.003,.005-.099*math.sin(th)*math.cos(ang),1.664+.115*math.cos(th)+.003*math.sin(t*math.pi)))
 strand('Hair guide strand',pts,random.uniform(.0003,.0007))
# Separate physical dark-brown irises and pupils; the eye surfaces remain true 3D.
irisMat=mat('Natural dark brown iris',(.075,.030,.009),.30)
pupilMat=mat('Black pupil',(.001,.001,.0008),.18)
for side in [-1,1]:
 for name,yy,sc,ma in [('Iris',-.1107,(.0056,.0012,.0056),irisMat),('Pupil',-.1118,(.0024,.00065,.0024),pupilMat)]:
  bpy.ops.mesh.primitive_uv_sphere_add(segments=32,ring_count=16,location=(side*.0307,yy,1.637))
  o=bpy.context.object;o.name='Beka '+name+' '+str(side);o.scale=sc
  bpy.ops.object.transform_apply(location=True,rotation=False,scale=True);o.data.materials.append(ma)
  for p in o.data.polygons:p.use_smooth=True
  bindhead(o)
# Capture deformed geometry and make an unambiguous 175cm static review mesh.
# Review export has no claim of final retargeting/facial rig. Editable weighted source is retained separately.
depg=bpy.context.evaluated_depsgraph_get();export_objects=[]
for o in list(bpy.context.scene.objects):
 if o.type!='MESH':continue
 if o.hide_render:continue
 ev=o.evaluated_get(depg)
 me=bpy.data.meshes.new_from_object(ev,depsgraph=depg)
 new=bpy.data.objects.new(o.name+'_Review',me);bpy.context.collection.objects.link(new)
 new.matrix_world=o.matrix_world.copy()
 for p in new.data.polygons:p.use_smooth=True
 export_objects.append(new);o.hide_render=True;o.hide_set(True)
allcoords=[o.matrix_world@v.co for o in export_objects for v in o.data.vertices]
zmin=min(v.z for v in allcoords);zmax=max(v.z for v in allcoords);factor=1.75/(zmax-zmin)
for o in export_objects:
 world=o.matrix_world.copy()
 for v in o.data.vertices:
  p=world@v.co;p.z-=zmin;p*=factor;v.co=p
 o.matrix_world.identity()
# Bake procedural fabric values used by original materials into glTF-safe PBR constants.
for m in bpy.data.materials:
 if not m.use_nodes:continue
 p=m.node_tree.nodes.get('Principled BSDF')
 if p and 'Metallic' in p.inputs and p.inputs['Metallic'].default_value==0:p.inputs['Roughness'].default_value=max(.4,p.inputs['Roughness'].default_value)
bpy.ops.object.select_all(action='DESELECT')
for o in export_objects:o.select_set(True)
bpy.context.view_layer.objects.active=export_objects[0]
bpy.ops.export_scene.gltf(filepath=str(OUT/'Beka_reconstruction_draft_v1.glb'),use_selection=True,export_format='GLB',export_animations=False,export_apply=True)
bpy.ops.export_scene.fbx(filepath=str(OUT/'Beka_reconstruction_draft_v1.fbx'),use_selection=True,object_types={'MESH'},add_leaf_bones=False,bake_anim=False,axis_forward='-Z',axis_up='Y',path_mode='COPY',embed_textures=True)
# Native packed blend stores hidden fitting sources plus visible assembled model.
sc=bpy.context.scene
world=bpy.data.worlds.new('Neutral review studio');world.use_nodes=True;world.node_tree.nodes['Background'].inputs[0].default_value=(.15,.17,.19,1);world.node_tree.nodes['Background'].inputs[1].default_value=.4;sc.world=world
for name,pos,power,size in [('Key',(-2,-3,3.4),220,3),('Fill',(2,-2,2.4),150,3),('Rim',(0,2,2.5),240,2.5)]:
 bpy.ops.object.light_add(type='AREA',location=pos);o=bpy.context.object;o.name=name;o.data.energy=power;o.data.shape='DISK';o.data.size=size;o.rotation_euler=(Vector((0,0,1))-o.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(0,-3.5,.92));cam=bpy.context.object;cam.rotation_euler=(Vector((0,0,.88))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=2.05;sc.camera=cam
sc.render.engine='CYCLES';sc.cycles.samples=24;sc.cycles.use_denoising=True;sc.render.resolution_x=1080;sc.render.resolution_y=1440;sc.render.resolution_percentage=100;sc.render.image_settings.file_format='PNG';sc.render.film_transparent=False;sc.view_settings.view_transform='AgX'
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Beka_reconstruction_draft_v1.blend'))
report={'stage':'manual_draft','source_geometry':'Existing exported MetaHuman candidate and authored photo-specific outfit. Not a successful photogrammetry reconstruction.','model_height_m':1.75,'source_photo_resolution':'1086x1448','actual_native_4k_textures':False,'camera_projection':'Four approximate directional photo projections; seams and likeness need review.','animation_ready':False,'game_replaced':False,'vertices':sum(len(o.data.vertices) for o in export_objects),'triangles':sum(sum(len(p.vertices)-2 for p in o.data.polygons) for o in export_objects),'objects':len(export_objects),'images':[im.name for im in images.values()],'views':[]}
(REVIEW/'build_report.json').write_text(json.dumps(report,indent=2),encoding='utf8')
views=[('front',(0,-3.5,.92),.88,2.05),('three_quarter',(2.5,-3.3,1.0),.95,2.05),('back',(0,3.5,.92),.88,2.05),('face',(0,-2,1.62),1.62,.36)]
for name,pos,targetz,scale in views:
 cam.location=pos;cam.rotation_euler=(Vector((0,0,targetz))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=scale;sc.render.filepath=str(REVIEW/(name+'.png'));bpy.ops.render.render(write_still=True);report['views'].append(name+'.png');(REVIEW/'build_report.json').write_text(json.dumps(report,indent=2),encoding='utf8')
print('ECHO_MANUAL_DRAFT_DONE '+json.dumps(report))

