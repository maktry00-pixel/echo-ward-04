"""Original organic forearm geometry and quiet title-scene foley."""
from pathlib import Path
import math,json,wave
import numpy as np
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'unreal/EchoWard/RawAssets/MenuEvents';OUT.mkdir(parents=True,exist_ok=True)
verts=[];uvs=[];faces=[];RINGS=19;SIDES=48
for j in range(RINGS):
    t=j/(RINGS-1);radius=7*(1-t)+3.4*t+.7*math.sin(t*math.pi*3)
    for i in range(SIDES):
        a=i*math.tau/SIDES;r=radius*(1+.06*math.sin(a*3+t*11)+.03*math.sin(a*7-t*14))
        verts.append((t*100,math.cos(a)*r,math.sin(a)*r*.86+math.sin(t*math.pi)*1.8));uvs.append((i/SIDES,t))
for j in range(RINGS-1):
    for i in range(SIDES):
        a=j*SIDES+i+1;b=j*SIDES+(i+1)%SIDES+1;faces.append((a,b,b+SIDES,a+SIDES))
lines=['# Original Echo Ward elongated forearm, centimetres','s 1']
lines+=['v %.6f %.6f %.6f'%p for p in verts];lines+=['vt %.6f %.6f'%p for p in uvs]
lines+=['f '+' '.join(f'{i}/{i}' for i in f) for f in faces]
(OUT/'forearm.obj').write_text('\n'.join(lines))
SR=48000;rng=np.random.default_rng(40861);audio=[]
def save(name,seconds,code):
    t=np.arange(int(SR*seconds))/SR;n=rng.normal(size=len(t));f=np.fft.rfftfreq(len(t),1/SR)
    noise=np.fft.irfft(np.fft.rfft(n)/(1+(f/700)**4),n=len(t));noise/=max(abs(noise))
    x=code(t,noise);x*=np.minimum(1,t/.08)*np.minimum(1,(seconds-t)/.2);x=np.clip(x,-.8,.8)
    with wave.open(str(OUT/(name+'.wav')),'wb') as w:w.setparams((1,2,SR,0,'NONE','not compressed'));w.writeframes((x*32767).astype('<i2').tobytes())
    audio.append({'name':name,'seconds':seconds,'peak':float(max(abs(x)))})
save('menu_roll',3.2,lambda t,n:(n*.19+np.sin(t*math.tau*68)*.055)*(1-.76*t/3.2)*(.65+.35*np.cos(t*(31-3*t))**6))
save('menu_drag',2.2,lambda t,n:(n*.16+np.sin(t*math.tau*(103+4*np.sin(t*7)))*.035)*(.45+.55*np.sin(t*17)**2))
(OUT/'manifest.json').write_text(json.dumps({'success':True,'geometry':'original forearm OBJ, 912 vertices; centimetres','audio':audio,'head_and_hand':'Existing HorrorGameMaker CC0 skeletal mesh, cropped by pre-skinned local position; no changes to gameplay monster'},indent=2))
print(json.dumps(audio))
