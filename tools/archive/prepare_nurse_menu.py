"""Author original title-scene geometry and foley. No image manipulation."""
from pathlib import Path
import math, shutil, wave, json
import numpy as np
ROOT=Path(__file__).resolve().parents[1]
DEST=ROOT/'unreal/EchoWard/RawAssets/MenuNurse'
DEST.mkdir(parents=True,exist_ok=True)
shutil.copyfile(ROOT/'assets/models/rocketbox/LICENSE.md',DEST/'LICENSE_Microsoft.txt')

def surface(name,rings,segments=64):
    vertices=[];uv=[];faces=[]
    for j,(z,rx,ry,front) in enumerate(rings):
        for i in range(segments):
            a=i*2*math.pi/segments
            vertices.append((math.cos(a)*rx+front,math.sin(a)*ry,z))
            uv.append((i/segments,j/max(1,len(rings)-1)))
    for j in range(len(rings)-1):
        for i in range(segments):
            n=(i+1)%segments;a=j*segments+i+1;b=j*segments+n+1
            faces.append((a,b,b+segments,a+segments))
    lines=['# Original Echo Ward folded cloth, centimetres','s 1']
    lines += ['v %.6f %.6f %.6f'%p for p in vertices]
    lines += ['vt %.6f %.6f'%p for p in uv]
    lines += ['f '+' '.join(f'{i}/{i}' for i in f) for f in faces]
    (DEST/(name+'.obj')).write_text('\n'.join(lines))

# A shallow rounded cloth crown with a folded flared rim, rather than a cube hat.
surface('nurse_cap',[(6,8.8,9.3,0),(7.5,10,10.5,0),(9,10.5,10.8,0),
                     (9.6,10.6,10.9,0),(9,9.6,9.9,0),(12,8,8.6,-.8),
                     (14,5.4,6,-1.2),(14.5,.3,.3,-1.2)])
# Overlapping irregular cloth strips around the upper face. Open below the nose.
surface('nurse_wrap',[(0,9.3,7.2,1),(1,9.7,7.5,1),(2.6,10,7.7,1),
                      (3.8,9.9,7.8,1),(5.5,9.8,7.7,.8),(7,9.3,7.5,.5)])

SR=48000;rng=np.random.default_rng(89404)
def wav(name,x):
    x=np.asarray(x);peak=float(max(abs(x)));x=x/max(1,peak/.94)
    with wave.open(str(DEST/(name+'.wav')),'wb') as f:
        f.setparams((1,2,SR,0,'NONE','not compressed'));f.writeframes((x*32767).astype('<i2').tobytes())
    return {'name':name,'seconds':len(x)/SR,'peak':float(max(abs(x))),'rms':float(np.sqrt(np.mean(x*x)))}
audio=[]
t=np.arange(int(SR*3.4))/SR
n=rng.normal(size=len(t));f=np.fft.rfftfreq(len(t),1/SR)
scrape=np.fft.irfft(np.fft.rfft(n)/(1+(f/1700)**4),n=len(t))
scrape/=max(abs(scrape))
gate=np.zeros_like(t)
for start,duration in [(.05,.65),(.85,.7),(1.85,.18)]:
    p=np.maximum(t-start,0);g=(t>=start)&(t<start+duration)
    gate+=g*np.sin(np.pi*np.clip(p/duration,0,1))**.6
door=.38*scrape*gate+.12*np.sin(2*np.pi*(79*t+3*np.sin(t*9)))*gate
for at in [1.86,2.18]:
    p=np.maximum(t-at,0);door+=(t>=at)*(.36*np.sin(2*np.pi*92*p)+.12*rng.normal(size=len(t)))*np.exp(-p*19)
audio.append(wav('menu_door',door))
t=np.arange(int(SR*1.7))/SR
env=(1-np.exp(-t*130))*np.exp(-t*3.9)
# Layered inharmonic vocal-like howl, bowed-metal rise and a short impact tail.
phase=2*np.pi*(310*t-91*t*t+8*np.sin(t*13))
howl=sum(np.sin(phase*k+.6*np.sin(t*(23+k*2)))/k**.8 for k in range(1,12))
rough=rng.normal(size=len(t));f=np.fft.rfftfreq(len(t),1/SR)
rough=np.fft.irfft(np.fft.rfft(rough)*np.exp(-((f-2300)/1600)**2),n=len(t))
scare=(.32*howl+.30*rough+.48*np.sin(2*np.pi*58*t)*np.exp(-t*12))*env
audio.append(wav('menu_scare',np.tanh(scare*1.7)))
t=np.arange(int(SR*.55))/SR
audio.append(wav('menu_footstep',(rng.normal(size=len(t))*.18+np.sin(2*np.pi*87*t)*.2)*np.exp(-t*22)))
(DEST/'manifest.json').write_text(json.dumps({'success':True,'geometry':'original procedural folded cloth; centimetres','audio':audio,'base_mesh':'Microsoft Rocketbox Medical_Female_01, MIT','source':'https://github.com/microsoft/Microsoft-Rocketbox'},indent=2))
print(json.dumps(audio,indent=2))

