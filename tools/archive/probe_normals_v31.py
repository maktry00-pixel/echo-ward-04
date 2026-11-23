import bpy,math,json
from pathlib import Path
from mathutils import Matrix
m=bpy.data.meshes.new('normaltest');m.from_pydata([(0,0,0),(1,0,0),(0,1,0)],[],[(0,1,2)]);m.normals_split_custom_set([(1,0,0)]*3);print('BEFORE',[tuple(x.vector) for x in m.corner_normals]);m.transform(Matrix.Rotation(math.pi/2,4,'Z'));m.update();print('AFTER',[tuple(x.vector) for x in m.corner_normals],flush=True)
R=Path('D:/Projects/ECHO')
for slug in ['mercedes-benz-maybach-2022','free-1975-porsche-911-930-turbo']:
 d=json.loads((R/'artifacts/echo_cars_v31'/f'{slug}_scene.json').read_text());print('HELPERS',slug)
 for o in d['objects']:
  b=o.get('bounds_world')
  if b and b[0][2]<-.1:print(o['name'],o.get('triangles'),b)
