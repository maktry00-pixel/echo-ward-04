"""Author six measured replacement assemblies for wrong stretched legacy bindings."""
import bpy,json,math,hashlib
from pathlib import Path
from mathutils import Vector
R=Path('D:/Projects/ECHO');O=R/'unreal/EchoWard/RawAssets/HospitalV37';data=json.loads((O/'manifest.json').read_text());data['assets']=[a for a in data['assets'] if a['id']<3350];materials={}
def reset():
 bpy.ops.wm.read_factory_settings(use_empty=True);materials.clear()
 for key,c in [('ivory',(.45,.46,.42,1)),('alloy',(.25,.28,.29,1)),('dark',(.015,.02,.02,1)),('rubber',(.01,.01,.01,1)),('teal',(.04,.12,.14,1))]:m=bpy.data.materials.new('D27_'+key);m.diffuse_color=c;materials[key]=m
def cube(at,size,mat='alloy',bevel=.006):
 bpy.ops.mesh.primitive_cube_add(size=1,location=at);o=bpy.context.object;o.dimensions=size;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(materials[mat]);mod=o.modifiers.new('soft_edges','BEVEL');mod.width=bevel;mod.segments=2;return o
def tube(a,b,r=.015,mat='alloy',verts=16):
 a,b=Vector(a),Vector(b);v=b-a;bpy.ops.mesh.primitive_cylinder_add(vertices=verts,radius=r,depth=v.length,location=(a+b)/2);o=bpy.context.object;o.rotation_mode='QUATERNION';o.rotation_quaternion=v.to_track_quat('Z','Y');o.data.materials.append(materials[mat]);return o
def done(id,title):
 bpy.ops.object.select_all(action='SELECT');bpy.context.view_layer.objects.active=next(o for o in bpy.data.objects if o.type=='MESH');bpy.ops.object.join();p=O/f'H37_{id}.obj';bpy.ops.wm.obj_export(filepath=str(p),export_selected_objects=True,apply_modifiers=True,global_scale=100,forward_axis='Y',up_axis='Z',export_materials=True,export_triangulated_mesh=True)
 vs=[list(map(float,l.split()[1:4])) for l in p.read_text().splitlines() if l.startswith('v ')];bb=[[min(v[i] for v in vs) for i in range(3)],[max(v[i] for v in vs) for i in range(3)]];data['assets'].append(dict(id=id,slug=f'original_{id}',title=title,file=str(p),bounds_cm=bb,materials=list(materials),source='Project authored: tools/create_hospital_replacements37.py',license='Project-authored original',source_sha256=hashlib.sha256(p.read_bytes()).hexdigest()))
reset()
# Shower pan, riser, mixer, head and hose; 0.85 x 0.85 x 2.20 m.
cube((0,0,.035),(.85,.85,.07),'ivory',.02)
for x in [-.40,.40]:cube((x,0,.07),(.035,.82,.07),'ivory')
for y in [-.40,.40]:cube((0,y,.07),(.82,.035,.07),'ivory')
tube((0,-.35,.08),(0,-.35,2.14),.017);tube((0,-.35,2.14),(0,-.15,2.14),.017);tube((0,-.15,2.12),(0,-.15,2.20),.07)
tube((-.13,-.32,1.05),(.13,-.32,1.05),.025)
for x in [-.1,.1]:tube((x,-.32,1.04),(x,-.27,1.04),.035)
for i in range(10):a=i/10;b=(i+1)/10;tube((.12+math.sin(a*3)*.10,-.30,.45+a*.60),(.12+math.sin(b*3)*.10,-.30,.45+b*.60),.007,'rubber')
done(3350,'Complete shower assembly 0.85 x 0.85 x 2.20 m')
reset()
# One locker module, vent slots, handles, hinges and raised feet.
cube((0,0,.96),(.45,.555,1.78),'teal',.012);cube((0,.285,.99),(.40,.018,1.66),'ivory',.007)
for z in [.44,1.28]:
 for i in range(5):cube((0,.297,z+i*.035),(.27,.006,.009),'dark',.002)
tube((.12,.31,.86),(.12,.31,1.01),.008)
for x in [-.17,.17]:
 for y in [-.21,.21]:cube((x,y,.045),(.045,.045,.09),'alloy')
for z in [.35,1.52]:tube((-.205,.30,z),(-.205,.30,z+.06),.012)
done(3351,'Ventilated steel locker 0.45 x 0.555 x 1.85 m')
reset()
cube((0,0,.495),(.75,.50,.05),'ivory',.025)
for x in [-.31,.31]:
 for y in [-.19,.19]:tube((x,y,.015),(x,y,.47),.018);cube((x,y,.012),(.045,.045,.024),'rubber')
cube((0,0,.15),(.65,.40,.018),'teal');done(3352,'Waiting-room coffee table 0.75 x 0.50 x 0.52 m')
reset()
tube((0,0,0),(0,0,3),.018)
for i in range(9):
 z=1.6+i*.15;w=.6-i*.025;tube((-w,0,z),(w,0,z),.008)
tube((-.35,0,.7),(0,0,1.5),.008);tube((.35,0,.7),(0,0,1.5),.008);done(3353,'Roof antenna with mast and transverse elements')
reset()
cube((0,0,.42),(1.05,.7275,.84),'teal',.025);cube((0,0,1.065),(1.00,.68,.47),'ivory',.025)
tube((0,.30,1.065),(0,.375,1.065),.19,'alloy',32);tube((0,.376,1.065),(0,.39,1.065),.14,'dark',32)
for x in [-.11,.11]:tube((x,.415,1.01),(x,.415,1.12),.009)
for x in [-.40,.40]:tube((x,.36,1.2),(x,.39,1.2),.035,'dark')
done(3354,'Floor autoclave and service base 1.05 x 0.7275 x 1.30 m')
reset()
for x in [-.37,.37]:
 tube((x,-.60,.05),(x,-.60,1.46),.025);tube((x,-.60,.05),(x,.62,.05),.025);tube((x,-.60,1.46),(x,.32,1.46),.025)
tube((-.37,-.60,1.46),(.37,-.60,1.46),.025)
for i in range(10):cube((0,-.48,.18+i*.04),(.33,.23,.035),'dark')
for x in [-.25,.25]:tube((x,-.45,.30),(x,.25,1.38),.004,'rubber');tube((x,.25,1.38),(x,.36,1.30),.014,'rubber')
cube((0,.12,.64),(.38,.40,.10),'teal',.035);tube((0,.12,.05),(0,.12,.59),.03)
done(3355,'Rehabilitation pulley station 0.85 x 1.35 x 1.50 m')
(O/'manifest.json').write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8');print('ORIGINAL_REPLACEMENTS37',len(data['assets']))
