"""Actual 3D scene assembled from exported kit; no generated background image."""
import sys,math,bpy
from pathlib import Path
from mathutils import Vector,Matrix
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
import build_echo_modular_assets as kit
OUT=ROOT/'assets/models/echo_custom';SHOW=OUT/'showcase';SHOW.mkdir(exist_ok=True)
kit.setup_scene();kit.current=0;kit.parts=[]

def instance(i,loc,rot=0):
    before=set(bpy.data.objects);bpy.ops.import_scene.gltf(filepath=str(OUT/f'EH_{i:03d}'/f'EH_{i:03d}.glb'))
    new=set(bpy.data.objects)-before
    root=bpy.data.objects.new('Kit_'+str(i),None);bpy.context.collection.objects.link(root)
    # Importer converts Y-up GLB back into Blender Z-up.
    for ob in new:
        if ob.parent not in new:ob.parent=root
    root.location=loc;root.rotation_euler.z=rot
    return root

# 4 m wide corridor, 12 m long. Doors are placed inside real wall apertures.
for y in [1.5,4.5,7.5,10.5]:
    for side in [-1,1]:
        rot=math.pi/2 if side==-1 else -math.pi/2
        aperture=(y in [4.5,10.5])
        instance(9 if aperture else 1,(side*2.11,y,0),rot)
        if aperture:instance(63 if y==4.5 else 61,(side*2.11,y,.01),rot)
        instance(20,(side*2.11,y,0),rot)
instance(10,(0,12,0));instance(65,(0,12,.01))
for x in [-1.76,1.76]:kit.wall_section(x,.52,0,3.2,y=12)
floor=kit.box('corridor_floor',(0,6,-.06),(4.05,12.3,.12),'034',.002)
wet=material=kit.material('034').copy();wet.name='Showcase_wet_hospital_floor';floor.data.materials.clear();floor.data.materials.append(wet)
bs=wet.node_tree.nodes.get('Principled BSDF')
base=next(link.from_socket for link in wet.node_tree.links if link.to_socket==bs.inputs['Base Color'])
mix=wet.node_tree.nodes.new('ShaderNodeMixRGB');mix.blend_type='MULTIPLY';mix.inputs[0].default_value=1;mix.inputs[2].default_value=(.47,.54,.51,1);wet.node_tree.links.new(base,mix.inputs[1]);wet.node_tree.links.new(mix.outputs['Color'],bs.inputs['Base Color'])
for link in list(wet.node_tree.links):
    if link.to_socket==bs.inputs['Roughness']:wet.node_tree.links.remove(link)
bs.inputs['Roughness'].default_value=.23;bs.inputs['Coat Weight'].default_value=.55
for x in [-1.8,-1.2,-.6,0,.6,1.2,1.8]:
    for j in range(21):
        y=j*.6
        if (j+int(x*10))%29==0:continue
        # economical smooth panel preview; asset EH_051 remains the detailed source.
        kit.box('ceiling_panel',(x,y,3.2),(.59,.59,.025),'ceiling_fibre',.002)
for y in [1.8,5.8,9.8]:
    if (OUT/'EH_121/EH_121.glb').exists():instance(121,(0,y,3.13))
    else:
        kit.box('luminaire',(0,y,3.10),(1.25,.28,.06),'028',.01)
        kit.box('diffuser',(0,y,3.06),(1.15,.20,.015),kit.emission('phosphor',(.65,.85,1),2),.006)
    if y!=5.8:
        bpy.ops.object.light_add(type='AREA',location=(0,y,3.04));ob=bpy.context.object;ob.data.energy=65;ob.data.color=(.40,.75,.88);ob.data.shape='RECTANGLE';ob.data.size=1.1;ob.data.size_y=.25
for x,y in [(-1.97,8.6),(1.97,2.7)]:
    kit.box('emergency_fixture',(x,y,2.40),(.10,.22,.18),'031',.025)
    kit.box('emergency_red_lens',(x+(.06 if x<0 else -.06),y,2.40),(.025,.16,.12),kit.emission('red_lens',(1,.012,.004),3),.02)
    bpy.ops.object.light_add(type='POINT',location=(x+(.18 if x<0 else -.18),y,2.4));ob=bpy.context.object;ob.data.energy=36;ob.data.color=(1,.015,.004);ob.data.shadow_soft_size=.12
for y in [2.2,5.3,8.5,10.7]:
    before=len(kit.parts);kit.floor_detail(50)
    for ob in kit.parts[before:]:ob.location=(.3*math.sin(y*2),y,.003);ob.scale=(.8,.8,.8)
for side in [-1,1]:
    for y in [1.5,4.5,7.5,10.5]:
        for z in [2.7,2.9]:kit.cylinder('service_pipe',(side*1.91,y-1.5,z),(side*1.91,y+1.5,z),.026,'031')
        for z in [2.7,2.9]:kit.box('pipe_clip',(side*1.94,y,z),(.12,.03,.07),'032')

before=len(kit.parts);kit.hospital_bed()
kit.softbox('mattress',(0,0,.76),(.86,1.93,.14),'028',.06)
kit.softbox('pillow',(0,.63,.88),(.54,.34,.10),'028',.045)
kit.cloth('bed_sheet',1.22,1.8,.85,'028',.24)
for ob in kit.parts[before:]:ob.location+=Vector((.83,7.4,0));ob.rotation_euler.z+=-.13
before=len(kit.parts);kit.cabinet(.48,.43,.70,drawers=3)
for ob in kit.parts[before:]:
    if ob.parent is None:ob.location+=Vector((-1.58,6.6,0))
for x,y,z,label in [(-1.99,4.5,1.70,'ПАЛАТА 04'),(1.99,10.5,1.70,'ПРОЦЕДУРНАЯ')]:
    # small numbered plates placed alongside doors, facing into corridor
    ob=kit.box('room_plate',(x,y+.73,z),(.015,.35,.12),'026',.008)
kit.box('exit_plate',(0,11.85,2.65),(.64,.04,.20),kit.emission('exit_lens',(.20,.6,.38),.8),.008)
kit.text('ВЫХОД',(0,11.82,2.60),.105,'white')

scene=bpy.context.scene;scene.world.use_nodes=True;scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.015,.025,.03,1);scene.world.node_tree.nodes['Background'].inputs[1].default_value=.04
bpy.ops.object.camera_add(location=(.05,.35,1.58));camera=bpy.context.object;camera.rotation_euler=(Vector((0,9.5,1.3))-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.lens=24;scene.camera=camera
scene.render.resolution_x=1800;scene.render.resolution_y=1100;scene.cycles.samples=96
scene.view_settings.exposure=-.45
fog=bpy.data.materials.new('Showcase_air_haze');fog.use_nodes=True;fn=fog.node_tree.nodes;fn.clear();volume=fn.new('ShaderNodeVolumePrincipled');volume.inputs['Density'].default_value=.008;volume.inputs['Color'].default_value=(.35,.48,.49,1);volume.inputs['Anisotropy'].default_value=.25;output=fn.new('ShaderNodeOutputMaterial');fog.node_tree.links.new(volume.outputs['Volume'],output.inputs['Volume'])
bpy.ops.mesh.primitive_cube_add(size=1,location=(0,6,1.6));air=bpy.context.object;air.name='Showcase_haze_volume';air.dimensions=(4,12,3.2);air.data.materials.append(fog)
# Subtle optical bloom around the actual emissive lamp meshes.
scene.use_nodes=True;nodes=scene.node_tree.nodes;nodes.clear();rl=nodes.new('CompositorNodeRLayers');glow=nodes.new('CompositorNodeGlare');glow.glare_type='FOG_GLOW';glow.threshold=1.4;glow.quality='HIGH';glow.mix=-.85;comp=nodes.new('CompositorNodeComposite');scene.node_tree.links.new(rl.outputs['Image'],glow.inputs['Image']);scene.node_tree.links.new(glow.outputs['Image'],comp.inputs['Image'])
for image in bpy.data.images:
    if image.source=='FILE' and image.filepath:image.filepath=bpy.path.relpath(bpy.path.abspath(image.filepath),start=str(SHOW))
bpy.ops.wm.save_as_mainfile(filepath=str(SHOW/'ECHO_corridor.blend'))
scene.render.filepath=str(SHOW/'ECHO_corridor.png');bpy.ops.render.render(write_still=True)
print('CORRIDOR_COMPLETE',flush=True)
