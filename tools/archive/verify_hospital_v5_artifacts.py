"""Validate the final downloadable atlas and its embedded game data."""
from pathlib import Path
import csv,hashlib,io,json
from pypdf import PdfReader
from PIL import Image,ImageDraw

root=Path(__file__).resolve().parents[1];out=root/'output/pdf/ECHO_Hospital_v5';art=root/'artifacts/unreal_hospital_v5'
pdf=out/'ЭХО_карта_v5_новые_этажи_тоннели_укрытия.pdf';reader=PdfReader(pdf)
source=root/'unreal/EchoWard/Content/CampaignData/hospital_world.json';sha=hashlib.sha256(source.read_bytes()).hexdigest()
verification=json.loads((art/'verification.json').read_text(encoding='utf-8'))
assert verification['success'] and verification['world_checks']==76 and verification['map_sha256']==sha
assert len(reader.pages)==95
assert reader.attachments['hospital_world_source.json'][0]==source.read_bytes()
for f in out.iterdir():
    if f.suffix in ('.csv','.json','.md'):assert reader.attachments[f.name][0]==f.read_bytes(),f.name
rules=list(csv.DictReader(io.StringIO(reader.attachments['monster_rules.csv'][0].decode('utf-8-sig')),delimiter=';'))
listener=next(r for r in rules if r['id']=='listener')
assert listener['combat_class']=='B' and listener['kill_condition']=='finale_only'
text='\n'.join(p.extract_text() or '' for p in reader.pages)
d=json.loads(source.read_text(encoding='utf-8'));zones=[r for l in d['locations'] for r in l['rooms']]
assert len(zones)==562
for r in zones:assert r['id'] in text and r['name'] in text,r['id']
folder=root/'tmp/pdfs/hospital_expansion_v5';files=sorted((folder/'render').glob('page-*.png'));assert len(files)==95
for start in range(0,len(files),20):
    im=Image.new('RGB',(1640,1375),'#edf1ef');draw=ImageDraw.Draw(im)
    for i,f in enumerate(files[start:start+20]):
        p=Image.open(f).convert('RGB');p.thumbnail((400,242));x=i%4*410;y=i//4*275
        im.paste(p,(x,y+23));draw.text((x+5,y+4),f.stem,fill='black')
    im.save(folder/f'contact_{start//20+1}.png')
result=dict(success=True,pages=95,zones=562,attachments=len(reader.attachments),size_mb=round(pdf.stat().st_size/1024**2,2),
    embedded_listener='B / finale_only',source_sha256=sha,all_attachment_bytes_match=True)
(art/'pdf_check.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(result))
