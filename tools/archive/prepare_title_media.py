"""Create original quiet menu foley and stage actual game frames for the gallery."""
from pathlib import Path
import wave, shutil
import numpy as np
ROOT=Path(__file__).resolve().parents[1]
DEST=ROOT/'unreal/EchoWard/RawAssets/Menu'
DEST.mkdir(parents=True,exist_ok=True)
SR=24000
rng=np.random.default_rng(404)
def wav(name,x):
    x=np.clip(x,-.95,.95)
    with wave.open(str(DEST/(name+'.wav')),'wb') as f:
        f.setparams((1,2,SR,0,'NONE','not compressed'));f.writeframes((x*32767).astype('<i2').tobytes())
t=np.arange(SR*24)/SR
# Integer cycles at the loop boundary. No musical notes or startling intro.
noise=rng.normal(size=len(t))
freq=np.fft.rfftfreq(len(t),1/SR)
wind=np.fft.irfft(np.fft.rfft(noise)/(1+(freq/230)**2),n=len(t))
wind/=max(abs(wind))
vent=.12*np.sin(2*np.pi*60*t)+.055*np.sin(2*np.pi*120*t)+.22*wind
wav('menu_vent',vent)
t=np.arange(SR*2)/SR
drop=np.sin(2*np.pi*(1100*t+230*np.exp(-t*8)))*np.exp(-t*16)*.26
drop+=.06*rng.normal(size=len(t))*np.exp(-t*32)
for delay,gain in [(2200,.15),(5300,.08),(9000,.035)]:drop[delay:]+=drop[:-delay].copy()*gain
wav('menu_drip',drop)
t=np.arange(int(SR*.6))/SR
click=(np.sin(2*np.pi*168*t)*.27+np.sin(2*np.pi*387*t)*.11+np.sin(2*np.pi*967*t)*.06)*np.exp(-t*21)
click+=rng.normal(size=len(t))*np.exp(-t*150)*.06
wav('menu_click',click)
t=np.arange(int(SR*.18))/SR
wav('menu_hover',np.sin(2*np.pi*1830*t)*np.exp(-t*90)*.055+rng.normal(size=len(t))*np.exp(-t*160)*.014)
t=np.arange(SR*8)/SR
wav('menu_electric',(np.sin(2*np.pi*100*t)*.08+np.sin(2*np.pi*200*t)*.02+rng.normal(size=len(t))*.013)*(1+.17*np.sin(2*np.pi*2*t)))
captures=ROOT/'unreal/EchoWard/Saved/PackageTests/Capture/Saved/Prototype/CampaignCaptures'
for name,index in [('ward',22),('stalker',24)]:
    room=ROOT/'unreal/EchoWard/Saved/Prototype/MenuCaptures/room_photo_1080.png'
    shutil.copyfile(room if name=='ward' and room.exists() else captures/f'view_{index}.png',DEST/(name+'.png'))
menu=ROOT/'unreal/EchoWard/Saved/Prototype/MenuCaptures/corridor_1080.png'
if menu.exists():shutil.copyfile(menu,DEST/'corridor.png')
else:shutil.copyfile(captures/'view_22.png',DEST/'corridor.png')
print(DEST)
