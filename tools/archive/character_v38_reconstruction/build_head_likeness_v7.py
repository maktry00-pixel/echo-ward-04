from pathlib import Path
src=Path(__file__).with_name('build_head_likeness_v6.py').read_text(encoding='utf8')
src=src.replace("O=R/'artifacts/echo_hero_v38/likeness_head_v6';O.mkdir(exist_ok=True)","O=R/'artifacts/echo_hero_v38/likeness_head_v7';O.mkdir(exist_ok=True)")
src=src.replace("rows=json.loads((O/'texture_landmarks.json').read_text(encoding='utf8'))", "rows=json.loads((R/'artifacts/echo_hero_v38/likeness_head_v6/texture_landmarks.json').read_text(encoding='utf8'))")
src=src.replace("O/'approved_portrait_eyes_open.png'", "R/'artifacts/echo_hero_v38/likeness_head_v6/approved_portrait_eyes_open.png'")
needle="coords=np.array([tuple(v.co) for v in head.data.vertices]);projected=source_uv(coords)"
src=src.replace(needle,needle+'''
# Fit the evaluated static mesh, avoiding an old facial rig shifting the fitted result.
# Camera scale and roll are registered by the two outer eye corners.
sid=src[[33,263]];tid=tex[[33,263]]
su=(sid[1]-sid[0]);tu=(tid[1]-tid[0]);su/=np.linalg.norm(su);tu/=np.linalg.norm(tu)
sf=np.stack([su,[-su[1],su[0]]],axis=1);tf=np.stack([tu,[-tu[1],tu[0]]],axis=1)
target=(tex-tex[[33,263]].mean(axis=0))@tf*(np.linalg.norm(sid[1]-sid[0])/np.linalg.norm(tex[263]-tex[33]))@sf.T+sid.mean(axis=0)
gc=src[ids];gd=(target-src)[ids]
# Forehead hidden by hair is not reliable anatomy. Suppress those controls.
for k,idx in enumerate(ids):
 if idx in [10,338,297,332,284,251,389,127,162,21,54,103,67,109]:gd[k]*=.2
gc=np.vstack([gc,[[.15,.2],[.85,.2],[.08,.9],[.92,.9],[.5,1.15]]]);gd=np.vstack([gd,np.zeros((5,2))])
gp=np.column_stack((np.ones(len(gc)),gc));ga=np.block([[kernel(gc[:,None]-gc[None,:])+np.eye(len(gc))*1e-4,gp],[gp.T,np.zeros((3,3))]])
gcoef=np.linalg.solve(ga,np.vstack([gd,np.zeros((3,2))]))
xy=np.column_stack([.5+coords[:,0]/.33,.5-(coords[:,2]-1.62)/.33])
delta=kernel(xy[:,None]-gc[None,:])@gcoef[:len(gc)]+np.column_stack([np.ones(len(xy)),xy])@gcoef[len(gc):]
skinpoly=[p.material_index for p in head.data.polygons]
for v,d in zip(head.data.vertices,delta):
 p=v.co;w=float(np.clip((.06-p.y)/.10,0,1)*np.clip((p.z-1.49)/.05,0,1))
 p.x+=float(np.clip(d[0]*.33,-.012,.012))*w;p.z-=float(np.clip(d[1]*.33,-.012,.012))*w
 # Reduce the pronounced projecting chin of the generic source in the real profile.
 chin=math.exp(-((p.z-1.535)/.023)**2-(p.x/.050)**4)*float(np.clip((-.025-p.y)/.055,0,1))
 p.y+=.0045*chin
head.data.update()
''')
start=src.index('# Hair shell follows')
end=src.index('# Join only actual head/hair surfaces')
src=src[:start]+'''
# A continuous scalp cap with a traced fringe boundary replaces the perforated color-threshold shell.
from mathutils.bvhtree import BVHTree
hairline=np.array([[.10,.43],[.16,.385],[.23,.365],[.30,.36],[.36,.34],[.42,.32],[.49,.296],[.55,.274],[.60,.28],[.66,.309],[.72,.333],[.79,.346],[.84,.332],[.9,.40]])
hairflag=[]
for v,xy in zip(head.data.vertices,projected):
 p=v.co
 boundary=float(np.interp(xy[0],hairline[:,0],hairline[:,1]))
 front_flag=p.z>1.642 and xy[1]<boundary
 back_flag=p.z>1.645
 side_flag=p.z>1.662
 hairflag.append(front_flag if p.y<-.035 else back_flag if p.y>.025 else side_flag)
polys=[p for p,mi in zip(head.data.polygons,skinpoly) if mi==0 and any(hairflag[i] for i in p.vertices)]
used=sorted({i for p in polys for i in p.vertices});lookup={v:i for i,v in enumerate(used)}
vs=[]
for idx in used:
 v=head.data.vertices[idx];p=v.co.copy();n=v.normal
 top=float(np.clip((p.z-1.66)/.085,0,1))
 lift=.006+.023*top
 # More crown volume and a fuller swept fringe on the reference's image-left side.
 lift+=.007*math.exp(-((p.x+.028)/.05)**2-((p.z-1.71)/.06)**2)*float(np.clip(-p.y/.07,0,1))
 p+=n*lift;vs.append(tuple(p))
hairme=bpy.data.meshes.new('Continuous fitted scalp cap');hairme.from_pydata(vs,[],[tuple(lookup[i] for i in p.vertices) for p in polys]);hairme.update()
hair=bpy.data.objects.new('Beka continuous swept scalp',hairme);bpy.context.collection.objects.link(hair)
for p in hairme.polygons:p.use_smooth=True
hairmat=bpy.data.materials.new('Dark chestnut black scalp');hairmat.use_nodes=True
hn=hairmat.node_tree.nodes;hl=hairmat.node_tree.links;hp=hn.get('Principled BSDF')
hp.inputs['Base Color'].default_value=(.0035,.0024,.0018,1);hp.inputs['Roughness'].default_value=.48;hp.inputs['Specular IOR Level'].default_value=.14
noise=hn.new('ShaderNodeTexNoise');noise.inputs['Scale'].default_value=720;noise.inputs['Detail'].default_value=2
bump=hn.new('ShaderNodeBump');bump.inputs['Strength'].default_value=.19;bump.inputs['Distance'].default_value=.00025;hl.new(noise.outputs['Fac'],bump.inputs['Height']);hl.new(bump.outputs['Normal'],hp.inputs['Normal'])
hairme.materials.append(hairmat)
# Hair is actual tapered strand geometry, following the cap; no photograph cards or floating plates.
capverts=[Vector(v) for v in vs];capfaces=[list(p.vertices) for p in hairme.polygons]
capbvh=BVHTree.FromPolygons(capverts,capfaces,all_triangles=False)
rng=random.Random(3807)
strandmat=bpy.data.materials.new('Black brown strands');strandmat.use_nodes=True
sp=strandmat.node_tree.nodes.get('Principled BSDF');sp.inputs['Base Color'].default_value=(.009,.006,.0038,1);sp.inputs['Roughness'].default_value=.42;sp.inputs['Specular IOR Level'].default_value=.18
cv=bpy.data.curves.new('Side swept short hair strands','CURVE');cv.dimensions='3D';cv.resolution_u=1;cv.bevel_depth=.00011;cv.bevel_resolution=0;cv.resolution_u=1;cv.materials.append(strandmat)
center=Vector((0,.015,1.659))
def shell(phi,theta):
 d=Vector((math.sin(theta)*math.sin(phi)*.089,-math.sin(theta)*math.cos(phi)*.115,math.cos(theta)*.115))
 q=center+d;hit=capbvh.find_nearest(q)
 if not hit[0]:return q,Vector((0,0,1))
 return hit[0],hit[1]
for i in range(6200):
 phi=rng.uniform(-math.pi,math.pi);theta=rng.uniform(.07,1.19)
 # Crown and front sweep toward image-left, with shorter sides and downwards back.
 front=math.cos(phi)>.10
 length=rng.uniform(.35,.72) if front else rng.uniform(.25,.5)
 points=[]
 for j in range(18):
  t=j/17;ang=theta+t*length
  ph=phi-(.36*t+.10*math.sin(t*math.pi)) if front else phi+.08*math.sin(t*math.pi)
  p,n=shell(ph,ang)
  loft=.0004+(.0015+rng.random()*.0006)*math.sin(t*math.pi)+.0003*math.sin(t*8+i)
  p+=n*loft;points.append(p)
 # Skip strands extending below the traced hair cap.
 if (points[-1]-points[-2]).length>.014:continue
 spl=cv.splines.new('POLY');spl.points.add(len(points)-1)
 for j,p in enumerate(points):spl.points[j].co=(*p,1);spl.points[j].radius=max(.10,math.sin((j/17*.85+.10)*math.pi))
strands=bpy.data.objects.new('Beka hair individual strands',cv);bpy.context.collection.objects.link(strands)
# Keep the scalp and strands as PBR geometry, independent from facial albedo baking.
''' + src[end:]
src=src.replace("head.select_set(True);hair.select_set(True)","head.select_set(True)")
src=src.replace("head.data.materials.clear();head.data.materials.append(mat)","n.get('Principled BSDF').inputs['Specular IOR Level'].default_value=.22\nn.get('Principled BSDF').inputs['Subsurface Weight'].default_value=.055\nhead.data.materials.clear();head.data.materials.append(mat)")
src=src.replace("bpy.ops.export_scene.gltf(filepath", "hair.select_set(True);strands.select_set(True)\nbpy.context.view_layer.objects.active=strands;bpy.ops.object.convert(target='MESH');strands=bpy.context.object;bpy.context.view_layer.objects.active=head\nbpy.ops.export_scene.gltf(filepath")
src=src.replace("sc.cycles.samples=28", "sc.cycles.samples=40")
src=src.replace("bpy.ops.wm.save_as_mainfile(filepath", "sc.view_settings.view_transform='AgX'\nbpy.ops.wm.save_as_mainfile(filepath")
src=src.replace("v6", "v7")
# Prior landmarks and photo are inputs from v6, not outputs of this iteration.
src=src.replace("likeness_head_v7/texture_landmarks", "likeness_head_v6/texture_landmarks")
src=src.replace("likeness_head_v7/approved_portrait", "likeness_head_v6/approved_portrait")
exec(compile(src,str(Path(__file__)), 'exec'))
