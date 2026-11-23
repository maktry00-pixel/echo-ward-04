from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
import json
R=Path('D:/Projects/ECHO');D=R/'artifacts/echo_cars_v31';base=R/'unreal/EchoWard/Saved/PackageTests/CarsV31/Saved/Prototype/BookWorldCaptures';data=json.loads((R/'unreal/EchoWard/Content/CampaignData/echo_cars_v31.json').read_text());font=ImageFont.truetype('C:/Windows/Fonts/arial.ttf',19)
for batch in range(2):
 canvas=Image.new('RGB',(1920,1240),(12,15,17));draw=ImageDraw.Draw(canvas)
 for i,v in enumerate(data['captures'][batch*16:(batch+1)*16]):
  p=base/('world_'+v['id']+'.png');im=Image.open(p).convert('RGB');im.thumbnail((480,270));x=(i%4)*480;y=(i//4)*310;canvas.paste(im,(x,y));draw.text((x+8,y+277),v['id'].replace('v31_','')[:43],font=font,fill='white')
 canvas.save(D/f'contact_{batch+1}.jpg',quality=92)
p=R/'unreal/EchoWard/Saved/PackageTests/CarsV31/Saved/Prototype/book_world_check.json';x=json.loads(p.read_text(encoding='utf-8-sig'));print('rendered world',x['success'],x['count'],[c['name'] for c in x['checks'] if not c['passed']])
