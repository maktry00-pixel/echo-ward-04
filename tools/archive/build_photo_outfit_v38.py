"""Original photo-specific clothing prototype on exported MetaHuman proportions.
No image synthesis, no third-party garment geometry, no game replacement.
"""
import bpy,bmesh,math,json,random
from pathlib import Path
from mathutils import Vector
from mathutils.kdtree import KDTree
ROOT=Path('D:/Projects/ECHO');OUT=ROOT/'assets/models/hero_custom/photo_outfit_v1';OUT.mkdir(parents=True,exist_ok=True)
REF=ROOT/'artifacts/echo_hero_v38/photo_fit_v2'
random.seed(38)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.preferences.filepaths.save_version=0
bpy.ops.import_scene.fbx(filepath=str(REF/'body.fbx'),use_anim=False)
rig=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
body=max((o for o in bpy.context.scene.objects if o.type=='MESH'),key=lambda o:len(o.data.vertices))
body.hide_render=True
for o in bpy.context.scene.objects:
 if o.type=='EMPTY':o.hide_render=True
parts=[]
def mat(name,color,rough=.7,metal=0.,weave=False):
 m=bpy.data.materials.new(name);m.diffuse_color=(*color,1);m.use_nodes=True
 n=m.node_tree.nodes;p=n.get('Principled BSDF');p.inputs['Base Color'].default_value=(*color,1);p.inputs['Roughness'].default_value=rough;p.inputs['Metallic'].default_value=metal
 if weave:
  tex=n.new('ShaderNodeTexNoise');tex.inputs['Scale'].default_value=450.;tex.inputs['Detail'].default_value=2.
  bump=n.new('ShaderNodeBump');bump.inputs['Strength'].default_value=.22;bump.inputs['Distance'].default_value=.00022
  m.node_tree.links.new(tex.outputs['Fac'],bump.inputs['Height']);m.node_tree.links.new(bump.outputs['Normal'],p.inputs['Normal'])
 return m
fleece=mat('Black charcoal fleece',(.004,.006,.005),.76,weave=True)
rib=mat('Black rib knit cuffs',(.009,.012,.011),.85,weave=True)
trim=mat('Muted graphite V seam tape',(.055,.063,.058),.65)
inside=mat('Hood lining',(.012,.017,.014),.93,weave=True)
zipmat=mat('Blackened zipper metal',(.065,.075,.068),.35,.7)
denim=mat('Dark olive trousers',(.020,.023,.016),.84,weave=True)
shoemat=mat('Black sneaker panels',(.012,.015,.014),.59,weave=True)
solemat=mat('Off white rubber soles',(.50,.46,.36),.87)
lacemat=mat('Black woven laces',(.008,.011,.009),.94,weave=True)

def mesh(name,vs,fs,material,sub=0):
 me=bpy.data.meshes.new(name);me.from_pydata(vs,[],fs);me.update();o=bpy.data.objects.new(name,me);bpy.context.collection.objects.link(o);o.data.materials.append(material)
 for p in me.polygons:p.use_smooth=True
 if sub:
  mod=o.modifiers.new('Tailoring surface','SUBSURF');mod.levels=sub;mod.render_levels=sub
 parts.append(o);return o

def loft(name,rings,material,N=64,wrinkle=.001,sub=1):
 # ring: center, horizontal axis, perpendicular axis, width/depth radii
 vs=[];fs=[]
 for j,(center,a,b,ra,rb) in enumerate(rings):
  center=Vector(center);a=Vector(a);b=Vector(b)
  for k in range(N):
   t=2*math.pi*k/N
   wave=wrinkle*(math.sin(k*.43+j*1.15)+.3*math.sin(k*.88-j*.7))*math.sin(math.pi*j/max(1,len(rings)-1))
   vs.append(tuple(center+a*((ra+wave)*math.cos(t))+b*((rb+wave)*math.sin(t))))
 for j in range(len(rings)-1):
  for k in range(N):fs.append((j*N+k,j*N+(k+1)%N,(j+1)*N+(k+1)%N,(j+1)*N+k))
 o=mesh(name,vs,fs,material,sub)
 uv=o.data.uv_layers.new(name='TailorUV')
 for p in o.data.polygons:
  for li in p.loop_indices:
   vi=o.data.loops[li].vertex_index;uv.data[li].uv=((vi%N)/N,(vi//N)/(len(rings)-1))
 return o

def curve(name,coords,radius,material):
 cu=bpy.data.curves.new(name,'CURVE');cu.dimensions='3D';cu.resolution_u=12;cu.bevel_depth=radius;cu.bevel_resolution=2
 sp=cu.splines.new('POLY');sp.points.add(len(coords)-1)
 for p,co in zip(sp.points,coords):p.co=(*co,1)
 o=bpy.data.objects.new(name,cu);bpy.context.collection.objects.link(o);o.data.materials.append(material);parts.append(o);return o

def box(name,center,scale,material,bevel=.001):
 bpy.ops.mesh.primitive_cube_add(size=1,location=center);o=bpy.context.object;o.name=name;o.scale=scale;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(material)
 if bevel:
  mod=o.modifiers.new('Edge rounding','BEVEL');mod.width=bevel;mod.segments=2
 parts.append(o);return o

# Jacket: body, actual folded hood, full-length front zip, bound seams and sleeve pocket.
profiles=[(.925,.175,.118),(.94,.184,.127),(.98,.189,.132),(1.04,.191,.132),(1.10,.193,.136),(1.16,.199,.143),(1.22,.211,.150),(1.28,.222,.148),(1.33,.218,.140),(1.38,.205,.125),(1.42,.173,.108),(1.455,.104,.081),(1.47,.080,.076)]
loft('Full zip hoodie body',[( (0,0,z),(1,0,0),(0,1,0),rx,ry) for z,rx,ry in profiles],fleece,96,.0012,2)
loft('Elastic waistband',[( (0,0,z),(1,0,0),(0,1,0),rx,ry) for z,rx,ry in [(.917,.177,.119),(.924,.180,.122),(.945,.182,.124),(.952,.181,.123)]],rib,96,0.,1)
# Closed garment collar is separate from the hood folded behind the neck.
loft('Zip collar',[( (0,.003,z),(1,0,0),(0,1,0),rx,ry) for z,rx,ry in [(1.43,.091,.081),(1.46,.088,.080),(1.49,.085,.078),(1.505,.083,.076)]],fleece,64,.0007,1)
# Collapsed hood: open U-shaped shell, real inner lining and thickness, resting behind the neck.
vs=[];fs=[];N=56;M=14
for j in range(M):
 v=j/(M-1)
 for k in range(N):
  t=math.pi*k/(N-1)
  # Raised soft rim and hollow centre, forward opening around collar.
  rx=.083+.035*math.sin(v*math.pi);ry=.080+.065*math.sin(v*math.pi)
  x=rx*math.cos(t);y=.014+ry*math.sin(t);z=1.43+.075*v+.030*math.sin(t)-.020*math.sin(v*math.pi)
  vs.append((x,y,z))
for j in range(M-1):
 for k in range(N-1):fs.append((j*N+k,j*N+k+1,(j+1)*N+k+1,(j+1)*N+k))
hood=mesh('Folded hood with lining',vs,fs,inside,2);solid=hood.modifiers.new('Double layer hood','SOLIDIFY');solid.thickness=.003
# A-pose sleeve centreline is fitted to the actual exported joints.
for side in [-1,1]:
 shoulder=Vector((side*.120,.014,1.335));elbow=Vector((side*.328,.013,1.166));wrist=Vector((side*.462,-.151,1.006))
 rings=[]
 for j in range(29):
  t=j/28
  if t<.5:center=shoulder.lerp(elbow,t*2);tangent=(elbow-shoulder).normalized()
  else:center=elbow.lerp(wrist,(t-.5)*2);tangent=(wrist-elbow).normalized()
  a=tangent.cross(Vector((0,1,0))).normalized();b=tangent.cross(a).normalized();rad=.078*(1-t)+.039*t
  rad+=.004*math.exp(-((t-.53)/.11)**2)*math.sin(t*70)
  rings.append((center,a,b,rad,rad*.93))
 loft(('Left' if side==1 else 'Right')+' tailored sleeve',rings,fleece,56,.0012,1)
 cr=[]
 for j in range(5):
  center=wrist+(wrist-elbow).normalized()*((j/4-.5)*.036)
  cr.append((center,rings[-1][1],rings[-1][2],.039,.037))
 loft(('Left' if side==1 else 'Right')+' ribbed cuff',cr,rib,48,.0003,1)
 # V panel trim: small relief in the geometry rather than painted stripes.
 curve('V chest seam '+str(side),[(side*.012,-.148,1.207),(side*.06,-.146,1.23),(side*.12,-.134,1.260),(side*.183,-.100,1.291)],.0031,trim)
 curve('Raglan shoulder seam '+str(side),[(side*.075,-.061,1.45),(side*.11,-.095,1.41),(side*.17,-.108,1.36),(side*.215,-.080,1.30)],.0009,rib)
 curve('Hip pocket zipper '+str(side),[(side*.153,-.09,1.045),(side*.171,-.076,1.095),(side*.181,-.073,1.15)],.0018,zipmat)
# Full front zipper. Front is negative Y in the FBX coordinate system.
zipline=[]
for j in range(70):
 z=.933+j/69*.566
 ry=next((profiles[q][2]+(profiles[q+1][2]-profiles[q][2])*(z-profiles[q][0])/(profiles[q+1][0]-profiles[q][0]) for q in range(len(profiles)-1) if profiles[q][0]<=z<=profiles[q+1][0]),.078)
 zipline.append((0,-ry-.002,z))
curve('Full length zipper tape',zipline,.0034,rib)
curve('Full length zipper centre',[(0,y-.001,z) for x,y,z in zipline],.0013,zipmat)
# Joined batch of zipper teeth, not hundreds of separate actors.
vs=[];fs=[]
for i in range(170):
 t=i/169;z=.938+t*.552;ind=min(68,int(t*69));y=zipline[ind][1]-.002
 for sign in [-1,1]:
  x=sign*.0017;dx=.0015;dy=.001;dz=.0011;k=len(vs)
  vs.extend([(x+a*dx,y+b*dy,z+c*dz) for a,b,c in [(-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)]])
  fs.extend([tuple(k+v for v in f) for f in [(0,1,2,3),(4,7,6,5),(0,4,5,1),(1,5,6,2),(2,6,7,3),(4,0,3,7)]])
mesh('Zipper teeth',vs,fs,zipmat)
box('Zipper pull',(0,-.081,1.482),(.009,.004,.021),zipmat,.0015)
# Left arm zipped utility pocket.
curve('Left sleeve pocket border',[(.217,-.045,1.37),(.251,-.05,1.33),(.266,-.045,1.295),(.244,-.066,1.283),(.209,-.065,1.325),(.217,-.045,1.37)],.0022,rib)
curve('Left sleeve pocket zip',[(.217,-.070,1.351),(.246,-.068,1.311)],.002,zipmat)
box('Sleeve zip pull',(.235,-.073,1.324),(.005,.003,.013),zipmat,.001)
# Trousers: straight olive legs, waistband, belt loops, fly and folded cuffs.
# Derive a connected trouser shell from the fitted anatomy so the crotch has real topology.
coords=[body.matrix_world@v.co for v in body.data.vertices]
chosen=[p for p in body.data.polygons if all(.135<=coords[i].z<=.938 and abs(coords[i].x)<.26 for i in p.vertices)]
used=sorted({i for p in chosen for i in p.vertices});lookup={v:i for i,v in enumerate(used)};vs=[]
for i in used:
 p=coords[i].copy();n=(body.matrix_world.to_3x3()@body.data.vertices[i].normal).normalized();p+=n*.014
 if p.z<.78:
  side=1 if p.x>0 else -1;center=side*(.124-.035*min(1.,max(0.,(p.z-.145)/.715)))
  factor=1.12+.30*max(0.,(.48-p.z)/.48)
  p.x=center+(p.x-center)*factor;p.y=-.012+(p.y+.012)*factor
  p.y+=.0014*math.sin(p.z*55+p.x*80)*math.exp(-((p.z-.48)/.20)**2)
 vs.append(tuple(p))
trousers=mesh('Connected olive trousers',vs,[tuple(lookup[i] for i in p.vertices) for p in chosen],denim,0)
bm=bmesh.new();bm.from_mesh(trousers.data);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.003);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(trousers.data);bm.free()
smooth=trousers.modifiers.new('Relax woven trouser drape','SMOOTH');smooth.factor=.65;smooth.iterations=12
solid=trousers.modifiers.new('Woven cloth thickness','SOLIDIFY');solid.thickness=.0018
for side in [-1,1]:
 loft('Folded trouser cuff '+str(side),[( (side*.123,-.005,z),(1,0,0),(0,1,0),.066,.071) for z in [.137,.144,.165,.174]],denim,64,.001,1)
 curve('Trouser outer seam '+str(side),[(side*(.124+.063),-.005,.17),(side*.19,-.01,.45),(side*.183,-.017,.70),(side*.166,-.02,.93)],.0008,rib)
 for x,y in [(side*.11,-.10),(side*.15,.055)]:box('Belt loop',(x,y,.931),(.011,.003,.037),denim,.001)
curve('Trouser fly',[(.004,-.125,.93),(.009,-.13,.885),(.007,-.127,.84)],.001,rib)
# High-top sneakers: rubber outsole, leather upper panels, ankle collar, tongue and real laces.
for side in [-1,1]:
 cx=side*.127;cy=-.075
 outline=[(-.046,.105),(-.053,.072),(-.052,-.035),(-.051,-.104),(-.036,-.147),(0,-.156),(.036,-.147),(.051,-.104),(.052,-.035),(.053,.072),(.046,.105)]
 def shoe_shell(name,levels,material):
  vs=[];fs=[];N=len(outline)
  for z,sx,sy in levels:
   for x,y in outline:vs.append((cx+x*sx,cy+y*sy,z))
  for j in range(len(levels)-1):
   for k in range(N):fs.append((j*N+k,j*N+(k+1)%N,(j+1)*N+(k+1)%N,(j+1)*N+k))
  fs.append(tuple(range(N-1,-1,-1)));fs.append(tuple((len(levels)-1)*N+k for k in range(N)))
  return mesh(name,vs,fs,material,2)
 shoe_shell('Cream rubber sole '+str(side),[(.002,1.,1.),(.008,1.04,1.02),(.027,1.04,1.02),(.034,1.,1.)],solemat)
 shoe_shell('Black sneaker upper '+str(side),[(.028,1.,1.),(.048,.98,.98),(.083,.91,.88),(.107,.70,.55)],shoemat)
 loft('Sneaker ankle collar '+str(side),[( (cx,.008,z),(1,0,0),(0,1,0),rx,ry) for z,rx,ry in [(.077,.048,.055),(.118,.044,.052),(.145,.041,.046)]],shoemat,48,.0007,1)
 for j in range(7):
  y=-.14+j*.017;z=.091+j*.004
  curve('Crossed sneaker lace',[(cx-.023,y,z),(cx+.023,y+.015,z+.003)],.0016,lacemat)
  curve('Crossed sneaker lace',[(cx+.023,y,z+.001),(cx-.023,y+.015,z+.003)],.0016,lacemat)
 curve('Sneaker sole stitch',[(cx+x,cy+y,.029) for x,y in outline+[outline[0]]],.0007,trim)
# Convert to actual exportable meshes and transfer anatomical weights from the MetaHuman base.
for o in list(parts):
 bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
 if o.type=='CURVE':bpy.ops.object.convert(target='MESH')
 for mod in list(o.modifiers):
  if mod.type!='ARMATURE':bpy.ops.object.modifier_apply(modifier=mod.name)
 bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
 transfer=o.modifiers.new('Photo fit skin weights','DATA_TRANSFER');transfer.object=body;transfer.use_vert_data=True;transfer.data_types_verts={'VGROUP_WEIGHTS'};transfer.vert_mapping='POLYINTERP_NEAREST'
 bpy.ops.object.datalayout_transfer(modifier=transfer.name)
 bpy.ops.object.modifier_apply(modifier=transfer.name)
 o.parent=rig;o.matrix_parent_inverse=rig.matrix_world.inverted();arm=o.modifiers.new('MetaHuman body skeleton','ARMATURE');arm.object=rig
 # UVs for accessories and any original seam geometry.
 if not o.data.uv_layers:
  bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.smart_project(island_margin=.015);bpy.ops.object.mode_set(mode='OBJECT')
# Export real meshes; keep source body only as an invisible fitting reference in the .blend.
bpy.ops.object.select_all(action='DESELECT');rig.select_set(True)
for o in parts:o.select_set(True)
bpy.context.view_layer.objects.active=rig
bpy.ops.export_scene.fbx(filepath=str(OUT/'beka_photo_outfit_v1.fbx'),use_selection=True,object_types={'ARMATURE','MESH'},add_leaf_bones=False,bake_anim=False,axis_forward='-Z',axis_up='Y',use_mesh_modifiers=True)
bpy.ops.export_scene.gltf(filepath=str(OUT/'beka_photo_outfit_v1.glb'),use_selection=True,export_format='GLB',export_animations=False,export_apply=False)
# Studio garment review, with lighting that makes dark trousers readable.
world=bpy.data.worlds.new('Neutral studio');world.use_nodes=True;world.node_tree.nodes['Background'].inputs[0].default_value=(.12,.14,.15,1);world.node_tree.nodes['Background'].inputs[1].default_value=.6;bpy.context.scene.world=world
for name,pos,power,size in [('Key',(-2,-3,3),600,3),('Fill',(2,-2,2),450,2),('Rim',(0,2,2.4),700,2)]:
 bpy.ops.object.light_add(type='AREA',location=pos);o=bpy.context.object;o.name=name;o.data.energy=power;o.data.shape='DISK';o.data.size=size;o.rotation_euler=(Vector((0,0,1))-o.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(0,-3.3,1.02));cam=bpy.context.object;cam.rotation_euler=(Vector((0,0,.80))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=1.75;bpy.context.scene.camera=cam
sc=bpy.context.scene;sc.view_settings.exposure=-.7;sc.render.engine='CYCLES';sc.cycles.samples=24;sc.cycles.use_denoising=True;sc.render.resolution_x=1080;sc.render.resolution_y=1350;sc.render.resolution_percentage=100;sc.render.image_settings.file_format='PNG';sc.render.film_transparent=False;sc.render.filepath=str(OUT/'garment_review.png')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'beka_photo_outfit_v1.blend'))
bpy.ops.render.render(write_still=True)
report={'prototype':True,'final_quality':False,'game_changed':False,'parts':len(parts),'vertices':sum(len(o.data.vertices) for o in parts),'triangles':sum(sum(len(p.vertices)-2 for p in o.data.polygons) for o in parts),'reference':'hero_original.jpg','features':['folded hood','full length zipper and pull','V chest seams','raglan shoulder seams','left sleeve zipped pocket','waist pockets','ribbed waistband and cuffs','olive straight trousers and folded cuffs','black high top sneakers with off-white soles and mesh laces'],'skin_weights':'nearest body polygon interpolation; animation validation pending','exports':['beka_photo_outfit_v1.fbx','beka_photo_outfit_v1.glb','beka_photo_outfit_v1.blend'],'success':True}
(OUT/'build_report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('ECHO_PHOTO_OUTFIT '+json.dumps(report))



