from pathlib import Path
import sys,json,math
sys.path.insert(0,'D:/Projects/ECHO/.tools/hero38-fit')
import numpy as np,mediapipe as mp
root=Path('D:/Projects/ECHO');out=root/'artifacts/echo_hero_v38/measurements';out.mkdir(parents=True,exist_ok=True)
inputs={'approved_closeup':root/'assets/models/hero_custom/reference_v38/hero_face_closeup_ai_v1.png','original':root/'assets/models/hero_custom/reference_v38/hero_original.jpg','previous_model':root/'artifacts/echo_hero_v38/review_verified_4k/front.png','photo_v2':root/'artifacts/echo_hero_v38/photo_fit_v2/review/front.png','photo_v3':root/'artifacts/echo_hero_v38/photo_fit_v3/review/front.png'}
options=mp.tasks.vision.FaceLandmarkerOptions(base_options=mp.tasks.BaseOptions(model_asset_path=str(root/'.tools/hero38-fit/face_landmarker.task')),running_mode=mp.tasks.vision.RunningMode.IMAGE,num_faces=1,output_facial_transformation_matrixes=True)
r={'processing':'local_only','uploaded':False,'measurements_are_likeness_approval':False,'images':{}}
with mp.tasks.vision.FaceLandmarker.create_from_options(options) as detector:
 for label,path in inputs.items():
  if not path.exists():continue
  image=mp.Image.create_from_file(str(path));result=detector.detect(image)
  if not result.face_landmarks:r['images'][label]={'face_found':False};continue
  xy=np.array([[p.x*image.width,p.y*image.height] for p in result.face_landmarks[0]])
  left,right=xy[33],xy[263];origin=(left+right)/2;u=(right-left)/np.linalg.norm(right-left);v=np.array([-u[1],u[0]]);scale=np.linalg.norm(right-left)
  if v[1]<0:v=-v
  norm=np.stack(((xy-origin)@u/scale,(xy-origin)@v/scale),axis=1)
  dist=lambda a,b:float(np.linalg.norm(norm[a]-norm[b]))
  metrics={'cheek_width':dist(234,454),'jaw_width':dist(172,397),'nose_width':dist(98,327),'mouth_width':dist(61,291),'lip_outer_height':dist(0,17),'eye_to_nose':float(norm[1,1]),'eye_to_mouth':float(norm[13,1]),'eye_to_chin':float(norm[152,1])}
  row={'face_found':True,'size':[image.width,image.height],'eye_span_pixels':float(scale),'metrics_in_eye_spans':metrics,'landmarks_px':xy.tolist(),'normalized_landmarks':norm.tolist(),'transform':np.asarray(result.facial_transformation_matrixes[0]).tolist()}
  r['images'][label]=row
  print(label,json.dumps(metrics))
(out/'face_proportions.json').write_text(json.dumps(r,ensure_ascii=False,indent=2),encoding='utf-8')
