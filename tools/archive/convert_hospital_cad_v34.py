from pathlib import Path
import sys,json,numpy as np
R=Path('D:/Projects/ECHO');sys.path.insert(0,str(R/'.tools/python_gmsh'));import gmsh
O=R/'assets/models/hospital_v34';D=json.loads((R/'assets/external/hospital_v34/SOURCE_MANIFEST.json').read_text());rows=[]
gmsh.initialize();gmsh.option.setNumber('General.Terminal',0)
for row in D['assets']:
 if Path(row['file']).suffix!='.step':continue
 gmsh.clear();gmsh.option.setString('Geometry.OCCTargetUnit','M');gmsh.open(row['file']);gmsh.model.occ.synchronize();bbox=gmsh.model.getBoundingBox(-1,-1);largest=max(bbox[i+3]-bbox[i] for i in range(3));assert .02<largest<20,(row['id'],bbox)
 gmsh.option.setNumber('Mesh.MeshSizeMin',max(.0007,largest/1500));gmsh.option.setNumber('Mesh.MeshSizeMax',min(.025,largest/25));gmsh.option.setNumber('Mesh.MeshSizeFromCurvature',24);gmsh.option.setNumber('Mesh.ElementOrder',1);gmsh.model.mesh.generate(2);tags,coords,_=gmsh.model.mesh.getNodes();vertices=np.array(coords).reshape(-1,3);mapping={int(n):i+1 for i,n in enumerate(tags)};folder=O/row['slug'];folder.mkdir(parents=True,exist_ok=True);obj=folder/'cad_tessellated.obj';tri=0;parts=[]
 with obj.open('w') as f:
  for v in vertices:f.write('v %.7f %.7f %.7f\n'%tuple(v))
  for dim,tag in gmsh.model.getEntities(2):
   types,element_tags,node_tags=gmsh.model.mesh.getElements(dim,tag);name=gmsh.model.getEntityName(dim,tag) or 'surface_'+str(tag);f.write('g '+name.replace(' ','_')+'\n');part_tri=0
   for typ,nodes in zip(types,node_tags):
    if typ!=2:continue
    for t in np.array(nodes).reshape(-1,3):f.write('f %d %d %d\n'%tuple(mapping[int(n)] for n in t));tri+=1;part_tri+=1
   parts.append(dict(name=name,triangles=part_tri))
 rows.append(dict(**row,tessellated=str(obj),bounds_m=bbox,triangles=tri,surfaces=parts,conversion='Gmsh 4.15.2 / OpenCASCADE, surfaces only, metres'));print('CAD_CONVERTED',row['id'],row['title'],tri,flush=True)
gmsh.finalize();(R/'artifacts/echo_hospital_sources_v34/cad_converted.json').write_text(json.dumps(dict(success=True,models=rows),ensure_ascii=False,indent=2));print('CAD_COMPLETE',len(rows))