"""Unpack user vehicle sources with explicit path and archive-size checks."""
from pathlib import Path
import zipfile,json,hashlib,subprocess
R=Path(__file__).resolve().parents[1];ROOT=R/'assets/models/cars';OUT=ROOT/'extracted_v31';OUT.mkdir(exist_ok=True);rows=[]
model_ext={'.glb','.gltf','.fbx','.obj','.blend','.dae','.usd','.usda','.usdc','.usdz','.c4d','.max','.3ds','.skp'}
for z in sorted(ROOT.glob('*.zip')):
 dst=OUT/z.stem;dst.mkdir(exist_ok=True)
 queue=[(z,dst,0)];seen=set()
 while queue:
  arc,folder,depth=queue.pop(0);assert depth<=5
  if str(arc) in seen:continue
  seen.add(str(arc))
  if arc.suffix.lower()=='.zip':
   with zipfile.ZipFile(arc) as f:
    assert sum(x.file_size for x in f.infolist())<3_000_000_000
    for q in f.infolist():
     if q.is_dir():continue
     rel=Path(q.filename.replace('\\','/'));target=(folder/rel).resolve();assert target.is_relative_to(dst.resolve()),q.filename
     if target.suffix.lower() in {'.exe','.dll','.bat','.cmd','.ps1','.py','.js'}:continue
     target.parent.mkdir(parents=True,exist_ok=True)
     if not target.exists() or target.stat().st_size!=q.file_size:target.write_bytes(f.read(q))
     if target.suffix.lower() in {'.zip','.rar','.7z'}:queue.append((target,target.parent/(target.stem+'_unpacked'),depth+1))
  else:
   folder.mkdir(exist_ok=True)
   names=subprocess.run(['tar.exe','-tf',str(arc)],capture_output=True,text=True,encoding='utf8',errors='replace')
   if names.returncode:print('UNSUPPORTED_ARCHIVE',arc,names.stderr[:200]);continue
   for name in names.stdout.splitlines():assert (folder/name.replace('\\','/')).resolve().is_relative_to(dst.resolve()),name
   result=subprocess.run(['tar.exe','-xf',str(arc),'-C',str(folder)],capture_output=True,text=True,encoding='utf8',errors='replace')
   if result.returncode:print('EXTRACT_ERROR',arc,result.stderr[:200])
 models=[p for p in dst.rglob('*') if p.is_file() and p.suffix.lower() in model_ext]
 row=dict(archive=z.name,sha256=hashlib.sha256(z.read_bytes()).hexdigest(),models=[dict(file=str(p),bytes=p.stat().st_size) for p in models],extracted_bytes=sum(p.stat().st_size for p in dst.rglob('*') if p.is_file()))
 rows.append(row);print(z.name,[(p.name,p.stat().st_size) for p in models],flush=True)
(R/'artifacts/echo_cars_v31/inventory.json').write_text(json.dumps(dict(archives=len(rows),vehicles=rows),ensure_ascii=False,indent=2),encoding='utf8')
