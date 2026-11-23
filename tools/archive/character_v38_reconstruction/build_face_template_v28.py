"""Refine lower-face volume and silhouette only; no material or clothing edits."""
from pathlib import Path
import bpy,json,numpy as np,math
R=Path('D:/Projects/ECHO');S=R/'artifacts/echo_hero_v38/metahuman_conform_v27';O=R/'artifacts/echo_hero_v38/metahuman_conform_v28';O.mkdir(exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(S/'Beka_head_photo_target_v27.blend'));bpy.context.preferences.filepaths.save_version=0
head=bpy.data.objects['BekaTarget_Head'];before=np.array([tuple(v.co) for v in head.data.vertices]);after=before.copy()
for i,p in enumerate(before):
    x,y,z=p;front=float(np.clip((.025-y)/.07,0,1));neck=float(np.clip((z-1.49)/.02,0,1))
    lower=math.exp(-((z-1.540)/.031)**2);center=math.exp(-(x/.047)**4)
    # Round the narrow pointed chin and soften the overly projecting lower jaw.
    after[i,0]+=x*.048*lower*front*neck
    after[i,2]+=.0042*math.exp(-((z-1.522)/.022)**2)*math.exp(-(x/.052)**4)*front*neck
    after[i,1]+=.0058*math.exp(-((z-1.537)/.026)**2)*center*front*neck
    # Reduce the protruding lip shelf without changing lip color or the UV atlas.
    after[i,1]+=.0016*math.exp(-((z-1.574)/.013)**2-(x/.031)**4)*front*neck
    # The source brow line is flatter than the arched stock sculpt.
    after[i,2]-=.0013*math.exp(-((abs(x)-.038)/.017)**2-((z-1.669)/.014)**2)*front*neck
triangles=np.array([p.vertices[:3] for p in head.data.polygons]);a,b,c=triangles.T;old=np.cross(before[b]-before[a],before[c]-before[a])
def folded():return np.where(np.einsum('ij,ij->i',old,np.cross(after[b]-after[a],after[c]-after[a]))<0)[0]
steps=0
while len(folded()) and steps<24:
    bad=set(triangles[folded()].ravel().tolist());near=set()
    for poly in head.data.polygons:
        if any(i in bad for i in poly.vertices):near.update(poly.vertices)
    for i in near:after[i]=before[i]+(after[i]-before[i])*(.60 if i in bad else .85)
    steps+=1
change=np.linalg.norm(after-before,axis=1)
report={'source':'Face-only V27 fitted to real photo','skin_color_changed':False,'clothing_changed':False,'body_changed':False,'likeness_approved':False,'parts':{},'flipped_faces':int(len(folded())),'max_incremental_change_mm':float(change.max()*1000),'neck_max_change_mm':float(change[before[:,2]<1.49].max()*1000),'corrections':['wider rounder lower jaw','shorter less projecting chin','subtle flatter brow volume','reduced lip shelf']}
if report['flipped_faces'] or report['neck_max_change_mm']>.01:raise RuntimeError('Unsafe lower-face refinement')
for v,p in zip(head.data.vertices,after):v.co=p
head.data.update()
for name in ['Head','LeftEye','RightEye']:
    ob=bpy.data.objects['BekaTarget_'+name];bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);bpy.context.view_layer.objects.active=ob
    bpy.ops.export_scene.fbx(filepath=str(O/('BekaTarget_'+name+'.fbx')),use_selection=True,object_types={'MESH'},add_leaf_bones=False,bake_anim=False,axis_forward='-Z',axis_up='Y',path_mode='AUTO')
    report['parts'][name]={'vertices':len(ob.data.vertices),'uvs':[uv.name for uv in ob.data.uv_layers]}
bpy.ops.wm.save_as_mainfile(filepath=str(O/'Beka_head_photo_target_v28.blend'));(O/'template_report.json').write_text(json.dumps(report,indent=2),encoding='utf8');print('FACE_V28_TARGET',json.dumps(report))
