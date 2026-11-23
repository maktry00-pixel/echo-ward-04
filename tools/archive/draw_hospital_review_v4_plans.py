"""Small metric diagrams clarifying the DG support and service tunnel review."""
from pathlib import Path
import json
from PIL import Image,ImageDraw,ImageFont

root=Path(__file__).resolve().parents[1];d=json.loads((root/'unreal/EchoWard/Content/CampaignData/hospital_world.json').read_text(encoding='utf-8'))
out=root/'artifacts/unreal_hospital_v4';sites={l['id']:l for l in d['locations']}
font_path=str(root/'unreal/EchoWard/Content/CampaignData/Fonts/NotoSans.ttf')
font=ImageFont.truetype(font_path,18);small=ImageFont.truetype(font_path,14);title=ImageFont.truetype(font_path,28)

for key in ('DG','DG2','UT'):
    site=sites[key];ox,oy,z=site['origin'];bx,by,bw,bh=site['bounds']
    scale=min(1640/bw,1400/bh);im=Image.new('RGB',(1840,1640),'#f0f5f5');draw=ImageDraw.Draw(im)
    draw.text((80,25),f'ЭХО / {key} / отметка пола Z={z:g} м',font=title,fill='#16343b')
    draw.text((80,68),'Сетка 10 м. X и Y - мировые метры; ось Y направлена вниз.',font=font,fill='#334e56')
    def xy(x,y):return 100+(x-ox-bx)*scale,125+(y-oy-by)*scale
    def rect(r,fill=None,outline=None,width=1):
        x,y,w,h=r;a,b=xy(x,y);c,e=xy(x+w,y+h);draw.rectangle((a,b,c,e),fill=fill,outline=outline,width=width)
    def local(r):return [ox+r[0],oy+r[1],*r[2:]]
    for x in range(int((ox+bx)//10)*10,int(ox+bx+bw)+1,10):
        a,b=xy(x,oy+by);c,e=xy(x,oy+by+bh);draw.line((a,b,c,e),fill='#d0dcde');draw.text((a,105),str(x),font=small,fill='#526c74')
    for y in range(int((oy+by)//10)*10,int(oy+by+bh)+1,10):
        a,b=xy(ox+bx,y);c,e=xy(ox+bx+bw,y);draw.line((a,b,c,e),fill='#d0dcde');draw.text((48,b-8),str(y),font=small,fill='#526c74')
    for r in site['corridors']:rect(local(r),'#c7d9dc','#5b7e88')
    if key=='DG':
        for r in d['passages']:
            if r[4]==0 and r[0]==-139 and r[1] in (46,68):rect(r[:4],'#b9dbd3','#47796d')
    for r in site['rooms']:
        if r.get('hidden'):continue
        rr=local(r['rect']);rect(rr,'#e9eded','#213f48',2);a,b=xy(rr[0]+.6,rr[1]+.5)
        draw.text((a,b),r['id'],font=small,fill='#132e36')
    for n in d['escape_niches']:
        if n['site']==key:rect(n['rect'],'#83b5a2','#49745e')
    for s in d['stairs']:
        if s['id']==key or (key=='DG2' and s['id']=='DG') or key=='UT':
            if not s['z_min']<=z<=s['z_max']:continue
            if key=='UT' and s['id'] in ('A','B','C'):continue
            rect(s['rect'],'#edc68a','#936427',2);a,b=xy(s['rect'][0]+.3,s['rect'][1]+8.5)
            draw.text((a,b),'Лестн. '+s['id'],font=small,fill='#76551c')
    if key=='DG':
        for s in d['structural_members']:
            if s['kind']=='column':x,y,_=s['at'];rect([x-.18,y-.18,.36,.36],'#b8393e','#b8393e',3)
        for x,y in ((-134,53),(-134,76)):a,b=xy(x,y);draw.text((a,b),'Нижний зал / опоры DG2',font=small,fill='#306357')
    if key=='UT':
        for link in d['utility_links'][:6]:
            points=[xy(p[0],p[1]) for p in link['points']];draw.line(points,fill='#b93540',width=4)
        for n in d['infrastructure_zones']:
            x,y,w,h=n['rect'];a,b=xy(x+w/2,y+4);draw.text((a-35,b),n['id'],font=small,fill='#18343e')
        r=next(r for r in sites['SV']['rooms'] if r['id']=='SV24');rr=[sites['SV']['origin'][0]+r['rect'][0],sites['SV']['origin'][1]+r['rect'][1],*r['rect'][2:]]
        rect(rr,None,'#b57924',3);a,b=xy(rr[0]-3,rr[1]-12);draw.text((a,b),'SV24: Z=0 м',font=font,fill='#8b5b14')
        draw.line([xy(q[0],q[1]) for q in d['utility_links'][-1]['points'][1:]],fill='#b57924',width=4)
        a,b=xy(204,316);draw.text((a,b),'Подъём SV,\nдалее по Z=0',font=small,fill='#8b5b14')
        a,b=xy(54,55);draw.text((a,b),'Связь с B1\nна том же Z',font=small,fill='#284c55')
        a,b=xy(53,145);draw.text((a,b),'U_SPINE\nZ=-4.8 м',font=small,fill='#8d2931')
    draw.text((80,1555),'Зелёный: коридорные ниши | Жёлтый: лестница | Красный: опоры / маршрут тоннеля',font=font,fill='#284c55')
    draw.text((80,1590),'Схема построена из hospital_world.json. Точные конструкции и дверные проёмы - в CSV и игре.',font=font,fill='#284c55')
    im.save(out/f'plan_{key}_review.png')
print('DG / DG2 / UT metric diagrams written')
