"""Original creature PBR with mottling, pores and shallow branching scars."""
from pathlib import Path
import numpy as np
from PIL import Image,ImageDraw,ImageFilter
import math,json,random
OUT=Path(__file__).resolve().parents[1]/'assets/models/echo_custom/materials';N=2048
rng=np.random.default_rng(2191);random.seed(2191)
def field(cells):return np.asarray(Image.fromarray((rng.random((cells,cells))*255).astype('uint8')).resize((N,N),Image.Resampling.BICUBIC),dtype=np.float32)/255
large=field(15);small=field(120);pores=rng.random((N,N),dtype=np.float32)
scar=Image.new('L',(N,N));d=ImageDraw.Draw(scar)
for j in range(40):
    p=(random.randrange(N),random.randrange(N))
    for k in range(random.randint(15,60)):
        q=(p[0]+random.randint(-12,14),p[1]+random.randint(1,15));d.line([p,q],fill=random.randint(80,200),width=random.randint(1,4));p=q
s=np.asarray(scar.filter(ImageFilter.GaussianBlur(.6)),dtype=np.float32)/255
for key,a,b in [('creature_skin',[.10,.135,.115],[.28,.31,.24]),('necrotic_skin',[.055,.075,.067],[.18,.22,.18]),('exposed_flesh',[.10,.018,.012],[.34,.095,.06]),('ivory_bone',[.28,.24,.15],[.68,.64,.49])]:
    f=OUT/key;f.mkdir(exist_ok=True);blend=large*.65+small*.35
    color=np.asarray(a)*(1-blend[...,None])+np.asarray(b)*blend[...,None];color*=1-s[...,None]*.45
    h=(pores-.5)*.0002+(small-.5)*.0006-s*.0007
    dx=(np.roll(h,-1,1)-np.roll(h,1,1))*N/6;dy=(np.roll(h,1,0)-np.roll(h,-1,0))*N/6
    normal=np.stack((-dx,-dy,np.ones_like(dx)),2);normal/=np.linalg.norm(normal,axis=2,keepdims=True)
    rough=.59+small*.27
    maps={'BaseColor':color,'Normal':normal*.5+.5,'Roughness':rough,'Height':(h-h.min())/(h.max()-h.min()),'ORM':np.stack((np.ones_like(rough),rough,np.zeros_like(rough)),2)}
    for name,data in maps.items():Image.fromarray((np.clip(data,0,1)*255).astype('uint8')).save(f/(name+'.png'))
    (f/'material.json').write_text(json.dumps({'name':key,'original_art':True,'resolution':[N,N],'normal_convention':'OpenGL +Y'}))
    print('SKIN',key,flush=True)
