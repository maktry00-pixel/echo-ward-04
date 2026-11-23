"""Face-only profile and eyelid refinement; preserve accepted appearance assets."""
from pathlib import Path
import bpy,json,numpy as np,math
R=Path('D:/Projects/ECHO');S=R/'artifacts/echo_hero_v38/metahuman_conform_v28';O=R/'artifacts/echo_hero_v38/metahuman_conform_v29';O.mkdir(exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(S/'Beka_head_photo_target_v28.blend'));bpy.context.preferences.filepaths.save_version=0
head=bpy.data.objects['BekaTarget_Head'];before=np.array([tuple(v.co) for v in head.data.vertices]);after=before.copy()
peak=before[(abs(before[:,0])<.009)&(before[:,2]>1.587)&(before[:,2]<1.628)]
nose=peak[np.argmin(peak[:,1])]
for i,p in enumerate(before):
    x,y,z=p;front=float(np.clip((-.067-y)/.035,0,1))
    # Correct the too projecting stock nasal profile while keeping frontal widths.
    after[i,1]+=.0036*math.exp(-(x/.020)**2-((z-nose[2])/.020)**2)*front
    after[i,1]+=.0011*math.exp(-(x/.013)**2-((z-1.627)/.020)**2)*front
    after[i,1]+=.0012*math.exp(-(x/.028)**4-((z-1.577)/.015)**2)*front
eye_report={}
for name in ['LeftEye','RightEye']:
    eye=np.array([tuple(v.co) for v in bpy.data.objects['BekaTarget_'+name].data.vertices]);M=np.column_stack([2*eye,np.ones(len(eye))]);v=np.linalg.lstsq(M,np.sum(eye*eye,axis=1),rcond=None)[0];center=v[:3];radius=math.sqrt(max(0.,v[3]+np.dot(center,center)))
    for i,p in enumerate(before):
        d=p-center
        if d[1]>-.001 or abs(d[0])>.025 or abs(d[2])>.022:continue
        wt=math.exp(-(d[0]/.020)**6-(d[2]/.015)**4)
        after[i,2]-=d[2]*.19*wt
        dist=np.linalg.norm(d)
        if radius-.001<dist<radius+.006 and abs(d[0])<radius*1.35 and abs(d[2])<radius*1.15:
            q=after[i];under=dist*dist-(q[0]-center[0])**2-(q[2]-center[2])**2
            if under>0:after[i,1]=center[1]-math.sqrt(under)
    eye_report[name]={'center_m':center.tolist(),'radius_m':radius}
tri=np.array([p.vertices[:3] for p in head.data.polygons]);a,b,c=tri.T;normal=np.cross(before[b]-before[a],before[c]-before[a])
def folded():return np.where(np.einsum('ij,ij->i',normal,np.cross(after[b]-after[a],after[c]-after[a]))<0)[0]
initial=len(folded());steps=0
while len(folded()) and steps<24:
    bad=set(tri[folded()].ravel().tolist());near=set()
    for p in head.data.polygons:
        if any(i in bad for i in p.vertices):near.update(p.vertices)
    for i in near:after[i]=before[i]+(after[i]-before[i])*(.60 if i in bad else .85)
    steps+=1
change=np.linalg.norm(after-before,axis=1);report={'source':'Face-only V28','skin_color_changed':False,'clothing_changed':False,'body_changed':False,'likeness_approved':False,'parts':{},'eyes':eye_report,'nose_peak_m':nose.tolist(),'flipped_faces':int(len(folded())),'initial_flipped_faces':initial,'repair_steps':steps,'max_incremental_change_mm':float(change.max()*1000),'neck_max_change_mm':float(change[before[:,2]<1.49].max()*1000)}
if report['flipped_faces'] or report['neck_max_change_mm']>.01:raise RuntimeError('Unsafe profile/eyelid target')
for v,p in zip(head.data.vertices,after):v.co=p
head.data.update()
for name in ['Head','LeftEye','RightEye']:
    ob=bpy.data.objects['BekaTarget_'+name];bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);bpy.context.view_layer.objects.active=ob
    bpy.ops.export_scene.fbx(filepath=str(O/('BekaTarget_'+name+'.fbx')),use_selection=True,object_types={'MESH'},add_leaf_bones=False,bake_anim=False,axis_forward='-Z',axis_up='Y',path_mode='AUTO')
    report['parts'][name]={'vertices':len(ob.data.vertices),'uvs':[uv.name for uv in ob.data.uv_layers]}
bpy.ops.wm.save_as_mainfile(filepath=str(O/'Beka_head_photo_target_v29.blend'));(O/'template_report.json').write_text(json.dumps(report,indent=2),encoding='utf8');print('FACE_V29_TARGET',json.dumps(report))
