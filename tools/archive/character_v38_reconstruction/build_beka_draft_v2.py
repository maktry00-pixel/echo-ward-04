import pathlib
stage=pathlib.Path(__file__).parent
s=(stage/'build_beka_reconstruction_draft.py').read_text(encoding='utf8')
s=s.replace('beka_reconstruction_draft_v1','beka_reconstruction_draft_v2').replace('Beka_reconstruction_draft_v1','Beka_reconstruction_draft_v2').replace("manual_assembly_v1'","manual_assembly_v2'")
s=s.replace("if p.material_index not in [0,1,3,4] or all((face.matrix_world@v.co).z<1.455 for v in p.verts)","if p.material_index not in [0,1,3,4] or all((face.matrix_world@v.co).z<1.455 or ((face.matrix_world@v.co).z<1.515 and abs((face.matrix_world@v.co).x)>.064) for v in p.verts)")
# Weighted local deformation using measured 2D landmarks as frontal constraints.
insertion = r"""
import numpy as np
bones=json.loads((ROOT/'artifacts/echo_hero_v38/manual_assembly_v1/facial_bones.json').read_text(encoding='utf8'))
target=json.loads((ROOT/'artifacts/echo_hero_v38/manual_assembly_v1/target_face_landmarks.json').read_text(encoding='utf8'))['landmarks_px']
pairs=[('FACIAL_R_EyeCornerOuter',33),('FACIAL_R_EyeCornerInner',133),('FACIAL_L_EyeCornerInner',362),('FACIAL_L_EyeCornerOuter',263),('FACIAL_C_NoseTip',1),('FACIAL_C_NoseLower',2),('FACIAL_R_LipCorner',61),('FACIAL_L_LipCorner',291),('FACIAL_C_LipUpper',0),('FACIAL_C_LipLower',17),('FACIAL_C_UnderChin',152)]
controls=[];delta=[]
for name,idx in pairs:
 p=np.array(bones[name],dtype=float)
 if p[2]>1.49:p[0]*=1-.045*min(1,max(0,(p[2]-1.49)/.10))
 desired=np.array([(target[idx][0]-535)/750,(1404-target[idx][1])/750])
 controls.append([p[0],p[2]-1.6]);delta.append(desired-np.array([p[0],p[2]]))
# Boundary anchors prevent unsupported skull or neck deformation.
for x,z in [(-.087,1.66),(.087,1.66),(-.078,1.54),(.078,1.54),(0,1.745),(0,1.70),(-.065,1.47),(.065,1.47)]:
 controls.append([x,z-1.6]);delta.append([0,0])
C=np.array(controls);D=np.array(delta)
def kernel(d):
 r2=np.maximum(np.sum(d*d,axis=-1),1e-18)
 return r2*np.log(r2)
K=kernel(C[:,None,:]-C[None,:,:]);P=np.column_stack((np.ones(len(C)),C))
A=np.block([[K+np.eye(len(C))*2e-7,P],[P.T,np.zeros((3,3))]])
coef=np.linalg.solve(A,np.vstack((D,np.zeros((3,2)))))
def warp_points(points):
 arr=np.array(points);X=arr[:,[0,2]].copy();X[:,1]-=1.6
 disp=kernel(X[:,None,:]-C[None,:,:])@coef[:len(C)]+np.column_stack((np.ones(len(X)),X))@coef[len(C):]
 w=np.clip((.045-arr[:,1])/.08,0,1)*np.clip((arr[:,2]-1.48)/.06,0,1)
 arr[:,0]+=disp[:,0]*w;arr[:,2]+=disp[:,1]*w
 return arr
mw=face.matrix_world.copy();inv=mw.inverted()
newpoints=warp_points([tuple(mw@v.co) for v in face.data.vertices])
for v,p in zip(face.data.vertices,newpoints):v.co=inv@Vector(p)
face.data.update()
"""
s=s.replace("projection_uv(face,'head')",insertion+"\nprojection_uv(face,'head')")
s=s.replace("a=0 if -n.y>=abs(n.x) else 180 if n.y>=abs(n.x) else 90 if n.x>0 else 270","a=0 if kind=='head' else (0 if -n.y>=abs(n.x) else 180 if n.y>=abs(n.x) else 90 if n.x>0 else 270)")
s=s.replace("if o.name.startswith(('Full zip hoodie body','Connected olive trousers')):\n  projection_uv(o,'cloth')","if o.name.startswith('Connected olive trousers'):\n  o.data.materials.clear();o.data.materials.append(bpy.data.materials['Dark olive trousers'])")
# Full hoodie remains original woven material; photo contains unwanted arm and background silhouettes.
s=s.replace("(.009,.008,.006),.46","(.003,.0025,.002),.76")
s=s.replace("boundary=1.42+.25*back+.07*front*(.5+.5*math.sin(ph))","boundary=1.60+.50*back+.10*front*(.5+.5*math.sin(ph))")
s=s.replace("y=.005-(.096+wave)*math.sin(th)*math.cos(ph)","y=.005-(.122-.012*math.cos(ph)+wave)*math.sin(th)*math.cos(ph)")
s=s.replace("for i in range(100):\n s=i/99","for i in range(64):\n s=i/63")
s=s.replace("x=x0-.020*math.sin(math.pi*t)\n  y=-.027-.070*t\n  z=1.748+.028*math.sin(math.pi*t)-(.066+.012*(1-s))*t","x=x0-.024*math.sin(math.pi*t)\n  y=-.025-.083*t\n  z=1.752+.030*math.sin(math.pi*t)-(.084+.008*(1-s))*t")
s=s.replace("random.uniform(.0006,.00115)","random.uniform(.00025,.00050)")
s=s.replace("for i in range(95):","for i in range(460):")
s=s.replace(".005-.099*math.sin(th)*math.cos(ang)",".005-(.124-.012*math.cos(ang))*math.sin(th)*math.cos(ang)")
s=s.replace("random.uniform(.0003,.0007)","random.uniform(.00015,.00028)")
s=s.replace("bpy.ops.object.transform_apply(location=True,rotation=False,scale=True);o.data.materials.append(ma)","bpy.ops.object.transform_apply(location=True,rotation=False,scale=True);o.data.materials.append(ma)\n  for vert,wp in zip(o.data.vertices,warp_points([tuple(v.co) for v in o.data.vertices])):vert.co=wp")
# Skin material: smooth blend of a single landmark-aligned frontal photo and neutral side skin.
# Bake to regular UV color map to make the result portable in GLB/FBX.
bake = r"""
# Uniform portable cloth color and fine woven microstructure.
for name in ['Black charcoal fleece','Black rib knit cuffs','Hood lining']:
 m=bpy.data.materials.get(name)
 if m:
  p=m.node_tree.nodes.get('Principled BSDF')
  p.inputs['Base Color'].default_value=(.009,.011,.010,1);p.inputs['Roughness'].default_value=.87
denim=bpy.data.materials['Dark olive trousers'];p=denim.node_tree.nodes.get('Principled BSDF')
p.inputs['Base Color'].default_value=(.033,.037,.024,1);p.inputs['Roughness'].default_value=.91
# Vertex weights blend front colors into uniform side skin without hard polygon borders.
attribute=face.data.color_attributes.new(name='PhotoBlend',type='FLOAT_COLOR',domain='POINT')
for v in face.data.vertices:
 co=face.matrix_world@v.co;n=(face.matrix_world.to_3x3()@v.normal).normalized()
 w=max(0,min(1,(-n.y-.10)/.55))
 w*=max(0,min(1,(co.z-1.48)/.055))
 if co.y>.01:w=0
 attribute.data[v.index].color=(w,w,w,1)
for a,m in photo_skin.items():
 n=m.node_tree.nodes;links=m.node_tree.links;p=n.get('Principled BSDF')
 tex=next(x for x in n if x.type=='TEX_IMAGE')
 uvn=n.new('ShaderNodeUVMap');uvn.uv_map=face.data.uv_layers.active.name;links.new(uvn.outputs['UV'],tex.inputs['Vector'])
 attr=n.new('ShaderNodeVertexColor');attr.layer_name='PhotoBlend'
 mix=n.new('ShaderNodeMixRGB');mix.blend_type='MIX';mix.inputs[1].default_value=(.46,.28,.20,1)
 links.new(attr.outputs['Color'],mix.inputs[0]);links.new(tex.outputs['Color'],mix.inputs[2]);links.new(mix.outputs['Color'],p.inputs['Base Color'])
 emission=n.new('ShaderNodeEmission');links.new(mix.outputs['Color'],emission.inputs['Color'])
 output=n.get('Material Output');links.new(emission.outputs['Emission'],output.inputs['Surface'])
for material in [white,teeth]:
 n=material.node_tree.nodes;links=material.node_tree.links;p=n.get('Principled BSDF')
 em=n.new('ShaderNodeEmission');em.inputs['Color'].default_value=p.inputs['Base Color'].default_value;links.new(em.outputs['Emission'],n.get('Material Output').inputs['Surface'])
sourceUV=face.data.uv_layers.active.name
uvBake=face.data.uv_layers.new(name='HeadBakeUV');face.data.uv_layers.active_index=len(face.data.uv_layers)-1
bpy.ops.object.select_all(action='DESELECT');face.select_set(True);bpy.context.view_layer.objects.active=face
bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.smart_project(angle_limit=1.15,island_margin=.008);bpy.ops.object.mode_set(mode='OBJECT')
atlas=bpy.data.images.new('Beka_head_albedo_2048',width=2048,height=2048,alpha=False)
for m in face.data.materials:
 n=m.node_tree.nodes;im=n.new('ShaderNodeTexImage');im.image=atlas;n.active=im;im.select=True
sc=bpy.context.scene;sc.render.engine='CYCLES';sc.cycles.samples=1
sc.render.bake.use_clear=True;sc.render.bake.margin=12
bpy.ops.object.bake(type='EMIT')
atlas.filepath_raw=str(OUT/'Beka_head_albedo_2048.png');atlas.file_format='PNG';atlas.save();atlas.pack()
finalskin=mat('Baked skin portable albedo',(.46,.28,.20),.70);n=finalskin.node_tree.nodes;im=n.new('ShaderNodeTexImage');im.image=atlas
uvn=n.new('ShaderNodeUVMap');uvn.uv_map='HeadBakeUV';finalskin.node_tree.links.new(uvn.outputs['UV'],im.inputs['Vector']);finalskin.node_tree.links.new(im.outputs['Color'],n.get('Principled BSDF').inputs['Base Color'])
face.data.materials.clear();face.data.materials.append(finalskin)
for poly in face.data.polygons:poly.material_index=0
# Keep bake UV as the default exportable UV0, instead of the old source projection map.
for uv in list(face.data.uv_layers):
 if uv.name!='HeadBakeUV':face.data.uv_layers.remove(uv)
face.data.uv_layers.active_index=0
"""
s=s.replace("# Capture deformed geometry",bake+"\n# Capture deformed geometry")
s=s.replace("'Four approximate directional photo projections; seams and likeness need review.'","'Landmark-aligned frontal photo blended to side skin, baked to 2048 albedo; manual hair/clothing. Likeness not approved.'")
s=s.replace("views=[('front',(0,-3.5,.92),.88,2.05),('three_quarter',(2.5,-3.3,1.0),.95,2.05),('back',(0,3.5,.92),.88,2.05),('face',(0,-2,1.62),1.62,.36)]","views=[('face',(0,-2,1.62),1.62,.36),('front',(0,-3.5,.92),.88,2.05),('three_quarter',(2.5,-3.3,1.0),.95,2.05),('back',(0,3.5,.92),.88,2.05)]")
exec(compile(s,str(stage/'generated_build_v2.py'),'exec'))

