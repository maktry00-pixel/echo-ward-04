"""Read-only copy verification and lossless preparation of old ECHO evidence.

No removal is performed here. Cloud receipts and verified archives are required
before a separate, explicitly scoped filesystem cleanup.
"""
import argparse,collections,concurrent.futures,hashlib,json,os,time,zipfile
from pathlib import Path

def digest(path):
    h=hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda:f.read(4*1024*1024),b''):h.update(chunk)
    return h.hexdigest()

def files(root):
    for base,ds,fs in os.walk(root,followlinks=False):
        ds[:]=[d for d in ds if not (Path(base)/d).is_symlink() and not (Path(base)/d).is_junction()]
        for name in sorted(fs):yield Path(base)/name

def write(path,data):
    path.write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf8')

def verify(source,target,evidence):
    evidence.mkdir(parents=True,exist_ok=True);paths=list(files(source));records=[];bad=[];total=0
    def one(p):
        rel=p.relative_to(source).as_posix();q=target/rel
        if not q.is_file():return dict(path=rel,ok=False,error='missing destination')
        size=p.stat().st_size
        if q.stat().st_size!=size:return dict(path=rel,ok=False,error='different size')
        a=digest(p);b=digest(q)
        return dict(path=rel,ok=a==b,size=size,sha256=a,target_sha256=b)
    with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
        for i,row in enumerate(pool.map(one,paths),1):
            records.append(row);total+=row.get('size',0)
            if not row['ok']:bad.append(row)
            if i%2000==0:print('VERIFY_PROGRESS',i,len(paths),round(total/2**30,2),'GiB',flush=True)
    extras=sorted({p.relative_to(target).as_posix() for p in files(target)}-{r['path'] for r in records})
    result=dict(success=not bad and not extras,source=str(source),destination=str(target),files=len(records),bytes=total,failed=bad,extra_files=extras,records=records)
    write(evidence/'copy_verification.json',result);print('COPY_VERIFICATION',json.dumps({k:v for k,v in result.items() if k!='records'}),flush=True)
    if not result['success']:raise SystemExit(1)

def archive(root,out):
    out.mkdir(parents=True,exist_ok=True);selected=[];cleanup=[]
    protected={'artifacts/echo_quality_v22/world_v22_court_S01.png','artifacts/echo_quality_v22/world_v21_detail_126.png'}
    media={'.png','.jpg','.jpeg','.webp','.mp4','.avi','.gif'}
    for folder in sorted((root/'artifacts').iterdir()):
        if not folder.is_dir() or folder.name=='echo_real_v23' or not folder.name.startswith(('unreal_','echo_')):continue
        for p in files(folder):
            rel=p.relative_to(root).as_posix();selected.append((p,rel,'old_art'))
            if p.suffix.lower() in media and rel not in protected:cleanup.append(rel)
    active={'World','WorldVisualV23','StoryV7Visual','NativeMenuV23'}
    for folder in sorted((root/'unreal/EchoWard/Saved/PackageTests').iterdir()):
        if not folder.is_dir() or folder.name in active:continue
        for p in files(folder):
            rel=p.relative_to(root).as_posix();selected.append((p,rel,'old_tests'));cleanup.append(rel)
    grouped=[];batch=[];batch_bytes=0
    for entry in sorted(selected,key=lambda e:(e[2],e[1])):
        size=entry[0].stat().st_size
        if batch and (batch_bytes+size>64*1024**2 or entry[2]!=batch[-1][2]):grouped.append(batch);batch=[];batch_bytes=0
        batch.append(entry);batch_bytes+=size
    if batch:grouped.append(batch)
    archives=[];all_records=[]
    for index,batch in enumerate(grouped,1):
        path=out/f'ECHO_{batch[0][2]}_{index:02}.zip';rows=[]
        if path.exists():raise RuntimeError(f'Refusing to overwrite an existing archive: {path}')
        with zipfile.ZipFile(path,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=1,allowZip64=True) as z:
            for p,rel,kind in batch:
                row=dict(path=rel,size=p.stat().st_size,sha256=digest(p),cleanup_after_verified_upload=rel in cleanup);rows.append(row);z.write(p,rel)
            z.writestr('ECHO_ARCHIVE_MANIFEST.json',json.dumps(rows,ensure_ascii=False,indent=2))
        with zipfile.ZipFile(path) as z:
            bad=z.testzip()
            if bad:raise RuntimeError(f'Archive CRC failure: {bad}')
            for row in rows:
                h=hashlib.sha256()
                with z.open(row['path']) as f:
                    for chunk in iter(lambda:f.read(4*1024*1024),b''):h.update(chunk)
                if h.hexdigest()!=row['sha256']:raise RuntimeError(f'Archive SHA failure: {row["path"]}')
        entry=dict(file=path.name,path=str(path),bytes=path.stat().st_size,sha256=digest(path),files=len(rows),original_bytes=sum(r['size'] for r in rows));archives.append(entry);all_records.extend(rows)
        write(out/'archive_manifest.partial.json',dict(archives=archives,records=all_records));print('ARCHIVE_READY',json.dumps(entry),flush=True)
    write(out/'archive_manifest.json',dict(success=True,project=str(root),archives=archives,records=all_records,cleanup_candidates=cleanup,protected_files=sorted(protected),active_test_profiles=sorted(active)))
    print('ARCHIVES_COMPLETE',len(archives),sum(a['bytes'] for a in archives),flush=True)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('mode',choices=['verify','archive']);p.add_argument('--source',type=Path,required=True);p.add_argument('--destination',type=Path,required=True);p.add_argument('--evidence',type=Path);a=p.parse_args()
    root=a.source.resolve();target=a.destination.resolve()
    if not root.is_dir() or root==target or root in target.parents:raise SystemExit('Unsafe source/destination relationship')
    if a.mode=='verify':verify(root,target,a.evidence.resolve())
    else:archive(root,target)

