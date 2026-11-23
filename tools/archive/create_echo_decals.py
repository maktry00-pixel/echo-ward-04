"""Original transparent decal maps and forest ground PBR, all offline."""
from pathlib import Path
import numpy as np
from PIL import Image,ImageDraw,ImageFilter,ImageFont
import json,math,random
OUT=Path(__file__).resolve().parents[1]/'assets/models/echo_custom/materials';N=2048
random.seed(40926);rng=np.random.default_rng(40926)
font=ImageFont.truetype('C:/Windows/Fonts/segoeuib.ttf',400)
y,x=np.mgrid[:N,:N];grain=rng.random((N,N));cloud=np.asarray(Image.fromarray((rng.random((40,40))*255).astype('uint8')).resize((N,N),Image.Resampling.BICUBIC))/255
for i in range(506,531):
    folder=OUT/f'{i:03d}_'+Path('x') if False else OUT/f'{i:03d}_surface'
    folder.mkdir(exist_ok=True);mask=Image.new('L',(N,N));d=ImageDraw.Draw(mask)
    color=(70,57,40);transparent=i>=510
    if i<=509:
        colors={506:((37,30,20),(92,74,45)),507:((50,44,31),(118,101,69)),508:((66,68,61),(139,136,119)),509:((22,39,18),(74,96,42))}
        a,b=map(np.array,colors[i]);c=a[None,None,:]*(1-cloud[...,None])+b[None,None,:]*cloud[...,None]
        c*=.85+grain[...,None]*.3;rgba=np.dstack((np.clip(c,0,255).astype('uint8'),np.full((N,N),255,dtype='uint8')))
        rough=.88+grain*.1;h=grain*.001+cloud*.003
    else:
        if i in (510,512,513,515,519,520,524,525):
            for j in range(240):
                px=random.gauss(N/2,N*.15);py=random.gauss(N/2,N*.16);r=random.uniform(5,180)
                d.ellipse((px-r,py-r,px+r,py+r),fill=random.randint(60,200))
            if i in (512,515):
                m=np.asarray(mask,dtype=np.float32)*np.clip((cloud-.27)*2,0,1);mask=Image.fromarray(m.astype('uint8'))
            else:mask=mask.filter(ImageFilter.GaussianBlur(8 if i!=519 else 1))
            color={510:(29,37,30),512:(21,34,19),513:(82,36,12),515:(143,146,119),519:(82,8,5),520:(47,5,4),524:(84,58,26),525:(12,14,13)}[i]
        elif i in (511,518,521,523):
            count=22 if i!=523 else 2
            for j in range(count):
                px=random.randint(450,1600) if i!=523 else (700+j*650);width=random.randint(6,60);length=random.randint(300,1400)
                d.line([(px,250),(px+random.randint(-35,35),250+length)],fill=random.randint(80,210),width=width)
            mask=mask.filter(ImageFilter.GaussianBlur(5));color=(52,57,43) if i in (511,523) else (70,8,5)
        elif i in (514,526):
            for j in range(12 if i==514 else 5):
                p=(random.randint(400,1300),random.randint(350,1500))
                for q in range(30):
                    v=(p[0]+random.randint(-20,30),p[1]+random.randint(10,26));d.line([p,v],fill=200,width=random.randint(2,5));p=v
            color=(22,24,21) if i==514 else (100,80,56)
        elif i in (516,517):
            # Anatomical palm plus five unequal fingers; distressed alpha.
            d.ellipse((760,850,1260,1390),fill=205)
            for j,(length,dx) in enumerate([(390,-50),(610,0),(690,0),(610,0)]):
                px=775+j*135;d.line([(px,1000),(px+dx,1000-length)],fill=220,width=96);d.ellipse((px+dx-48,1000-length-48,px+dx+48,1000-length+48),fill=220)
            d.line([(800,1120),(555,820)],fill=190,width=115);d.ellipse((497,762,612,877),fill=190)
            alpha=np.asarray(mask,dtype=np.float32)*np.clip(cloud*1.6,.1,1)*(grain*.55+.45);mask=Image.fromarray(alpha.astype('uint8'));color=(35,32,23) if i==516 else (83,10,7)
        elif i==522:
            d.rounded_rectangle((790,500,1250,1350),radius=180,fill=205);d.ellipse((820,1370,1220,1700),fill=195)
            for yy in range(650,1330,95):d.line([(800,yy),(1240,yy+45)],fill=0,width=25)
            for xx in [930,1100]:d.line([(xx,550),(xx,1300)],fill=0,width=20)
            color=(30,28,20)
        elif i==527:
            d.text((N/2,820),'04',font=font,fill=220,anchor='mm');color=(125,133,112)
        elif i==528:
            poly=[(450,350),(1600,370),(1530,1100),(1370,1020),(1290,1210),(1130,1170),(1030,1380),(810,1290),(610,1540),(440,1100)];d.polygon(poly,fill=200);color=(154,147,121)
        elif i==529:
            d.rectangle((750,0,1000,N),fill=230);d.polygon([(1200,1400),(1200,700),(1080,700),(1400,400),(1710,700),(1590,700),(1590,1400)],fill=230);color=(190,182,139)
        elif i==530:
            d.ellipse((250,250,1798,1798),outline=245,width=65);d.rectangle((680,600,800,1450),fill=240);d.rectangle((1240,600,1360,1450),fill=240);d.rectangle((760,970,1270,1080),fill=240);color=(200,195,161)
        alpha=np.asarray(mask,dtype=np.float32)*(.75+grain*.25)
        rgba=np.empty((N,N,4),dtype='uint8');rgba[:,:,:3]=np.clip(np.array(color)[None,None,:]*(.8+cloud[...,None]*.4),0,255).astype('uint8');rgba[:,:,3]=alpha.astype('uint8')
        rough=np.full((N,N),.78 if i not in (517,518,519,520) else .28);h=(alpha/255)*.0002
    Image.fromarray(rgba,'RGBA').save(folder/'BaseColor.png')
    dx=(np.roll(h,-1,1)-np.roll(h,1,1))*N/6;dy=(np.roll(h,1,0)-np.roll(h,-1,0))*N/6
    normal=np.stack((-dx,-dy,np.ones_like(dx)),2);normal/=np.linalg.norm(normal,axis=2,keepdims=True)
    Image.fromarray(((normal*.5+.5)*255).astype('uint8')).save(folder/'Normal.png')
    Image.fromarray((rough*255).astype('uint8')).save(folder/'Roughness.png')
    Image.fromarray((h/(h.max()+1e-10)*255).astype('uint8')).save(folder/'Height.png')
    Image.fromarray((np.stack((np.ones_like(rough),rough,np.zeros_like(rough)),2)*255).astype('uint8')).save(folder/'ORM.png')
    (folder/'material.json').write_text(json.dumps({'id':i,'resolution':[N,N],'alpha':transparent,'normal_convention':'OpenGL +Y','original_art':True}))
    print('DECAL_MAPS',i,flush=True)
