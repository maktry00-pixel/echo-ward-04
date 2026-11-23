import bpy,json,math,sys
from pathlib import Path
from mathutils import Vector,Matrix
R=Path('D:/Projects/ECHO');EXT=R/'assets/external/hospital_v34';OUT=R/'assets/models/hospital_v34';OUT.mkdir(parents=True,exist_ok=True);REPORT=R/'artifacts/echo_hospital_sources_v34';REPORT.mkdir(parents=True,exist_ok=True)
rows=json.loads((EXT/'SOURCE_MANIFEST.json').read_text())['assets'];done=[]
for row in rows:
 if Path(row['file']).suffix.lower()!='.glb':continue
 bpy.ops.wm.read_factory_settings(use_empty=True);bpy.context.preferences.filepaths.use_scripts_auto_execute=False;bpy.ops.import_scene.gltf(filepath=row['file']);scene=bpy.context.scene;scene.frame_set(0)
 meshes=[o for o in scene.objects if o.type=='MESH'];pts=[o.matrix_world@Vector(v) for o in meshes for v in o.bound_box];lo=Vector([min(p[i] for p in pts) for i in range(3)]);hi=Vector([max(p[i] for p in pts) for i in range(3)]);shift=Vector((-(lo.x+hi.x)/2,-(lo.y+hi.y)/2,-lo.z));roots=[o for o in scene.objects if not o.parent]
 for o in roots:o.matrix_world=Matrix.Translation(shift)@o.matrix_world
 folder=OUT/row['slug'];folder.mkdir(exist_ok=True);bpy.ops.object.select_all(action='SELECT');bpy.ops.export_scene.gltf(filepath=str(folder/'model.glb'),export_format='GLB',use_selection=True,export_animations=False)
 scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1;bpy.ops.file.pack_all();bpy.ops.wm.save_as_mainfile(filepath=str(folder/'model.blend'))
 mats={m for o in meshes for m in o.data.materials if m};textures={n.image.name:dict(width=n.image.size[0],height=n.image.size[1]) for m in mats if m.use_nodes for n in m.node_tree.nodes if n.type=='TEX_IMAGE' and n.image};tri=sum(sum(len(p.vertices)-2 for p in o.data.polygons) for o in meshes)
 record=dict(id=row['id'],slug=row['slug'],title=row['title'],source=row['source'],license=row['license'],source_sha256=row['sha256'],size_m=list(hi-lo),parts=len(meshes),triangles=tri,materials=len(mats),textures=textures,model=str(folder/'model.glb'),blend=str(folder/'model.blend'),preview=str(folder/'preview.png'),status='source_normalized_not_yet_integrated')
 (folder/'asset.json').write_text(json.dumps(record,ensure_ascii=False,indent=2));done.append(record)
 floor=bpy.data.materials.new('Preview neutral floor');floor.diffuse_color=(.11,.12,.13,1);bpy.ops.mesh.primitive_plane_add(size=200);plane=bpy.context.object;plane.data.materials.append(floor)
 for loc,power,size in [((3,-4,5),1100,4),((-3,-1,3),650,3),((0,4,5),1000,3)]:
  bpy.ops.object.light_add(type='AREA',location=loc);light=bpy.context.object;light.data.energy=power;light.data.shape='DISK';light.data.size=size;light.rotation_euler=(Vector((0,0,1))-light.location).to_track_quat('-Z','Y').to_euler()
 size=hi-lo;center=Vector((0,0,size.z*.5));distance=max(size)*1.8;bpy.ops.object.camera_add(location=center+Vector((distance,-distance,distance*.72)));camera=bpy.context.object;camera.rotation_euler=(center-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.type='ORTHO';camera.data.ortho_scale=max(size)*1.7;scene.camera=camera;scene.world=scene.world or bpy.data.worlds.new('Preview world');scene.world.color=(.15,.15,.15);scene.render.engine='CYCLES';scene.cycles.samples=16;scene.cycles.use_denoising=True;scene.cycles.device='GPU'
 prefs=bpy.context.preferences.addons['cycles'].preferences
 try:
  prefs.compute_device_type='OPTIX';prefs.get_devices()
  for device in prefs.devices:device.use=device.type!='CPU'
 except:scene.cycles.device='CPU'
 scene.render.resolution_x=640;scene.render.resolution_y=640;scene.render.resolution_percentage=100;scene.render.image_settings.file_format='PNG';scene.render.filepath=str(folder/'preview.png');bpy.ops.render.render(write_still=True);print('SOURCE_PREPARED',row['id'],row['slug'],'tris',tri,flush=True)
(REPORT/'prepared_glb.json').write_text(json.dumps(dict(success=True,models=done),ensure_ascii=False,indent=2));print('PREPARED_COMPLETE',len(done),flush=True)