import bpy,bmesh,math,json,random,numpy as np
from pathlib import Path
from mathutils import Vector
R=Path('D:/Projects/ECHO');O=R/'artifacts/echo_hero_v38/likeness_head_v11';O.mkdir(exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(R/'artifacts/echo_hero_v38/likeness_geometry_v5/Beka_head_geometry_study_v5b.blend'))
bpy.context.preferences.filepaths.save_version=0
face=bpy.data.objects['Neutral_Source_Head']
for o in list(bpy.data.objects):
 if o.type=='MESH' and o!=face:bpy.data.objects.remove(o,do_unlink=True)
# World-space static mesh preserves the fitted geometry, avoiding dependence on the old facial armature.
deps=bpy.context.evaluated_depsgraph_get();me=bpy.data.meshes.new_from_object(face.evaluated_get(deps),depsgraph=deps)
mw=face.matrix_world.copy();head=bpy.data.objects.new('Beka_real_photo_fitted_head',me);bpy.context.collection.objects.link(head)
for v in head.data.vertices:v.co=mw@v.co
bpy.data.objects.remove(face,do_unlink=True)
for o in list(bpy.data.objects):
 if o.type=='ARMATURE':bpy.data.objects.remove(o,do_unlink=True)
rows=json.loads((R/'artifacts/echo_hero_v38/likeness_head_v6/texture_landmarks.json').read_text(encoding='utf8'))
src=np.array(rows['fitted_clay']['landmarks'],float)/1024
tex=np.array(rows['open_portrait']['landmarks'],float)/np.array([rows['open_portrait']['width'],rows['open_portrait']['height']])
ids=[10,338,297,332,284,251,389,356,454,323,361,288,397,365,379,378,400,377,152,148,176,149,150,136,172,58,132,93,234,127,162,21,54,103,67,109,
33,7,163,144,145,153,154,155,133,173,157,158,159,160,161,246,
263,249,390,373,374,380,381,382,362,398,384,385,386,387,388,466,
70,63,105,66,107,336,296,334,293,300,
168,6,197,195,5,4,1,2,98,97,99,64,49,48,115,220,45,275,440,344,278,279,294,326,327,328,329,290,250,460,
0,17,61,291,37,39,40,185,267,269,270,409,146,91,181,84,314,405,321,375,18,200,199,175]
ids=sorted(set(ids));C=src[ids];T=tex[ids]
# Explicit scalp/neck extrapolation guides are only texture placement, not new anatomy.
C=np.vstack((C,[[.50,.12],[.26,.27],[.74,.27],[.27,.94],[.73,.94]]))
T=np.vstack((T,[[.50,.05],[.17,.19],[.83,.19],[.20,.91],[.80,.91]]))
def kernel(x):
 r2=np.maximum(np.sum(x*x,axis=-1),1e-18);return r2*np.log(r2)
P=np.column_stack((np.ones(len(C)),C));A=np.block([[kernel(C[:,None]-C[None,:])+np.eye(len(C))*8e-6,P],[P.T,np.zeros((3,3))]])
coef=np.linalg.solve(A,np.vstack((T,np.zeros((3,2)))))
def source_uv(points):
 a=np.array(points);pixels=np.column_stack((.5+a[:,0]/.33,.5-(a[:,2]-1.62)/.33))
 out=kernel(pixels[:,None,:]-C[None,:,:])@coef[:len(C)]+np.column_stack((np.ones(len(pixels)),pixels))@coef[len(C):]
 return out
coords=np.array([tuple(v.co) for v in head.data.vertices]);projected=source_uv(coords)
# Fit the evaluated static mesh, avoiding an old facial rig shifting the fitted result.
# Camera scale and roll are registered by the two outer eye corners.
geomtex=tex.copy();geomtex[:,1]*=rows['open_portrait']['height']/rows['open_portrait']['width']
sid=src[[33,263]];tid=geomtex[[33,263]]
su=(sid[1]-sid[0]);tu=(tid[1]-tid[0]);su/=np.linalg.norm(su);tu/=np.linalg.norm(tu)
sf=np.stack([su,[-su[1],su[0]]],axis=1);tf=np.stack([tu,[-tu[1],tu[0]]],axis=1)
target=(geomtex-geomtex[[33,263]].mean(axis=0))@tf*(np.linalg.norm(sid[1]-sid[0])/np.linalg.norm(geomtex[263]-geomtex[33]))@sf.T+sid.mean(axis=0)
gc=src[ids];gd=(target-src)[ids]
# Forehead hidden by hair is not reliable anatomy. Suppress those controls.
for k,idx in enumerate(ids):
 if idx in [10,338,297,332,284,251,389,127,162,21,54,103,67,109]:gd[k]*=.2
gc=np.vstack([gc,[[.15,.2],[.85,.2],[.08,.9],[.92,.9],[.5,1.15]]]);gd=np.vstack([gd,np.zeros((5,2))])
gp=np.column_stack((np.ones(len(gc)),gc));ga=np.block([[kernel(gc[:,None]-gc[None,:])+np.eye(len(gc))*1e-4,gp],[gp.T,np.zeros((3,3))]])
gcoef=np.linalg.solve(ga,np.vstack([gd,np.zeros((3,2))]))
xy=np.column_stack([.5+coords[:,0]/.33,.5-(coords[:,2]-1.62)/.33])
delta=kernel(xy[:,None]-gc[None,:])@gcoef[:len(gc)]+np.column_stack([np.ones(len(xy)),xy])@gcoef[len(gc):]
skinpoly=[p.material_index for p in head.data.polygons]
for v,d in zip(head.data.vertices,delta):
 p=v.co;w=float(np.clip((.06-p.y)/.10,0,1)*np.clip((p.z-1.49)/.05,0,1))
 p.x+=float(np.clip(d[0]*.33,-.012,.012))*w;p.z-=float(np.clip(d[1]*.33,-.012,.012))*w
 # Reduce the pronounced projecting chin of the generic source in the real profile.
 chin=math.exp(-((p.z-1.535)/.023)**2-(p.x/.050)**4)*float(np.clip((-.025-p.y)/.055,0,1))
 p.y+=.0045*chin
head.data.update()

uv=head.data.uv_layers.new(name='PortraitProjection')
for p in head.data.polygons:
 for li in p.loop_indices:
  xy=projected[head.data.loops[li].vertex_index];uv.data[li].uv=(xy[0],1-xy[1])
photo=bpy.data.images.load(str(R/'artifacts/echo_hero_v38/likeness_head_v6/approved_portrait_eyes_open.png'),check_existing=True);photo.pack()
W,H=photo.size;pixels=np.array(photo.pixels[:]).reshape(H,W,4)
head.data.update()
blend=head.data.color_attributes.new(name='PortraitBlend',type='FLOAT_COLOR',domain='POINT')
for v in head.data.vertices:
 p=v.co;n=v.normal
 w=np.clip((-n.y-.03)/.60,0,1)*np.clip((.02-p.y)/.05,0,1)
 w*=np.clip((p.z-1.485)/.06,0,1)
 nosefade=math.exp(-((p.z-1.601)/.010)**4-(p.x/.023)**4)*float(np.clip((-p.y-.075)/.035,0,1))
 w*=1-.45*nosefade
 blend.data[v.index].color=(w,w,w,1)
def projection_material(name,base):
 m=bpy.data.materials.new(name);m.use_nodes=True;n=m.node_tree.nodes;links=m.node_tree.links;n.clear()
 out=n.new('ShaderNodeOutputMaterial');em=n.new('ShaderNodeEmission')
 im=n.new('ShaderNodeTexImage');im.image=photo;uvn=n.new('ShaderNodeUVMap');uvn.uv_map='PortraitProjection';links.new(uvn.outputs['UV'],im.inputs['Vector'])
 attr=n.new('ShaderNodeVertexColor');attr.layer_name='PortraitBlend';mix=n.new('ShaderNodeMixRGB');mix.inputs[1].default_value=(*base,1);links.new(attr.outputs['Color'],mix.inputs[0]);links.new(im.outputs['Color'],mix.inputs[2]);links.new(mix.outputs['Color'],em.inputs['Color']);links.new(em.outputs[0],out.inputs['Surface'])
 return m
skin=projection_material('Real portrait frontal color',(.48,.25,.15))
head.data.materials.clear();head.data.materials.append(skin)
for p in head.data.polygons:p.material_index=0

# A continuous scalp cap with a traced fringe boundary replaces the perforated color-threshold shell.
from mathutils.bvhtree import BVHTree
hairline=np.array([[.10,.43],[.16,.385],[.23,.365],[.30,.36],[.36,.34],[.42,.32],[.49,.296],[.55,.274],[.60,.28],[.66,.309],[.72,.333],[.79,.346],[.84,.332],[.9,.40]])
hairflag=[]
for v,xy in zip(head.data.vertices,projected):
 p=v.co
 boundary=float(np.interp(xy[0],hairline[:,0],hairline[:,1]))
 front_flag=p.z>1.642 and xy[1]<boundary
 back_flag=p.z>1.645
 side_flag=p.z>1.662
 hairflag.append(front_flag if p.y<-.035 else back_flag if p.y>.025 else side_flag)
polys=[p for p,mi in zip(head.data.polygons,skinpoly) if mi==0 and any(hairflag[i] for i in p.vertices)]
used=sorted({i for p in polys for i in p.vertices});lookup={v:i for i,v in enumerate(used)}
vs=[]
for idx in used:
 v=head.data.vertices[idx];p=v.co.copy();n=v.normal
 top=float(np.clip((p.z-1.66)/.085,0,1))
 lift=.006+.014*top
 # More crown volume and a fuller swept fringe on the reference's image-left side.
 lift+=.007*math.exp(-((p.x+.028)/.05)**2-((p.z-1.71)/.06)**2)*float(np.clip(-p.y/.07,0,1))
 ph=math.atan2(p.x,-p.y+.015)
 lift+=.0035*math.sin(ph*10+p.z*45)+.0018*math.sin(ph*19+p.z*80)
 p+=n*lift
 p.z-=.005*float(np.clip((p.z-1.70)/.075,0,1))
 vs.append(tuple(p))
hairme=bpy.data.meshes.new('Continuous fitted scalp cap');hairme.from_pydata(vs,[],[tuple(lookup[i] for i in p.vertices) for p in polys]);hairme.update()
hair=bpy.data.objects.new('Beka continuous swept scalp',hairme);bpy.context.collection.objects.link(hair)
for p in hairme.polygons:p.use_smooth=True
hairmat=bpy.data.materials.new('Dark chestnut black scalp');hairmat.use_nodes=True
hn=hairmat.node_tree.nodes;hl=hairmat.node_tree.links;hp=hn.get('Principled BSDF')
hp.inputs['Base Color'].default_value=(.002,.0014,.001,1);hp.inputs['Roughness'].default_value=.66;hp.inputs['Specular IOR Level'].default_value=.14
noise=hn.new('ShaderNodeTexNoise');noise.inputs['Scale'].default_value=720;noise.inputs['Detail'].default_value=2
bump=hn.new('ShaderNodeBump');bump.inputs['Strength'].default_value=.19;bump.inputs['Distance'].default_value=.00025;hl.new(noise.outputs['Fac'],bump.inputs['Height']);hl.new(bump.outputs['Normal'],hp.inputs['Normal'])
hairme.materials.append(hairmat)
# Hair is actual tapered strand geometry, following the cap; no photograph cards or floating plates.
capverts=[Vector(v) for v in vs];capfaces=[list(p.vertices) for p in hairme.polygons]
capbvh=BVHTree.FromPolygons(capverts,capfaces,all_triangles=False)
rng=random.Random(3807)
strandmat=bpy.data.materials.new('Black brown strands');strandmat.use_nodes=True
sp=strandmat.node_tree.nodes.get('Principled BSDF');sp.inputs['Base Color'].default_value=(.0038,.0025,.0015,1);sp.inputs['Roughness'].default_value=.42;sp.inputs['Specular IOR Level'].default_value=.18
cv=bpy.data.curves.new('Side swept short hair strands','CURVE');cv.dimensions='3D';cv.resolution_u=1;cv.bevel_depth=.000075;cv.bevel_resolution=0;cv.resolution_u=1;cv.materials.append(strandmat)
center=Vector((0,.015,1.659))
def shell(phi,theta):
 d=Vector((math.sin(theta)*math.sin(phi)*.089,-math.sin(theta)*math.cos(phi)*.115,math.cos(theta)*.115))
 q=center+d;hit=capbvh.find_nearest(q)
 if not hit[0]:return q,Vector((0,0,1))
 return hit[0],hit[1]
for i in range(4700):
 phi=rng.uniform(-math.pi,math.pi);theta=rng.uniform(.07,1.19)
 # Crown and front sweep toward image-left, with shorter sides and downwards back.
 front=math.cos(phi)>.10
 length=rng.uniform(.18,.42) if front else rng.uniform(.25,.5)
 points=[]
 for j in range(18):
  t=j/17;ang=theta+t*length
  ph=phi-(.95*t+.18*math.sin(t*math.pi)+.12*math.sin(t*math.pi*2)) if front else phi+.08*math.sin(t*math.pi)
  p,n=shell(ph,ang)
  loft=.0004+(.0015+rng.random()*.0006)*math.sin(t*math.pi)+.0003*math.sin(t*8+i)
  p+=n*loft;points.append(p)
 # Skip strands extending below the traced hair cap.
 if (points[-1]-points[-2]).length>.014:continue
 spl=cv.splines.new('POLY');spl.points.add(len(points)-1)
 for j,p in enumerate(points):spl.points[j].co=(*p,1);spl.points[j].radius=max(.10,math.sin((j/17*.85+.10)*math.pi))

# Long continuous curved fringe guides originate around the reference's off-centre part.
# They cover the lower front cap instead of ending halfway down the skull.
edgecounts0={}
for p in hairme.polygons:
 for e in p.edge_keys:edgecounts0[e]=edgecounts0.get(e,0)+1
ends=[]
for (a,b),count in edgecounts0.items():
 if count==1:
  va,vb=hairme.vertices[a],hairme.vertices[b]
  if (va.co.y+vb.co.y)<-.06 and 1.64<(va.co.z+vb.co.z)/2<1.71:ends.append((va.co.copy(),vb.co.copy()))
for i in range(7800):
 va,vb=rng.choice(ends);end=va.lerp(vb,rng.random());side=-1 if end.x<.018 else 1
 root=Vector((rng.uniform(.015,.04),rng.uniform(-.015,.02),rng.uniform(1.745,1.765)))
 c1=root+Vector((side*rng.uniform(.020,.040),-.030,.008))
 c2=end+Vector((-side*rng.uniform(.018,.035),.024,.025))
 spl=cv.splines.new('POLY');spl.points.add(25)
 for j in range(26):
  t=j/25;u=1-t;q=u*u*u*root+3*u*u*t*c1+3*u*t*t*c2+t*t*t*end
  hit=capbvh.find_nearest(q);p=hit[0].copy();n=hit[1]
  p+=n*(.0005+.002*math.sin(t*math.pi)+.0005*math.sin(t*10+i*.13))
  if t>.94:p+=Vector((side*.001,0,-.001))*((t-.94)/.06)
  spl.points[j].co=(*p,1);spl.points[j].radius=max(.06,math.sin((t*.87+.08)*math.pi))
# Smooth only the polygonal cap boundary; the visible silhouette comes from the strand tips.
sub=hair.modifiers.new('Soft scalp boundary','SUBSURF');sub.levels=1
bpy.ops.object.select_all(action='DESELECT');hair.select_set(True);bpy.context.view_layer.objects.active=hair;bpy.ops.object.modifier_apply(modifier=sub.name)

strands=bpy.data.objects.new('Beka hair individual strands',cv);bpy.context.collection.objects.link(strands)
# Loosen the edge silhouette with fine, irregular tapered fringe and sideburn strands.
edgecounts={}
for p in hairme.polygons:
 for e in p.edge_keys:edgecounts[e]=edgecounts.get(e,0)+1
for (a,b),count in edgecounts.items():
 if count!=1:continue
 va,vb=hairme.vertices[a],hairme.vertices[b]
 if (va.co.z+vb.co.z)/2>1.705:continue
 for k in range(8):
  t=rng.random();root=va.co.lerp(vb.co,t);n=va.normal.lerp(vb.normal,t).normalized()
  down=Vector((-.45 if root.y<-.02 else 0,-.18 if root.y<-.02 else .05,-1)).normalized()
  spl=cv.splines.new('POLY');spl.points.add(8)
  le=rng.uniform(.0015,.004)
  for j in range(9):
   q=j/8;p=root+down*(le*q)+n*(.0007+.0005*math.sin(q*math.pi));p.x+=.0008*math.sin(q*4+k)
   spl.points[j].co=(*p,1);spl.points[j].radius=max(.04,1-q)
# Keep the scalp and strands as PBR geometry, independent from facial albedo baking.
# Join only actual head/hair surfaces and bake portable albedo without altering source photographs.
bpy.ops.object.select_all(action='DESELECT');head.select_set(True);bpy.context.view_layer.objects.active=head;bpy.ops.object.join()
head.name='Beka_head_likeness_candidate_v11'
head.data.update();bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT')
bakeuv=head.data.uv_layers.new(name='BakedHeadUV');head.data.uv_layers.active_index=len(head.data.uv_layers)-1
bpy.ops.uv.smart_project(angle_limit=1.12,island_margin=.005);bpy.ops.object.mode_set(mode='OBJECT')
atlas=bpy.data.images.new('Beka_v11_albedo_2048',width=2048,height=2048,alpha=False)
for m in head.data.materials:
 im=m.node_tree.nodes.new('ShaderNodeTexImage');im.image=atlas;m.node_tree.nodes.active=im
sc=bpy.context.scene;sc.render.engine='CYCLES';sc.cycles.samples=1;sc.render.bake.margin=10;bpy.ops.object.bake(type='EMIT')
atlas.filepath_raw=str(O/'Beka_v11_albedo_2048.png');atlas.file_format='PNG';atlas.save();atlas.pack()
mat=bpy.data.materials.new('Beka baked review skin and hair');mat.use_nodes=True;n=mat.node_tree.nodes
im=n.new('ShaderNodeTexImage');im.image=atlas;uvn=n.new('ShaderNodeUVMap');uvn.uv_map='BakedHeadUV';mat.node_tree.links.new(uvn.outputs['UV'],im.inputs['Vector']);mat.node_tree.links.new(im.outputs['Color'],n.get('Principled BSDF').inputs['Base Color']);n.get('Principled BSDF').inputs['Roughness'].default_value=.78
n.get('Principled BSDF').inputs['Specular IOR Level'].default_value=.22
n.get('Principled BSDF').inputs['Subsurface Weight'].default_value=.055
head.data.materials.clear();head.data.materials.append(mat)
for p in head.data.polygons:p.material_index=0;p.use_smooth=True
for layer in list(head.data.uv_layers):
 if layer.name!='BakedHeadUV':head.data.uv_layers.remove(layer)
head.data.uv_layers.active_index=0
bpy.ops.object.select_all(action='DESELECT');head.select_set(True);bpy.context.view_layer.objects.active=head
hair.select_set(True);strands.select_set(True)
bpy.context.view_layer.objects.active=strands;bpy.ops.object.convert(target='MESH');strands=bpy.context.object;bpy.context.view_layer.objects.active=head
bpy.ops.export_scene.gltf(filepath=str(O/'Beka_head_likeness_candidate_v11.glb'),use_selection=True,export_format='GLB',export_apply=True,export_animations=False)
cam=sc.camera;cam.data.ortho_scale=.36;sc.render.resolution_x=1024;sc.render.resolution_y=1024;sc.cycles.samples=24
sc.view_settings.view_transform='AgX'
bpy.ops.wm.save_as_mainfile(filepath=str(O/'Beka_head_likeness_candidate_v11.blend'))
report={'status':'likeness_candidate_not_approved','head_only':True,'geometry':'Fitted neutral geometry v5b based on real frontal photo and profile estimates','texture':'Approved enlarged portrait, eyes-open image_gen edit, landmark-aligned to the rendered geometry','texture_landmark_controls':len(ids),'hair_shell_faces':len(polys),'source_portrait_resolution':[W,H],'albedo_atlas_resolution':[2048,2048],'game_changed':False,'face_likeness_verified':False,'views':[]}
(O/'build_report.json').write_text(json.dumps(report,indent=2),encoding='utf8')
for name,pos in [('front',(0,-1,1.63)),('three_quarter',(-.65,-1,1.63)),('profile',(-1,0,1.63)),('back',(0,1,1.63))]:
 cam.location=pos;cam.rotation_euler=(Vector((0,0,1.63))-cam.location).to_track_quat('-Z','Y').to_euler();sc.render.filepath=str(O/(name+'.png'));bpy.ops.render.render(write_still=True);report['views'].append(name+'.png');(O/'build_report.json').write_text(json.dumps(report,indent=2),encoding='utf8')
print('HEAD_LIKENESS_V6_RENDERED')

