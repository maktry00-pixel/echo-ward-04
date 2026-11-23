"""Original modular hospital parts with licensed scanned plaster surfaces."""
import bpy,math,json,random
from pathlib import Path
from mathutils import Vector
R=Path('D:/Projects/ECHO');O=R/'assets/models/hospital_v34';A=R/'artifacts/echo_hospital_sources_v34';TX=R/'assets/external/hospital_v34/textures/plastered_wall_02';results=[]
def material(name,color,metal=0,rough=.7,texture=None):
 m=bpy.data.materials.new(name);m.diffuse_color=(*color,1);m.use_nodes=True;n=m.node_tree.nodes;b=n.get('Principled BSDF');b.inputs['Base Color'].default_value=(*color,1);b.inputs['Metallic'].default_value=metal;b.inputs['Roughness'].default_value=rough
 if texture:
  t=n.new('ShaderNodeTexImage');t.image=bpy.data.images.load(str(texture),check_existing=True);m.node_tree.links.new(t.outputs['Color'],b.inputs['Base Color']);nm=n.new('ShaderNodeTexImage');nm.image=bpy.data.images.load(str(TX/'plastered_wall_02_nor_gl_2k.jpg'),check_existing=True);nm.image.colorspace_settings.name='Non-Color';normal=n.new('ShaderNodeNormalMap');normal.inputs['Strength'].default_value=.35;m.node_tree.links.new(nm.outputs['Color'],normal.inputs['Color']);m.node_tree.links.new(normal.outputs['Normal'],b.inputs['Normal']);rt=n.new('ShaderNodeTexImage');rt.image=bpy.data.images.load(str(TX/'plastered_wall_02_rough_2k.jpg'),check_existing=True);rt.image.colorspace_settings.name='Non-Color';m.node_tree.links.new(rt.outputs['Color'],b.inputs['Roughness'])
 return m
def box(name,at,size,mat,bevel=.004):
 bpy.ops.mesh.primitive_cube_add(size=1,location=at);ob=bpy.context.object;ob.name=name;ob.dimensions=size;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);ob.data.materials.append(mat)
 bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.cube_project(cube_size=2);bpy.ops.object.mode_set(mode='OBJECT')
 if bevel>0:
  mod=ob.modifiers.new('Manufactured edge radius','BEVEL');mod.width=min(bevel,min(size)*.2);mod.segments=3;bpy.ops.object.modifier_apply(modifier=mod.name);mod=ob.modifiers.new('Face weighted normals','WEIGHTED_NORMAL');mod.keep_sharp=True;bpy.ops.object.modifier_apply(modifier=mod.name)
 return ob
def cylinder(name,at,radius,length,mat,axis='Z'):
 bpy.ops.mesh.primitive_cylinder_add(vertices=24,radius=radius,depth=length,location=at);ob=bpy.context.object;ob.name=name
 if axis=='X':ob.rotation_euler.y=math.pi/2
 if axis=='Y':ob.rotation_euler.x=math.pi/2
 ob.data.materials.append(mat);return ob
def setup():
 bpy.ops.wm.read_factory_settings(use_empty=True);bpy.context.preferences.filepaths.use_scripts_auto_execute=False;bpy.context.scene.frame_start=0;random.seed(3340)
 return dict(plaster=material('Scanned worn grey plaster',(.7,.72,.71),texture=O/'materials/plaster_grey_4k.jpg'),paint=material('Faded blue grey hospital paint',(.15,.26,.28),texture=O/'materials/paint_bluegrey_4k.jpg'),trim=material('Aged painted skirting',(.1,.15,.16),rough=.68),metal=material('Brushed stainless fittings',(.3,.34,.35),metal=.85,rough=.3),core=material('Exposed grey plaster core',(.33,.32,.29),rough=.95),tile=material('Worn glazed ceramic',(.35,.39,.38),rough=.28),grout=material('Dark grout',(.065,.07,.07),rough=.95))
def wear_patch(name,x,y,z,radius,mat):
 count=9;verts=[]
 for i in range(count):
  a=i*math.tau/count;rr=radius*random.uniform(.55,1.1);verts.append((x+math.cos(a)*rr,y,z+math.sin(a)*rr*.65))
 mesh=bpy.data.meshes.new(name);mesh.from_pydata(verts,[],[list(range(count))]);mesh.materials.append(mat);ob=bpy.data.objects.new(name,mesh);bpy.context.collection.objects.link(ob);return ob
def wall_segment(x,width,z,height,m):
 if width<.001 or height<.001:return
 box('Plaster wall body',(x,0,z),(width,.2,height),m['plaster'])
 low=max(0,min(z+height/2,1.4)-max(z-height/2,0))
 if low>0:
  mid=(min(z+height/2,1.4)+max(z-height/2,0))/2
  for y in [-.103,.103]:box('Lower painted surface',(x,y,mid),(width,.006,low),m['paint'],.001)
 if low>.15:
  for k in range(max(3,int(width*8))):
   xx=x+random.uniform(-width*.44,width*.44);zz=random.uniform(max(z-height/2+.035,.18),min(z+height/2-.035,1.36));rad=min(width*.075,random.uniform(.012,.065));wear_patch('Chipped paint exposing old plaster',xx,-.107,zz,rad,m['core'])
  for edge in [-1,1]:
   for k in range(3):wear_patch('Skirting edge damage',x+edge*width*.46,-.107,max(z-height/2+.05,.16)+k*.11,min(.045,width*.04),m['core'])
 if z-height/2<.14:
  for y in [-.112,.112]:box('Skirting',(x,y,.07),(width,.02,.14),m['trim'],.003)
 if z-height/2<1.05<z+height/2:
  for y in [-.122,.122]:box('Protective crash rail',(x,y,1.05),(width,.035,.105),m['trim'],.009)
def finish(id,slug,title,m,details):
 scene=bpy.context.scene;scene.frame_set(0);meshes=[o for o in scene.objects if o.type=='MESH'];pts=[o.matrix_world@Vector(v) for o in meshes for v in o.bound_box];lo=Vector([min(v[j] for v in pts) for j in range(3)]);hi=Vector([max(v[j] for v in pts) for j in range(3)]);folder=O/slug;folder.mkdir(exist_ok=True);bpy.ops.object.select_all(action='SELECT');bpy.ops.export_scene.gltf(filepath=str(folder/'model.glb'),export_format='GLB',use_selection=True,export_animations=True);bpy.ops.file.pack_all();bpy.ops.wm.save_as_mainfile(filepath=str(folder/'model.blend'));tri=sum(sum(len(f.vertices)-2 for f in o.data.polygons) for o in meshes);row=dict(id=id,slug=slug,title=title,source='Original Echo geometry; scanned plaster by Poly Haven',license='Project original geometry; CC0 texture derivatives',size_m=list(hi-lo),parts=len(meshes),triangles=tri,model=str(folder/'model.glb'),blend=str(folder/'model.blend'),preview=str(folder/'preview.png'),details=details,status='prepared_not_yet_integrated');(folder/'asset.json').write_text(json.dumps(row,ensure_ascii=False,indent=2));results.append(row)
 floor=material('Preview ground',(.09,.1,.11));box('Preview floor',(0,0,lo.z-.035),(100,100,.05),floor,0)
 for loc,power,size in [((3,-4,6),1300,4),((-3,-2,3),700,3),((0,4,5),1000,3)]:
  bpy.ops.object.light_add(type='AREA',location=loc);light=bpy.context.object;light.data.energy=power;light.data.shape='DISK';light.data.size=size;light.rotation_euler=(Vector((0,0,1.5))-light.location).to_track_quat('-Z','Y').to_euler()
 center=(lo+hi)/2;distance=max(hi-lo)*1.6;bpy.ops.object.camera_add(location=center+Vector((distance,-distance,distance*.7)));cam=bpy.context.object;cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=max(hi-lo)*1.65;scene.camera=cam;scene.world=bpy.data.worlds.new('Preview world');scene.world.color=(.13,.13,.13);scene.render.engine='CYCLES';scene.cycles.samples=24;scene.cycles.use_denoising=True;scene.cycles.device='GPU';prefs=bpy.context.preferences.addons['cycles'].preferences
 try:
  prefs.compute_device_type='OPTIX';prefs.get_devices()
  for dev in prefs.devices:dev.use=dev.type!='CPU'
 except:scene.cycles.device='CPU'
 scene.render.resolution_x=900;scene.render.resolution_y=900;scene.render.resolution_percentage=100;scene.render.filepath=str(folder/'preview.png');bpy.ops.render.render(write_still=True);print('ORIGINAL_PART',id,slug,tri,flush=True)
for id,kind in [(3340,'wall_plain'),(3341,'wall_door_opening'),(3342,'wall_window_opening'),(3343,'wall_corner'),(3344,'floor_tiles'),(3345,'ceiling_damaged'),(3346,'hospital_door')]:
 m=setup();details={}
 if kind=='wall_plain':wall_segment(0,3,1.9,3.8,m)
 elif kind=='wall_door_opening':
  for x in [-1.0625,1.0625]:wall_segment(x,.875,1.9,3.8,m)
  wall_segment(0,1.25,3.2,1.2,m);details=dict(opening_width_m=1.25,opening_height_m=2.6)
 elif kind=='wall_window_opening':
  for x in [-1.15,1.15]:wall_segment(x,.7,1.9,3.8,m)
  wall_segment(0,1.6,.45,.9,m);wall_segment(0,1.6,3.05,1.5,m);box('Window sill',(0,-.015,.895),(1.72,.35,.05),m['tile']);details=dict(opening_width_m=1.6,opening_height_m=1.4,sill_m=.9)
 elif kind=='wall_corner':
  wall_segment(0,3,1.9,3.8,m);before=set(bpy.context.scene.objects);wall_segment(0,3,1.9,3.8,m)
  for ob in set(bpy.context.scene.objects)-before:
   x,y,z=ob.location;ob.location=(-1.4-y,1.4+x,z);ob.rotation_euler.z+=math.pi/2
 elif kind=='floor_tiles':
  box('Continuous floor backing',(0,0,.02),(3,3,.04),m['grout'])
  for i in range(10):
   for j in range(10):box('Ceramic tile %02d_%02d'%(i,j),(-1.35+i*.3,-1.35+j*.3,.0475),(.296,.296,.015),m['tile'],.002)
  details=dict(tile_width_m=.3,grout_width_m=.004,walking_surface_z_m=.055)
 elif kind=='ceiling_damaged':
  for i in range(6):
   box('Tee main rail',(i*.6-1.5,0,.015),(.024,3,.03),m['metal'],.002);box('Cross tee rail',(0,i*.6-1.5,.015),(3,.024,.03),m['metal'],.002)
  for i in range(5):
   for j in range(5):
    if (i,j) in [(1,2),(3,3)]:continue
    tile=box('Acoustic ceiling tile',(i*.6-1.2,j*.6-1.2,.04),(.576,.576,.025),m['plaster'],.001)
    if (i,j)==(1,3):tile.rotation_euler.x=.08
  for x in [-1.2,1.2]:
   for y in [-1.2,1.2]:cylinder('Hanging rod',(x,y,.4),.004,.75,m['metal'])
  details=dict(grid_pitch_m=.6,missing_tiles=2)
 elif kind=='hospital_door':
  width,height=1.15,2.5
  for x in [-.625,.625]:box('Door frame jamb',(x,0,1.3),(.07,.17,2.6),m['metal'])
  box('Door frame head',(0,0,2.565),(1.32,.17,.07),m['metal']);bpy.ops.object.empty_add(type='PLAIN_AXES',location=(-width/2,0,0));hinge=bpy.context.object;hinge.name='Door_Hinge_Openable';before=set(bpy.context.scene.objects)
  # Four solid pieces leave a genuine glazed opening, 0.28 x 0.65m.
  box('Leaf left solid',(-.2275,0,1.25),(.695,.052,2.5),m['paint']);box('Leaf right solid',(.4875,0,1.25),(.175,.052,2.5),m['paint']);box('Leaf below window',(.26,0,.65),(.28,.052,1.3),m['paint']);box('Leaf above window',(.26,0,2.225),(.28,.052,.55),m['paint']);glass=material('Vision panel glass',(.18,.25,.27),rough=.12);glass.node_tree.nodes.get('Principled BSDF').inputs['Transmission Weight'].default_value=.8;box('Separate vision glass',(.26,0,1.625),(.28,.008,.65),glass,.001)
  for k in range(18):wear_patch('Door paint scuff',random.uniform(-.5,.5),-.027,random.uniform(.06,.85),random.uniform(.008,.04),m['core'])
  for y in [-.03,.03]:
   box('Handle backing plate',(.41,y,1.06),(.1,.008,.23),m['metal'],.008);cylinder('Lever grip',(.32,y*2,1.06),.014,.2,m['metal'],'X');box('Kick plate',(0,y,.19),(1.08,.005,.3),m['metal'],.003)
  for z in [.22,1.25,2.27]:cylinder('Hinge barrel',(-.59,0,z),.016,.13,m['metal'])
  for ob in set(bpy.context.scene.objects)-before:
   world=ob.matrix_world.copy();ob.parent=hinge;ob.matrix_world=world
  hinge.rotation_euler.z=0;hinge.keyframe_insert(data_path='rotation_euler',frame=0);hinge.rotation_euler.z=-math.radians(70);hinge.keyframe_insert(data_path='rotation_euler',frame=60);hinge.rotation_euler.z=0;bpy.context.scene.frame_end=60;details=dict(openable=True,hinge_node='Door_Hinge_Openable',open_degrees=70,real_glazed_opening=True)
 finish(id,kind,'Echo original '+kind,m,details)
(A/'original_modules.json').write_text(json.dumps(dict(success=True,models=results),ensure_ascii=False,indent=2));print('ORIGINAL_MODULES_COMPLETE',len(results))