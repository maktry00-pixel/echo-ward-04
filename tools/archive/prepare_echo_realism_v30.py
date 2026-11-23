"""Metric, reversible preparation of verified CC0 sources; existing library stays intact."""
import ast,base64,hashlib,json,struct,copy
from pathlib import Path
import numpy as np
R=Path(__file__).resolve().parents[1];EXT=R/'assets/external/quality_v30';OUT=R/'unreal/EchoWard/RawAssets/EchoRealV30';OUT.mkdir(parents=True,exist_ok=True)
materials={};assets=[];alpha=json.loads((EXT/'ALPHA_MANIFEST.json').read_text());select={}
# Reuse the established handedness/UV/normals converter without its batch side effects.
old=ast.parse((R/'tools/prepare_echo_real_v23.py').read_text())
for n in old.body:
 if isinstance(n,ast.FunctionDef) and n.name in ('transform','export'):exec(compile(ast.Module(body=[n],type_ignores=[]),'<metric-converter>','exec'))
p=EXT/'graphite_four_door_sedan';b=(p/'sedan.glb').read_bytes();magic,ver,size=struct.unpack_from('<4sII',b);assert magic==b'glTF' and ver==2 and size==len(b)
o=12;chunks={}
while o<len(b):
 length,kind=struct.unpack_from('<II',b,o);chunks[kind]=b[o+8:o+8+length];o+=8+length
j=json.loads(chunks[0x4e4f534a]);binary=chunks[0x004e4942];(p/'sedan.bin').write_bytes(binary);j['buffers'][0]['uri']='sedan.bin'
for i,im in enumerate(j.get('images',[])):
 v=j['bufferViews'][im.pop('bufferView')];assert v.get('buffer',0)==0
 name='embedded_%02d.png'%i;(p/name).write_bytes(binary[v.get('byteOffset',0):v.get('byteOffset',0)+v['byteLength']]);im['uri']=name
for n in j['nodes']:
 if n.get('name')=='Shark fin antenna':n.pop('mesh',None)
for m in j['materials']:
 # Opaque shells have consistent outward normals; glass stays double sided.
 m['doubleSided']='glass' in m['name'] or 'lens' in m['name']
(p/'sedan.gltf').write_text(json.dumps(j))
source=dict(id=920,slug='graphite_four_door_sedan',title='Graphite four-door sedan',file=str(p/'sedan.gltf'),source='https://www.innerscene.com/tools/library/3d-parts/graphite-four-door-sedan-eed96d12',authors={'Innerscene':'Publisher'},license='CC0-1.0')
sources=[source]+json.loads((EXT/'SOURCE_MANIFEST.json').read_text())['assets']
for a in sources:
 if a['id']==926:a['variant_nodes']=['modular_chainlink_fence']
 if a['id']==925:a['variant_nodes']=['rollershutter_door']
 export(a)
# Give every new material its own namespace; no existing MI is overwritten.
renamed={k:'RP30_'+k.removeprefix('RP23_') for k in materials}
for a in assets:
 a['materials']=[renamed[k] for k in a['materials']]
 for suffix in ['.obj','.mtl']:
  f=OUT/(a['name']+suffix);t=f.read_text()
  for before,after in renamed.items():t=t.replace(before,after)
  f.write_text(t)
materials={renamed[k]:v for k,v in materials.items()}
(OUT/'manifest.json').write_text(json.dumps(dict(version=30,units='cm',assets=assets,materials=materials),ensure_ascii=False,indent=2,default=lambda x:x.item()),encoding='utf8')
print('V30_PREPARED',len(assets),len(materials))
