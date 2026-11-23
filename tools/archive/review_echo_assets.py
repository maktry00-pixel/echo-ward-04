"""Validate actual exported files; maintain an honest 575-item progress ledger."""
from pathlib import Path
import json,struct,hashlib,re,math
from PIL import Image,ImageDraw,ImageFont
from echo_asset_io import externalize_images
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'assets/models/echo_custom'
catalog={int(m[1]):m[2] for line in (OUT/'REQUESTED_CATALOG.txt').read_text(encoding='utf-8-sig').splitlines() if (m:=re.match(r'^(\d+)\.\s+(.+)',line))}
records=[];failures=[];present=[];checked_images=set()
for i,title in catalog.items():
    d=OUT/f'EH_{i:03d}';meta=d/'asset.json';p=d/f'EH_{i:03d}.glb'
    entry={'id':i,'title':title,'status':'not_started'}
    if meta.exists():
        try:
            externalize_images(p)
            raw=p.read_bytes();magic,ver,total=struct.unpack_from('<III',raw)
            assert magic==0x46546c67 and ver==2 and total==len(raw),'GLB header'
            n,kind=struct.unpack_from('<II',raw,12);assert kind==0x4e4f534a
            doc=json.loads(raw[20:20+n]);assert doc.get('meshes'),'No mesh'
            assert doc.get('accessors'),'No geometry accessors'
            # Binary buffers are self-contained; all external image URIs resolve.
            for im in doc.get('images',[]):
                if 'uri' in im:
                    target=(d/im['uri']).resolve();assert target.is_relative_to(OUT.resolve()),'Unsafe image path'
                    assert target.is_file(),'Missing image '+im['uri']
                    if target not in checked_images:
                        with Image.open(target) as image:assert min(image.size)>=256 and max(image.size)<=8192
                        checked_images.add(target)
            for mesh in doc['meshes']:
                for primitive in mesh['primitives']:
                    attr=primitive['attributes'];assert 'POSITION' in attr and 'NORMAL' in attr
                    pos=doc['accessors'][attr['POSITION']];assert pos['count']>0
                    assert all(math.isfinite(v) for k in ['min','max'] for v in pos.get(k,[]))
                    if 'material' in primitive:
                        mat=doc['materials'][primitive['material']]
                        if 'baseColorTexture' in mat.get('pbrMetallicRoughness',{}):assert 'TEXCOORD_0' in attr
            if i>=563:
                assert doc.get('skins'),'Character has no exported skeleton'
                assert any('JOINTS_0' in pr['attributes'] and 'WEIGHTS_0' in pr['attributes'] for me in doc['meshes'] for pr in me['primitives']),'Character has no skin weights'
            assert (d/f'EH_{i:03d}.blend').is_file(),'Missing editable source'
            with Image.open(d/'preview.png') as image:
                assert image.size==(1000,1000)
                assert image.convert('L').getextrema()[1]-image.convert('L').getextrema()[0]>15,'Blank preview'
            metadata=json.loads(meta.read_text(encoding='utf-8'));metadata['sha256']=hashlib.sha256(raw).hexdigest()
            metadata['status']='files_validated_visual_review_pending';meta.write_text(json.dumps(metadata,ensure_ascii=False,indent=2),encoding='utf-8')
            entry.update(status=metadata['status'],folder=d.name,triangles=metadata['triangles'],meshes=len(doc['meshes']),images=len(doc.get('images',[])))
            present.append(i)
        except Exception as e:entry.update(status='validation_failed',error=str(e));failures.append(entry)
    records.append(entry)
ledger={'total_requested':len(catalog),'files_validated':len(present),'remaining':len(catalog)-len(present),'failures':failures,'assets':records}
(OUT/'progress.json').write_text(json.dumps(ledger,ensure_ascii=False,indent=2),encoding='utf-8')
font=ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf',16)
small=ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf',12)
for start in range(0,len(present),30):
    ids=present[start:start+30];cols=6;rows=math.ceil(len(ids)/cols)
    contact=Image.new('RGB',(cols*240,rows*276),(16,24,25));draw=ImageDraw.Draw(contact)
    for j,i in enumerate(ids):
        x=(j%cols)*240;y=(j//cols)*276
        image=Image.open(OUT/f'EH_{i:03d}'/'preview.png').convert('RGB');image.thumbnail((232,232));contact.paste(image,(x+4,y+4))
        draw.text((x+8,y+237),f'{i:03d}  {catalog[i][:24]}',font=small,fill=(205,221,214))
    contact.save(OUT/f'contact_{start//30+1:02d}.jpg',quality=92)
print(json.dumps({'validated':len(present),'requested':len(catalog),'failures':failures},ensure_ascii=False))
