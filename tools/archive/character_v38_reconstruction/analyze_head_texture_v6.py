import sys,json
from pathlib import Path
sys.path.insert(0,'D:/Projects/ECHO/.tools/hero38-fit')
import mediapipe as mp
R=Path('D:/Projects/ECHO');O=R/'artifacts/echo_hero_v38/likeness_head_v6'
sources={'fitted_clay':R/'artifacts/echo_hero_v38/likeness_geometry_v5/smoothed_front.png','open_portrait':O/'approved_portrait_eyes_open.png'}
opt=mp.tasks.vision.FaceLandmarkerOptions(base_options=mp.tasks.BaseOptions(model_asset_path=str(R/'.tools/hero38-fit/face_landmarker.task')),running_mode=mp.tasks.vision.RunningMode.IMAGE,num_faces=1)
records={}
with mp.tasks.vision.FaceLandmarker.create_from_options(opt) as det:
 for name,p in sources.items():
  im=mp.Image.create_from_file(str(p));r=det.detect(im)
  if not r.face_landmarks:raise RuntimeError('No landmarks:'+name)
  records[name]={'path':str(p),'width':im.width,'height':im.height,'landmarks':[[v.x*im.width,v.y*im.height] for v in r.face_landmarks[0]]}
(O/'texture_landmarks.json').write_text(json.dumps(records,indent=2),encoding='utf8')
print('TEXTURE_LANDMARKS_READY')

