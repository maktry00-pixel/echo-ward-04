import sys,json
from pathlib import Path
sys.path.insert(0,'D:/Projects/ECHO/.tools/hero38-fit')
import cv2,numpy as np,mediapipe as mp
R=Path('D:/Projects/ECHO');O=R/'artifacts/echo_hero_v38/likeness_geometry_v5'
specs=[
 ('real_front',Path('C:/Users/YOUR_WINDOWS_USER/Downloads/photo_5409270847438003747_y.jpg'),(405,195,575,365)),
 ('real_nose_right',Path('C:/Users/YOUR_WINDOWS_USER/Downloads/photo_5409270847438003749_y.jpg'),(390,155,620,350)),
 ('real_nose_left',Path('C:/Users/YOUR_WINDOWS_USER/Downloads/photo_5409270847438003750_y.jpg'),(360,135,565,335)),
 ('approved_closeup',R/'assets/models/hero_custom/reference_v38/hero_face_closeup_ai_v1.png',None),
 ('source_front',O/'source_front.png',None),
 ('source_nose_right',O/'source_nose_right.png',None),
 ('source_nose_left',O/'source_nose_left.png',None)]
opt=mp.tasks.vision.FaceLandmarkerOptions(base_options=mp.tasks.BaseOptions(model_asset_path=str(R/'.tools/hero38-fit/face_landmarker.task')),running_mode=mp.tasks.vision.RunningMode.IMAGE,num_faces=1)
rows={}
with mp.tasks.vision.FaceLandmarker.create_from_options(opt) as d:
 for name,path,box in specs:
  if not path.exists():continue
  a=cv2.cvtColor(cv2.imread(str(path)),cv2.COLOR_BGR2RGB);H,W=a.shape[:2]
  x0,y0,x1,y1=box or (0,0,W,H);crop=a[y0:y1,x0:x1]
  # Resampling is only transient input for local feature analysis. No edited raster is written.
  c=cv2.resize(crop,(768,768),interpolation=cv2.INTER_CUBIC) if box else crop
  r=d.detect(mp.Image(image_format=mp.ImageFormat.SRGB,data=np.ascontiguousarray(c)))
  if not r.face_landmarks:rows[name]={'detected':False,'source':str(path)};print(name,'NO_FACE');continue
  xy=[[p.x*(x1-x0)+x0,p.y*(y1-y0)+y0] for p in r.face_landmarks[0]]
  rows[name]={'detected':True,'source':str(path),'original_size':[W,H],'analysis_crop':box,'landmarks_px':xy}
  print(name,'detected', 'nose',xy[1],'chin',xy[152])
(O/'landmarks_analysis.json').write_text(json.dumps({'local_only':True,'images':rows},ensure_ascii=False,indent=2),encoding='utf8')

