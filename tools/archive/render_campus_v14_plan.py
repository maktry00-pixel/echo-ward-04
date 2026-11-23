"""Metric ground-level overview from the same JSON loaded by the game."""
import json
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parents[1]
def render():
    d=json.loads((ROOT/'unreal/EchoWard/Content/CampaignData/hospital_world.json').read_text(encoding='utf-8'))
    fontpath=str(ROOT/'unreal/EchoWard/Content/CampaignData/Fonts/NotoSans.ttf');f=ImageFont.truetype(fontpath,18);small=ImageFont.truetype(fontpath,14);title=ImageFont.truetype(fontpath,30)
    im=Image.new('RGB',(1550,1990),'#0d171b');dr=ImageDraw.Draw(im)
    def pt(x,y):return (90+(x+180)*2.9,190+(y+190)*2.9)
    def rect(r,fill,outline=None):
        x,y,w,h=r;dr.rectangle([*pt(x,y),*pt(x+w,y+h)],fill=fill,outline=outline,width=2)
    dr.text((70,25),'ЭХО · БОЛЬНИЧНЫЙ КОМПЛЕКС v14',font=title,fill='#d5e8e6')
    dr.text((70,75),'460 × 570 м · корпуса, периметр, крытые галереи и внешняя торговая зона',font=f,fill='#8eaaa9')
    dr.text((70,110),'Уровень земли. Этажи и подвалы сохранены в игре; полный план открывается по M.',font=f,fill='#8eaaa9')
    for x in range(-180,281,20):dr.line([*pt(x,-190),*pt(x,380)],fill='#18262b',width=1)
    for y in range(-190,381,20):dr.line([*pt(-180,y),*pt(280,y)],fill='#18262b',width=1)
    for road in d['environment_dressing']['roads']:rect(road,'#394347')
    for a in d['campus_plan']['districts']:rect(a['rect'],'#203d30','#3b6650')
    for x,y,w,h,z in d['passages']:
        if z==0 and (w>10 or h>10):rect([x,y,w,h],'#3e6570')
    labels=[]
    for s in d['locations']:
        if s['origin'][2]!=0:continue
        ox,oy,_=s['origin']
        if s['id']=='ST':
            for r in s['rooms']:
                if r['enclosed']:
                    x,y,w,h=r['rect'];rect([ox+x,oy+y,w,h],'#6d5b3b','#b39b6d');labels.append((pt(ox+x+1,oy+y+1),r['id']))
        else:
            for r in s['rooms']:
                x,y,w,h=r['rect'];rect([ox+x,oy+y,w,h],'#283c46','#536a73')
            for x,y,w,h in s['corridors']:rect([ox+x,oy+y,w,h],'#3e6570')
            x,y,w,h=s['bounds'];labels.append((pt(ox+x+4,oy+y+36),s['id']+' · '+s['name']))
    for a,b in d['campus_plan']['clinical_boundary']['segments']:dr.line([*pt(*a),*pt(*b)],fill='#df6156',width=5)
    for g in d['campus_plan']['clinical_boundary']['gates']:
        x,y,_=g['centre'];dr.ellipse([*pt(x-1.5,y-1.5),*pt(x+1.5,y+1.5)],fill='#e4b66e')
    for i in d['campus_plan']['items']:
        if i.get('mount') in ('outside','checkpoint'):
            b=i['bounds'];rect([b[0],b[1],b[3]-b[0],b[4]-b[1]],'#8f9a7b')
    for pos,text in labels:dr.text(pos,text,font=small,fill='#e2edec',stroke_width=1,stroke_fill='#152025')
    for x in range(-180,281,40):dr.text((pt(x,-190)[0]-15,pt(x,-190)[1]-30),str(x),font=small,fill='#859b9c')
    for y in range(-190,381,40):dr.text((25,pt(-180,y)[1]-8),str(y),font=small,fill='#859b9c')
    for text,col,x in [('Стена','#df6156',75),('Крытая галерея','#3e6570',300),('Торговля','#6d5b3b',620),('Дворы','#203d30',840)]:
        dr.rectangle((x,1900,x+22,1922),fill=col);dr.text((x+32,1896),text,font=f,fill='#d5e8e6')
    dr.text((75,1943),'Координаты и сетка в метрах. Схема построена из текущих данных игры.',font=f,fill='#a0bfba')
    im.save(ROOT/'artifacts/unreal_hospital_v14/campus_overview.png')
if __name__=='__main__':render()
