"""Fit only native head vertices to photo features; keep clothing/color/body unchanged."""
from pathlib import Path
import bpy,bmesh,json,math,numpy as np
from mathutils import Vector,Matrix
from mathutils.bvhtree import BVHTree
R=Path('D:/Projects/ECHO');S=R/'artifacts/echo_hero_v38/metahuman_conform_v23/face_v27';O=R/'artifacts/echo_hero_v38/metahuman_conform_v27';O.mkdir(exist_ok=True)
measure=json.loads((O/'face_measurements.json').read_text(encoding='utf8'));native=np.array(measure['native_front']['landmarks_px'])[:,:2];W=measure['native_front']['width'];H=measure['native_front']['height']
def align(points):
    p=np.asarray(points)[:,:2];a=p[263]-p[33];b=native[263]-native[33];sa=np.linalg.norm(a);sb=np.linalg.norm(b);a/=sa;b/=sb
    A=np.column_stack([a,[-a[1],a[0]]]);B=np.column_stack([b,[-b[1],b[0]]])
    return ((p-p[[33,263]].mean(axis=0))@A@B.T)*(sb/sa)+native[[33,263]].mean(axis=0)
target=.70*align(measure['real_front']['landmarks_px'])+.30*align(measure['approved_enlargement']['landmarks_px'])
loops=[([33,7,163,144,145,153,154,155,133,173,157,158,159,160,161,246],33,133),([263,249,390,373,374,380,381,382,362,398,384,385,386,387,388,466],263,362)]
for ids,outer,inner in loops:
    A=native[inner]-native[outer];la=np.linalg.norm(A);A/=la;P=np.array([-A[1],A[0]])
    inner_target=.25*target[inner]+.75*native[inner];outer_target=native[outer];B=inner_target-outer_target;lb=np.linalg.norm(B);B/=lb;Q=np.array([-B[1],B[0]])
    for i in ids:
        d=native[i]-native[outer];target[i]=outer_target+B*np.dot(d,A)*(lb/la)+Q*np.dot(d,P)*.72
ids=[234,454,323,93,132,361,58,288,172,397,136,365,150,379,149,378,176,400,148,377,152,162,389,127,356,21,251,70,63,105,66,107,336,296,334,293,300,168,6,197,195,5,4,1,2,98,97,99,64,49,48,115,220,45,275,440,344,278,279,294,326,327,328,329,290,250,460,0,17,61,291,37,39,40,185,267,269,270,409,146,91,181,84,314,405,321,375,18,200,199,175,13,14,78,308]
ids=sorted(set(ids+[i for loop,_,_ in loops for i in loop]));C=native[ids]/W;D=(target-native)[ids]/W
anchors=np.array([[140,220],[940,220],[540,90],[100,950],[980,950],[250,1340],[830,1340],[540,1440]],float)/W
C=np.vstack([C,anchors]);D=np.vstack([D,np.zeros((len(anchors),2))])
def kernel(d):
    r2=np.maximum(np.sum(d*d,axis=-1),1e-18);return r2*np.log(r2)
P=np.column_stack([np.ones(len(C)),C]);K=kernel(C[:,None]-C[None,:]);A=np.block([[K+np.eye(len(C))*2e-5,P],[P.T,np.zeros((3,3))]])
coef=np.linalg.solve(A,np.vstack([D,np.zeros((3,2))]))
def warp(p):return kernel(p[:,None]-C[None,:])@coef[:len(C)]+np.column_stack([np.ones(len(p)),p])@coef[len(C):]
bpy.ops.wm.read_factory_settings(use_empty=True);bpy.context.preferences.filepaths.save_version=0;bpy.ops.import_scene.fbx(filepath=str(S/'Beka_Face_BeforeV27.fbx'),use_anim=False)
base=max((o for o in bpy.data.objects if o.type=='MESH'),key=lambda o:len(o.data.vertices));parts={};report={'photo_primary':'real_front','photo_detail_guide':'approved_enlargement','skin_color_changed':False,'clothing_changed':False,'body_changed':False,'scan':False,'likeness_approved':False,'parts':{}}
for name,mi in [('Head',0),('LeftEye',3),('RightEye',4)]:
    me=base.data.copy();ob=bpy.data.objects.new('BekaTarget_'+name,me);bpy.context.collection.objects.link(ob);bm=bmesh.new();bm.from_mesh(me);bmesh.ops.delete(bm,geom=[f for f in bm.faces if f.material_index!=mi],context='FACES');bmesh.ops.delete(bm,geom=[v for v in bm.verts if not v.link_faces],context='VERTS');bm.to_mesh(me);bm.free();me.update()
    for v in me.vertices:v.co=base.matrix_world@v.co
    ob.matrix_world=Matrix.Identity(4);parts[name]=ob
head=parts['Head'];before=np.array([tuple(v.co) for v in head.data.vertices]);tris=[tuple(p.vertices) for p in head.data.polygons];bvh=BVHTree.FromPolygons([Vector(p) for p in before],tris,all_triangles=False)
checkpoint=json.loads((S/'accepted_appearance_and_face_base.json').read_text(encoding='utf8'));lm=checkpoint['landmarks_cm'];focus=(lm[18][2]+lm[22][2])*.005-.008;camera=Vector((0,-2.1,focus));f=(W/2)/math.tan(math.radians(4.5))
def project(coords):
    depth=coords[:,1]+2.1
    return np.column_stack([W/2+f*coords[:,0]/depth,H/2-f*(coords[:,2]-focus)/depth])
pixels=project(before);delta_px=warp(pixels/W)*W;depth=before[:,1]+2.1
delta=np.column_stack([delta_px[:,0]*depth/f,np.zeros(len(before)),-delta_px[:,1]*depth/f])
front=np.clip((.04-before[:,1])/.08,0,1);neck=np.clip((before[:,2]-1.49)/.018,0,1);weight=front*neck
delta*=weight[:,None];length=np.linalg.norm(delta,axis=1);delta*=np.minimum(1.,.009/np.maximum(length,1e-12))[:,None];after=before+delta
# A small profile correction to the nose tip, supported by the real side views.
point=native[1];direction=Vector(((point[0]-W/2)/f,1,(H/2-point[1])/f)).normalized();nose=bvh.ray_cast(camera,direction,3.)[0]
if nose is None:raise RuntimeError('Reference camera did not hit the native nose; stop before importing')
for i,p in enumerate(after):
    factor=math.exp(-((p[0]-nose.x)/.020)**2-((p[2]-(nose.z+delta[i,2]))/.018)**2)*float(np.clip((-before[i,1]-.080)/.035,0,1));after[i,1]+=.0025*factor
# Preserve the skin's distance from each eye sphere while narrowing the eyelid opening.
for name in ['LeftEye','RightEye']:
    eye=np.array([tuple(v.co) for v in parts[name].data.vertices]);matrix=np.column_stack([2*eye,np.ones(len(eye))]);rhs=np.sum(eye*eye,axis=1);solution=np.linalg.lstsq(matrix,rhs,rcond=None)[0];center=solution[:3];radius=math.sqrt(max(0.,solution[3]+np.dot(center,center)))
    for i,p in enumerate(after):
        old=before[i];dist=np.linalg.norm(old-center)
        if old[1]<center[1] and radius-.001<dist<radius+.006 and abs(old[0]-center[0])<radius*1.35 and abs(old[2]-center[2])<radius*1.15:
            under=dist*dist-(p[0]-center[0])**2-(p[2]-center[2])**2
            if under>0:after[i,1]=center[1]-math.sqrt(under)
    report[name+'_sphere']={'center_m':center.tolist(),'radius_m':radius}
triangles=np.array([p.vertices[:3] for p in head.data.polygons]);a,b,c=triangles.T;old_normals=np.cross(before[b]-before[a],before[c]-before[a])
def inverted():return np.where(np.einsum('ij,ij->i',old_normals,np.cross(after[b]-after[a],after[c]-after[a]))<0)[0]
initial_flips=len(inverted());steps=0
while len(inverted()) and steps<24:
    bad=set(triangles[inverted()].ravel().tolist());near=set()
    for p in head.data.polygons:
        if any(i in bad for i in p.vertices):near.update(p.vertices)
    for i in near:after[i]=before[i]+(after[i]-before[i])*(.60 if i in bad else .85)
    steps+=1
for v,p in zip(head.data.vertices,after):v.co=p
head.data.update();flips=len(inverted());report.update(initial_folded_faces=initial_flips,local_fold_repair_steps=steps)
changes=np.linalg.norm(after-before,axis=1);report.update(controls=len(ids),max_vertex_change_mm=float(changes.max()*1000),mean_vertex_change_mm=float(changes.mean()*1000),flipped_faces=flips,neck_max_change_mm=float(changes[before[:,2]<1.49].max()*1000),camera_focus_z_m=focus,nose_ray_m=list(nose))
if flips or report['max_vertex_change_mm']>14 or report['neck_max_change_mm']>.01:raise RuntimeError('Head target failed shape safety checks '+json.dumps(report))
for ob in list(bpy.data.objects):
    if ob not in parts.values():bpy.data.objects.remove(ob,do_unlink=True)
for name,ob in parts.items():
    for uv in list(ob.data.uv_layers)[1:]:ob.data.uv_layers.remove(uv)
    ob.data.uv_layers.active_index=0;ob.data.uv_layers[0].active_render=True;ob.data.materials.clear();mat=bpy.data.materials.new('Photo target '+name);ob.data.materials.append(mat)
    for p in ob.data.polygons:p.material_index=0;p.use_smooth=True
    bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);bpy.context.view_layer.objects.active=ob
    bpy.ops.export_scene.fbx(filepath=str(O/('BekaTarget_'+name+'.fbx')),use_selection=True,object_types={'MESH'},add_leaf_bones=False,bake_anim=False,axis_forward='-Z',axis_up='Y',path_mode='AUTO')
    report['parts'][name]={'vertices':len(ob.data.vertices),'uvs':[uv.name for uv in ob.data.uv_layers]}
if len(head.data.vertices)!=24414:raise RuntimeError('Native UV head vertex count changed')
bpy.ops.wm.save_as_mainfile(filepath=str(O/'Beka_head_photo_target_v27.blend'));(O/'template_report.json').write_text(json.dumps(report,indent=2),encoding='utf8');print('PHOTO_FACE_V27_TARGET',json.dumps(report))
