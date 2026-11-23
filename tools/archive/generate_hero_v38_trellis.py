"""Official free TRELLIS.2 demo; photo transfer explicitly approved in chat."""
from pathlib import Path
import os,sys,time,json,shutil,traceback
R=Path(__file__).resolve().parents[1];A=R/'artifacts/echo_hero_v38';O=R/'assets/models/hero_custom/candidate_v38';O.mkdir(parents=True,exist_ok=True)
os.environ['HF_HUB_DISABLE_IMPLICIT_TOKEN']='1';os.environ['GRADIO_ANALYTICS_ENABLED']='False'
sys.path.insert(0,str(R/'.tools/hero38-client'))
from gradio_client import Client,handle_file
state={'provider':'Microsoft TRELLIS.2 official public demo','endpoint':'https://microsoft-trellis-2.hf.space','resolution':'1536','export_triangles':500000,'texture_size':4096,'seed':38,'status':'connecting','paid_api_used':False,'installed_in_game':False,'photo_transfer_approved':True}
def save():
 (A/'generation_status.json').write_text(json.dumps(state,ensure_ascii=False,indent=2),encoding='utf-8')
def run(client,name,**kwargs):
 print('CALL',name,flush=True);state['stage']=name;save();job=client.submit(api_name=name,**kwargs);started=time.monotonic();last=''
 while not job.done():
  status=job.status();s=str(status.code)
  if s!=last:print('STATUS',name,s,'queue',getattr(status,'rank',None),flush=True);last=s
  if time.monotonic()-started>900:raise TimeoutError('Job still pending; do not submit a duplicate without checking original status')
  time.sleep(5)
 return job.result()
try:
 save();client=Client(state['endpoint'],token=False,download_files=O,verbose=False,analytics_enabled=False)
 run(client,'/start_session')
 src=R/'assets/models/hero_custom/reference_v38/hero_eyes_open_reference.png'
 pre=run(client,'/preprocess_image',input=handle_file(str(src)))
 print('PREPROCESS_RETURN',type(pre).__name__,flush=True)
 if isinstance(pre,(list,tuple)):pre=pre[0]
 params={p['parameter_name']:p['parameter_default'] for p in json.loads((A/'trellis_api.json').read_text())['named_endpoints']['/image_to_3d']['parameters'] if p['parameter_has_default']}
 params.update(image=handle_file(pre),seed=38,resolution='1536')
 state['status']='generating';save();html=run(client,'/image_to_3d',**params)
 (A/'provider_preview.html').write_text(html if isinstance(html,str) else str(html),encoding='utf-8')
 state['status']='extracting';save();result=run(client,'/extract_glb',decimation_target=500000,texture_size=4096)
 files=[]
 for x in result if isinstance(result,(list,tuple)) else [result]:
  p=Path(x) if isinstance(x,str) else Path(x['path']) if isinstance(x,dict) and 'path' in x else None
  if p and p.exists() and p.suffix.lower()=='.glb':files.append(p)
 if not files:raise RuntimeError('No downloaded GLB in export result')
 final=O/'hero_trellis2_1536_4k.glb';shutil.copy2(files[0],final)
 state.update(status='candidate_downloaded',candidate=str(final),bytes=final.stat().st_size);save();print('CANDIDATE_DOWNLOADED',str(final),final.stat().st_size,flush=True)
except Exception as e:
 state.update(status='blocked_or_failed',error_type=type(e).__name__,error=str(e));save();print('GENERATION_STOPPED',type(e).__name__,str(e),flush=True)
 traceback.print_exc();sys.exit(1)
