from pathlib import Path
import sys,json
sys.path.insert(0,'D:/Projects/ECHO/.tools/hero38-fit')
import numpy as np,mediapipe as mp,cv2
root=Path('D:/Projects/ECHO')
p=root/'assets/models/hero_custom/turnaround_synthetic_v1/images/hero_000.png'
a=cv2.cvtColor(cv2.imread(str(p)),cv2.COLOR_BGR2RGB)
# Analysis-only crop; no image editing or synthetic pixels are written.
x0,y0,x1,y1=420,70,650,300
crop=np.ascontiguousarray(a[y0:y1,x0:x1])
options=mp.tasks.vision.FaceLandmarkerOptions(base_options=mp.tasks.BaseOptions(model_asset_path=str(root/'.tools/hero38-fit/face_landmarker.task')),running_mode=mp.tasks.vision.RunningMode.IMAGE,num_faces=1)
with mp.tasks.vision.FaceLandmarker.create_from_options(options) as d:
 r=d.detect(mp.Image(image_format=mp.ImageFormat.SRGB,data=crop))
 if not r.face_landmarks:raise RuntimeError('Face not detected')
 xy=[[q.x*(x1-x0)+x0,q.y*(y1-y0)+y0] for q in r.face_landmarks[0]]
out=root/'artifacts/echo_hero_v38/manual_assembly_v1/target_face_landmarks.json'
out.write_text(json.dumps({'source':str(p),'synthetic_reference':True,'local_analysis':True,'landmarks_px':xy},indent=2),encoding='utf8')
for name,idx in [('left_eye_outer',33),('left_eye_inner',133),('right_eye_inner',362),('right_eye_outer',263),('nose_tip',1),('nose_left',98),('nose_right',327),('lip_left',61),('lip_right',291),('upper_lip',0),('lower_lip',17),('chin',152),('cheek_left',234),('cheek_right',454),('forehead',10)]:print(name,idx,xy[idx])
