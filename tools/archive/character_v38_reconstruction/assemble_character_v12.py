import bpy,bmesh,math,json,random
from pathlib import Path
from mathutils import Vector
from mathutils.bvhtree import BVHTree
R=Path('D:/Projects/ECHO');O=R/'artifacts/echo_hero_v38/character_candidate_v12';O.mkdir(exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(R/'artifacts/echo_hero_v38/likeness_head_v11/Beka_head_likeness_candidate_v11.blend'))
bpy.context.preferences.filepaths.save_version=0
headparts=[o for o in bpy.context.scene.objects if o.type=='MESH']
head=next(o for o in headparts if 'likeness_candidate' in o.name)
bm=bmesh.new();bm.from_mesh(head.data)
bmesh.ops.delete(bm,geom=[p for p in bm.faces if all(v.co.z<1.485 for v in p.verts)],context='FACES')
bm.to_mesh(head.data);bm.free()
for o in headparts:
 for v in o.data.vertices:v.co*=.985
before=set(bpy.data.objects)
bpy.ops.import_scene.gltf(filepath=str(R/'assets/models/hero_custom/beka_reconstruction_draft_v4/Beka_reconstruction_draft_v4.glb'))
old=[o for o in bpy.data.objects if o not in before and o.type=='MESH']
for o in list(old):
 if o.name in ['Beka_Head','Beka_Hair','Beka_Eyes']:
  old.remove(o);bpy.data.objects.remove(o,do_unlink=True)
for o in old:
 bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o;bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
hood=next(o for o in old if o.name=='Beka_Hoodie');pants=next(o for o in old if o.name=='Beka_Trousers')
hands=next(o for o in old if o.name.startswith('Beka_Hands'))
# Remove exposed belt-loop islands at the hem and repair the trouser surface as a closed volume.
bm=bmesh.new();bm.from_mesh(pants.data)
drop=[]
for p in bm.faces:
 mat=pants.data.materials[p.material_index]
 if 'rib' in mat.name.lower() and all(v.co.z>.84 for v in p.verts):drop.append(p)
bmesh.ops.delete(bm,geom=drop,context='FACES');bm.to_mesh(pants.data);bm.free()
bpy.ops.object.select_all(action='DESELECT');pants.select_set(True);bpy.context.view_layer.objects.active=pants
rem=pants.modifiers.new('Repair trouser crotch and seam openings','REMESH');rem.mode='VOXEL';rem.voxel_size=.003;rem.use_smooth_shade=True;bpy.ops.object.modifier_apply(modifier=rem.name)
sm=pants.modifiers.new('Relax repeated diagonal bands','SMOOTH');sm.factor=.60;sm.iterations=20;bpy.ops.object.modifier_apply(modifier=sm.name)
pants.data.update()
print('PANTS_REMESH_READY '+str(len(pants.data.vertices)),flush=True)
pnormals=[v.normal.copy() for v in pants.data.vertices]
for v,n in zip(pants.data.vertices,pnormals):
 p=v.co
 # Broad irregular compression folds around knees, pockets and rolled cuffs.
 fold=.0022*math.sin(p.z*112+p.x*19+p.y*13)*math.exp(-((p.z-.48)/.12)**2)
 fold+=.0017*math.sin(p.z*153-p.x*35)*math.exp(-((p.z-.19)/.06)**2)
 fold+=.0008*math.sin(p.z*92+p.x*22)*math.exp(-((p.z-.79)/.08)**2)
 v.co+=n*fold
for p in pants.data.polygons:p.use_smooth=True
# Give the folded hood depth behind the neck. Keep the full zip and reference's chest tape.
hoodlining={i for i,m in enumerate(hood.data.materials) if 'Hood lining' in m.name}
hoodverts={i for p in hood.data.polygons if p.material_index in hoodlining for i in p.vertices}
for idx in hoodverts:
 p=hood.data.vertices[idx].co
 if p.y>.025:
  w=float(max(0,min(1,(1.51-p.z)/.12)));p.y+=.022*w;p.z-=.035*w
hood.data.update()
clothverts={i for p in hood.data.polygons if 'fleece' in hood.data.materials[p.material_index].name.lower() for i in p.vertices}
hnormals=[v.normal.copy() for v in hood.data.vertices]
for idx in clothverts:
 v=hood.data.vertices[idx];p=v.co;n=hnormals[idx]
 waist=math.exp(-((p.z-.99)/.07)**2)*math.exp(-(p.x/.2)**6)
 elbow=math.exp(-((abs(p.x)-.37)/.06)**2-((p.z-1.13)/.1)**2)
 d=.0018*waist*math.sin(p.z*140+p.x*18)+.002*elbow*math.sin(p.z*130+p.x*24)
 v.co+=n*d

def simplemat(name,color,rough=.8):
 m=bpy.data.materials.new(name);m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*color,1);p.inputs['Roughness'].default_value=rough;return m
thread=simplemat('Low contrast sewn thread',(.022,.024,.020));olive=simplemat('Olive patch pockets',(.027,.029,.020))
def bvh(o):return BVHTree.FromPolygons([v.co.copy() for v in o.data.vertices],[list(p.vertices) for p in o.data.polygons],all_triangles=False)
pv=bvh(pants);hv=bvh(hood)
details=[]
def surf(vtree,x,z,back=False,offset=.001):
 hit=vtree.ray_cast(Vector((x,.5 if back else -.5,z)),Vector((0,-1 if back else 1,0)),2)
 if hit[0]:return hit[0]+hit[1]*offset
 raise RuntimeError('Garment projection missed '+str((x,z,back)))
def line(name,points,mat,r=.00035):
 c=bpy.data.curves.new(name,'CURVE');c.dimensions='3D';c.bevel_depth=r;c.bevel_resolution=1
 sp=c.splines.new('POLY');sp.points.add(len(points)-1)
 for v,p in zip(sp.points,points):v.co=(*p,1)
 ob=bpy.data.objects.new(name,c);bpy.context.collection.objects.link(ob);ob.data.materials.append(mat)
 bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);bpy.context.view_layer.objects.active=ob;bpy.ops.object.convert(target='MESH');details.append(bpy.context.object);return bpy.context.object
for side in [-1,1]:
 cx=side*.099
 outline=[(cx-.032,.858),(cx+.032,.858),(cx+.030,.791),(cx,.780),(cx-.030,.791)]
 points=[surf(pv,x,z,True,.0015) for x,z in outline]
 me=bpy.data.meshes.new('Sewn rear pocket patch');me.from_pydata([tuple(p) for p in points],[],[tuple(range(5))]);me.update()
 ob=bpy.data.objects.new('Rear trouser pocket '+str(side),me);bpy.context.collection.objects.link(ob);me.materials.append(olive);details.append(ob)
 line('Rear pocket stitching '+str(side),points+[points[0]],thread)
 # Outer seams follow actual legs rather than straight floating lines.
 seam=[]
 for j in range(55):
  z=.20+j/54*.66;x=side*(.14+.04*(z-.20)/.66)
  hit=pv.find_nearest(Vector((x,0,z)))
  if hit[0]:seam.append(hit[0]+hit[1]*.0009)
 line('Outer trouser seam '+str(side),seam,thread,.0003)
# Small chest mark on the wearer's left, following the fabric surface.
swoosh=[(.092,1.328),(.096,1.320),(.104,1.318),(.128,1.327),(.101,1.324),(.097,1.326)]
pts=[surf(hv,x,z,False,.0012) for x,z in swoosh]
me=bpy.data.meshes.new('Chest mark');me.from_pydata([tuple(p) for p in pts],[],[tuple(range(len(pts)))]);me.update()
ob=bpy.data.objects.new('Reference chest mark',me);bpy.context.collection.objects.link(ob);me.materials.append(thread);details.append(ob)
# Knit and woven microstructure are native 3D material nodes, then baked for portable exports.
for ob in [hood,pants]:
 for mat in ob.data.materials:
  if not mat or not mat.use_nodes:continue
  p=mat.node_tree.nodes.get('Principled BSDF')
  if not p or p.inputs['Metallic'].default_value>.1:continue
  nodes=mat.node_tree.nodes;links=mat.node_tree.links
  tex=nodes.new('ShaderNodeTexCoord');noise=nodes.new('ShaderNodeTexNoise');noise.inputs['Scale'].default_value=460;noise.inputs['Detail'].default_value=2;links.new(tex.outputs['Generated'],noise.inputs['Vector'])
  bump=nodes.new('ShaderNodeBump');bump.inputs['Strength'].default_value=.26;bump.inputs['Distance'].default_value=.00018;links.new(noise.outputs['Fac'],bump.inputs['Height']);links.new(bump.outputs['Normal'],p.inputs['Normal'])
  base=p.inputs['Base Color'].default_value[:]
  ramp=nodes.new('ShaderNodeValToRGB');ramp.color_ramp.elements[0].color=tuple(c*.72 for c in base[:3])+(1,);ramp.color_ramp.elements[1].color=tuple(c*1.15 for c in base[:3])+(1,)
  links.new(noise.outputs['Fac'],ramp.inputs[0]);links.new(ramp.outputs['Color'],p.inputs['Base Color']);p.inputs['Roughness'].default_value=.88
# Texture bake uses each garment's atlas; original material roughness and hardware remain separate.
sc=bpy.context.scene;sc.render.engine='CYCLES';sc.cycles.samples=1;sc.render.bake.margin=10
baked=[]
for ob in [hood,pants]:
 print('BAKING_GARMENT '+ob.name,flush=True)
 bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);bpy.context.view_layer.objects.active=ob
 uv=ob.data.uv_layers.new(name='GarmentDetailUV');ob.data.uv_layers.active_index=len(ob.data.uv_layers)-1
 bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.smart_project(angle_limit=1.1,island_margin=.006);bpy.ops.object.mode_set(mode='OBJECT')
 al=bpy.data.images.new(ob.name+'_albedo_2048',width=2048,height=2048,alpha=False)
 no=bpy.data.images.new(ob.name+'_normal_2048',width=2048,height=2048,alpha=False);no.colorspace_settings.name='Non-Color'
 restore=[]
 for m in ob.data.materials:
  if not m:continue
  n=m.node_tree.nodes;l=m.node_tree.links;out=n.get('Material Output');bs=n.get('Principled BSDF');em=n.new('ShaderNodeEmission')
  if bs.inputs['Base Color'].is_linked:l.new(bs.inputs['Base Color'].links[0].from_socket,em.inputs[0])
  else:em.inputs[0].default_value=bs.inputs['Base Color'].default_value
  oldsock=out.inputs['Surface'].links[0].from_socket;restore.append((m,oldsock,em));l.new(em.outputs[0],out.inputs[0])
  target=n.new('ShaderNodeTexImage');target.image=al;n.active=target
 bpy.ops.object.bake(type='EMIT')
 al.filepath_raw=str(O/(ob.name+'_albedo_2048.png'));al.file_format='PNG';al.save();al.pack()
 for m,sock,em in restore:
  m.node_tree.links.new(sock,m.node_tree.nodes.get('Material Output').inputs[0]);m.node_tree.nodes.remove(em)
  ti=m.node_tree.nodes.new('ShaderNodeTexImage');ti.image=no;m.node_tree.nodes.active=ti
 bpy.ops.object.bake(type='NORMAL')
 no.filepath_raw=str(O/(ob.name+'_normal_2048.png'));no.file_format='PNG';no.save();no.pack()
 for m in ob.data.materials:
  n=m.node_tree.nodes;l=m.node_tree.links;bs=n.get('Principled BSDF');u=n.new('ShaderNodeUVMap');u.uv_map='GarmentDetailUV'
  ai=n.new('ShaderNodeTexImage');ai.image=al;l.new(u.outputs[0],ai.inputs[0]);l.new(ai.outputs['Color'],bs.inputs['Base Color'])
  ni=n.new('ShaderNodeTexImage');ni.image=no;l.new(u.outputs[0],ni.inputs[0]);nm=n.new('ShaderNodeNormalMap');l.new(ni.outputs['Color'],nm.inputs['Color']);l.new(nm.outputs['Normal'],bs.inputs['Normal'])
 for layer in list(ob.data.uv_layers):
  if layer.name!='GarmentDetailUV':ob.data.uv_layers.remove(layer)
 baked.extend([al.name,no.name])
objects=[o for o in sc.objects if o.type=='MESH']
zs=[(o.matrix_world@v.co).z for o in objects for v in o.data.vertices];fac=1.75/(max(zs)-min(zs));bottom=min(zs)
for ob in objects:
 mw=ob.matrix_world.copy()
 for v in ob.data.vertices:
  p=mw@v.co;p.z-=bottom;v.co=p*fac
 ob.matrix_world.identity()
bpy.ops.object.select_all(action='DESELECT')
for o in objects:o.select_set(True)
bpy.context.view_layer.objects.active=head
sc.camera.data.type='ORTHO';sc.cycles.samples=24;sc.render.resolution_x=900;sc.render.resolution_y=1200;sc.render.resolution_percentage=100
sc.render.image_settings.file_format='PNG'
bpy.ops.wm.save_as_mainfile(filepath=str(O/'Beka_character_candidate_v12.blend'))
bpy.ops.export_scene.gltf(filepath=str(O/'Beka_character_candidate_v12.glb'),use_selection=True,export_format='GLB',export_apply=True,export_animations=False)
report={'status':'candidate_not_approved','height_m':1.75,'game_replaced':False,'rigged':False,'likeness_verified':False,'source_head':'likeness_head_v11','outfit_changes':['Repaired trouser volume and removed artificial repeat folds','Added rear patch pockets and stitched seams','Deepened folded hood','Added cloth microstructure and portable baked normals','Added small reference chest mark'],'baked_images':baked,'views':[]}
for name,pos,z,scale in [('front',(0,-3,1.0),.88,2.02),('three_quarter',(-2,-3,1.0),.88,2.02),('back',(0,3,1.0),.88,2.02),('face',(0,-1,1.62),1.62,.34)]:
 cam=sc.camera;cam.data.ortho_scale=scale;cam.location=pos;cam.rotation_euler=(Vector((0,0,z))-cam.location).to_track_quat('-Z','Y').to_euler();sc.render.filepath=str(O/(name+'.png'));bpy.ops.render.render(write_still=True);report['views'].append(name+'.png');(O/'build_report.json').write_text(json.dumps(report,indent=2),encoding='utf8')
print('CHARACTER_V12_REVIEW_RENDERED')
