"""Convert the authored static GLBs to the project's verified OBJ import path.
Never modifies the source library. Skeletal assets keep their existing gameplay.
"""
import hashlib,json,struct
from pathlib import Path
import numpy as np
ROOT=Path(__file__).resolve().parents[1];LIB=ROOT/'assets/models/echo_custom';OUT=ROOT/'unreal/EchoWard/RawAssets/EchoCustom'
OUT.mkdir(parents=True,exist_ok=True)
materials={};assets=[]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def export(i):
    path=LIB/f'EH_{i:03}/EH_{i:03}.glb';raw=path.read_bytes();n=struct.unpack_from('<I',raw,12)[0];d=json.loads(raw[20:20+n]);binary=raw[28+n:]
    def acc(k):
        a=d['accessors'][k];v=d['bufferViews'][a['bufferView']];kind={5121:'u1',5123:'<u2',5125:'<u4',5126:'<f4'}[a['componentType']];c={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4,'MAT4':16}[a['type']];dt=np.dtype(kind)
        return np.ndarray((a['count'],c),dtype=dt,buffer=binary,offset=v.get('byteOffset',0)+a.get('byteOffset',0),strides=(v.get('byteStride',dt.itemsize*c),dt.itemsize)).copy()
    mats=[]
    for m in d.get('materials',[]):
        pbr=m.get('pbrMetallicRoughness',{});spec=dict(name=m.get('name',''),color=pbr.get('baseColorFactor',[1,1,1,1]),roughness=pbr.get('roughnessFactor',1),metallic=pbr.get('metallicFactor',1),alpha=m.get('alphaMode','OPAQUE'),emission=m.get('emissiveFactor',[0,0,0]))
        for key,q in [('albedo',pbr.get('baseColorTexture')),('orm',pbr.get('metallicRoughnessTexture')),('normal',m.get('normalTexture'))]:
            if q:spec[key]=str((path.parent/d['images'][d['textures'][q['index']]['source']]['uri']).resolve())
        key='EC_'+hashlib.sha256(json.dumps(spec,sort_keys=True).encode()).hexdigest()[:16];materials[key]=spec;mats.append(key)
    parent={ch:k for k,node in enumerate(d['nodes']) for ch in node.get('children',[])};world={}
    def mat(k):
        if k in world:return world[k]
        node=d['nodes'][k]
        if 'matrix' in node:M=np.array(node['matrix']).reshape(4,4).T
        else:
            x,y,z,w=node.get('rotation',[0,0,0,1]);M=np.eye(4);M[:3,:3]=np.array([[1-2*(y*y+z*z),2*(x*y-z*w),2*(x*z+y*w)],[2*(x*y+z*w),1-2*(x*x+z*z),2*(y*z-x*w)],[2*(x*z-y*w),2*(y*z+x*w),1-2*(x*x+y*y)]])@np.diag(node.get('scale',[1,1,1]));M[:3,3]=node.get('translation',[0,0,0])
        if k in parent:M=mat(parent[k])@M
        world[k]=M;return M
    primitives=[]
    for k,node in enumerate(d['nodes']):
        if 'mesh' not in node:continue
        M=mat(k);root=k;moving=False
        while True:
            moving|='DoorPivot' in d['nodes'][root].get('name','')
            if root not in parent:break
            root=parent[root]
        for p in d['meshes'][node['mesh']]['primitives']:
            assert p.get('mode',4)==4
            a=p['attributes'];v=acc(a['POSITION']);v=(np.c_[v,np.ones(len(v))]@M.T)[:,:3]
            normal=acc(a['NORMAL'])@np.linalg.inv(M[:3,:3]);normal/=np.maximum(np.linalg.norm(normal,axis=1)[:,None],1e-9)
            uv=acc(a['TEXCOORD_0']) if 'TEXCOORD_0' in a else np.zeros((len(v),2));uv[:,1]=1-uv[:,1]
            inds=acc(p['indices']).astype(np.int64).reshape(-1,3) if 'indices' in p else np.arange(len(v)).reshape(-1,3)
            # GLTF Y-up -> Blender Z-up -> legacy OBJ counter-reflection.
            primitives.append((mats[p.get('material',0)],v[:,[0,2,1]]*100,normal[:,[0,2,1]],uv,inds[:,::-1],moving))
    def write(name,parts,leaf=False):
        if not parts:return
        p=OUT/(name+'.obj');slotkeys=list(dict.fromkeys(q[0] for q in parts));allv=np.concatenate([q[1] for q in parts]);low=allv.min(0);high=allv.max(0)
        # Door leaf local X starts at its hinge, just like the current runtime.
        shift=np.array([low[0],0,0]) if leaf else np.zeros(3)
        with p.open('w',encoding='utf8') as f:
            f.write('mtllib '+name+'.mtl\n');offset=1
            for key,v,n,uv,inds,_ in parts:
                f.write('g '+name+'\nusemtl '+key+'\n')
                for row in v-shift:f.write('v %.6f %.6f %.6f\n'%tuple(row))
                for row in uv:f.write('vt %.7f %.7f\n'%tuple(row))
                for row in n:f.write('vn %.7f %.7f %.7f\n'%tuple(row))
                for tri in inds+offset:f.write('f '+' '.join(f'{j}/{j}/{j}' for j in tri)+'\n')
                offset+=len(v)
        (OUT/(name+'.mtl')).write_text(''.join(f'newmtl {key}\nKd .5 .5 .5\n\n' for key in slotkeys))
        # Final UE bound undoes the OBJ Y reflection.
        outlow=(allv-shift).min(0);outhigh=(allv-shift).max(0);bounds=[[outlow[0],-outhigh[1],outlow[2]],[outhigh[0],-outlow[1],outhigh[2]]]
        assets.append(dict(id=i,name=name,file=str(p),bounds_cm=bounds,materials=slotkeys,triangles=sum(len(q[4]) for q in parts),source_sha256=sha(path),door_leaf=leaf))
    write(f'EH_{i:03}',primitives)
    if i in (61,62,63,64,67,68,69,70,71,72,73):write(f'EH_{i:03}_Leaf',[p for p in primitives if p[5]],True)
for i in range(1,563):export(i);print('PREPARED',i,flush=True) if i%40==0 else None
result=dict(version=20,units='cm',assets=assets,materials=materials,skeletal_deferred=list(range(563,576)),source_library=str(LIB))
(OUT/'manifest.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf8')
print(json.dumps(dict(meshes=len(assets),materials=len(materials),triangles=sum(a['triangles'] for a in assets))),flush=True)

