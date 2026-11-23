import bpy,bmesh,json,math,struct,hashlib
from pathlib import Path
from mathutils import Vector
R=Path('D:/Projects/ECHO');O=R/'artifacts/echo_hero_v38/character_candidate_v15';path=O/'Beka_character_candidate_v15.glb'
bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.gltf(filepath=str(path))
obs=[o for o in bpy.context.scene.objects if o.type=='MESH'];points=[o.matrix_world@v.co for o in obs for v in o.data.vertices]
lo=[min(p[i] for p in points) for i in range(3)];hi=[max(p[i] for p in points) for i in range(3)]
images=[{'name':im.name,'size':list(im.size),'packed':bool(im.packed_file)} for im in bpy.data.images if im.type=='IMAGE']
missing=[im.name for im in bpy.data.images if not im.packed_file and im.filepath and not Path(bpy.path.abspath(im.filepath)).exists()]
pants=next(o for o in obs if o.name=='Beka_Trousers');bm=bmesh.new();bm.from_mesh(pants.data);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.000001);boundary=sum(e.is_boundary for e in bm.edges);nonmanifold=sum(not e.is_manifold for e in bm.edges);bm.free()
uv=pants.data.uv_layers.active;zero=0
for p in pants.data.polygons:
 a,b,c=[uv.data[i].uv.copy() for i in list(p.loop_indices)[:3]]
 if abs((b-a).x*(c-a).y-(b-a).y*(c-a).x)<1e-12:zero+=1
buf=path.read_bytes();ln=struct.unpack_from('<I',buf,12)[0];gl=json.loads(buf[20:20+ln]);pm=next(m for m in gl['materials'] if m.get('name')=='Olive woven cloth v15')
report={'import_glb_passed':True,'geometry_finite':all(math.isfinite(v) for p in points for v in p),'bounds_m':[lo,hi],'height_m':hi[2]-lo[2],'height_175cm_passed':abs(hi[2]-lo[2]-1.75)<.002,'mesh_objects':len(obs),'triangles':sum(len(p.vertices)-2 for o in obs for p in o.data.polygons),'vertices':sum(len(o.data.vertices) for o in obs),'missing_images':missing,'images':images,'pants_boundary_edges_after_weld':boundary,'pants_nonmanifold_edges_after_weld':nonmanifold,'pants_zero_uv_triangle_ratio':zero/len(pants.data.polygons),'pants_exported_normal_texture':bool(pm.get('normalTexture')),'pants_exported_albedo_texture':bool(pm.get('pbrMetallicRoughness',{}).get('baseColorTexture')),'animations':len(gl.get('animations',[])),'rigged':any(o.type=='ARMATURE' for o in bpy.context.scene.objects),'likeness_approved':False,'game_ready':False,'game_changed':False,'sha256':hashlib.sha256(buf).hexdigest()}
(O/'roundtrip_validation.json').write_text(json.dumps(report,indent=2),encoding='utf8')
assert report['geometry_finite'] and report['height_175cm_passed'] and not missing
assert not boundary and not nonmanifold and report['pants_zero_uv_triangle_ratio']<.01
assert report['pants_exported_normal_texture'] and report['pants_exported_albedo_texture']
world=bpy.data.worlds.new('Roundtrip studio');world.use_nodes=True;world.node_tree.nodes['Background'].inputs[0].default_value=(.13,.14,.15,1);world.node_tree.nodes['Background'].inputs[1].default_value=.4;bpy.context.scene.world=world
for pos,power,size in [((-1,-1.5,2.4),70,1.3),((1,-.9,1.9),45,1),((0,1,2.3),70,1)]:
 bpy.ops.object.light_add(type='AREA',location=pos);ob=bpy.context.object;ob.data.energy=power;ob.data.size=size;ob.rotation_euler=(Vector((0,0,1.62))-ob.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(0,-3,1.0));cam=bpy.context.object;cam.rotation_euler=(Vector((0,0,.88))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=2.02
sc=bpy.context.scene;sc.camera=cam;sc.render.engine='CYCLES';sc.cycles.samples=16;sc.cycles.use_denoising=True;sc.render.resolution_x=900;sc.render.resolution_y=1200;sc.render.resolution_percentage=100;sc.render.image_settings.file_format='PNG';sc.render.filepath=str(O/'glb_roundtrip.png');bpy.ops.render.render(write_still=True)
print('ROUNDTRIP_TECHNICAL_PASS_NOT_LIKENESS_APPROVAL',flush=True)
