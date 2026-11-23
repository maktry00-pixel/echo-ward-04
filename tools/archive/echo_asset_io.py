"""Lossless GLB image deduplication: shared PNGs, unchanged mesh accessors."""
import hashlib,json,struct
from pathlib import Path

def externalize_images(path):
    path=Path(path);raw=path.read_bytes()
    magic,version,total=struct.unpack_from('<III',raw)
    assert magic==0x46546c67 and version==2 and total==len(raw)
    json_size,json_type=struct.unpack_from('<II',raw,12)
    doc=json.loads(raw[20:20+json_size]);offset=20+json_size
    binary=b''
    if offset<len(raw):
        size,kind=struct.unpack_from('<II',raw,offset);assert kind==0x004e4942
        binary=raw[offset+8:offset+8+size]
    shared=path.parent.parent/'textures_shared';shared.mkdir(exist_ok=True)
    removed=set()
    for im in doc.get('images',[]):
        if 'bufferView' not in im:continue
        vi=im.pop('bufferView');bv=doc['bufferViews'][vi]
        data=binary[bv.get('byteOffset',0):bv.get('byteOffset',0)+bv['byteLength']]
        suffix='.png' if im.get('mimeType')=='image/png' else '.jpg'
        filename=hashlib.sha256(data).hexdigest()+suffix
        target=shared/filename
        if not target.exists():target.write_bytes(data)
        im['uri']='../textures_shared/'+filename;removed.add(vi)
    if not removed:return
    mapping={};views=[];chunks=[];size=0
    for old,bv in enumerate(doc.get('bufferViews',[])):
        if old in removed:continue
        start=bv.get('byteOffset',0);payload=binary[start:start+bv['byteLength']]
        pad=(-size)%4
        if pad:chunks.append(b'\0'*pad);size+=pad
        mapping[old]=len(views);bv['byteOffset']=size;views.append(bv);chunks.append(payload);size+=len(payload)
    for accessor in doc.get('accessors',[]):
        if 'bufferView' in accessor:accessor['bufferView']=mapping[accessor['bufferView']]
        for field in ['indices','values']:
            if field in accessor.get('sparse',{}):
                a=accessor['sparse'][field];a['bufferView']=mapping[a['bufferView']]
    doc['bufferViews']=views;doc['buffers']=[{'byteLength':size}]
    encoded=json.dumps(doc,separators=(',',':'),ensure_ascii=False).encode();encoded+=b' '*((-len(encoded))%4)
    binout=b''.join(chunks);binout+=b'\0'*((-len(binout))%4)
    header=struct.pack('<III',magic,version,12+8+len(encoded)+8+len(binout))
    path.write_bytes(header+struct.pack('<II',len(encoded),0x4e4f534a)+encoded+struct.pack('<II',len(binout),0x004e4942)+binout)
