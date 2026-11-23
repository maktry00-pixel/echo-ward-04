"""Reversible horror lighting/material study using the prepared hospital library."""
import bpy,math,json,random
from pathlib import Path
from mathutils import Vector,Matrix
R=Path('D:/Projects/ECHO');P=R/'assets/models/hospital_v34';O=R/'artifacts/echo_hospital_horror_study_v34';O.mkdir(parents=True,exist_ok=True);random.seed(3450)
bpy.ops.wm.read_factory_settings(use_empty=True);bpy.context.preferences.filepaths.use_scripts_auto_execute=False;bpy.context.preferences.filepaths.save_version=0;scene=bpy.context.scene;scene.frame_start=0;scene.frame_end=240;used=[]
def material(name,color,metal=0,rough=.7,emission=None):
 m=bpy.data.materials.new(name);m.use_nodes=True;b=m.node_tree.nodes.get('Principled BSDF');b.inputs['Base Color'].default_value=(*color,1);b.inputs['Metallic'].default_value=metal;b.inputs['Roughness'].default_value=rough
 if emission:b.inputs['Emission Color'].default_value=(*color,1);b.inputs['Emission Strength'].default_value=emission
 return m
def cube(name,at,size,mat,bevel=0):
 bpy.ops.mesh.primitive_cube_add(size=1,location=at);ob=bpy.context.object;ob.name=name;ob.dimensions=size;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);ob.data.materials.append(mat)
 if bevel:
  mod=ob.modifiers.new('Rounded manufactured edge','BEVEL');mod.width=bevel;mod.segments=3
 return ob
def tube(name,a,b,r,mat):
 a,b=Vector(a),Vector(b);bpy.ops.mesh.primitive_cylinder_add(vertices=24,radius=r,depth=(b-a).length,location=(a+b)/2);o=bpy.context.object;o.name=name;o.rotation_euler=(b-a).to_track_quat('Z','Y').to_euler();o.data.materials.append(mat);return o
def curve(name,pts,r,mat):
 c=bpy.data.curves.new(name,'CURVE');c.dimensions='3D';c.bevel_depth=r;c.bevel_resolution=3;s=c.splines.new('BEZIER');s.bezier_points.add(len(pts)-1)
 for p,co in zip(s.bezier_points,pts):p.co=co;p.handle_left_type=p.handle_right_type='AUTO'
 ob=bpy.data.objects.new(name,c);scene.collection.objects.link(ob);ob.data.materials.append(mat);return ob
def append(slug,at=(0,0,0),angle=0,door_open=0):
 path=P/slug/'model.blend'
 with bpy.data.libraries.load(str(path),link=False) as (src,dst):dst.objects=src.objects
 objects=[o for o in dst.objects if o];group=bpy.data.objects.new('Study_'+slug,None);scene.collection.objects.link(group)
 for ob in objects:
  scene.collection.objects.link(ob)
  if ob.animation_data:ob.animation_data_clear()
 roots=[o for o in objects if not o.parent]
 for ob in roots:ob.parent=group;ob.matrix_parent_inverse=Matrix.Identity(4)
 group.location=at;group.rotation_euler.z=math.radians(angle)
 for ob in objects:
  if ob.name.startswith('Door_Hinge_Openable'):ob.rotation_euler.z=math.radians(door_open)
 used.append(dict(slug=slug,at=at,yaw=angle));return group,objects
# Six 3m bays, 3.4m clear corridor; real side apertures retained.
for i in range(6):
 y=1.5+i*3
 for side in [-1,1]:
  door=i in ([1,4] if side<0 else [2,4]);append('wall_door_opening' if door else 'wall_plain',(side*1.8,y,0),90 if side<0 else -90)
  if door:append('hospital_door',(side*1.8,y,0),90 if side<0 else -90,door_open=(-48 if (i,side)==(2,1) else -12 if (i,side)==(1,-1) else 0))
 append('floor_tiles',(0,y,-.055));append('ceiling_damaged',(0,y,3.65))
append('wall_door_opening',(0,18,0));append('hospital_door',(0,18,0),door_open=-14)
paint=material('Study neutral faded paint',(.075,.13,.15),rough=.87);metal=material('Study worn steel',(.09,.11,.12),metal=.72,rough=.53);ceramic=material('Study cut ceramic',(.24,.29,.29),rough=.21);black=material('Cable rubber',(.006,.009,.012),rough=.85);blood=material('Old dark burgundy traces',(.025,.0015,.002),rough=.38);paper=material('Old paper',(.42,.39,.31),rough=.93)
for side in [-1,1]:
 for j in range(60):cube('Cut perimeter tile',(side*1.6,.15+j*.3,-.0075),(.196,.296,.015),ceramic,.0015)
 tube('Overhead service pipe',(side*1.57,0,3.14),(side*1.57,18,3.14),.036,metal)
 for y in [1,4,7,10,13,16]:
  cube('Pipe support',(side*1.65,y,3.14),(.18,.045,.045),metal);cube('Roof service backing',(0,y,4.42),(3.8,3,.08),black)
# Small side rooms prevent black voids beyond the doors.
for side,y in [(-1,4.5),(1,7.5),(-1,13.5),(1,13.5)]:
 cube('Side room floor',(side*3.4,y,-.05),(3.4,3,.1),ceramic)
 cube('Side room back wall',(side*4.9,y,1.9),(.2,3,3.8),paint)
 for yy in [y-1.5,y+1.5]:cube('Side room return',(side*3.4,yy,1.9),(3.2,.18,3.8),paint)
 cube('Side room roof',(side*3.4,y,3.82),(3.4,3,.12),black)
append('manual-wheelchair-e5061094',(-1.02,5.9,0),72)
append('three-shelf-medical-supply-cart-45cc326c',(1.14,9.4,0),-8)
append('patient-transport-stretcher-3ff4d512',(-1.05,12.5,0),84)
append('patient-bedside-cabinet-19c0b99c',(1.12,15.7,0),-90)
append('nurse-station-with-low-accessible-counter-ef78cf3d',(-3.3,4.5,0),90)
append('hospital-bed-raised-backrest-6bb95542',(3.6,7.4,0),90)
# Reversible material treatment, using world-space streaks rather than green mold.
matcache={}
for ob in list(scene.objects):
 if ob.type!='MESH':continue
 for slot in ob.material_slots:
  base=slot.material
  if not base or not base.use_nodes:continue
  if base.name in matcache:slot.material=matcache[base.name];continue
  m=base.copy();m.name=base.name+'_HorrorStudy';n=m.node_tree.nodes;l=m.node_tree.links;bs=n.get('Principled BSDF')
  if not bs:continue
  if bs.inputs['Transmission Weight'].default_value>.3:matcache[base.name]=m;slot.material=m;continue
  geo=n.new('ShaderNodeNewGeometry');scale=n.new('ShaderNodeVectorMath');scale.operation='MULTIPLY';scale.inputs[1].default_value=(5,5,.25);l.new(geo.outputs['Position'],scale.inputs[0]);noise=n.new('ShaderNodeTexNoise');noise.inputs['Scale'].default_value=1;noise.inputs['Detail'].default_value=3;l.new(scale.outputs['Vector'],noise.inputs['Vector']);ramp=n.new('ShaderNodeValToRGB');ramp.color_ramp.elements[0].position=.3;ramp.color_ramp.elements[0].color=(.32,.3,.27,1);ramp.color_ramp.elements[1].position=.67;ramp.color_ramp.elements[1].color=(.84,.84,.82,1);l.new(noise.outputs['Fac'],ramp.inputs[0]);mix=n.new('ShaderNodeMixRGB');mix.blend_type='MULTIPLY';mix.inputs[0].default_value=.58
  old=bs.inputs['Base Color'].links[0].from_socket if bs.inputs['Base Color'].links else None
  if old:l.new(old,mix.inputs[1])
  else:mix.inputs[1].default_value=bs.inputs['Base Color'].default_value
  l.new(ramp.outputs['Color'],mix.inputs[2]);l.new(mix.outputs[0],bs.inputs['Base Color']);matcache[base.name]=m;slot.material=m
  if 'ceramic' in base.name.lower():
   rr=n.new('ShaderNodeValToRGB');rr.color_ramp.elements[0].position=.4;rr.color_ramp.elements[0].color=(.09,.09,.09,1);rr.color_ramp.elements[1].position=.62;rr.color_ramp.elements[1].color=(.4,.4,.4,1);l.new(noise.outputs['Fac'],rr.inputs[0]);l.new(rr.outputs['Color'],bs.inputs['Roughness']);bs.inputs['Coat Weight'].default_value=.2
# Irregular traces, fallen paperwork, cable loops and damaged roof services.
for k in range(24):
 x=random.uniform(-1.35,1.35);y=random.uniform(1,17);radius=random.uniform(.025,.16);verts=[]
 for i in range(32):
  a=i*math.tau/32;r=radius*random.uniform(.78,1.12);verts.append((x+math.cos(a)*r,y+math.sin(a)*r*1.8,.001))
 mesh=bpy.data.meshes.new('Old floor trace');mesh.from_pydata(verts,[],[list(range(32))]);ob=bpy.data.objects.new('Old floor trace',mesh);scene.collection.objects.link(ob);ob.data.materials.append(blood)
for k in range(24):
 ob=cube('Dropped clinical form',(random.uniform(-1.45,1.45),random.uniform(1,17),.003),(.21,.297,.001),paper);ob.rotation_euler.z=random.uniform(-math.pi,math.pi)
for y in [3.6,8.6,14.4]:curve('Loose cable from ceiling',[(-.15,y,3.86),(.1,y+.2,3.3),(.26,y+.6,3.55),(.5,y+1,3.83)],.008,black)
curve('Abandoned floor cable',[(-1.5,3,.015),(-.6,4,.016),(.3,4.8,.015),(1.5,5.2,.015)],.009,black)
# Break repeated modular damage patterns; keep only scattered local paint loss.
for ob in list(scene.objects):
 if ob.name.startswith(('Chipped paint','Skirting edge damage','Door paint scuff')) and random.random()<.84:ob.hide_render=True
# Readable wayfinding and period fixtures.
font=bpy.data.fonts.load('C:/Windows/Fonts/arial.ttf')
def text(name,body,at,size,rotation=(math.pi/2,0,0),mat=paper):
 c=bpy.data.curves.new(name,'FONT');c.body=body;c.font=font;c.size=size;c.align_x='CENTER';c.extrude=.0004;ob=bpy.data.objects.new(name,c);scene.collection.objects.link(ob);ob.location=at;ob.rotation_euler=rotation;ob.data.materials.append(mat);return ob
cube('Archive sign backing',(0,17.84,2.91),(1.6,.04,.38),black,.01);text('Archive destination','АРХИВ',(0,17.81,2.82),.23)
for side,y,label in [(-1,4.5,'04'),(1,7.5,'05')]:
 # Text faces into the hall, above the doorway.
 text('Ward number '+label,label,(side*1.685,y,2.8),.23,(math.pi/2,0,math.pi/2 if side<0 else -math.pi/2))
white=material('Flickering fluorescent tubes',(.68,.85,1),emission=1.7);red=material('Emergency lamp glass',(1,.012,.004),emission=10)
def area(name,at,target,power,color,size=1.2):
 bpy.ops.object.light_add(type='AREA',location=at);ob=bpy.context.object;ob.name=name;ob.data.energy=power;ob.data.color=color;ob.data.shape='RECTANGLE';ob.data.size=size;ob.data.size_y=.35;ob.rotation_euler=(Vector(target)-ob.location).to_track_quat('-Z','Y').to_euler();return ob
for i,y in enumerate([2.3,8.1,14.9]):
 cube('Fluorescent housing',(0,y,3.53),(1.14,.24,.1),metal,.01)
 for yy in [y-.065,y+.065]:tube('Fluorescent tube',(-.49,yy,3.465),(.49,yy,3.465),.017,white)
 light=area('Cold broken ceiling light',(0,y,3.42),(0,y,0),22 if i==0 else 30 if i==1 else 70,(.57,.76,1),1)
 if i==1:
  for f,v in [(0,30),(37,30),(39,2),(42,30),(100,30),(102,0),(108,30),(240,30)]:light.data.energy=v;light.data.keyframe_insert(data_path='energy',frame=f)
for side,y in [(1,7.2),(-1,13.1)]:
 cube('Red emergency fixture',(side*1.65,y,2.65),(.11,.2,.36),metal,.01);cube('Red emergency lens',(side*1.57,y,2.65),(.055,.12,.26),red,.025);bpy.ops.object.light_add(type='POINT',location=(side*1.38,y,2.55));ob=bpy.context.object;ob.name='Pulsing emergency lamp';ob.data.color=(1,.018,.005);ob.data.shadow_soft_size=.12
 for f,v in [(0,38),(60,65),(120,35),(180,61),(240,38)]:ob.data.energy=v;ob.data.keyframe_insert(data_path='energy',frame=f)
area('Cold light from side room',(-3.2,4.4,2.6),(-.4,5.0,1),55,(.5,.7,.85));area('Low red room spill',(3.8,7.6,2.5),(0,7.5,1),33,(1,.03,.012))
cube('Beyond archive doorway',(0,19.5,-.05),(3.4,3,.1),ceramic);cube('Far darkness',(0,21,1.9),(3.8,.15,3.8),black);area('Archive slit of light',(.2,20,2.8),(0,17,1),12,(1,.1,.025),.3)
# A thin floor-level mist leaves the route and equipment readable.
fog=bpy.data.materials.new('Low grey floor mist');fog.use_nodes=True;nodes=fog.node_tree.nodes;nodes.clear();out=nodes.new('ShaderNodeOutputMaterial');vol=nodes.new('ShaderNodeVolumePrincipled');vol.inputs['Density'].default_value=.055;vol.inputs['Color'].default_value=(.36,.43,.48,1);vol.inputs['Anisotropy'].default_value=.18;fog.node_tree.links.new(vol.outputs['Volume'],out.inputs['Volume']);cube('Floor mist volume',(0,9,.18),(3.3,18,.34),fog)
scene.world=bpy.data.worlds.new('Very low cold ambient');scene.world.use_nodes=True;scene.world.node_tree.nodes.get('Background').inputs['Color'].default_value=(.16,.23,.33,1);scene.world.node_tree.nodes.get('Background').inputs['Strength'].default_value=.045
bpy.ops.object.camera_add(location=(-.3,-.65,1.56));camera=bpy.context.object;camera.name='Study camera slow approach';camera.rotation_euler=(Vector((.12,14,1.52))-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.lens=25;scene.camera=camera;camera.keyframe_insert(data_path='location',frame=0);camera.location.y+=.08;camera.keyframe_insert(data_path='location',frame=240)
scene.render.engine='CYCLES';scene.cycles.samples=96;scene.cycles.use_denoising=True;scene.cycles.device='GPU';prefs=bpy.context.preferences.addons['cycles'].preferences
try:
 prefs.compute_device_type='OPTIX';prefs.get_devices()
 for dev in prefs.devices:dev.use=dev.type!='CPU'
except:scene.cycles.device='CPU'
scene.render.resolution_x=1920;scene.render.resolution_y=1080;scene.render.resolution_percentage=100;scene.render.image_settings.file_format='PNG';scene.view_settings.look='AgX - Medium High Contrast';scene.view_settings.exposure=-.65;scene.frame_set(0);bpy.ops.file.pack_all();bpy.ops.wm.save_as_mainfile(filepath=str(O/'corridor_horror_study.blend'));scene.render.filepath=str(O/'corridor_horror_study.png');bpy.ops.render.render(write_still=True)
report=dict(success=True,stage='Blender material and lighting study; not a packaged gameplay capture',resolution=[1920,1080],source_library='assets/models/hospital_v34',corridor_clear_width_m=3.4,length_m=18,models=used,animated=['camera forward 8cm over 240frames','cold lamp intermittent failure','red emergency pulse'],material_treatment='desaturated worn paint, grey plaster, local old burgundy traces, wet ceramic, low grey fog; no green mold layer',original_source_files_modified=False,game_build_changed=False,scene=str(O/'corridor_horror_study.blend'),image=str(O/'corridor_horror_study.png'));(O/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2));print('HORROR_STUDY_READY',len(used),flush=True)