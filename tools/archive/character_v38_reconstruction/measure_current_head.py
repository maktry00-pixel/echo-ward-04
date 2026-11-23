import sys,json,numpy as np
from pathlib import Path
sys.path.insert(0,'D:/Projects/ECHO/.tools/hero38-fit')
import mediapipe as mp
R=Path('D:/Projects/ECHO');O=R/'artifacts/echo_hero_v38/likeness_head_v7'
opt=mp.tasks.vision.FaceLandmarkerOptions(base_options=mp.tasks.BaseOptions(model_asset_path=str(R/'.tools/hero38-fit/face_landmarker.task')),running_mode=mp.tasks.vision.RunningMode.IMAGE,num_faces=1)
rec=json.loads((R/'artifacts/echo_hero_v38/likeness_head_v6/texture_landmarks.json').read_text(encoding='utf8'))
with mp.tasks.vision.FaceLandmarker.create_from_options(opt) as det:
 for v in [6,7]:
  im=mp.Image.create_from_file(str(R/f'artifacts/echo_hero_v38/likeness_head_v{v}/front.png'));res=det.detect(im)
  if res.face_landmarks:rec[f'candidate{v}']={'landmarks':[[l.x*im.width,l.y*im.height] for l in res.face_landmarks[0]],'width':im.width,'height':im.height}
for name,a in rec.items():
 p=np.array(a['landmarks']);w=np.linalg.norm(p[33]-p[263]);z=(p[33,1]+p[263,1])/2
 vals={k:round(float(x/w),3) for k,x in {'cheeks':np.linalg.norm(p[234]-p[454]),'jaw':np.linalg.norm(p[172]-p[397]),'nose':np.linalg.norm(p[98]-p[327]),'mouth':np.linalg.norm(p[61]-p[291]),'eye_height':np.linalg.norm(p[159]-p[145]),'chin_drop':p[152,1]-z,'nose_drop':p[1,1]-z,'mouth_drop':p[13,1]-z}.items()}
 print(name,vals)
(O/'measured_landmarks.json').write_text(json.dumps(rec),encoding='utf8')
