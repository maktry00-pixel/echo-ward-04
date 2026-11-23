"""Reconstruct the whole visible face from the approved photo, on native topology.

The depth guide is a single-image estimate, not a scan. The original neck,
outfit and UV layout remain intact. Facial color is baked in Blender's 3D
material system and matched to the already accepted skin tone.
"""
from pathlib import Path
import bpy,bmesh,json,math,numpy as np
from mathutils import Matrix
R=Path('D:/Projects/ECHO');O=R/'artifacts/echo_hero_v38/metahuman_conform_v30';O.mkdir(exist_ok=True)
data=json.loads((O/'face_measurements.json').read_text(encoding='utf8'))
native=np.array(data['native']['landmarks_px']);photo=np.array(data['photo_open']['landmarks_px']);neutral=np.array(data['photo_neutral']['landmarks_px'])
W=data['native']['width'];H=data['native']['height'];PW=data['photo_neutral']['width'];PH=data['photo_neutral']['height']
origin=native[[33,263],:2].mean(axis=0);axis=native[263,:2]-native[33,:2];axis/=np.linalg.norm(axis);frame=np.column_stack([axis,[-axis[1],axis[0]]]);width=np.linalg.norm(native[234,:2]-native[454,:2])
def register(p):
    center=p[[33,263],:2].mean(axis=0);a=p[263,:2]-p[33,:2];a/=np.linalg.norm(a);F=np.column_stack([a,[-a[1],a[0]]]);scale=width/np.linalg.norm(p[234,:2]-p[454,:2])
    result=p.copy();result[:,:2]=(p[:,:2]-center)@F@frame.T*scale+origin;result[:,2]=(p[:,2]-p[[33,263],2].mean())*scale+native[[33,263],2].mean()
    return result,(center,F,scale)
open_target,open_reg=register(photo);neutral_target,reg=register(neutral);target=.7*open_target+.3*neutral_target
# The eyes are fitted to the open-eye reference, rather than narrowed from a generic head.
eyeids=[33,7,163,144,145,153,154,155,133,173,157,158,159,160,161,246,263,249,390,373,374,380,381,382,362,398,384,385,386,387,388,466]
target[eyeids]=open_target[eyeids]
ids=list(range(468));C=native[ids,:2]/W;D=(target-native)[ids]/W
top=[10,338,297,332,284,251,389,127,162,21,54,103,67,109];D[top]*=.25
anchors=np.array([[80,110],[1000,110],[540,50],[60,900],[1020,900],[220,1360],[860,1360],[540,1440]],float)/W
C=np.vstack([C,anchors]);D=np.vstack([D,np.zeros((len(anchors),3))])
def kernel(d):
    q=np.maximum(np.sum(d*d,axis=-1),1e-18);return q*np.log(q)
P=np.column_stack([np.ones(len(C)),C]);A=np.block([[kernel(C[:,None]-C[None,:])+np.eye(len(C))*4e-6,P],[P.T,np.zeros((3,3))]])
coef=np.linalg.solve(A,np.vstack([D,np.zeros((3,3))]))
def warp(p):
    out=[]
    for start in range(0,len(p),2048):
        q=p[start:start+2048];out.append(kernel(q[:,None]-C[None,:])@coef[:len(C)]+np.column_stack([np.ones(len(q)),q])@coef[len(C):])
    return np.vstack(out)
bpy.ops.wm.read_factory_settings(use_empty=True);bpy.context.preferences.filepaths.save_version=0
bpy.ops.import_scene.fbx(filepath=str(R/'artifacts/echo_hero_v38/metahuman_conform_v29/verification/Beka_Face_V29.fbx'),use_anim=False)
base=max((o for o in bpy.data.objects if o.type=='MESH'),key=lambda o:len(o.data.vertices));parts={}
for name,mi in [('Head',0),('LeftEye',3),('RightEye',4)]:
    me=base.data.copy();ob=bpy.data.objects.new('BekaTarget_'+name,me);bpy.context.collection.objects.link(ob);bm=bmesh.new();bm.from_mesh(me);bmesh.ops.delete(bm,geom=[f for f in bm.faces if f.material_index!=mi],context='FACES');bmesh.ops.delete(bm,geom=[v for v in bm.verts if not v.link_faces],context='VERTS');bm.to_mesh(me);bm.free();me.update()
    for v in me.vertices:v.co=base.matrix_world@v.co
    ob.matrix_world=Matrix.Identity(4);parts[name]=ob
head=parts['Head'];before=np.array([tuple(v.co) for v in head.data.vertices])
setup=json.loads((R/'artifacts/echo_hero_v38/metahuman_conform_v29/face_review/review_setup.json').read_text(encoding='utf8'));lm=setup['landmarks_cm'];focus=(lm[18][2]+lm[22][2])*.005-.008;f=(W/2)/math.tan(math.radians(4.5))
def project(p):return np.column_stack([W/2+f*p[:,0]/(p[:,1]+2.1),H/2-f*(p[:,2]-focus)/(p[:,1]+2.1)])
pixels=project(before);d=warp(pixels/W)*W;depth=before[:,1]+2.1
delta=np.column_stack([d[:,0]*depth/f,np.clip(d[:,2]*depth/f*.65,-.004,.004),-d[:,1]*depth/f])
front=np.clip((.035-before[:,1])/.085,0,1);neck=np.clip((before[:,2]-1.49)/.022,0,1);scalp=np.clip((1.735-before[:,2])/.045,0,1);delta*=np.column_stack([front*neck*scalp]*3)
length=np.linalg.norm(delta,axis=1);delta*=np.minimum(1,.012/np.maximum(length,1e-12))[:,None];after=before+delta
eye_report={}
for name,indices in [('LeftEye',[263,362]),('RightEye',[33,133])]:
    ob=parts[name];eye=np.array([tuple(v.co) for v in ob.data.vertices]);M=np.column_stack([2*eye,np.ones(len(eye))]);solution=np.linalg.lstsq(M,np.sum(eye*eye,axis=1),rcond=None)[0];center=solution[:3];radius=math.sqrt(max(0,solution[3]+np.dot(center,center)))
    # Match the iris/eyelid center location; preserve spherical eyeball geometry.
    centerpx=project(center[None])[0];movement=warp(centerpx[None]/W)[0]*W;ed=center[1]+2.1;shift=np.array([movement[0]*ed/f,np.clip(movement[2]*ed/f*.65,-.003,.003),-movement[1]*ed/f]);newcenter=center+shift
    for v,p in zip(ob.data.vertices,eye+shift):v.co=p
    for i,p in enumerate(after):
        old=before[i];dist=np.linalg.norm(old-center)
        if old[1]<center[1] and radius-.001<dist<radius+.006 and abs(old[0]-center[0])<radius*1.4 and abs(old[2]-center[2])<radius*1.3:
            q=dist*dist-(p[0]-newcenter[0])**2-(p[2]-newcenter[2])**2
            if q>0:after[i,1]=newcenter[1]-math.sqrt(q)
    eye_report[name]={'center_shift_mm':(shift*1000).tolist(),'radius_m':radius}
tri=np.array([p.vertices[:3] for p in head.data.polygons]);a,b,c=tri.T;norm=np.cross(before[b]-before[a],before[c]-before[a])
def folded():return np.where(np.einsum('ij,ij->i',norm,np.cross(after[b]-after[a],after[c]-after[a]))<0)[0]
initial=len(folded());steps=0
while len(folded()) and steps<32:
    bad=set(tri[folded()].ravel().tolist());near=set()
    for poly in head.data.polygons:
        if any(i in bad for i in poly.vertices):near.update(poly.vertices)
    for i in near:after[i]=before[i]+(after[i]-before[i])*(.62 if i in bad else .9)
    steps+=1
changes=np.linalg.norm(after-before,axis=1)
report={'source':'Approved open-eye portrait, original photo identity; whole-face fitting instead of generic eye corners','control_points':468,'clothing_changed':False,'body_changed':False,'skin_tone_locked':True,'depth_is_single_image_estimate':True,'likeness_approved':False,'eyes':eye_report,'parts':{},'initial_flipped_faces':initial,'flipped_faces':int(len(folded())),'repair_steps':steps,'max_incremental_change_mm':float(changes.max()*1000),'neck_max_change_mm':float(changes[before[:,2]<1.49].max()*1000),'camera_focus_z_m':focus}
if report['flipped_faces'] or report['neck_max_change_mm']>.01:raise RuntimeError('Unsafe full-face target')
for v,p in zip(head.data.vertices,after):v.co=p
head.data.update()
for ob in list(bpy.data.objects):
    if ob not in parts.values():bpy.data.objects.remove(ob,do_unlink=True)
for name,ob in parts.items():
    for uv in list(ob.data.uv_layers)[1:]:ob.data.uv_layers.remove(uv)
    ob.data.uv_layers.active_index=0;ob.data.uv_layers[0].active_render=True;ob.data.materials.clear();ob.data.materials.append(bpy.data.materials.new('Target '+name))
    for p in ob.data.polygons:p.material_index=0;p.use_smooth=True
    bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);bpy.context.view_layer.objects.active=ob
    bpy.ops.export_scene.fbx(filepath=str(O/('BekaTarget_'+name+'.fbx')),use_selection=True,object_types={'MESH'},add_leaf_bones=False,bake_anim=False,axis_forward='-Z',axis_up='Y')
    report['parts'][name]={'vertices':len(ob.data.vertices),'uvs':[uv.name for uv in ob.data.uv_layers]}
# Map the neutral photo directly to this reconstructed surface, then bake to native UV.
center,F,scale=reg;screen=project(after);q=((screen-origin)@frame@F.T)/scale+center;uvs=np.column_stack([q[:,0]/PW,1-q[:,1]/PH])
uv=head.data.uv_layers.new(name='PhotoFaceV30')
for loop in head.data.loops:uv.data[loop.index].uv=uvs[loop.vertex_index]
old=bpy.data.images.load(str(R/'artifacts/echo_hero_v38/metahuman_conform_v23/Beka_SkinDiffuse_NativeUV_4096.png'));photoim=bpy.data.images.load(data['photo_neutral']['file']);old.colorspace_settings.name='sRGB';photoim.colorspace_settings.name='sRGB'
oldpixels=np.array(old.pixels[:]).reshape(old.size[1],old.size[0],4);photopixels=np.array(photoim.pixels[:]).reshape(PH,PW,4)
nativeuv=head.data.uv_layers[0];vertexuv={loop.vertex_index:tuple(nativeuv.data[loop.index].uv) for loop in head.data.loops};ratios=[]
for i,p in enumerate(after):
    if p[1]>-.06 or not 1.58<p[2]<1.68 or not .028<abs(p[0])<.060:continue
    u0,v0=vertexuv[i];up,vp=uvs[i]
    if not (0<up<1 and 0<vp<1):continue
    oc=oldpixels[int(v0*(old.size[1]-1)),int(u0*(old.size[0]-1)),:3];pc=photopixels[int(vp*(PH-1)),int(up*(PW-1)),:3]
    if pc.min()>.08 and oc.min()>.08 and oc.max()<.85 and pc.max()<.85:ratios.append(oc/pc)
gain=np.clip(np.median(np.array(ratios),axis=0),.72,1.35) if len(ratios)>20 else np.ones(3);report['skin_tone_gain_linear']=gain.tolist();report['skin_tone_samples']=len(ratios)
attr=head.data.color_attributes.new(name='PhotoFaceWeight',type='FLOAT_COLOR',domain='POINT')
for i,v in enumerate(head.data.vertices):
    x,y,z=v.co;n=v.normal;up,vp=uvs[i];wt=float(np.clip((-n.y-.18)/.58,0,1)*np.clip((-.015-y)/.060,0,1)*np.clip((z-1.525)/.040,0,1)*np.clip((1.713-z)/.028,0,1)*np.clip((.082-abs(x))/.018,0,1))
    wt*=float(0.12<up<.89 and .18<vp<.88);attr.data[i].color=(wt*.92,wt*.92,wt*.92,1)
mat=bpy.data.materials.new('Photo face reconstruction preserving accepted skin tone');mat.use_nodes=True;nodes=mat.node_tree.nodes;links=mat.node_tree.links;nodes.clear();out=nodes.new('ShaderNodeOutputMaterial');emit=nodes.new('ShaderNodeEmission');links.new(emit.outputs[0],out.inputs['Surface'])
nt=nodes.new('ShaderNodeTexImage');nt.image=old;nu=nodes.new('ShaderNodeUVMap');nu.uv_map=nativeuv.name;links.new(nu.outputs[0],nt.inputs[0]);pt=nodes.new('ShaderNodeTexImage');pt.image=photoim;pu=nodes.new('ShaderNodeUVMap');pu.uv_map='PhotoFaceV30';links.new(pu.outputs[0],pt.inputs[0]);multiply=nodes.new('ShaderNodeMixRGB');multiply.blend_type='MULTIPLY';multiply.inputs[0].default_value=1; multiply.inputs[2].default_value=(*gain,1);links.new(pt.outputs['Color'],multiply.inputs[1]);mask=nodes.new('ShaderNodeVertexColor');mask.layer_name=attr.name;mix=nodes.new('ShaderNodeMixRGB');links.new(mask.outputs['Color'],mix.inputs[0]);links.new(nt.outputs['Color'],mix.inputs[1]);links.new(multiply.outputs[0],mix.inputs[2]);links.new(mix.outputs[0],emit.inputs['Color']);head.data.materials.clear();head.data.materials.append(mat)
atlas=bpy.data.images.new('Beka_PhotoFaceV30_4096',width=4096,height=4096,alpha=False);dest=nodes.new('ShaderNodeTexImage');dest.image=atlas;nodes.active=dest;head.data.uv_layers.active_index=0;head.data.uv_layers[0].active_render=True
bpy.ops.object.select_all(action='DESELECT');head.select_set(True);bpy.context.view_layer.objects.active=head;sc=bpy.context.scene;sc.render.engine='CYCLES';sc.cycles.samples=1;sc.render.bake.margin=12;sc.render.image_settings.color_mode='RGB';sc.render.image_settings.color_depth='8';bpy.ops.object.bake(type='EMIT');atlas.filepath_raw=str(O/'Beka_PhotoFaceV30_NativeUV_4096.png');atlas.file_format='PNG';atlas.save();report['atlas']={'resolution':[4096,4096],'source_resolution':[PW,PH],'new_real_4k_detail':False,'method':'3D material projection to native UV; accepted skin tone matched; old atlas retained outside visible face'}
bpy.ops.wm.save_as_mainfile(filepath=str(O/'Beka_photo_face_replacement_v30.blend'));(O/'template_report.json').write_text(json.dumps(report,indent=2),encoding='utf8');print('PHOTO_FACE_V30_TARGET',json.dumps(report))
