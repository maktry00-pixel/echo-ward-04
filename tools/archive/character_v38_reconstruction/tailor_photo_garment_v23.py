"""Tailor the licensed, weighted cloth mesh to the real photo. Not a final likeness approval."""
import bpy,bmesh,math,json
from pathlib import Path
from mathutils import Vector,Matrix
from mathutils.bvhtree import BVHTree
R=Path('D:/Projects/ECHO');T=R/'artifacts/echo_hero_v38/cloth_tailoring_v23';O=T/'photo_tailored';O.mkdir(exist_ok=True)
inputs=sorted((R/'artifacts/echo_hero_v38/metahuman_conform_v23/fitted_exports').glob('Beka_FittedCloth_*.fbx'))
if not inputs:raise RuntimeError('Need the fitted cloth export; do not silently use an unfitted body')
bpy.ops.wm.read_factory_settings(use_empty=True);bpy.context.preferences.filepaths.save_version=0
bpy.ops.import_scene.fbx(filepath=str(inputs[0]),use_anim=False)
garment=max((o for o in bpy.context.scene.objects if o.type=='MESH'),key=lambda o:len(o.data.vertices));rig=garment.find_armature()
if not rig:raise RuntimeError('Clothing skinning missing')
# The cloth source has its own bind proportions. Rebind its weighted geometry to the
# exported 175 cm body before placing photo-specific details in world metres.
source_rig=rig;before_objects=set(bpy.data.objects)
bpy.ops.import_scene.fbx(filepath=str(R/'artifacts/echo_hero_v38/metahuman_conform_v23/fitted_exports/Beka_Body.fbx'),use_anim=False)
added=[o for o in bpy.data.objects if o not in before_objects];target_rig=next(o for o in added if o.type=='ARMATURE')
transforms={}
for group in garment.vertex_groups:
    if group.name in source_rig.data.bones and group.name in target_rig.data.bones:
        old=source_rig.matrix_world@source_rig.data.bones[group.name].matrix_local;new=target_rig.matrix_world@target_rig.data.bones[group.name].matrix_local;transforms[group.index]=new@old.inverted()
positions=[];displacements=[]
for v in garment.data.vertices:
    old=garment.matrix_world@v.co;new=Vector();total=0.
    for g in v.groups:
        if g.weight<1e-8:continue
        if g.group not in transforms:raise RuntimeError('Missing body bone for weighted clothing group '+garment.vertex_groups[g.group].name)
        new+=(transforms[g.group]@old)*g.weight;total+=g.weight
    if total<.01:raise RuntimeError('Unweighted source cloth vertex')
    new/=total;positions.append(new);displacements.append((new-old).length)
garment.parent=None;garment.matrix_world=Matrix.Identity(4)
for v,p in zip(garment.data.vertices,positions):v.co=p
for mod in garment.modifiers:
    if mod.type=='ARMATURE':mod.object=target_rig
rig=target_rig
for ob in added:
    if ob.type=='MESH':ob.hide_render=True;ob.hide_set(True)
bpy.data.objects.remove(source_rig,do_unlink=True)
names=[m.name.lower() if m else '' for m in garment.data.materials]
jacket_ids={i for i,n in enumerate(names) if 'jacket' in n};pants_ids={i for i,n in enumerate(names) if 'pants' in n};shoe_ids={i for i,n in enumerate(names) if 'shoe' in n}
if not all([jacket_ids,pants_ids,shoe_ids]):raise RuntimeError('Expected jacket/pants/shoes material groups')
# Remove only detached cargo-pocket pieces; the continuous trouser panels remain intact.
adj={}
for p in garment.data.polygons:
    if p.material_index not in pants_ids:continue
    for a,b in p.edge_keys:adj.setdefault(a,set()).add(b);adj.setdefault(b,set()).add(a)
left=set(adj);remove=set();removed_islands=0
while left:
    seed=left.pop();group={seed};stack=[seed]
    while stack:
        for v in adj[stack.pop()]:
            if v in left:left.remove(v);group.add(v);stack.append(v)
    zmin=min(garment.data.vertices[i].co.z for i in group);zmax=max(garment.data.vertices[i].co.z for i in group)
    xmax=max(abs(garment.data.vertices[i].co.x) for i in group)
    if len(group)<900 and zmin>.54 and zmax<.90 and xmax>.165:
        # Retain the stitched panel surface; flatten its cargo bag to avoid a hole.
        for i in group:
            p=garment.data.vertices[i].co;p.x-=math.copysign(.005,p.x)
        removed_islands+=1
if remove:
    bm=bmesh.new();bm.from_mesh(garment.data);bm.verts.ensure_lookup_table();bmesh.ops.delete(bm,geom=[bm.verts[i] for i in remove],context='VERTS');bm.to_mesh(garment.data);bm.free();garment.data.update()
# Blend the source's separate upper sleeve shell into the long fleece sleeve.
adj={}
for p in garment.data.polygons:
    if p.material_index not in jacket_ids:continue
    for a,b in p.edge_keys:adj.setdefault(a,set()).add(b);adj.setdefault(b,set()).add(a)
left=set(adj);hem_remove=set();jacket_islands=[]
while left:
    seed=left.pop();group={seed};stack=[seed]
    while stack:
        for i in adj[stack.pop()]:
            if i in left:left.remove(i);group.add(i);stack.append(i)
    jacket_islands.append(group)
inner_ids={i for g in jacket_islands if 1400<len(g)<1700 for i in g}
inner_polys=[tuple(p.vertices) for p in garment.data.polygons if p.material_index in jacket_ids and all(i in inner_ids for i in p.vertices)]
inner_bvh=BVHTree.FromPolygons([v.co.copy() for v in garment.data.vertices],inner_polys,all_triangles=False)
for group in jacket_islands:
    points=[garment.data.vertices[i].co for i in group]
    # Original polo collar and detached chest/pocket flaps are replaced below.
    if len(group) in [325,71,95,81,39,33] and max(p.z for p in points)>1.06:
        hem_remove.update(group);continue
    if not 380<len(group)<750:continue
    if min(abs(p.x) for p in points)<.16 or min(p.z for p in points)<1.15:continue
    if len(group)<450 and len(group)!=410:continue
    side='l' if sum(p.x for p in points)>0 else 'r'
    shoulder=rig.matrix_world@rig.data.bones['upperarm_'+side].head_local;elbow=rig.matrix_world@rig.data.bones['lowerarm_'+side].head_local
    axis=elbow-shoulder
    for i in group:
        p=garment.data.vertices[i].co;t=(p-shoulder).dot(axis)/axis.length_squared;center=shoulder+axis*t
        # Collapse the lower short-sleeve shell onto the actual long sleeve,
        # retaining both lining surfaces and native weights to keep it closed.
        w=max(0.,min(1.,(1.345-p.z)/.060));w=w*w*(3-2*w)
        pin=max(0.,min(1.,(abs(p.x)-.235)/.065));w*=pin*pin*(3-2*pin)
        hit,normal,_,_=inner_bvh.find_nearest(p)
        if hit:garment.data.vertices[i].co=p.lerp(hit+normal*.0004,.98*w)
if hem_remove:
    bm=bmesh.new();bm.from_mesh(garment.data);bm.verts.ensure_lookup_table();bmesh.ops.delete(bm,geom=[bm.verts[i] for i in hem_remove],context='VERTS');bm.to_mesh(garment.data);bm.free();garment.data.update()
# Release the tight jogger ankle slightly to resemble the straight trousers in the photo.
pant_vertices={i for p in garment.data.polygons if p.material_index in pants_ids for i in p.vertices}
for i in pant_vertices:
    p=garment.data.vertices[i].co
    if p.z<.60:
        factor=1+.20*max(0,min(1,(.60-p.z)/.45));cx=.12 if p.x>0 else -.12;p.x=cx+(p.x-cx)*factor
    if p.z<.34:
        ankle=rig.matrix_world@rig.data.bones['foot_l' if p.x>0 else 'foot_r'].head_local
        t=max(0,min(1,(.34-p.z)/.19));factor=1+.48*t;p.x=ankle.x+(p.x-ankle.x)*factor;p.y=ankle.y+(p.y-ankle.y)*factor
worldcoords=[garment.matrix_world@v.co for v in garment.data.vertices]
original_material_indices=[p.material_index for p in garment.data.polygons]
jacket_faces=[tuple(p.vertices) for p in garment.data.polygons if p.material_index in jacket_ids]
jverts={i for p in jacket_faces for i in p};top=max(worldcoords[i].z for i in jverts);hem=min(worldcoords[i].z for i in jverts);base_top=top
face_state=json.loads((R/'artifacts/echo_hero_v38/metahuman_conform_v23/live_report.json').read_text(encoding='utf8'))
chin=(face_state.get('landmarks_after') or face_state['landmarks_before'])[8][2]/100.
top=max(base_top,chin-.014);height=top-hem
bvh=BVHTree.FromPolygons(worldcoords,jacket_faces,all_triangles=False)
def front(x,z,offset=.0015):
    if z>base_top-.075 and abs(x)<.078:
        return Vector((x,-.010-.072*math.sqrt(max(0.,1-(x/.080)**2))-offset,z))
    hit,n,idx,dist=bvh.ray_cast(Vector((x,-.65,z)),Vector((0,1,0)),1.3)
    if hit and hit.y<.025:return Vector((x,hit.y-offset,z))
    hit,n,idx,dist=bvh.find_nearest(Vector((x,-.12,z)))
    return hit+n*offset if hit else Vector((x,-.1-offset,z))
def material(name,color,rough,normal=None,micro=True,metal=0.):
    m=bpy.data.materials.new(name);m.use_nodes=True;m.diffuse_color=(*color,1);nodes=m.node_tree.nodes;links=m.node_tree.links;p=nodes.get('Principled BSDF')
    p.inputs['Base Color'].default_value=(*color,1);p.inputs['Roughness'].default_value=rough;p.inputs['Metallic'].default_value=metal;p.inputs['Specular IOR Level'].default_value=.28
    if normal:
        tex=nodes.new('ShaderNodeTexImage');tex.image=bpy.data.images.load(str(T/normal),check_existing=True);tex.image.colorspace_settings.name='Non-Color'
        sep=nodes.new('ShaderNodeSeparateColor');comb=nodes.new('ShaderNodeCombineColor');inv=nodes.new('ShaderNodeMath');inv.operation='SUBTRACT';inv.inputs[0].default_value=1.
        links.new(tex.outputs['Color'],sep.inputs[0]);links.new(sep.outputs[0],comb.inputs[0]);links.new(sep.outputs[1],inv.inputs[1]);links.new(inv.outputs[0],comb.inputs[1]);links.new(sep.outputs[2],comb.inputs[2])
        nm=nodes.new('ShaderNodeNormalMap');nm.inputs['Strength'].default_value=.65;links.new(comb.outputs[0],nm.inputs['Color']);links.new(nm.outputs[0],p.inputs['Normal'])
    if micro:
        uv=nodes.new('ShaderNodeTexCoord');scale=nodes.new('ShaderNodeVectorMath');scale.operation='SCALE';scale.inputs[3].default_value=100.
        tex=nodes.new('ShaderNodeTexImage');tex.image=bpy.data.images.load(str(T/'micro_poplin_height.png'),check_existing=True);tex.image.colorspace_settings.name='Non-Color';links.new(uv.outputs['UV'],scale.inputs[0]);links.new(scale.outputs[0],tex.inputs['Vector'])
        bump=nodes.new('ShaderNodeBump');bump.inputs['Strength'].default_value=.16;bump.inputs['Distance'].default_value=.00008;links.new(tex.outputs['Color'],bump.inputs['Height'])
        if normal:links.new(nm.outputs[0],bump.inputs['Normal'])
        links.new(bump.outputs[0],p.inputs['Normal'])
    return m
normal_file='techwear_bodyShapeH_Jacket_Normal.png' if any('bodyshapeh' in n for n in names) else 'techwear_bodyShapeG_Normal.png'
fleece=material('Beka black technical fleece',(.007,.010,.008),.78)
detailfleece=material('Beka hood outer fleece',(.007,.010,.008),.74)
olive=material('Beka dark olive trousers',(.030,.032,.021),.79)
leather=material('Beka black high tops',(.011,.013,.011),.51,'techwear_Shoes_Normal.png',False)
rubber=material('Beka warm ivory rubber soles',(.58,.53,.43),.84,None,False)
trim=material('Beka graphite V trim',(.034,.039,.037),.59,None,False)
liner=material('Beka hood lining',(.009,.012,.010),.90)
zipmat=material('Beka blackened zip hardware',(.037,.043,.039),.34,None,False,.65)
patchmat=material('Beka sleeve pocket fabric',(.008,.011,.009),.76)
logo=material('Beka small graphite chest mark',(.046,.051,.046),.60,None,False)
garment.data.materials.clear()
for m in [fleece,olive,leather,rubber]:garment.data.materials.append(m)
shoes_zmin=min(worldcoords[i].z for p,mi in zip(garment.data.polygons,original_material_indices) if mi in shoe_ids for i in p.vertices)
for p,old in zip(garment.data.polygons,original_material_indices):
    if old in jacket_ids:p.material_index=0
    elif old in pants_ids:p.material_index=1
    else:p.material_index=3 if sum(worldcoords[i].z for i in p.vertices)/len(p.vertices)<shoes_zmin+.034 else 2
details=[]
def mesh(name,verts,faces,mat,uvgrid=None):
    me=bpy.data.meshes.new(name);me.from_pydata([tuple(v) for v in verts],[],faces);me.update();ob=bpy.data.objects.new(name,me);bpy.context.collection.objects.link(ob);me.materials.append(mat)
    for p in me.polygons:p.use_smooth=True
    uv=me.uv_layers.new(name=garment.data.uv_layers[0].name)
    for p in me.polygons:
        for li in p.loop_indices:
            vi=me.loops[li].vertex_index;co=me.vertices[vi].co;uv.data[li].uv=uvgrid[vi] if uvgrid else (co.x*5,co.z*5)
    details.append(ob);return ob
def polycurve(name,points,radius,mat):
    cu=bpy.data.curves.new(name,'CURVE');cu.dimensions='3D';cu.bevel_depth=radius;cu.bevel_resolution=2;cu.resolution_u=3
    sp=cu.splines.new('POLY');sp.points.add(len(points)-1)
    for p,co in zip(sp.points,points):p.co=(*co,1)
    ob=bpy.data.objects.new(name,cu);bpy.context.collection.objects.link(ob);cu.materials.append(mat);details.append(ob);return ob
# The photographed jacket has a raised soft collar under the chin, unlike the source polo.
vs=[];uvs=[];N=96;M=10
for j in range(M):
    v=j/(M-1);z=base_top-.075+(top-base_top+.075)*v
    for k in range(N):
        t=2*math.pi*k/N;rx=.088-.008*v;ry=.079-.007*v;fold=.0007*math.sin(7*t+v*2)*math.sin(v*math.pi)
        vs.append(((rx+fold)*math.sin(t),-.010-(ry+fold)*math.cos(t),z));uvs.append((k/N,v))
collar=mesh('Raised soft full zip collar',vs,[(j*N+k,j*N+(k+1)%N,(j+1)*N+(k+1)%N,(j+1)*N+k) for j in range(M-1) for k in range(N)],detailfleece,uvs)
solid=collar.modifiers.new('Collar cloth thickness','SOLIDIFY');solid.thickness=.0017
polycurve('Soft collar top binding',[Vector(vs[(M-1)*N+k]) for k in list(range(N))+[0]],.0012,detailfleece)
# A surface-following, muted V tape. It is not a floating planar decal.
for side in [-1,1]:
    vs=[];uvs=[];N=72
    for i in range(N):
        t=i/(N-1);x=side*.205*t;z=hem+height*.55+.102*t+.005*math.sin(t*math.pi)
        for dz in [-.0026,.0026]:vs.append(front(x,z+dz,.002));uvs.append((t,0. if dz<0 else 1.))
    mesh(('Left' if side>0 else 'Right')+' graphite chest chevron',vs,[(i*2,i*2+1,i*2+3,i*2+2) for i in range(N-1)],trim,uvs)
# Full front closure, with separate tape, coil teeth, slider and pull loop.
zs=[hem+.008+(top-hem-.02)*i/160 for i in range(161)]
vs=[front(x,z,.0027) for z in zs for x in [-.0032,.0032]]
mesh('Full length centre zipper tape',vs,[(i*2,i*2+1,i*2+3,i*2+2) for i in range(len(zs)-1)],patchmat)
vs=[];fs=[]
for z in [hem+.012+i*.0032 for i in range(int((top-hem-.026)/.0032))]:
    for side in [-1,1]:
        p=front(side*.0015,z,.0034);base=len(vs)
        for dz,dx,dy in [(-.0008,-.0007,0),(-.0008,.0007,0),(.0008,.0007,0),(.0008,-.0007,0),(-.0008,-.0007,-.0007),(-.0008,.0007,-.0007),(.0008,.0007,-.0007),(.0008,-.0007,-.0007)]:vs.append(p+Vector((dx,dy,dz)))
        fs.extend(tuple(base+j for j in f) for f in [(0,1,2,3),(4,7,6,5),(0,4,5,1),(1,5,6,2),(2,6,7,3),(3,7,4,0)])
mesh('Fine zipper teeth',vs,fs,zipmat)
pull=front(0,top-.032,.007);polycurve('Zipper pull loop',[pull+Vector((.0032*math.cos(a),-.001,.007*math.sin(a))) for a in [i*2*math.pi/40 for i in range(41)]],.00065,zipmat)
# Folded, open hood behind the collar: lined shell, doubled cloth and an actual rolled rim.
vs=[];uvs=[];N=64;M=18
for j in range(M):
    v=j/(M-1)
    for k in range(N):
        t=math.pi*k/(N-1);rx=.085+.030*math.sin(v*math.pi);ry=.075+.063*math.sin(v*math.pi)
        vs.append((rx*math.cos(t),.012+ry*math.sin(t),top-.080+.085*v+.030*math.sin(t)-.016*math.sin(v*math.pi)+.0018*math.sin(t*9+v*5)))
        uvs.append((k/(N-1),v))
hood=mesh('Folded hood with soft lining',vs,[(j*N+k,j*N+k+1,(j+1)*N+k+1,(j+1)*N+k) for j in range(M-1) for k in range(N-1)],liner,uvs)
sub=hood.modifiers.new('Smooth cloth folds','SUBSURF');sub.levels=1;sub.render_levels=1;sol=hood.modifiers.new('Double cloth thickness','SOLIDIFY');sol.thickness=.0025
polycurve('Bound hood rim',[Vector(vs[(M-1)*N+k]) for k in range(N)],.0028,detailfleece)
# One left upper-arm pocket, aligned with the arm rather than facing the camera as a box.
shoulder=rig.matrix_world@rig.data.bones['upperarm_l'].head_local;elbow=rig.matrix_world@rig.data.bones['lowerarm_l'].head_local
axis=(elbow-shoulder).normalized();cross=Vector((-axis.z,0,axis.x)).normalized();center=shoulder.lerp(elbow,.47)
vs=[];uvs=[];NU=13;NV=10
for j in range(NV):
    v=j/(NV-1)
    for i in range(NU):
        q=i/(NU-1);p=center+axis*((q-.5)*.092)+cross*((v-.5)*.058);p=front(p.x,p.z,.003+.002*math.sin(q*math.pi)*math.sin(v*math.pi));vs.append(p);uvs.append((q,v))
mesh('Left sleeve sewn pocket',vs,[(j*NU+i,j*NU+i+1,(j+1)*NU+i+1,(j+1)*NU+i) for j in range(NV-1) for i in range(NU-1)],patchmat,uvs)
polycurve('Left sleeve pocket seam',[vs[i] for i in range(NU)]+[vs[j*NU+NU-1] for j in range(1,NV)]+[vs[(NV-1)*NU+i] for i in range(NU-2,-1,-1)]+[vs[j*NU] for j in range(NV-2,-1,-1)],.00065,trim)
polycurve('Left sleeve pocket zip',[vs[2*NU+i]+Vector((0,-.001,0)) for i in range(1,NU-1)],.0010,zipmat)
# Small chest mark in the same restrained position as the photo.
cx=.078;cz=top-.135
shape=[(-.014,-.001),(-.010,-.005),(-.005,-.006),(.001,-.004),(.021,.009),(.006,.003),(-.003,0),(-.007,.001),(-.009,.004),(-.010,.001)]
points=[front(cx+x,cz+z,.0025) for x,z in shape]
mesh('Small graphite chest swoosh',points,[tuple(range(len(points)))],logo)
# Continuous vulcanized rubber sidewalls and real laces replace the source's
# exposed jagged tactical soles. Each detail receives the native foot weights.
shoe_faces=[tuple(p.vertices) for p,mi in zip(garment.data.polygons,original_material_indices) if mi in shoe_ids]
shoe_bvh=BVHTree.FromPolygons(worldcoords,shoe_faces,all_triangles=False)
def hull2(points):
    ps=sorted(set((round(p.x,6),round(p.y,6)) for p in points))
    def cross(o,a,b):return (a[0]-o[0])*(b[1]-o[1])-(a[1]-o[1])*(b[0]-o[0])
    low=[];up=[]
    for p in ps:
        while len(low)>=2 and cross(low[-2],low[-1],p)<=0:low.pop()
        low.append(p)
    for p in reversed(ps):
        while len(up)>=2 and cross(up[-2],up[-1],p)<=0:up.pop()
        up.append(p)
    return low[:-1]+up[:-1]
for side in [-1,1]:
    pts=[worldcoords[i] for f in shoe_faces for i in f if side*worldcoords[i].x>0]
    floor_z=min(p.z for p in pts);outline=hull2([p for p in pts if p.z<floor_z+.045])
    for _ in range(2):
        outline=[q for a,b in zip(outline,outline[1:]+outline[:1]) for q in [(.75*a[0]+.25*b[0],.75*a[1]+.25*b[1]),(.25*a[0]+.75*b[0],.25*a[1]+.75*b[1])]]
    cx=sum(p[0] for p in outline)/len(outline);cy=sum(p[1] for p in outline)/len(outline);N=len(outline);vs=[]
    for z,scale in [(floor_z+.002,.975),(floor_z+.006,1.015),(floor_z+.038,1.015),(floor_z+.044,.985)]:
        for x,y in outline:vs.append((cx+(x-cx)*scale,cy+(y-cy)*scale,z))
    mesh(('Left' if side>0 else 'Right')+' cream rubber cupsole',vs,[(j*N+k,j*N+(k+1)%N,(j+1)*N+(k+1)%N,(j+1)*N+k) for j in range(3) for k in range(N)],rubber)
    polycurve('Fine cupsole rim',[(cx+(x-cx)*1.016,cy+(y-cy)*1.016,floor_z+.036) for x,y in outline+[outline[0]]],.0007,rubber)
    # The foot points toward negative Y in the imported MetaHuman bind pose.
    for j in range(7):
        y=cy-.055+j*.014;lace=[]
        for t in range(17):
            x=cx+(-.022+.044*t/16);yy=y+.010*(t/16 if j%2==0 else 1-t/16)
            hit,_,_,_=shoe_bvh.ray_cast(Vector((x,yy,floor_z+.45)),Vector((0,0,-1)),.5)
            if hit:lace.append(hit+Vector((0,0,.0015)))
        if len(lace)>1:polycurve('Black woven crossing lace',lace,.0011,detailfleece)
# Normalize additions to the exact garment's existing skin weights and export one compatible mesh.
for ob in list(details):
    bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);bpy.context.view_layer.objects.active=ob
    if ob.type=='CURVE':bpy.ops.object.convert(target='MESH')
    if ob.data.uv_layers:ob.data.uv_layers[0].name=garment.data.uv_layers[0].name
    for mod in list(ob.modifiers):
        if mod.type in ['SUBSURF','SOLIDIFY']:bpy.ops.object.modifier_apply(modifier=mod.name)
    for vg in garment.vertex_groups:
        if vg.name not in ob.vertex_groups:ob.vertex_groups.new(name=vg.name)
    tr=ob.modifiers.new('Native garment weight transfer','DATA_TRANSFER');tr.object=garment;tr.use_vert_data=True;tr.data_types_verts={'VGROUP_WEIGHTS'};tr.vert_mapping='POLYINTERP_NEAREST';tr.layers_vgroup_select_src='ALL';tr.layers_vgroup_select_dst='NAME'
    bpy.ops.object.modifier_apply(modifier=tr.name)
    ar=ob.modifiers.new('Character body rig','ARMATURE');ar.object=rig
bpy.ops.object.select_all(action='DESELECT');garment.select_set(True)
for ob in details:ob.select_set(True)
bpy.context.view_layer.objects.active=garment;bpy.ops.object.join();garment.name='Beka photo black full zip outfit'
for image in bpy.data.images:
    if image.source=='FILE':image.pack()
rig.name='root';bpy.ops.wm.save_as_mainfile(filepath=str(O/'Beka_photo_tailored_v23.blend'))
bpy.ops.object.select_all(action='DESELECT');garment.select_set(True);rig.select_set(True);bpy.context.view_layer.objects.active=garment
bpy.ops.export_scene.fbx(filepath=str(O/'Beka_photo_tailored_v23.fbx'),use_selection=True,object_types={'MESH','ARMATURE'},add_leaf_bones=False,bake_anim=False,use_mesh_modifiers=True,mesh_smooth_type='FACE',path_mode='COPY',embed_textures=False)
report={'source':str(inputs[0]),'weighted':True,'native_groups':len(garment.vertex_groups),'vertices':len(garment.data.vertices),'triangles':sum(len(p.vertices)-2 for p in garment.data.polygons),'target_body_rebind_max_m':max(displacements),'cargo_islands_removed':removed_islands,'details_added':['open lined hood','raised soft collar','full zipper tape and teeth','zipper pull','surface-fitted graphite V tapes','single left upper-arm pocket','small chest mark','ivory soles'],'likeness_approved':False,'game_installed':False,'needs_visual_review':True}
(O/'tailoring_report.json').write_text(json.dumps(report,indent=2),encoding='utf8');print('PHOTO_OUTFIT_TAILORED',json.dumps(report))
