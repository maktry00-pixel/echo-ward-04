import bpy,bmesh,math,json
from pathlib import Path
from mathutils import Vector
from mathutils.bvhtree import BVHTree
R=Path('D:/Projects/ECHO');O=R/'artifacts/echo_hero_v38/character_candidate_v15';O.mkdir(exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(R/'artifacts/echo_hero_v38/character_candidate_v13/Beka_character_candidate_v13.blend'))
bpy.context.preferences.filepaths.save_version=0
for ob in list(bpy.data.objects):
 if ob.type=='MESH' and (ob.name=='Beka_Trousers' or ob.name.startswith(('Rear trouser pocket','Rear pocket stitching','Outer trouser seam'))):bpy.data.objects.remove(ob,do_unlink=True)
pieces=[]
def loft(name,rows,side=0,n=72):
 vs=[];fs=[]
 for z,rx,ry,cx in rows:
  for i in range(n):
   a=2*math.pi*i/n;vs.append((side*cx+rx*math.cos(a),ry*math.sin(a),z))
 for k in range(len(rows)-1):
  for i in range(n):fs.append((k*n+i,k*n+(i+1)%n,(k+1)*n+(i+1)%n,(k+1)*n+i))
 fs.append(tuple(range(n-1,-1,-1)));fs.append(tuple((len(rows)-1)*n+i for i in range(n)))
 me=bpy.data.meshes.new(name);me.from_pydata(vs,[],fs);me.update();ob=bpy.data.objects.new(name,me);bpy.context.collection.objects.link(ob)
 bm=bmesh.new();bm.from_mesh(me);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(me);bm.free();pieces.append(ob);return ob
legs=[(.132,.066,.073,.117),(.165,.068,.075,.116),(.22,.069,.077,.114),(.32,.071,.079,.111),(.43,.073,.080,.109),(.54,.074,.083,.107),(.65,.079,.085,.103),(.74,.085,.088,.098),(.83,.092,.090,.093),(.88,.093,.092,.093)]
for side in [-1,1]:loft('Closed trouser leg '+str(side),legs,side)
loft('Closed trouser hip',[(.747,.006,.008,0),(.752,.030,.035,0),(.76,.065,.063,0),(.77,.090,.079,0),(.79,.130,.094,0),(.81,.156,.103,0),(.84,.178,.108,0),(.87,.181,.109,0),(.90,.180,.107,0),(.933,.174,.102,0)])
bpy.ops.object.select_all(action='DESELECT')
for o in pieces:o.select_set(True)
bpy.context.view_layer.objects.active=pieces[0];bpy.ops.object.join();pants=bpy.context.object;pants.name='Beka_Trousers'
rem=pants.modifiers.new('Union closed leg and hip volumes','REMESH');rem.mode='VOXEL';rem.voxel_size=.003;rem.use_smooth_shade=True;bpy.ops.object.modifier_apply(modifier=rem.name)
sm=pants.modifiers.new('Blend real crotch volume','SMOOTH');sm.factor=.5;sm.iterations=12;bpy.ops.object.modifier_apply(modifier=sm.name)
pants.data.update();norms=[v.normal.copy() for v in pants.data.vertices]
for v,n in zip(pants.data.vertices,norms):
 p=v.co;side=1 if p.x>=0 else -1;cx=side*(.117-(.117-.093)*max(0,min(1,(p.z-.13)/.7)));a=math.atan2(p.y,p.x-cx)
 d=.0019*math.sin(p.z*105+a*2.3)*math.exp(-((p.z-.48)/.10)**2)
 d+=.0023*math.sin(p.z*142+a*2)*math.exp(-((p.z-.20)/.045)**2)
 d+=.0011*math.sin(p.z*95-a*2)*math.exp(-((p.z-.75)/.06)**2)
 p+=n*d
for p in pants.data.polygons:p.use_smooth=True
pants.data.update()
bm=bmesh.new();bm.from_mesh(pants.data);boundary=sum(e.is_boundary for e in bm.edges);nonmanifold=sum(not e.is_manifold for e in bm.edges);bm.free();print('NEW_TROUSERS_TOPOLOGY',len(pants.data.vertices),boundary,nonmanifold,flush=True)
uv=pants.data.uv_layers.new(name='TrouserCylindricalUV');uv.active_render=True
for p in pants.data.polygons:
 side=1 if p.center.x>=0 else -1;vals=[]
 for idx in p.vertices:
  co=pants.data.vertices[idx].co;cx=side*(.117-.024*max(0,min(1,(co.z-.13)/.7)));vals.append((math.atan2(co.y,co.x-cx)+math.pi)/(2*math.pi))
 if max(vals)-min(vals)>.5:vals=[x+1 if x<.5 else x for x in vals]
 for li,x in zip(p.loop_indices,vals):
  co=pants.data.vertices[pants.data.loops[li].vertex_index].co;uv.data[li].uv=(.012+(.5 if side==1 else 0)+x*.475,.025+(co.z-.10)/.86*.95)
m=bpy.data.materials.new('Olive woven cloth v15');m.use_nodes=True;n=m.node_tree.nodes;l=m.node_tree.links;bs=n.get('Principled BSDF');bs.inputs['Roughness'].default_value=.88
tc=n.new('ShaderNodeTexCoord');noise=n.new('ShaderNodeTexNoise');noise.inputs['Scale'].default_value=480;noise.inputs['Detail'].default_value=2;l.new(tc.outputs['Generated'],noise.inputs[0])
mix=n.new('ShaderNodeMixRGB');mix.inputs[1].default_value=(.024,.025,.017,1);mix.inputs[2].default_value=(.034,.035,.024,1);l.new(noise.outputs['Fac'],mix.inputs[0]);l.new(mix.outputs[0],bs.inputs['Base Color'])
bump=n.new('ShaderNodeBump');bump.inputs['Distance'].default_value=.00010;bump.inputs['Strength'].default_value=.18;l.new(noise.outputs['Fac'],bump.inputs['Height']);l.new(bump.outputs[0],bs.inputs['Normal']);pants.data.materials.append(m)
bpy.ops.object.select_all(action='DESELECT');pants.select_set(True);bpy.context.view_layer.objects.active=pants;sc=bpy.context.scene;sc.cycles.samples=1
al=bpy.data.images.new('Trouser_albedo_v15',width=2048,height=2048,alpha=False);no=bpy.data.images.new('Trouser_normal_v15',width=2048,height=2048,alpha=False);no.colorspace_settings.name='Non-Color'
out=n.get('Material Output');em=n.new('ShaderNodeEmission');l.new(mix.outputs[0],em.inputs[0]);l.new(em.outputs[0],out.inputs[0]);ti=n.new('ShaderNodeTexImage');ti.image=al;n.active=ti;bpy.ops.object.bake(type='EMIT');al.filepath_raw=str(O/'Trouser_albedo_2048.png');al.file_format='PNG';al.save();al.pack()
l.new(bs.outputs[0],out.inputs[0]);n.remove(em);ni=n.new('ShaderNodeTexImage');ni.image=no;n.active=ni;bpy.ops.object.bake(type='NORMAL');no.filepath_raw=str(O/'Trouser_normal_2048.png');no.file_format='PNG';no.save();no.pack()
u=n.new('ShaderNodeUVMap');u.uv_map='TrouserCylindricalUV';ai=n.new('ShaderNodeTexImage');ai.image=al;l.new(u.outputs[0],ai.inputs[0]);l.new(ai.outputs[0],bs.inputs['Base Color']);l.new(u.outputs[0],ni.inputs[0]);nm=n.new('ShaderNodeNormalMap');l.new(ni.outputs[0],nm.inputs['Color']);l.new(nm.outputs[0],bs.inputs['Normal'])
# Rear pockets fit the repaired surface, with closed thickness and low contrast stitching.
pv=BVHTree.FromPolygons([v.co.copy() for v in pants.data.vertices],[list(p.vertices) for p in pants.data.polygons],all_triangles=False)
pm=bpy.data.materials.new('Rear pocket olive');pm.use_nodes=True;pm.node_tree.nodes.get('Principled BSDF').inputs['Base Color'].default_value=(.027,.029,.020,1)

for side in [-1,1]:
 cx=side*.096;points=[];faces=[];nx=8;nz=9
 for j in range(nz):
  t=j/(nz-1)
  for i in range(nx):
   u=-1+2*i/(nx-1);x=cx+u*.034;bottom=.785+.009*u*u;z=bottom+(.858-bottom)*t
   hit=pv.ray_cast(Vector((x,.5,z)),Vector((0,-1,0)),1);points.append(tuple(hit[0]+hit[1]*.0009))
 for j in range(nz-1):
  for i in range(nx-1):
   q=j*nx+i;faces.append((q,q+1,q+1+nx,q+nx))
 me=bpy.data.meshes.new('Surface fitted sewn pocket');me.from_pydata(points,[],faces);me.update();ob=bpy.data.objects.new('Rear fitted pocket '+str(side),me);bpy.context.collection.objects.link(ob);me.materials.append(pm)
 for p in me.polygons:p.use_smooth=True
objects=[o for o in sc.objects if o.type=='MESH'];bpy.ops.object.select_all(action='DESELECT')
for ob in objects:ob.select_set(True)
bpy.context.view_layer.objects.active=pants
bpy.ops.wm.save_as_mainfile(filepath=str(O/'Beka_character_candidate_v15.blend'))
bpy.ops.export_scene.gltf(filepath=str(O/'Beka_character_candidate_v15.glb'),use_selection=True,export_format='GLB',export_apply=True,export_animations=False)
report={'status':'candidate_not_approved','likeness_verified':False,'game_changed':False,'rigged':False,'pants_boundary_edges':boundary,'pants_nonmanifold_edges':nonmanifold,'views':[]}
sc.render.resolution_x=900;sc.render.resolution_y=1200;sc.cycles.samples=24
for name,pos,z,scale in [('front',(0,-3,1.0),.88,2.02),('three_quarter',(-2,-3,1.0),.88,2.02),('back',(0,3,1.0),.88,2.02),('face',(0,-1,1.62),1.62,.34)]:
 cam=sc.camera;cam.data.ortho_scale=scale;cam.location=pos;cam.rotation_euler=(Vector((0,0,z))-cam.location).to_track_quat('-Z','Y').to_euler();sc.render.filepath=str(O/(name+'.png'));bpy.ops.render.render(write_still=True);report['views'].append(name+'.png');(O/'status.json').write_text(json.dumps(report,indent=2),encoding='utf8')
print('V14_CLOSED_TROUSERS_COMPLETE')
