"""Reduce exaggerated zinc contrast; update exported image links losslessly."""
from pathlib import Path
import hashlib,json,struct
from PIL import Image
import numpy as np
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'assets/models/echo_custom'
p=OUT/'materials/032_galvanized_metal/BaseColor.png'
previous=json.loads((OUT/'metal_retouch.json').read_text()) if (OUT/'metal_retouch.json').exists() else {}
old=hashlib.sha256(p.read_bytes()).hexdigest()
a=np.asarray(Image.open(p).convert('RGB'),dtype=np.float32)/255
# Same art direction, smaller scale of tonal contrast.
if a.std()>.025:
    a=np.array([.477,.517,.506])+(a-a.mean(axis=(0,1)))*.22
    Image.fromarray((np.clip(a,0,1)*255).astype('uint8')).save(p)
new=hashlib.sha256(p.read_bytes()).hexdigest();shared=OUT/'textures_shared';shared.mkdir(exist_ok=True)
(shared/(new+'.png')).write_bytes(p.read_bytes())
replacements={old:new}
if previous.get('old'):replacements[previous['old']]=new
for filename in ['Normal.png','ORM.png']:
    target=p.parent/filename;original=hashlib.sha256(target.read_bytes()).hexdigest()
    image=np.asarray(Image.open(target).convert('RGB'),dtype=np.float32)/255
    if filename=='Normal.png':
        image[:,:,:2]=.5+(image[:,:,:2]-.5)*.07;image[:,:,2]=1
    else:image[:,:,0]=1;image[:,:,1]=.39;image[:,:,2]=.92
    Image.fromarray((np.clip(image,0,1)*255).astype('uint8')).save(target)
    digest=hashlib.sha256(target.read_bytes()).hexdigest();(shared/(digest+'.png')).write_bytes(target.read_bytes());replacements[original]=digest
replacements.update({key:replacements.get(value,value) for key,value in previous.get('replacements',{}).items()})
affected=[]
for glb in sorted(OUT.glob('EH_*/*.glb')):
    raw=glb.read_bytes();n,kind=struct.unpack_from('<II',raw,12);doc=json.loads(raw[20:20+n]);changed=False
    for im in doc.get('images',[]):
        key=Path(im.get('uri','')).stem
        if key in replacements and key!=replacements[key]:
            im['uri']='../textures_shared/'+replacements[key]+'.png';changed=True
    if changed:
        encoded=json.dumps(doc,separators=(',',':'),ensure_ascii=False).encode();encoded+=b' '*((-len(encoded))%4)
        tail=raw[20+n:];header=struct.pack('<III',0x46546c67,2,20+len(encoded)+len(tail))
        glb.write_bytes(header+struct.pack('<II',len(encoded),kind)+encoded+tail);affected.append(int(glb.stem.split('_')[1]))
(OUT/'metal_retouch.json').write_text(json.dumps({'old':old,'new':new,'affected':affected,'replacements':replacements},indent=2))
print('RETOUCHED',affected)
