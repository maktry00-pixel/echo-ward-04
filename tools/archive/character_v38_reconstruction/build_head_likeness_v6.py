import bpy,bmesh,math,json,random,numpy as np
from pathlib import Path
from mathutils import Vector
R=Path('D:/Projects/ECHO');O=R/'artifacts/echo_hero_v38/likeness_head_v6';O.mkdir(exist_ok=True)
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
rows=json.loads((O/'texture_landmarks.json').read_text(encoding='utf8'))
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
uv=head.data.uv_layers.new(name='PortraitProjection')
for p in head.data.polygons:
 for li in p.loop_indices:
  xy=projected[head.data.loops[li].vertex_index];uv.data[li].uv=(xy[0],1-xy[1])
photo=bpy.data.images.load(str(O/'approved_portrait_eyes_open.png'),check_existing=True);photo.pack()
W,H=photo.size;pixels=np.array(photo.pixels[:]).reshape(H,W,4)
head.data.update()
blend=head.data.color_attributes.new(name='PortraitBlend',type='FLOAT_COLOR',domain='POINT')
for v in head.data.vertices:
 p=v.co;n=v.normal
 w=np.clip((-n.y-.03)/.60,0,1)*np.clip((.02-p.y)/.05,0,1)
 w*=np.clip((p.z-1.485)/.06,0,1)
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
# Hair shell follows the actual fitted scalp, with a boundary estimated from the portrait texture.
hairflag=[]
for v,xy in zip(head.data.vertices,projected):
 p=v.co
 ix=int(np.clip(xy[0]*W,0,W-1));iy=int(np.clip((1-xy[1])*H,0,H-1));color=pixels[iy,ix,:3]
 dark=float(np.mean(color))<.29
 front=p.y<-.02
 flag=(p.z>1.646 and dark and xy[1]<.41) if front else (p.z>1.635 if p.y>.025 else p.z>1.675)
 hairflag.append(bool(flag))
polys=[p for p in head.data.polygons if all(hairflag[i] for i in p.vertices)]
used=sorted({i for p in polys for i in p.vertices});lookup={v:i for i,v in enumerate(used)}
vs=[];normals=[]
for idx in used:
 v=head.data.vertices[idx];n=v.normal.copy();p=v.co.copy()
 lift=.004+.017*np.clip((p.z-1.66)/.09,0,1)
 lift*=.85+.15*math.sin(p.x*55+p.y*21)
 p+=n*lift
 vs.append(tuple(p));normals.append(n)
hairme=bpy.data.meshes.new('Scalp conforming hair shell');hairme.from_pydata(vs,[],[tuple(lookup[i] for i in p.vertices) for p in polys]);hairme.update()
hair=bpy.data.objects.new('Beka_swept_hair_surface',hairme);bpy.context.collection.objects.link(hair)
hairUV=hairme.uv_layers.new(name='PortraitProjection');hairblend=hairme.color_attributes.new(name='PortraitBlend',type='FLOAT_COLOR',domain='POINT')
for i,idx in enumerate(used):hairblend.data[i].color=blend.data[idx].color
for p in hairme.polygons:
 p.use_smooth=True
 for li in p.loop_indices:
  idx=used[hairme.loops[li].vertex_index];xy=projected[idx];hairUV.data[li].uv=(xy[0],1-xy[1])
hairmat=projection_material('Portrait hair front and black sides',(.006,.004,.003));hairme.materials.append(hairmat)
# Join only actual head/hair surfaces and bake portable albedo without altering source photographs.
bpy.ops.object.select_all(action='DESELECT');head.select_set(True);hair.select_set(True);bpy.context.view_layer.objects.active=head;bpy.ops.object.join()
head.name='Beka_head_likeness_candidate_v6'
head.data.update();bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT')
bakeuv=head.data.uv_layers.new(name='BakedHeadUV');head.data.uv_layers.active_index=len(head.data.uv_layers)-1
bpy.ops.uv.smart_project(angle_limit=1.12,island_margin=.005);bpy.ops.object.mode_set(mode='OBJECT')
atlas=bpy.data.images.new('Beka_v6_albedo_2048',width=2048,height=2048,alpha=False)
for m in head.data.materials:
 im=m.node_tree.nodes.new('ShaderNodeTexImage');im.image=atlas;m.node_tree.nodes.active=im
sc=bpy.context.scene;sc.render.engine='CYCLES';sc.cycles.samples=1;sc.render.bake.margin=10;bpy.ops.object.bake(type='EMIT')
atlas.filepath_raw=str(O/'Beka_v6_albedo_2048.png');atlas.file_format='PNG';atlas.save();atlas.pack()
mat=bpy.data.materials.new('Beka baked review skin and hair');mat.use_nodes=True;n=mat.node_tree.nodes
im=n.new('ShaderNodeTexImage');im.image=atlas;uvn=n.new('ShaderNodeUVMap');uvn.uv_map='BakedHeadUV';mat.node_tree.links.new(uvn.outputs['UV'],im.inputs['Vector']);mat.node_tree.links.new(im.outputs['Color'],n.get('Principled BSDF').inputs['Base Color']);n.get('Principled BSDF').inputs['Roughness'].default_value=.78
head.data.materials.clear();head.data.materials.append(mat)
for p in head.data.polygons:p.material_index=0;p.use_smooth=True
for layer in list(head.data.uv_layers):
 if layer.name!='BakedHeadUV':head.data.uv_layers.remove(layer)
head.data.uv_layers.active_index=0
bpy.ops.object.select_all(action='DESELECT');head.select_set(True);bpy.context.view_layer.objects.active=head
bpy.ops.export_scene.gltf(filepath=str(O/'Beka_head_likeness_candidate_v6.glb'),use_selection=True,export_format='GLB',export_apply=True,export_animations=False)
cam=sc.camera;cam.data.ortho_scale=.36;sc.render.resolution_x=1024;sc.render.resolution_y=1024;sc.cycles.samples=28
bpy.ops.wm.save_as_mainfile(filepath=str(O/'Beka_head_likeness_candidate_v6.blend'))
report={'status':'likeness_candidate_not_approved','head_only':True,'geometry':'Fitted neutral geometry v5b based on real frontal photo and profile estimates','texture':'Approved enlarged portrait, eyes-open image_gen edit, landmark-aligned to the rendered geometry','texture_landmark_controls':len(ids),'hair_shell_faces':len(polys),'source_portrait_resolution':[W,H],'albedo_atlas_resolution':[2048,2048],'game_changed':False,'face_likeness_verified':False,'views':[]}
(O/'build_report.json').write_text(json.dumps(report,indent=2),encoding='utf8')
for name,pos in [('front',(0,-1,1.63)),('three_quarter',(-.65,-1,1.63)),('profile',(-1,0,1.63)),('back',(0,1,1.63))]:
 cam.location=pos;cam.rotation_euler=(Vector((0,0,1.63))-cam.location).to_track_quat('-Z','Y').to_euler();sc.render.filepath=str(O/(name+'.png'));bpy.ops.render.render(write_still=True);report['views'].append(name+'.png');(O/'build_report.json').write_text(json.dumps(report,indent=2),encoding='utf8')
print('HEAD_LIKENESS_V6_RENDERED')

