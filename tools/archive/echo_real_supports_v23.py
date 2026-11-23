"""Measure actual horizontal supporting faces in the imported artwork."""
import collections,json,math
from pathlib import Path
import numpy as np
R=Path(__file__).resolve().parents[1];P=R/'unreal/EchoWard/RawAssets/EchoRealV23';D=json.loads((P/'manifest.json').read_text(encoding='utf8'));out={}
names=set('metal_office_desk painted_wooden_cabinet painted_wooden_cabinet_02 drawer_cabinet wooden_bookshelf_worn steel_frame_shelves_01 steel_frame_shelves_02 steel_frame_shelves_03 tool_cart industrial_storage_cart painted_wooden_nightstand vintage_wooden_drawer_01 worn_metal_rack wooden_picnic_table dining_table coffee_table_round_01 side_table_01'.split())
for a in D['assets']:
 if a['slug'] not in names:continue
 verts=[];normals=[];faces=[]
 for line in Path(a['file']).read_text().splitlines():
  if line.startswith('v '):v=list(map(float,line.split()[1:]));verts.append([v[0]/100,-v[1]/100,v[2]/100])
  elif line.startswith('vn '):normals.append(list(map(float,line.split()[1:])))
  elif line.startswith('f '):faces.append([[int(x)-1 for x in p.split('/')] for p in line.split()[1:]])
 v=np.array(verts);n=np.array(normals);f=np.array(faces);pos=v[f[:,:,0]];nor=n[f[:,:,2]];area=np.abs(np.cross(pos[:,1]-pos[:,0],pos[:,2]-pos[:,0])[:,2])*.5
 good=(nor[:,:,2].mean(1)>.9)&(np.ptp(pos[:,:,2],axis=1)<.003)&(area>.00001);groups=collections.defaultdict(list)
 for i in np.flatnonzero(good):groups[round(float(pos[i,:,2].mean())/.003)].append(i)
 surfaces=[]
 for level,ix in groups.items():
  pts=pos[ix];lo=pts.min((0,1));hi=pts.max((0,1));ar=float(area[ix].sum());width=hi[0]-lo[0];depth=hi[1]-lo[1];z=float(pts[:,:,2].mean())
  if ar>=.018 and width>.13 and depth>.13 and ar/(width*depth)>.38 and z>.12:surfaces.append(dict(rect=[float(lo[0]),float(lo[1]),float(hi[0]),float(hi[1])],z=z,area=ar,triangles=pts[:,:,:2].tolist()))
 # Drawer interiors and the underside frame are not usable shelves. Only the
 # exposed worktop is a support for these closed furniture designs.
 if surfaces and a['slug'] in {'metal_office_desk','painted_wooden_cabinet','painted_wooden_nightstand','vintage_wooden_drawer_01','wooden_picnic_table'}:surfaces=[max(surfaces,key=lambda p:p['z'])]
 if surfaces and a['slug']=='dining_table':surfaces=[max(surfaces,key=lambda p:p['area'])]
 out[str(a['id'])]=sorted(surfaces,key=lambda p:p['z'],reverse=True)
(P/'support_planes.json').write_text(json.dumps(out,indent=2),encoding='utf8');print('SUPPORT_PLANES',sum(len(p) for p in out.values()),'MODELS',len(out))
