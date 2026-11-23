"""Derive clothed, skinned geometry from the licensed Quaternius base.
Splits source triangle indices into skin, jacket, trousers and boots; preserves
all original skin weights, UVs and the source rig. No raster assets are edited.
"""
from pathlib import Path
import json, struct
root = Path('assets/models/survivor')
data = json.loads((root/'Superhero_Male_FullBody.gltf').read_text(encoding='utf-8'))
blob = bytearray((root/data['buffers'][0]['uri']).read_bytes())
def values(index):
    a=data['accessors'][index]; v=data['bufferViews'][a['bufferView']]
    size={'SCALAR':1,'VEC3':3}[a['type']]
    code={5123:'H',5125:'I',5126:'f'}[a['componentType']]
    stride=v.get('byteStride',struct.calcsize(code)*size)
    at=v.get('byteOffset',0)+a.get('byteOffset',0)
    return [struct.unpack_from('<'+code*size,blob,at+i*stride) for i in range(a['count'])]
material_ids={}
for name,color in [('Jacket',[.21,.28,.27,1]),('Trousers',[.10,.13,.14,1]),('Boots',[.055,.047,.04,1])]:
    material_ids[name]=len(data['materials'])
    data['materials'].append({'name':name,'pbrMetallicRoughness':{'baseColorFactor':color,'metallicFactor':0,'roughnessFactor':.92}})
for mesh in data['meshes']:
    if mesh['name']!='Sphere.005_Retopology.004': continue
    original=mesh['primitives'][0]; pos=values(original['attributes']['POSITION']); idx=[v[0] for v in values(original['indices'])]
    # Loose work jacket and straight trousers instead of painted muscle contours.
    changed=[]
    for x,y,z in pos:
        if .96 < y < 1.49 and abs(x)<.28:
            width=.235 if y<1.2 else .255
            x=(1 if x>=0 else -1)*max(abs(x)*1.08,width*min(abs(x)/.14,1))
            z=(1 if z>=0 else -1)*max(abs(z),.145*min(abs(z)/.075,1))
        elif .2<y<.95: x*=1.045; z*=1.12
        changed.append((x,y,z))
    a=data['accessors'][original['attributes']['POSITION']];v=data['bufferViews'][a['bufferView']]
    at=v.get('byteOffset',0)+a.get('byteOffset',0);stride=v.get('byteStride',12)
    for i,p in enumerate(changed): struct.pack_into('<3f',blob,at+i*stride,*p)
    import math
    normals=[[0.,0.,0.] for p in changed]
    for i in range(0,len(idx),3):
        ids=idx[i:i+3];p,q,r=[changed[j] for j in ids]
        u=[q[k]-p[k] for k in range(3)];w=[r[k]-p[k] for k in range(3)]
        n=[u[1]*w[2]-u[2]*w[1],u[2]*w[0]-u[0]*w[2],u[0]*w[1]-u[1]*w[0]]
        for j in ids:
            for k in range(3): normals[j][k]+=n[k]
    na=data['accessors'][original['attributes']['NORMAL']];nv=data['bufferViews'][na['bufferView']]
    nat=nv.get('byteOffset',0)+na.get('byteOffset',0);ns=nv.get('byteStride',12)
    for i,n in enumerate(normals):
        length=math.sqrt(sum(c*c for c in n)) or 1
        struct.pack_into('<3f',blob,nat+i*ns,*[c/length for c in n])
    groups={'Skin':[],'Jacket':[],'Trousers':[],'Boots':[]}
    for i in range(0,len(idx),3):
        tri=idx[i:i+3]; x=sum(pos[j][0] for j in tri)/3; y=sum(pos[j][1] for j in tri)/3
        group='Boots' if y<.17 else ('Trousers' if y<.96 else ('Jacket' if y<1.57 and abs(x)<.73 else 'Skin'))
        groups[group].extend(tri)
    mesh['primitives']=[]
    for name,indices in groups.items():
        if not indices: continue
        while len(blob)%4: blob.append(0)
        offset=len(blob);blob.extend(struct.pack('<'+'I'*len(indices),*indices))
        view=len(data['bufferViews']);data['bufferViews'].append({'buffer':0,'byteOffset':offset,'byteLength':len(indices)*4,'target':34963})
        acc=len(data['accessors']);data['accessors'].append({'bufferView':view,'componentType':5125,'count':len(indices),'type':'SCALAR','min':[min(indices)],'max':[max(indices)]})
        primitive=dict(original);primitive['indices']=acc;primitive['material']=original['material'] if name=='Skin' else material_ids[name]
        mesh['primitives'].append(primitive)
data['buffers']=[{'uri':'survivor.bin','byteLength':len(blob)}]
data.setdefault('extras',{})['outfit_geometry_version']=2
(root/'survivor.bin').write_bytes(blob)
(root/'survivor.gltf').write_text(json.dumps(data),encoding='utf-8')
print('Generated outfit; rig and skinning preserved')
