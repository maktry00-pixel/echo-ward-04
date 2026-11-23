"""Check the delivered atlas and each of its embedded source files."""
from pathlib import Path
import hashlib
import json
from pypdf import PdfReader

root=Path(__file__).resolve().parents[1]
out=root/'output/pdf/ECHO_Hospital_v4'
pdf=out/'ЭХО_полная_карта_со_всеми_исправлениями.pdf'
reader=PdfReader(pdf)
manifest=json.loads((out/'manifest.json').read_text(encoding='utf-8'))
source=root/'unreal/EchoWard/Content/CampaignData/hospital_world.json'
sha=hashlib.sha256(source.read_bytes()).hexdigest()
assert sha==manifest['source_sha256']
assert (out/'hospital_world_source.json').read_bytes()==source.read_bytes()
assert len(reader.pages)==57
files=[f for f in out.iterdir() if f.suffix in ('.csv','.json','.md')]
assert len(reader.attachments)==len(files)
for f in files:
    assert reader.attachments[f.name][0]==f.read_bytes(),f.name
text='\n'.join(p.extract_text() for p in reader.pages)
data=json.loads(source.read_text(encoding='utf-8'))
zones=[r for loc in data['locations'] for r in loc['rooms']]
assert len(zones)==334
for r in zones:
    assert r['id'] in text,r['id']
assert sha in text
result=dict(pages=len(reader.pages),zones=len(zones),attachments=len(files),
            size_mb=round(pdf.stat().st_size/1024**2,2),source_sha256=sha,
            all_attachment_bytes_match=True)
(root/'tmp/pdfs/hospital_review_v4/final_check.json').write_text(
    json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(result))
