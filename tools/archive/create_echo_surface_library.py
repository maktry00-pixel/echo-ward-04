"""Original, deterministic PBR surfaces for the ECHO modular hospital kit.
Run with bundled Python (Pillow + NumPy). No downloaded art or paid services.
"""
from pathlib import Path
import json, math
import numpy as np
from PIL import Image, ImageDraw, ImageFilter

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'assets/models/echo_custom/materials'
OUT.mkdir(parents=True, exist_ok=True)
N = 2048
rng = np.random.default_rng(20261024)

def noise(cells, seed):
    a = np.random.default_rng(seed).random((cells+1,cells+1))
    a[-1,:] = a[0,:]; a[:,-1] = a[:,0]
    return np.asarray(Image.fromarray((a*65535).astype('uint16')).resize((N,N), Image.Resampling.BICUBIC), dtype=np.float32)/65535

coarse=noise(10,1); medium=noise(47,2); fine=noise(240,3)
grain=rng.random((N,N),dtype=np.float32)
yy,xx=np.mgrid[0:N,0:N].astype(np.float32)/N
cloud=coarse*.55+medium*.30+fine*.15

def blend(a,b,t):
    return np.asarray(a,dtype=np.float32)*(1-t[...,None])+np.asarray(b,dtype=np.float32)*t[...,None]

def save(name, color, height, rough, metal=0):
    d=OUT/name; d.mkdir(exist_ok=True)
    color=np.clip(color,0,1)
    if color.ndim==1: color=np.broadcast_to(color,(N,N,3))
    Image.fromarray((color*255).astype('uint8')).save(d/'BaseColor.png')
    h=np.asarray(height,dtype=np.float32)
    if h.ndim==0: h=np.full((N,N),h)
    # OpenGL tangent-space normals; texels represent a 3 m square.
    dx=(np.roll(h,-1,1)-np.roll(h,1,1))*N/6
    dy=(np.roll(h,1,0)-np.roll(h,-1,0))*N/6
    normal=np.stack((-dx,-dy,np.ones_like(h)),axis=2)
    normal/=np.linalg.norm(normal,axis=2,keepdims=True)
    Image.fromarray(((normal*.5+.5)*255).astype('uint8')).save(d/'Normal.png')
    r=np.clip(np.broadcast_to(rough,(N,N)),0,1)
    Image.fromarray((r*255).astype('uint8')).save(d/'Roughness.png')
    Image.fromarray((np.clip((h-h.min())/(np.ptp(h)+1e-8),0,1)*255).astype('uint8')).save(d/'Height.png')
    m=np.broadcast_to(metal,(N,N))
    arm=np.stack((np.ones_like(r),r,m),axis=2)
    Image.fromarray((arm*255).astype('uint8')).save(d/'ORM.png')
    (d/'material.json').write_text(json.dumps({'name':name,'resolution':[N,N],'normal_convention':'OpenGL +Y','tile_size_m':3,'original_art':True,'maps':['BaseColor','Normal','Roughness','Height','ORM']},indent=2))
    print('SURFACE',name,flush=True)

cracks=Image.new('L',(N,N)); draw=ImageDraw.Draw(cracks)
prng=np.random.default_rng(43)
for j in range(27):
    p=np.array([prng.uniform(0,N),prng.uniform(0,N)])
    angle=prng.uniform(0,math.tau)
    for k in range(prng.integers(12,55)):
        q=p+np.array([math.cos(angle),math.sin(angle)])*prng.uniform(5,23)
        draw.line([tuple(p),tuple(q)],fill=int(prng.uniform(110,230)),width=int(prng.integers(1,3)))
        if k%9==0:
            v=q+np.array([math.cos(angle+1),math.sin(angle+1)])*prng.uniform(13,40)
            draw.line([tuple(q),tuple(v)],fill=100,width=1)
        p=q; angle+=prng.normal(0,.23)
crack=np.asarray(cracks,dtype=np.float32)/255
plaster=blend([.48,.46,.40],[.80,.78,.69],cloud)
plaster*=1-crack[...,None]*.7
plaster_h=(fine-.5)*.002+(medium-.5)*.004-crack*.002
save('025_old_plaster',plaster,plaster_h,.83+grain*.14)

chip=np.clip((cloud-.53)*12,0,1)
# fine islands erode the margins, while retaining broad readable paint fields.
chip=np.clip(chip+(fine-.5)*.45,0,1)
for name,base in [('026_peeling_green',[.26,.42,.35]),('027_peeling_blue',[.31,.46,.49]),('028_dirty_white',[.72,.71,.64])]:
    painted=np.asarray(base)[None,None,:]*(.85+cloud[...,None]*.25)
    painted=painted*(1-crack[...,None]*.8)
    col=painted*(1-chip[...,None])+plaster*chip[...,None]
    streak=(noise(175,23)*.22+noise(16,31)*.45)
    col*=1-streak[...,None]*.24
    save(name,col,plaster_h*.5+(1-chip)*.002,.56+(chip*.29)+grain*.08)
save('029_damp_concrete',blend([.19,.21,.20],[.50,.49,.43],cloud)*(1-crack[...,None]*.7),plaster_h*1.8,.82+fine*.15)

row=np.floor(yy*14); bx=(xx*7+(row%2)*.5)%1; by=(yy*14)%1
mortar=(bx<.045)|(by<.075)|(bx>.975)|(by>.94)
brick=blend([.24,.105,.065],[.55,.30,.18],cloud)
brick*=.85+.15*np.sin(np.floor(xx*7+(row%2)*.5)*23+row*11)[...,None]
brick[mortar]=blend([.35,.34,.29],[.58,.56,.47],cloud)[mortar]
save('030_old_brick',brick,(~mortar)*.013+(fine-.5)*.004-crack*.002,.86+grain*.12)
rust=np.clip((cloud-.42)*5,0,1)
steel=blend([.16,.18,.17],[.47,.49,.45],fine)
rustcol=blend([.12,.07,.035],[.48,.23,.075],medium)
save('031_rusted_metal',steel*(1-rust[...,None])+rustcol*rust[...,None],(fine-.5)*.001+rust*.001,.39+rust*.49,.75*(1-rust))
zinc=blend([.44,.48,.48],[.51,.55,.53],medium)
save('032_galvanized_metal',zinc,(fine-.5)*.0005,.34+medium*.27,.92)

tx=(xx*10)%1; ty=(yy*10)%1
grout=(tx<.018)|(ty<.018)|(tx>.982)|(ty>.982)
tile=blend([.55,.58,.52],[.80,.81,.71],cloud)
edge=np.minimum(np.minimum(tx,1-tx),np.minimum(ty,1-ty))
tile*=1-np.clip((.07-edge)*6,0,.4)[...,None]
tile[grout]=[.23,.25,.21]
save('033_dirty_wall_tile',tile,(~grout)*.004+fine*.0002,.24+cloud*.24)
tile2=blend([.26,.29,.25],[.49,.51,.44],cloud)*(1-crack[...,None]*.85)
tile2[grout]=[.13,.15,.13]
save('034_cracked_floor_tile',tile2,(~grout)*.004-crack*.003,.43+medium*.27)
chips=noise(500,66)>.66
terrazzo=blend([.34,.36,.32],[.61,.61,.52],cloud)
terrazzo[chips]=blend([.12,.16,.15],[.82,.79,.65],grain)[chips]
save('035_old_terrazzo',terrazzo,fine*.0003,.48+cloud*.19)
scuffs=np.maximum(0,np.sin(xx*430+np.sin(yy*20)*9))**70
linoleum=blend([.22,.32,.28],[.40,.46,.35],cloud)*(1-scuffs[...,None]*.16)
save('036_worn_linoleum',linoleum,fine*.00025,.50+cloud*.19)
save('037_basement_concrete',blend([.15,.16,.14],[.36,.35,.30],cloud)*(1-crack[...,None]*.75),plaster_h,.87+grain*.1)
save('038_lab_floor',blend([.17,.23,.25],[.33,.39,.37],cloud),fine*.0003,.38+cloud*.22)
asphalt=blend([.035,.045,.045],[.13,.15,.14],fine)
save('039_wet_asphalt',asphalt,fine*.003,.16+cloud*.3)
save('040_roof_membrane',blend([.07,.08,.075],[.20,.21,.18],cloud),fine*.0015,.82+grain*.12)
save('rubber',blend([.025,.032,.03],[.08,.09,.075],fine),fine*.001,.8)
save('ivory_enamel',blend([.56,.58,.51],[.78,.79,.70],cloud)*(1-rust[...,None]*.22),fine*.0004,.37+rust*.35,.12)
save('wood',blend([.12,.065,.028],[.34,.23,.115],noise(220,75)*.35+noise(12,71)*.65),fine*.0008,.61+cloud*.2)
save('ceiling_fibre',blend([.52,.52,.45],[.76,.75,.65],cloud),fine*.001,.93)
save('water',blend([.035,.045,.04],[.07,.09,.08],cloud),(fine-.5)*.00003,.08+cloud*.10)
weave=(np.sin(xx*math.tau*680)+np.sin(yy*math.tau*680))*.5
for name,a,b in [('fabric_dirty',[.37,.38,.32],[.69,.69,.56]),('fabric_blue',[.075,.16,.18],[.24,.35,.34])]:
    save(name,blend(a,b,cloud)*(1+weave[...,None]*.025),weave*.00018+(fine-.5)*.0002,.90+fine*.08)
print('DONE',flush=True)
