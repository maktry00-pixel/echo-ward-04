import bpy,bmesh,math,json,numpy as np
from pathlib import Path
from mathutils import Vector
from mathutils.bvhtree import BVHTree
ROOT=Path('D:/Projects/ECHO');OUT=ROOT/'artifacts/echo_hero_v38/likeness_geometry_v5'
bpy.ops.wm.open_mainfile(filepath=str(OUT/'neutral_source.blend'))
face=bpy.data.objects['Neutral_Source_Head']
records=json.loads((OUT/'landmarks_analysis.json').read_text(encoding='utf8'))['images']
src=np.array(records['source_front']['landmarks_px']);ref=np.array(records['real_front']['landmarks_px'])
S=.33;RES=1024;Z=1.62
# Register reference camera roll/scale using outer eye corners. Do not fit target identity with synthetic views.
def eye_frame(p):
 a,b=p[33],p[263];u=(b-a)/np.linalg.norm(b-a);v=np.array([-u[1],u[0]])
 if v[1]<0:v=-v
 return (a+b)/2,np.stack([u,v],axis=1),np.linalg.norm(b-a)
so,sb,ss=eye_frame(src);ro,rb,rs=eye_frame(ref)
target=(ref-ro)@rb*(ss/rs)@sb.T+so
verts=[face.matrix_world@v.co for v in face.data.vertices]
faces=[list(p.vertices) for p in face.data.polygons if p.material_index==0]
bvh=BVHTree.FromPolygons(verts,faces,all_triangles=False)
# Lower outline, nasal surface, lips and brow. Eyelid shape is deliberately excluded: real reference eyes closed.
oval=[10,338,297,332,284,251,389,356,454,323,361,288,397,365,379,378,400,377,152,148,176,149,150,136,172,58,132,93,234,127,162,21,54,103,67,109]
nose=[168,6,197,195,5,4,1,2,98,97,99,64,49,48,115,220,45,275,440,344,278,279,294,326,327,328,329,290,250,460]
lips=[0,17,61,291,37,39,40,185,267,269,270,409,146,91,181,84,314,405,321,375,18,200,199,175]
brows=[70,63,105,66,107,336,296,334,293,300,9,8,151]
# Only eye corners control spacing. Keep natural open eyelids of the 3D base.
corners=[33,133,263,362]
indices=sorted(set(oval+nose+lips+brows+corners))
samples=[];missed=[]
landmark3d={}
for idx in range(468):
 x=(src[idx,0]-512)*S/RES;z=Z+(512-src[idx,1])*S/RES
 hit,normal,poly,distance=bvh.ray_cast(Vector((x,-.8,z)),Vector((0,1,0)),2)
 if hit is None:
  near=bvh.find_nearest(Vector((x,-.11,z)),.035)
  hit=near[0] if near else None
 if hit is not None:landmark3d[idx]=np.array(hit)
for idx in indices:
 if idx not in landmark3d:missed.append(idx);continue
 p=landmark3d[idx];wanted=np.array([(target[idx,0]-512)*S/RES,Z+(512-target[idx,1])*S/RES])
 d=wanted-p[[0,2]]
 d=np.clip(d,-.018,.018)
 # Forehead points under hair are ambiguous; constrain them weakly toward the base.
 if idx in oval[:8] or idx in [127,162,21,54,103,67,109]:d*=.35
 samples.append((idx,p,d))
# TPS in frontal X/Z, with extra neutral skull/neck anchors.
C=[];D=[];metadata=[]
for idx,p,d in samples:
 C.append([p[0],p[2]-Z]);D.append(d);metadata.append({'index':idx,'source3d':p.tolist(),'delta_xz_m':d.tolist()})
for x,z in [(-.10,1.70),(.10,1.70),(0,1.75),(-.09,1.47),(.09,1.47),(0,1.43),(-.14,1.53),(.14,1.53)]:
 C.append([x,z-Z]);D.append([0,0])
C=np.array(C);D=np.array(D)
def kernel(d):
 r2=np.maximum(np.sum(d*d,axis=-1),1e-16)
 return r2*np.log(r2)
P=np.column_stack((np.ones(len(C)),C))
A=np.block([[kernel(C[:,None,:]-C[None,:,:])+np.eye(len(C))*3e-6,P],[P.T,np.zeros((3,3))]])
coef=np.linalg.solve(A,np.vstack((D,np.zeros((3,2)))))
def front_warp(arr):
 X=arr[:,[0,2]].copy();X[:,1]-=Z
 delta=kernel(X[:,None,:]-C[None,:,:])@coef[:len(C)]+np.column_stack((np.ones(len(X)),X))@coef[len(C):]
 weight=np.clip((.07-arr[:,1])/.15,0,1)*np.clip((arr[:,2]-1.46)/.06,0,1)
 result=arr.copy();result[:,0]+=np.clip(delta[:,0],-.018,.018)*weight;result[:,2]+=np.clip(delta[:,1],-.018,.018)*weight
 return result
# Real profile contour guides. These are manual estimates on low-resolution original-mask silhouettes,
# not calibrated biometric measurements. Anchor nose bridge/chin, then average two independent profiles.
profile_guides={
 'right_facing':{'bridge':[575,235],'chin':[569,309],'points':[[575,235],[581,245],[590,260],[579,270],[579,278],[578,285],[574,300],[569,309]],'sign':-1},
 'left_facing':{'bridge':[402,219],'chin':[407,290],'points':[[402,219],[396,230],[389,243],[398,253],[399,263],[402,273],[405,285],[407,290]],'sign':1}
}
nosebridge=front_warp(landmark3d[168][None,:])[0];chin=front_warp(landmark3d[152][None,:])[0]
curves=[]
for name,g in profile_guides.items():
 sign=g['sign']
 a=np.array(g['bridge'],float);b=np.array(g['chin'],float)
 destA=np.array([512+sign*nosebridge[1]*RES/S,512-(nosebridge[2]-Z)*RES/S])
 destB=np.array([512+sign*chin[1]*RES/S,512-(chin[2]-Z)*RES/S])
 u=(b-a)/np.linalg.norm(b-a);v=np.array([-u[1],u[0]]);dstu=(destB-destA)/np.linalg.norm(destB-destA);dstv=np.array([-dstu[1],dstu[0]])
 srcframe=np.stack([u,v],axis=1);dstframe=np.stack([dstu,dstv],axis=1)
 pp=(np.array(g['points'])-a)@srcframe*(np.linalg.norm(destB-destA)/np.linalg.norm(b-a))@dstframe.T+destA
 ys=(pp[:,0]-512)*S/RES/sign;zs=Z+(512-pp[:,1])*S/RES
 order=np.argsort(zs);curves.append((zs[order],ys[order]))
def profile_warp(arr):
 r=arr.copy()
 # Keep shape continuity: desired silhouette depth relative to original central facial surface.
 zgrid=np.linspace(1.515,1.66,140)
 original=np.array([min((v.y for v in verts if abs(v.x)<.006 and abs(v.z-z)<.0025),default=-.09) for z in zgrid])
 desired=np.mean([np.interp(zgrid,z,y) for z,y in curves],axis=0)
 dy=np.clip(desired-original,-.012,.012)
 factor=np.exp(-(arr[:,0]/.047)**4)*np.clip((.015-arr[:,1])/.10,0,1)
 factor*=np.clip((arr[:,2]-1.505)/.025,0,1)*np.clip((1.675-arr[:,2])/.025,0,1)
 r[:,1]+=np.interp(arr[:,2],zgrid,dy)*factor
 return r
maxdisp=0
for o in [o for o in bpy.context.scene.objects if o.type=='MESH']:
 mw=o.matrix_world.copy();inv=mw.inverted();original=np.array([tuple(mw@v.co) for v in o.data.vertices])
 transformed=profile_warp(front_warp(original))
 if o==face:maxdisp=float(np.linalg.norm(transformed-original,axis=1).max())
 for v,p in zip(o.data.vertices,transformed):v.co=inv@Vector(p)
 o.data.update()
# Keep original model untouched; new facial form is a separate shape-study mesh.
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Beka_head_geometry_study_v5.blend'))
bpy.ops.object.select_all(action='DESELECT')
for o in bpy.context.scene.objects:
 if o.type=='MESH':o.select_set(True)
bpy.context.view_layer.objects.active=face
bpy.ops.export_scene.gltf(filepath=str(OUT/'Beka_head_geometry_study_v5.glb'),use_selection=True,export_format='GLB',export_apply=True,export_animations=False)
report={'source':'Real frontal photograph plus real profile silhouette estimates. Approved enlarged face used for visual comparison only. No synthetic turntable used as target geometry.','front_landmark_controls':len(samples),'missed_landmarks':missed,'max_vertex_displacement_m':maxdisp,'profile_guides':profile_guides,'profile_guides_are_estimates':True,'clay_review':True,'hair_present':False,'likeness_approved':False,'game_changed':False,'controls':metadata}
(OUT/'fit_report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8')
cam=bpy.context.scene.camera;sc=bpy.context.scene;sc.cycles.samples=24
for name,pos in [('fitted_front',(0,-1,1.62)),('fitted_nose_right',(-1,0,1.62)),('fitted_nose_left',(1,0,1.62)),('fitted_three_quarter',(-.7,-1,1.62))]:
 cam.location=pos;cam.rotation_euler=(Vector((0,0,1.62))-cam.location).to_track_quat('-Z','Y').to_euler();sc.render.filepath=str(OUT/(name+'.png'));bpy.ops.render.render(write_still=True)
print('FACIAL_FORM_FIT_COMPLETE')

