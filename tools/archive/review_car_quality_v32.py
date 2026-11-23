from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
import json,shutil,csv,statistics
R=Path('D:/Projects/ECHO');D=R/'artifacts/echo_car_quality_v32';U=R/'unreal/EchoWard';data=json.loads((U/'Content/CampaignData/echo_cars_v31.json').read_text());base=U/'Saved/PackageTests/CarsV31/Saved/Prototype/BookWorldCaptures';font=ImageFont.truetype('C:/Windows/Fonts/arial.ttf',18)
for batch in range(2):
 im=Image.new('RGB',(1920,1240),(10,13,16));draw=ImageDraw.Draw(im)
 for i,v in enumerate(data['captures'][batch*16:(batch+1)*16]):
  p=base/('world_'+v['id']+'.png');source=Image.open(p).convert('RGB');assert source.size==(1920,1080);source.thumbnail((480,270));x=i%4*480;y=i//4*310;im.paste(source,(x,y));draw.text((x+8,y+275),v['id'].replace('v31_','')[:43],font=font,fill='white')
 im.save(D/f'contact_{batch+1}.jpg',quality=93)
p=D/'gpu_telemetry_repeat.csv';raw=D/'gpu_telemetry_repeat.raw.csv';shutil.copy2(p,raw) if not raw.exists() else None;rows=[r for r in csv.reader(raw.open()) if len(r)==8];p.write_text('\n'.join(','.join(r) for r in rows)+'\n');active=[r for r in rows if all(r[i].strip().split()[0].replace('.','',1).isdigit() for i in [1,2,3,4]) and float(r[2].strip().split()[0])>70]
if active:
 out=dict(valid_samples=len(rows),loaded_samples=len(active),mean_gpu_load_percent=statistics.mean(float(r[2].strip().split()[0]) for r in active),mean_power_w=statistics.mean(float(r[3].strip().split()[0]) for r in active),mean_graphics_mhz=statistics.mean(float(r[4].strip().split()[0]) for r in active),max_temperature_c=max(float(r[1].strip()) for r in active),note='Current v32 repeat only. Historical v31 hardware telemetry was not collected, so this cannot isolate the source of FPS differences. Incomplete final buffered row omitted; raw CSV retained.');(D/'gpu_summary.json').write_text(json.dumps(out,indent=2));print(out)
