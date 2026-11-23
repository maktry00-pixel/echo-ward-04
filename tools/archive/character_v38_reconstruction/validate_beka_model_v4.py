import bpy,json,math,struct,hashlib
from pathlib import Path
from mathutils import Vector
root=Path('D:/Projects/ECHO/assets/models/hero_custom/beka_reconstruction_draft_v4')
out=Path('D:/Projects/ECHO/artifacts/echo_hero_v38/manual_assembly_v4')
model=root/'Beka_reconstruction_draft_v4.glb'
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=str(model))
meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
coords=[o.matrix_world@v.co for o in meshes for v in o.data.vertices]
assert coords,'No mesh geometry'
assert all(math.isfinite(c) for p in coords for c in p),'NaN or infinite coordinates'
bounds=[[min(v[i] for v in coords) for i in range(3)],[max(v[i] for v in coords) for i in range(3)]]
height=bounds[1][2]-bounds[0][2]
assert abs(height-1.75)<.002, str(height)
for im in bpy.data.images:
 if im.source=='FILE':
  # Imported embedded images are lazy-loaded; force pixel decode before checking.
  _=im.pixels[0] if len(im.pixels) else None
missing_images=[im.name for im in bpy.data.images if im.source=='FILE' and (im.size[0]==0 or len(im.pixels)==0)]
assert not missing_images,missing_images
report={'import_glb_passed':True,'geometry_finite':True,'height_m':height,'height_tolerance_m':.002,'bounds_m':bounds,'mesh_objects':len(meshes),'vertices':sum(len(o.data.vertices) for o in meshes),'triangles':sum(sum(len(p.vertices)-2 for p in o.data.polygons) for o in meshes),'materials':len(bpy.data.materials),'missing_images':missing_images,'images':[{'name':im.name,'size':list(im.size),'packed':bool(im.packed_file)} for im in bpy.data.images if im.has_data],'animations':len(bpy.data.actions),'rigged':any(o.type=='ARMATURE' for o in bpy.data.objects),'game_ready':False,'likeness_approved':False}
with model.open('rb') as f:
 magic,version,length=struct.unpack('<4sII',f.read(12));assert magic==b'glTF' and version==2 and length==model.stat().st_size
 n,t=struct.unpack('<II',f.read(8));gltf=json.loads(f.read(n))
 report['gltf_external_buffers']=[b.get('uri') for b in gltf.get('buffers',[]) if b.get('uri')]
 assert not report['gltf_external_buffers'],'External buffers unexpectedly required'
 report['gltf_external_images']=[i.get('uri') for i in gltf.get('images',[]) if i.get('uri')]
 assert not report['gltf_external_images'],'External images unexpectedly required'
report['glb_sha256']=hashlib.sha256(model.read_bytes()).hexdigest()
(out/'roundtrip_validation.json').write_text(json.dumps(report,indent=2),encoding='utf8')
world=bpy.data.worlds.new('Round trip neutral');world.use_nodes=True;world.node_tree.nodes['Background'].inputs[0].default_value=(.15,.17,.19,1);world.node_tree.nodes['Background'].inputs[1].default_value=.4;bpy.context.scene.world=world
for pos,power in [((-2,-3,3),220),((2,-2,2.4),150),((0,2,2.5),240)]:
 bpy.ops.object.light_add(type='AREA',location=pos);o=bpy.context.object;o.data.energy=power;o.data.size=3;o.rotation_euler=(Vector((0,0,1))-o.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(2.5,-3.3,1));cam=bpy.context.object;cam.rotation_euler=(Vector((0,0,.95))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=2.05
sc=bpy.context.scene;sc.camera=cam;sc.render.engine='CYCLES';sc.cycles.samples=12;sc.cycles.use_denoising=True;sc.render.resolution_x=810;sc.render.resolution_y=1080;sc.render.resolution_percentage=100;sc.render.image_settings.file_format='PNG';sc.render.filepath=str(out/'glb_roundtrip.png');bpy.ops.render.render(write_still=True)
# Read FBX independently as a second format check.
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(root/'Beka_reconstruction_draft_v4.fbx'),use_anim=False)
objects=[o for o in bpy.context.scene.objects if o.type=='MESH'];assert objects
pts=[o.matrix_world@v.co for o in objects for v in o.data.vertices]
report['fbx_import_passed']=True;report['fbx_mesh_objects']=len(objects);report['fbx_height_m']=max(v.z for v in pts)-min(v.z for v in pts)
assert abs(report['fbx_height_m']-1.75)<.002,report['fbx_height_m']
report['fbx_missing_images']=[im.name for im in bpy.data.images if not im.has_data and im.source=='FILE']
(out/'roundtrip_validation.json').write_text(json.dumps(report,indent=2),encoding='utf8')
print('ECHO_MODEL_VALIDATION '+json.dumps(report))

