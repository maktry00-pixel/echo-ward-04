"""Build original numbered hospital assets. Blender 4.5, metres, Z up.
Usage: blender -b --python tools/build_echo_modular_assets.py -- --start 1 --end 60
All exports contain actual meshes and image-based portable PBR materials.
"""
import bpy, math, random, json, sys, re, hashlib, argparse
from pathlib import Path
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[1]
import tempfile
tempfile.tempdir=str(ROOT/'tmp'/'blender_asset_temp')
Path(tempfile.tempdir).mkdir(exist_ok=True)
bpy.context.preferences.filepaths.temporary_directory=tempfile.tempdir
sys.path.insert(0,str(ROOT/'tools'))
from echo_asset_io import externalize_images
bpy.context.preferences.filepaths.save_version=0
OUT=ROOT/'assets/models/echo_custom'
MAT=OUT/'materials'
random.seed(20261024)
args=argparse.ArgumentParser(); args.add_argument('--start',type=int,default=1); args.add_argument('--end',type=int,default=60)
args.add_argument('--no-render',action='store_true')
opt=args.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
try:
    cp=bpy.context.preferences.addons['cycles'].preferences
    cp.compute_device_type='OPTIX';cp.get_devices()
    for device in cp.devices:device.use=device.type=='OPTIX'
except Exception as exc:print('CPU render fallback',exc,flush=True)
catalog={int(m[1]):m[2] for line in (OUT/'REQUESTED_CATALOG.txt').read_text(encoding='utf-8-sig').splitlines() if (m:=re.match(r'^(\d+)\.\s+(.+)',line))}
M={}; parts=[]; current=0

def material(key):
    if key in M:return M[key]
    mat=bpy.data.materials.new(key); mat.use_nodes=True
    ns=mat.node_tree.nodes; links=mat.node_tree.links; bs=ns.get('Principled BSDF')
    folder=next((p for p in MAT.iterdir() if p.name==key or p.name.startswith(key+'_')),None)
    if folder:
        for name,socket in [('BaseColor','Base Color'),('Normal','Normal'),('ORM',None)]:
            t=ns.new('ShaderNodeTexImage'); t.image=bpy.data.images.load(str(folder/(name+'.png')),check_existing=True)
            t.label=name
            if name!='BaseColor':t.image.colorspace_settings.name='Non-Color'
            if name=='Normal':
                n=ns.new('ShaderNodeNormalMap'); links.new(t.outputs['Color'],n.inputs['Color']); links.new(n.outputs['Normal'],bs.inputs['Normal'])
            elif name=='ORM':
                n=ns.new('ShaderNodeSeparateColor'); links.new(t.outputs['Color'],n.inputs['Color'])
                links.new(n.outputs['Green'],bs.inputs['Roughness']); links.new(n.outputs['Blue'],bs.inputs['Metallic'])
            else:
                links.new(t.outputs['Color'],bs.inputs[socket])
                if key.isdigit() and 510<=int(key)<=530:
                    links.new(t.outputs['Alpha'],bs.inputs['Alpha']);mat.surface_render_method='DITHERED'
    else:
        colors={'steel':(.23,.28,.28,1),'black':(.018,.025,.025,1),'glass':(.38,.55,.52,1),'red':(.30,.035,.018,1),'white':(.75,.77,.68,1),'water':(.06,.09,.08,1),'amber':(.48,.20,.018,1)}
        bs.inputs['Base Color'].default_value=colors.get(key,(.15,.23,.20,1))
        bs.inputs['Roughness'].default_value=.26 if key in ['steel','glass','water'] else .65
        bs.inputs['Metallic'].default_value=.85 if key=='steel' else 0
        if key=='glass':
            bs.inputs['Transmission Weight'].default_value=.7; bs.inputs['IOR'].default_value=1.47
        if key=='water':bs.inputs['Coat Weight'].default_value=.8
    M[key]=mat;return mat

def uv(obj):
    mesh=obj.data
    if not mesh.uv_layers:mesh.uv_layers.new(name='UVMap')
    layer=mesh.uv_layers.active.data
    # Stable, world-sized projection, selected by face normal; no random UV scale.
    for poly in mesh.polygons:
        n=poly.normal; axis=max(range(3),key=lambda i:abs(n[i]))
        axes=[i for i in range(3) if i!=axis]
        for li in poly.loop_indices:
            v=obj.matrix_world@mesh.vertices[mesh.loops[li].vertex_index].co
            layer[li].uv=(v[axes[0]]/3,v[axes[1]]/3)

def finish(obj,name,mat,bevel=0):
    obj.name=f'EH_{current:03d}_{name}'; obj.data.materials.append(material(mat))
    bpy.ops.object.select_all(action='DESELECT');obj.select_set(True)
    bpy.context.view_layer.objects.active=obj
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    uv(obj)
    if bevel:
        mod=obj.modifiers.new('Manufactured edge bevel','BEVEL');mod.width=bevel;mod.segments=3
        mod=obj.modifiers.new('Corner normals','WEIGHTED_NORMAL');mod.keep_sharp=True
    parts.append(obj);return obj

def box(name,loc,size,mat='026',bevel=.004):
    bpy.ops.mesh.primitive_cube_add(size=1,location=loc)
    obj=bpy.context.object;obj.dimensions=size
    return finish(obj,name,mat,bevel)

def cylinder(name,a,b,r,mat='steel',vertices=24):
    d=Vector(b)-Vector(a);bpy.ops.mesh.primitive_cylinder_add(vertices=vertices,radius=r,depth=d.length,location=(Vector(a)+Vector(b))/2)
    o=bpy.context.object;o.rotation_mode='QUATERNION';o.rotation_quaternion=d.to_track_quat('Z','Y')
    o=finish(o,name,mat,.001)
    for p in o.data.polygons:p.use_smooth=len(p.vertices)==4
    return o

def mesh(name,verts,faces,mat='025',bevel=0):
    m=bpy.data.meshes.new(name);m.from_pydata(verts,[],faces);m.update()
    o=bpy.data.objects.new(name,m);bpy.context.collection.objects.link(o)
    return finish(o,name,mat,bevel)

def slab_polygon(name,outline,yfront,yback,mat):
    n=len(outline); v=[(x,yfront,z) for x,z in outline]+[(x,yback,z) for x,z in outline]
    f=[tuple(range(n-1,-1,-1)),tuple(range(n,n*2))]
    f += [(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
    return mesh(name,v,f,mat,.002)

def curve(name,points,r=.012,mat='steel'):
    cu=bpy.data.curves.new(name,'CURVE');cu.dimensions='3D';cu.resolution_u=12;cu.bevel_depth=r;cu.bevel_resolution=3
    s=cu.splines.new('POLY');s.points.add(len(points)-1)
    for p,v in zip(s.points,points):p.co=(*v,1)
    ob=bpy.data.objects.new(name,cu);bpy.context.collection.objects.link(ob)
    bpy.ops.object.select_all(action='DESELECT');bpy.context.view_layer.objects.active=ob;ob.select_set(True);bpy.ops.object.convert(target='MESH');ob.select_set(False)
    return finish(ob,name,mat)

def bolt(x,y,z,r=.01):
    cylinder('fastener',(x,y,z),(x,y-.006,z),r,'steel',6)
    box('screw_slot',(x,y-.007,z),(r*.9,.001,.002),'black',0)

def text(label,loc,size=.07,mat='white',rot=(math.pi/2,0,0)):
    cu=bpy.data.curves.new('label','FONT');cu.body=label;cu.size=size;cu.align_x='CENTER';cu.extrude=.00025
    fontpath=Path('C:/Windows/Fonts/segoeui.ttf')
    if fontpath.exists():cu.font=bpy.data.fonts.load(str(fontpath),check_existing=True)
    ob=bpy.data.objects.new('label',cu);bpy.context.collection.objects.link(ob);ob.location=loc;ob.rotation_euler=rot
    bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);bpy.context.view_layer.objects.active=ob;bpy.ops.object.convert(target='MESH');ob.select_set(False)
    return finish(ob,'inscription',mat)

def wall_section(x,width,z,height,mat='026',y=0,depth=.22):
    if width<=0 or height<=0:return
    # continuous two-tone hospital finish; do not fill architectural openings.
    low=max(0,min(z+height,1.45)-z)
    if low:box('lower_enamel',(x,y,z+low/2),(width,depth,low),mat,.003)
    upper=height-low
    if upper:box('upper_plaster',(x,y,z+low+upper/2),(width,depth,upper),'025',.003)
    if z<.10:box('coved_skirt',(x,y-.116,.055),(width,.026,.11),'rubber',.008)
    if z<=1.45<z+height:box('paint_divider',(x,y-.114,1.453),(width,.012,.016),'028',.003)

def wall(kind):
    if kind in (1,2,3,4,5,6):
        if kind==1:wall_section(0,3,0,3.2)
        elif kind==2:
            wall_section(0,3,0,3.2)
            # Real plaster edges sit in front of exposed masonry.
            for x,z,rx,rz in [(-.7,1.65,.5,.65),(.9,.65,.3,.4),(.35,2.6,.35,.32)]:
                outline=[(x+math.cos(t)*rx*random.uniform(.7,1.1),z+math.sin(t)*rz*random.uniform(.7,1.1)) for t in [i*math.tau/19 for i in range(19)]]
                slab_polygon('spalled_plaster_edge',outline,-.123,-.113,'029')
                # small exposed aggregate chips on actual damage borders
                for px,pz in outline[::3]:box('plaster_fragment',(px,-.129,pz),(.028,.014,.04),'025',.005)
        elif kind==3:
            box('exposed_brick_substrate',(0,0,1.6),(3,.22,3.2),'030',.005)
            for left in (True,False):
                for j in range(13):
                    z=.12+j*.245;w=random.uniform(.15,.35);x=(-1 if left else 1)*(1.5-w/2)
                    box('ragged_plaster_margin',(x,-.122,z),(w,.025,.24),'025',.008)
        elif kind==4:
            box('concrete',(0,0,1.6),(3,.22,3.2),'029',.009)
            for x in [-1.25,1.25]:
                for z in [.45,1.6,2.75]:cylinder('formwork_tie_hole',(x,-.112,z),(x,-.114,z),.018,'black')
            for z in [1.07,2.14]:box('casting_joint',(0,-.113,z),(3,.002,.004),'black',0)
        elif kind==5:
            box('tile_wall',(0,0,1.6),(3,.22,3.2),'033',.004)
            for x,z in [(-.53,.6),(.97,2.1)]:
                slab_polygon('broken_glaze',[(x-.10,z-.13),(x+.11,z-.13),(x+.09,z+.04),(x-.04,z+.12)],-.115,-.111,'029')
        else:
            box('metal_backing',(0,0,1.6),(3,.2,3.2),'032')
            for x in [-1,-0,1]:
                box('lab_wall_panel',(x,-.115,1.6),(.986,.035,3.18),'032',.012)
                for bx in [x-.43,x+.43]:
                    for z in [.12,1.6,3.08]:bolt(bx,-.138,z)
            box('service_rail',(0,-.16,.28),(3,.08,.13),'031',.01)
    elif kind in (7,8):
        wall_section(-.75,1.5,0,3.2)
        before=len(parts);wall_section(0,1.5,0,3.2)
        for o in parts[before:]:
            x,y,z=o.location
            if kind==7:o.location=(-y-.11,x+.75,z);o.rotation_euler.z=math.pi/2
            else:o.location=(y+.11,-x-.75,z);o.rotation_euler.z=-math.pi/2
    elif kind in (9,10,11,12,13):
        w,h,bottom={9:(1.10,2.30,0),10:(1.9,2.30,0),11:(1.65,1.55,.9),12:(1.5,1.1,1.05),13:(.65,.45,2.35)}[kind]
        side=(3-w)/2
        for s in [-1,1]:wall_section(s*(w/2+side/2),side,0,3.2)
        if bottom:wall_section(0,w,0,bottom)
        wall_section(0,w,bottom+h,3.2-bottom-h)
        # plaster returns on all four reveal surfaces
        for s in [-1,1]:box('opening_reveal',(s*(w/2+.012),0,bottom+h/2),(.024,.23,h),'025',.002)
        if bottom:box('sill',(0,-.025,bottom),(w+.10,.36,.05),'028',.008)
    elif kind==14:
        left=[(-1.5,0),(-.82,0),(-.86,.3),(-.73,.57),(-.85,.85),(-.69,1.13),(-.78,1.44),(-.62,1.75),(-.76,2.05),(-.55,2.29),(-.2,2.49),(-.36,2.65),(-.83,2.72),(-1.5,2.6)]
        right=[(.83,0),(1.5,0),(1.5,2.65),(.88,2.73),(.57,2.52),(.68,2.23),(.60,1.92),(.79,1.61),(.67,1.3),(.77,.95),(.65,.65),(.84,.32)]
        slab_polygon('broken_left',left,-.11,.11,'029');slab_polygon('broken_right',right,-.11,.11,'030')
        slab_polygon('broken_lintel',[(-1.5,2.6),(-.83,2.72),(-.3,2.65),(.1,2.77),(.38,2.60),(.88,2.73),(1.5,2.65),(1.5,3.2),(-1.5,3.2)],-.11,.11,'025')
        for x in [-.6,-.3,.05,.38]:
            curve('exposed_rebar',[(x,0,2.78),(x+.03,-.02,2.55),(x+.08,-.09,2.40)],.008,'031')
    elif kind==15:
        wall_section(0,3,0,1.12);box('counter',(0,-.08,1.15),(3.12,.5,.075),'wood',.025)
    elif kind==16:
        for x in [-1.46,0,1.46]:box('partition_mullion',(x,0,1.6),(.065,.09,3.2),'032')
        for z in [.04,1.0,3.16]:box('partition_crossmember',(0,0,z),(3,.09,.07),'032')
        for x in [-.73,.73]:
            box('opaque_spandrel',(x,0,.52),(1.40,.04,.90),'026')
            box('glass_pane',(x,0,2.08),(1.40,.012,2.06),'glass',.001)
    elif kind==17:
        for x in [-1.46,1.46]:box('fence_post',(x,0,1.6),(.06,.06,3.2),'031')
        for z in [.05,3.15]:box('fence_rail',(0,0,z),(2.96,.05,.05),'031')
        # actual woven diamond mesh, clipped to bounds
        for sign in [-1,1]:
            for k in range(-42,43):
                intercept=k*.12
                hits=[]
                for x in [-1.43,1.43]:
                    z=sign*x+intercept
                    if .09<=z<=3.11:hits.append((x,z))
                for z in [.09,3.11]:
                    x=(z-intercept)/sign
                    if -1.43<x<1.43:hits.append((x,z))
                if len(hits)>=2:cylinder('diamond_wire',(hits[0][0],-.012*sign,hits[0][1]),(hits[1][0],-.012*sign,hits[1][1]),.003,'032',6)
    elif kind in (18,19):
        box('structural_column',(0,0,1.6),(.55,.55,3.2),'029',.012)
        if kind==19:
            box('lower_painted_casing',(0,-.005,.72),(.59,.59,1.44),'026',.01)
            box('upper_plaster_casing',(0,-.005,2.32),(.59,.59,1.76),'025',.01)
        box('column_foot',(0,0,.06),(.65,.65,.12),'037',.012)
    elif kind in (20,21):
        if kind==20:
            cylinder('handrail',(-1.5,-.16,1),(1.5,-.16,1),.04,'wood')
            for x in [-1.22,0,1.22]:
                box('wall_mount',(x,0,.93),(.08,.025,.12),'032',.01)
                curve('bracket',[(x,-.025,.93),(x,-.15,.93),(x,-.16,.98)],.012,'032')
                bolt(x,-.017,.93)
        else:
            p=[(-.3,-.16,1),(-.16,-.16,1)]
            p += [(-.16+.16*math.cos(t),.16*math.sin(t),1) for t in [(-math.pi/2)+i*math.pi/2/16 for i in range(17)]]
            p += [(0,.30,1)];curve('rail_corner',p,.04,'wood')
    elif kind==22:
        box('corner_guard_a',(.0,0,1),(.075,.004,2),'032',.002);box('corner_guard_b',(.036,.036,1),(.004,.075,2),'032',.002)
        for z in [.12,.8,1.8]:bolt(-.015,-.004,z,.006)
    elif kind==23:
        box('coved_skirt',(0,0,.06),(3,.035,.12),'rubber',.015)
        box('skirt_cap',(0,-.018,.12),(3,.012,.012),'rubber',.005)
    elif kind==24:
        box('cornice',(0,0,.065),(3,.12,.13),'025',.018)
        box('cornice_lower_bead',(0,-.05,.016),(3,.06,.03),'028',.012)

def floor_detail(i):
    if i==41:
        slab_polygon('chipped_tile',[(-.15,0),(.13,0),(.15,.24),(.10,.30),(-.15,.30)],-.014,.014,'034')
        for o in parts:o.rotation_euler.x=math.pi/2
    elif i==42:
        box('mortar_bed',(0,0,.005),(.30,.30,.01),'029',.002)
        for s in [-1,1]:
            box('remaining_tile',(s*.18,0,.012),(.06,.42,.024),'034',.003)
            box('remaining_tile',(0,s*.18,.012),(.30,.06,.024),'034',.003)
    elif i in (43,44,48):
        w,l=(.3,.3) if i!=48 else (1.2,.65)
        if i==43:
            for s in [-1,1]:
                box('drain_pan_side',(s*w/2,0,-.065),(.015,l,.13),'031')
                box('drain_pan_end',(0,s*l/2,-.065),(w,.015,.13),'031')
            box('drain_pan_base',(0,0,-.13),(w,l,.012),'031')
            cylinder('outlet',(0,0,-.15),(0,0,-.13),.048,'black')
        else:
            for s in [-1,1]:
                box('grate_border',(s*w/2,0,.015),(.018,l+.018,.025),'031')
                box('grate_border',(0,s*l/2,.015),(w,.018,.025),'031')
            for j in range(int(w/.022)):
                x=-w/2+.018+j*.022
                box('drain_grating',(x,0,.012),(.008,l,.022),'032',.001)
            for y in [-l*.3,l*.3]:box('grate_support',(0,y,-.002),(w,.015,.015),'031')
    elif i==45:
        box('joint_rubber',(0,0,0),(3,.035,.02),'rubber')
        for y in [-.035,.035]:box('joint_flange',(0,y,0),(3,.025,.015),'032')
    elif i in (46,47):
        box('threshold',(0,0,.012),(1.1,.15,.024),'032',.009)
        for y in [-.045,0,.045]:box('threshold_groove',(0,y,.025),(1.05,.002,.001),'black',0)
        for x in [-.44,0,.44]:
            cylinder('countersunk_screw',(x,0,.026),(x,0,.028),.009,'steel',16)
        if i==47:
            o=box('bent_end',(.50,0,.035),(.12,.15,.02),'031',.002);o.rotation_euler.y=-.25
    elif i==49:
        box('entry_mat',(0,0,.012),(1.2,.7,.024),'rubber',.022)
        for j in range(42):box('mat_rib',(-.57+j*.028,0,.027),(.012,.64,.007),'rubber',.003)
    elif i==50:
        verts=[(0,0,.002)]
        for j in range(96):
            a=j*math.tau/96;r=1+.13*math.sin(a*5)+.08*math.sin(a*11)
            verts.append((math.cos(a)*r,math.sin(a)*r*.55,.002))
        mesh('puddle_surface',verts,[(0,j+1,(j+1)%96+1) for j in range(96)],'water')
    elif 51<=i<=55:
        if i in (51,52,53):box('acoustic_tile',(0,0,.012),(.595,.595,.024),'ceiling_fibre',.002)
        elif i==54:
            v=[];f=[];steps=28
            for j in range(steps+1):
                for k in range(steps+1):
                    x=-.2975+k*.595/steps;y=-.2975+j*.595/steps
                    z=-.09*(1-(x/.3)**2)*(1-(y/.3)**2)
                    v.append((x,y,z))
            for j in range(steps):
                for k in range(steps):
                    a=j*(steps+1)+k;f.append((a,a+1,a+steps+2,a+steps+1))
            o=mesh('sagging_fibreboard',v,f,'ceiling_fibre');m=o.modifiers.new('panel_thickness','SOLIDIFY');m.thickness=.02
        else:
            slab_polygon('broken_panel',[(-.3,0),(.3,0),(.3,.29),(.11,.31),(.03,.24),(-.09,.37),(-.18,.30),(-.3,.42)],-.01,.01,'ceiling_fibre')
            for o in parts:o.rotation_euler.x=math.pi/2
        if i==52:
            # irregular ring rather than a perfectly circular decal
            v=[];f=[]
            for radius in [.09,.14]:
                for k in range(64):
                    a=k*math.tau/64;r=radius*(1+.10*math.sin(a*5)+.05*math.cos(a*9));v.append((math.cos(a)*r,math.sin(a)*r,.025))
            f=[(k,(k+1)%64,(k+1)%64+64,k+64) for k in range(64)]
            mesh('water_tide_mark',v,f,'wood')
        if i==53:
            curve('panel_crack',[(-.29,-.2,.025),(-.15,-.08,.025),(-.04,-.04,.025),(.04,.09,.025),(.25,.17,.025)],.0018,'black')
    elif i==56:
        for y in [-.3,.3]:
            box('T_runner_flange',(0,y,0),(3,.024,.004),'032',.001)
            box('T_runner_web',(0,y,.016),(3,.003,.034),'032',.001)
        for x in [-1.5,-.9,-.3,.3,.9,1.5]:
            box('cross_T',(x,0,0),(.024,.6,.004),'032',.001)
            box('cross_T_web',(x,0,.016),(.003,.6,.034),'032',.001)
    elif i==57:
        cylinder('hanger_wire',(0,0,0),(0,0,.90),.0025,'032',10)
        box('adjustable_clip',(0,0,.48),(.025,.008,.065),'032',.002)
        curve('ceiling_hook',[(0,0,.9),(0,.025,.93),(0,.045,.9)],.0025,'032')
    elif i==58:
        box('raw_slab',(0,0,.10),(3,3,.20),'029',.008)
        for x in [-1.2,1.2]:box('structural_beam',(x,0,-.16),(.28,3,.32),'029',.01)
    elif i in (59,60):
        for s in [-1,1]:
            box('hatch_trim',(s*.31,0,.015),(.035,.65,.03),'032')
            box('hatch_trim',(0,s*.31,.015),(.65,.035,.03),'032')
        if i==59:
            box('inspection_cover',(0,0,.018),(.585,.585,.025),'028',.006)
            box('latch',(0,-.23,.035),(.055,.018,.008),'steel')
        else:
            hinge=bpy.data.objects.new('Hinge_pivot',None);bpy.context.collection.objects.link(hinge);hinge.location=(-.3,0,0);parts.append(hinge)
            o=box('open_hatch_cover',(0,0,.02),(.585,.585,.025),'028',.006);o.parent=hinge;o.matrix_parent_inverse=hinge.matrix_world.inverted();hinge.rotation_euler.y=-math.radians(68)
            for y in [-.21,.21]:cylinder('hinge_pin',(-.3,y-.04,0),(-.3,y+.04,0),.008,'steel')

def torus(name,loc,major,minor,mat='steel',rot=(math.pi/2,0,0)):
    bpy.ops.mesh.primitive_torus_add(major_radius=major,minor_radius=minor,major_segments=32,minor_segments=8,location=loc,rotation=rot)
    return finish(bpy.context.object,name,mat)

def hinge(parent,x,z):
    before=len(parts)
    cylinder('hinge_barrel',(x,-.02,z-.045),(x,-.02,z+.045),.012,'031')
    for s in [-1,1]:
        box('hinge_leaf',(x+s*.028,-.015,z),(.05,.008,.085),'032',.002)
        for zz in [z-.025,z+.025]:bolt(x+s*.034,-.021,zz,.005)
    for o in parts[before:]:
        if parent:o.parent=parent;o.matrix_parent_inverse=parent.matrix_world.inverted()

def pull_handle(x,y,z,length=.18):
    curve('pull_handle',[(x,y+.045,z-length/2),(x,y,z-length/2),(x,y,z+length/2),(x,y+.045,z+length/2)],.012,'steel')
    for zz in [z-length/2,z+length/2]:cylinder('handle_rose',(x,y+.045,zz),(x,y+.05,zz),.024,'032')

def lever_handle(x,y,z):
    cylinder('handle_rose',(x,y,z),(x,y-.01,z),.032,'032')
    curve('lever_handle',[(x,y-.012,z),(x,y-.06,z),(x+.13,y-.06,z),(x+.145,y-.04,z)],.012,'steel')
    box('key_escutcheon',(x,y-.003,z-.12),(.035,.008,.06),'032',.008)
    cylinder('keyhole',(x,y-.009,z-.12),(x,y-.011,z-.12),.006,'black',16)

def door_leaf(i,width=1.,height=2.2,offset=0,side=-1):
    pivot=bpy.data.objects.new(f'EH_{current:03d}_DoorPivot',None);bpy.context.collection.objects.link(pivot);pivot.location=(offset+side*width/2,0,0);parts.append(pivot)
    before=len(parts);mat='wood' if i in (61,64,73) else ('031' if i==71 else '026')
    window=i in (63,64,65,66,69)
    if i==72:
        for x in [offset-width/2+.035,offset+width/2-.035]:box('gate_stile',(x,0,height/2),(.07,.06,height),'031')
        for z in [.07,.85,1.5,height-.07]:box('gate_rail',(offset,0,z),(width,.06,.065),'031')
        for j in range(9):cylinder('security_bar',(offset-width*.42+j*width*.105,0,.1),(offset-width*.42+j*width*.105,0,height-.1),.012,'031')
    elif window:
        ww=width*.44;bottom=1.17;wh=.65
        for s in [-1,1]:box('door_stile',(offset+s*(ww/2+(width-ww)/4),0,height/2),((width-ww)/2,.055,height),mat,.008)
        box('lower_door_panel',(offset,0,bottom/2),(ww,.055,bottom),mat,.008)
        box('upper_door_panel',(offset,0,(bottom+wh+height)/2),(ww,.055,height-bottom-wh),mat,.008)
        for s in [-1,1]:box('vision_trim',(offset+s*(ww/2+.012),-.034,bottom+wh/2),(.027,.025,wh+.05),'032',.004)
        for z in [bottom,bottom+wh]:box('vision_trim',(offset,-.034,z),(ww+.055,.025,.027),'032',.004)
        glass=box('vision_glass',(offset,0,bottom+wh/2),(ww-.015,.009,wh-.015),'glass',.002)
        if i==64:glass.data.materials.clear();glass.data.materials.append(material('white'))
    else:
        box('door_leaf',(offset,0,height/2),(width,.055,height),mat,.009)
        if i==61:
            for z,h in [(.57,.75),(1.56,.8)]:
                box('raised_panel',(offset,-.035,z),(width-.24,.025,h),'026',.016)
                for s in [-1,1]:box('panel_moulding',(offset+s*(width/2-.11),-.048,z),(.025,.02,h+.055),'wood',.006)
        if i in (67,68,69,70):
            box('reinforcement_panel',(offset,-.035,1.15),(width-.16,.025,1.92),'032',.012)
            for x in [offset-width*.36,offset+width*.36]:
                for z in [.3,.7,1.1,1.5,1.9]:bolt(x,-.051,z,.008)
    if i!=72:
        box('kick_plate',(offset,-.039,.20),(width-.10,.018,.32),'031',.006)
        for x in [offset-width*.36,offset+width*.36]:
            for z in [.08,.32]:bolt(x,-.05,z,.006)
    hx=offset-side*(width/2-.14)
    if i in (66,70):pull_handle(hx,-.12,1.05,.30)
    elif i==68:
        box('panic_bar',(offset,-.12,1.05),(width*.72,.045,.045),'032',.014)
        for x in [offset-width*.33,offset+width*.33]:box('panic_bar_mount',(x,-.075,1.05),(.08,.10,.12),'032',.01)
    else:lever_handle(hx,-.04,1.02)
    if i==70:
        torus('pressure_handwheel',(offset,-.13,1.18),.18,.017,'031')
        for a in range(4):
            t=a*math.pi/2;cylinder('wheel_spoke',(offset,-.13,1.18),(offset+math.cos(t)*.17,-.13,1.18+math.sin(t)*.17),.009,'steel')
        for x in [offset-width*.4,offset+width*.4]:
            for z in [.35,1.1,1.85]:box('locking_dog',(x,-.10,z),(.09,.10,.09),'031',.009)
    for o in parts[before:]:o.parent=pivot;o.matrix_parent_inverse=pivot.matrix_world.inverted()
    for z in [.24,1.12,2.0]:hinge(None,offset+side*width/2,z)
    return pivot

def door(i):
    if 61<=i<=74:
        double=i in (65,66,74);w=1.9 if double else (1.15 if i==70 else 1.0);h=2.2
        for s in [-1,1]:box('jamb',(s*(w/2+.035),0,h/2),(.07,.16,h+.07),'028',.006)
        box('header',(0,0,h+.035),(w+.14,.16,.07),'028',.006)
        if i==74:
            for s in [-1,1]:
                for x in [s*w/4-w/4+.025,s*w/4+w/4-.025]:box('sliding_leaf_stile',(x,-.01,h/2),(.04,.05,h),'032')
                for z in [.03,.9,h-.03]:box('sliding_leaf_rail',(s*w/4,-.01,z),(w/2,.05,.06),'032')
                box('sliding_glass',(s*w/4,-.01,1.52),(w/2-.08,.012,1.2),'glass')
                box('sliding_spandrel',(s*w/4,-.01,.46),(w/2-.08,.02,.84),'026')
                pull_handle(s*.15,-.10,1.1,.25)
            box('sliding_motor_track',(0,0,2.32),(w+.2,.24,.16),'032',.014)
        else:
            if double:
                door_leaf(i,w/2-.012,h,-w/4,-1);door_leaf(i,w/2-.012,h,w/4,1)
            else:door_leaf(i,w-.012,h)
            if i in (67,68):
                box('closer_body',(-.28,-.105,2.1),(.24,.09,.07),'032',.009)
                curve('closer_arm',[(-.28,-.16,2.12),(-.08,-.21,2.18),(.14,-.08,2.22)],.009,'steel')
    elif i in (75,76):
        w=2.4 if i==75 else 3.0;h=2.7
        for s in [-1,1]:box('shutter_guide',(s*(w/2+.04),0,h/2),(.08,.13,h),'031')
        if i==75:
            for j in range(36):box('roll_shutter_slat',(0,0,(j+.5)*h/36),(w,.05,h/36-.004),'032',.011)
            cylinder('roller_drum',(-w/2,0,h+.13),(w/2,0,h+.13),.16,'031')
            box('roller_cover',(0,0,h+.16),(w+.25,.4,.36),'031',.035)
        else:
            for j in range(6):
                z=(j+.5)*h/6;box('garage_panel',(0,0,z),(w,.06,h/6-.012),'032',.01)
                for x in [-.9,0,.9]:box('pressed_panel',(x,-.035,z),(.75,.022,.30),'026',.025)
        pull_handle(0,-.1,.48,.2)
    elif i in (77,78):
        w,h=1.04,2.24
        for s in [-1,1]:box('door_trim',(s*(w/2+.04),0,h/2),(.08,.13 if i==77 else .025,h),'wood',.009)
        box('door_header',(0,0,h+.04),(w+.16,.13 if i==77 else .025,.08),'wood',.009)
    elif i==79:lever_handle(0,0,.18)
    elif i==80:pull_handle(0,-.06,.22,.30)
    elif i==81:hinge(None,0,.06)
    elif i==82:
        box('closer_body',(0,0,.05),(.25,.085,.08),'032',.01)
        curve('articulated_closer_arm',[(0,-.045,.08),(.22,-.15,.11),(.42,0,.11)],.009,'steel')
        box('closer_mount',(.42,0,.11),(.06,.025,.025),'032')
    elif i==83:
        cylinder('floor_stop',(0,0,0),(0,0,.055),.022,'steel');torus('rubber_bumper',(0,0,.042),.023,.007,'rubber',(0,0,0))
    elif i==84:
        for x in [0,.29]:
            box('chain_mount',(x,0,.07),(.06,.015,.1),'032',.006)
            for z in [.04,.1]:bolt(x,-.01,z,.005)
        for j in range(17):
            x=j*.017;z=.07-.08*math.sin(j/16*math.pi)
            o=torus('chain_link',(x,-.04,z),.009,.0025,'steel',(math.pi/2 if j%2 else 0,0,0));o.scale.x=1.35
    elif i==85:
        box('bolt_backplate',(0,0,.06),(.24,.012,.10),'031')
        cylinder('sliding_bolt',(-.12,-.035,.065),(.14,-.035,.065),.014,'steel')
        for x in [-.07,.07]:box('bolt_guide',(x,-.025,.065),(.03,.045,.05),'032',.006)
        cylinder('bolt_thumb',(-.02,-.035,.065),(-.02,-.085,.065),.012,'steel')
    elif i==86:
        box('padlock_body',(0,0,.045),(.075,.035,.09),'031',.012)
        p=[(-.025,0,.085),(-.025,0,.123)]+[(math.cos(t)*.025,0,.123+math.sin(t)*.025) for t in [math.pi-j*math.pi/20 for j in range(21)]]+[(.025,0,.085)]
        curve('padlock_shackle',p,.006,'steel');cylinder('lock_core',(0,-.019,.04),(0,-.02,.04),.01,'steel')
    elif i in (87,88):
        box('lock_plate',(0,0,.08),(.08,.012,.16),'032',.004)
        if i==87:
            box('hasp',(0,-.015,.09),(.035,.018,.065),'031')
            torus('hasp_eye',(0,-.035,.09),.014,.005,'steel')
        else:
            box('mortise_body',(.02,.045,.08),(.06,.09,.13),'031')
            box('latch',(.048,-.002,.11),(.045,.025,.025),'steel',.006)
            cylinder('keyway',(0,-.008,.05),(0,-.012,.05),.01,'black')
    elif 89<=i<=92:
        w,h=(.09,.15) if i==89 else (.11,.16)
        box('control_housing',(0,0,h/2),(w,.035,h),'031' if i<91 else 'red',.012)
        box('front_panel',(0,-.021,h/2),(w-.013,.008,h-.014),'black',.008)
        if i==89:
            box('card_symbol',(0,-.027,.07),(.04,.003,.027),'white',.002)
            for r in [.025,.033,.041]:
                curve('RFID_wave',[(r*math.cos(a),-.028,.115+r*math.sin(a)) for a in [j*math.pi/2/14 for j in range(-7,8)]],.001,'white')
            cylinder('status_led',(.025,-.026,.135),(.025,-.028,.135),.003,'red')
        elif i==90:
            box('display',(0,-.027,.13),(.07,.003,.027),'glass')
            for j in range(4):
                for k in range(3):
                    x=(k-1)*.027;z=.10-j*.025
                    box('keypad_key',(x,-.028,z),(.021,.006,.018),'032',.003)
                    text(str(j*3+k+1) if j<3 else ['*','0','#'][k],(x,-.033,z-.005),.011,'black')
        elif i==91:cylinder('push_button',(0,-.027,.08),(0,-.037,.08),.033,'steel')
        else:
            box('break_glass',(0,-.029,.08),(.075,.004,.07),'glass')
            text('PRESS',(0,-.033,.074),.012,'white')
    elif i==93:
        box('kickplate',(0,0,.16),(.88,.008,.32),'031',.005)
        for x in [-.4,.4]:
            for z in [.03,.29]:bolt(x,-.005,z,.006)

def sign(i):
    labels={94:'04 / 117',95:'ОТДЕЛЕНИЕ 04  >',96:'ВЫХОД',97:'ПОСТОРОННИМ ВХОД ЗАПРЕЩЁН',98:'БИООПАСНОСТЬ',99:'ВЫСОКОЕ НАПРЯЖЕНИЕ',100:'РАДИАЦИЯ',101:'ОТДЕЛЕНИЕ 04',102:'ПЛАН ЭВАКУАЦИИ',103:'РАСПИСАНИЕ ДЕЖУРСТВ',104:'ОБЪЯВЛЕНИЯ',105:''}
    w,h=(.34,.13) if i==94 else ((.5,.28) if i in (98,99,100) else (.65,.22))
    if i in (102,103,104):w,h=.75,.95
    if i==105:w,h=.25,.34
    box('sign_backing',(0,0,h/2),(w,.022,h),'031',.008)
    box('sign_face',(0,-.014,h/2),(w-.022,.006,h-.022),'white' if i>=102 else '026',.003)
    if i<=101:
        text(labels[i],(0,-.019,h*.32),min(h*.35,w/max(len(labels[i]),1)*1.2),'white')
        for x in [-w/2+.025,w/2-.025]:bolt(x,-.018,h/2,.005)
        if i==95:
            for x in [-.23,.23]:cylinder('sign_suspension',(x,0,h),(x,0,h+.3),.002,'steel',8)
    elif i in (102,103,104):
        text(labels[i],(0,-.02,h-.075),.038,'black')
        if i==102:
            for k in range(5):
                z=.17+k*.12;box('plan_wall',(0,-.02,z),(.51,.002,.008),'black',0)
                for x in [-.25,-.06,.12,.25]:box('plan_wall',(x,-.02,z+.06),(.008,.002,.12),'black',0)
            curve('escape_route',[(-.17,-.025,.2),(-.17,-.025,.38),(.05,-.025,.38),(.05,-.025,.71)],.006,'026')
            text('ВЫ ЗДЕСЬ',(-.02,-.027,.08),.024,'red')
        elif i==103:
            for k in range(8):
                z=.12+k*.085;box('schedule_rule',(0,-.02,z),(.60,.002,.002),'black',0)
                text(f'{8+k:02d}:00   WARD 04',(0,-.024,z+.018),.023,'black')
        else:
            for x,z,a in [(-.16,.63,.08),(.14,.54,-.07),(-.08,.24,.04)]:
                o=box('notice_paper',(x,-.023,z),(.26,.002,.27),'028',0);o.rotation_euler.y=a
                cylinder('pin',(x,-.025,z+.12),(x,-.032,z+.12),.006,'red')
                for k in range(5):box('text_line',(x,-.027,z-.07+k*.035),(.16,.001,.003),'black',0)
    else:
        box('document_pocket_back',(0,-.02,h/2),(w-.03,.015,h-.03),'032')
        box('document_pocket_front',(0,-.05,.1),(w-.03,.008,.18),'glass')
        for x in [-w/2+.015,w/2-.015]:box('pocket_side',(x,-.035,.1),(.008,.04,.18),'032')
        box('pocket_bottom',(0,-.035,.012),(w-.03,.04,.012),'032')

def window_detail(i):
    w,h=1.5,1.6
    if i in (106,107):
        for x in [-w/2,0,w/2]:box('window_stile',(x,0,h/2),(.065,.095,h+.06),'028',.008)
        for z in [0,h]:box('window_rail',(0,0,z),(w+.07,.095,.065),'028',.008)
        if i==107:
            pivot=bpy.data.objects.new('WindowHingePivot',None);bpy.context.collection.objects.link(pivot);pivot.location=(-w/2,0,0);parts.append(pivot)
            before=len(parts)
            for x in [-w/2+.04,-.04]:box('sash_stile',(x,-.065,h/2),(.035,.045,h-.07),'028')
            for z in [.04,h-.04]:box('sash_rail',(-w/4,-.065,z),(w/2-.07,.045,.035),'028')
            box('sash_glass',(-w/4,-.065,h/2),(w/2-.12,.008,h-.12),'glass')
            lever_handle(-.10,-.09,h/2)
            for o in parts[before:]:o.parent=pivot;o.matrix_parent_inverse=pivot.matrix_world.inverted()
            pivot.rotation_euler.z=-.35
    elif i==108:lever_handle(0,0,.15)
    elif i in (109,110):
        box('window_sill',(0,0,.025),(1.65,.34,.05),'028' if i==109 else '032',.009)
        if i==110:box('drip_edge',(0,-.17,.005),(1.65,.012,.06),'032',.002)
    elif i in (111,112,113,114):
        o=box('glass_pane',(0,0,h/2),(w,.008,h),'glass',.001)
        if i==113:
            mat=material('glass').copy();mat.name='frosted_glass';mat.node_tree.nodes.get('Principled BSDF').inputs['Roughness'].default_value=.48;o.data.materials.clear();o.data.materials.append(mat)
        if i==112:
            for k in range(20):
                x=random.uniform(-.7,.7);z=random.uniform(.2,1.5)
                curve('dirty_drip',[(x,-.005,z),(x+.005,-.005,z-random.uniform(.06,.3))],random.uniform(.001,.004),'025')
        if i==114:
            origin=(.13,-.006,.85)
            for j in range(11):
                a=j*math.tau/11
                curve('glass_fracture',[origin,(origin[0]+math.cos(a)*.25,-.006,origin[2]+math.sin(a)*.25),(max(-.74,min(.74,origin[0]+math.cos(a)*.9)),-.006,max(.01,min(1.59,origin[2]+math.sin(a)*.9)))],.0008,'white')
    elif i==115:
        for side in [-1,1]:
            for j in range(9):
                z=j*h/9;inner=side*(w/2-random.uniform(.04,.28))
                slab_polygon('remaining_shard',[(side*w/2,z),(side*w/2,z+h/9),(inner,z+random.uniform(.02,.16))],-.004,.004,'glass')
    elif i in (116,117):
        for x in [-.78,.78]:box('window_guard_frame',(x,0,.8),(.035,.05,1.65),'031')
        for z in [0,1.6]:box('window_guard_frame',(0,0,z),(1.6,.05,.035),'031')
        for j in range(12):
            x=-.72+j*.13;cylinder('window_guard_bar',(x,0,.03),(x,0,1.57),.009 if i==116 else .003,'031',12)
        for z in ([.55,1.05] if i==116 else [j*.1 for j in range(1,16)]):cylinder('cross_bar',(-.76,0,z),(.76,0,z),.009 if i==116 else .003,'031',12)
    elif i==118:
        box('blind_headrail',(0,0,1.6),(1.58,.06,.055),'028')
        for j in range(37):
            o=box('blind_slat',(0,0,.04+j*.041),(1.5,.035,.012),'028',.004);o.rotation_euler.x=.28
        for x in [-.55,.55]:cylinder('blind_lift_cord',(x,-.025,.04),(x,-.025,1.6),.0015,'white',8)
    elif i==119:
        cylinder('curtain_rod',(-.9,0,1.9),(.9,0,1.9),.013,'031')
        for x in [-.94,.94]:
            bpy.ops.mesh.primitive_uv_sphere_add(segments=16,ring_count=8,radius=.024,location=(x,0,1.9));finish(bpy.context.object,'rod_finial','steel')
        for x in [-.75,.75]:curve('rod_bracket',[(x,0,1.9),(x,.10,1.9),(x,.10,1.82)],.009,'032')
    elif i==120:
        v=[];f=[];nx,nz=90,32
        for j in range(nz+1):
            for k in range(nx+1):
                x=-.85+k*1.7/nx;z=j*1.85/nz
                y=.045*math.sin(k/nx*math.tau*12)+.009*math.sin(k/nx*math.tau*24+j*.14)
                v.append((x,y,z+.012*math.sin(k*.35)*(1-j/nz)))
        for j in range(nz):
            for k in range(nx):a=j*(nx+1)+k;f.append((a,a+1,a+nx+2,a+nx+1))
        ob=mesh('pleated_curtain',v,f,'028');mod=ob.modifiers.new('cloth_thickness','SOLIDIFY');mod.thickness=.0015
        for p in ob.data.polygons:p.use_smooth=True
        for j in range(13):torus('curtain_ring',(-.84+j*.14,0,1.87),.02,.003,'steel',(0,math.pi/2,0))

def emission(key,color,strength=2):
    mat=material(key);bs=mat.node_tree.nodes.get('Principled BSDF');bs.inputs['Emission Color'].default_value=(*color,1);bs.inputs['Emission Strength'].default_value=strength
    return key

def fluorescent(i):
    if i in (121,122,123,124,125):
        if i!=123:
            box('luminaire_backpan',(0,0,.045),(1.26,.31,.06),'028',.012)
            for y in [-.145,.145]:box('reflector_lip',(0,y,.007),(1.26,.018,.07),'032',.005)
        if i not in (122,124):
            for y in ([0] if i==123 else [-.07,.07]):
                cylinder('fluorescent_tube',(-.58,y,-.01),(.58,y,-.01),.014,emission('phosphor',(.67,.9,1),2))
                for x in [-.60,.60]:
                    cylinder('tube_endcap',(x-.015,y,-.01),(x+.015,y,-.01),.015,'032')
                    if i!=123:box('tube_holder',(x,y,.015),(.035,.04,.055),'white',.006)
        if i in (121,122):
            for j in range(12):box('louver_blade',(-.55+j*.10,0,-.035),(.009,.275,.04),'032',.002)
        if i==124:
            for x in [-.6,.6]:box('empty_tube_socket',(x,0,.006),(.035,.05,.055),'white',.005)
        if i==125:
            slab_polygon('broken_diffuser',[(-.62,0),(-.15,0),(-.25,.13),(-.1,.21),(-.3,.30),(-.62,.30)],-.045,-.044,'glass')
    elif i==126:
        # spun industrial reflector, open underside
        v=[];f=[];rings=[(.045,.42),(.07,.30),(.20,.10),(.27,.02)]
        for r,z in rings:
            for j in range(64):a=j*math.tau/64;v.append((r*math.cos(a),r*math.sin(a),z))
        for k in range(len(rings)-1):
            for j in range(64):a=k*64+j;b=k*64+(j+1)%64;f.append((a,b,b+64,a+64))
        mesh('spun_reflector',v,f,'026');cylinder('pendant_cable',(0,0,.42),(0,0,1.2),.006,'black')
        cylinder('bulb',(0,0,.09),(0,0,.19),.035,emission('warm_bulb',(1,.73,.43),3))
        torus('reflector_hem',(0,0,.02),.27,.008,'032',(0,0,0))
    elif i in (127,128,129,130):
        w,h=(.38,.12) if i==130 else (.24,.19)
        box('light_wall_mount',(0,0,h/2),(w,.055,h),'031',.018)
        box('lamp_lens',(0,-.05,h/2),(w-.025,.075,h-.025),emission('red_lens',(1,.02,.008),1.4) if i in (128,129) else emission('exit_lens',(.25,.8,.55),1.2),.025)
        if i==129:
            for z in [.035,.09,.15]:cylinder('lamp_guard',(-w/2,-.10,z),(w/2,-.10,z),.006,'031')
            for x in [-.08,.08]:curve('lamp_guard',[(x,0,0),(x,-.11,.02),(x,-.11,.17),(x,0,.19)],.006,'031')
        if i==130:text('ВЫХОД',(0,-.09,.036),.065,'white')
    elif i==131:
        cylinder('lamp_base',(0,0,0),(0,0,.025),.11,'026')
        curve('lamp_arm',[(0,0,.025),(0,0,.29),(.12,0,.45)],.012,'032')
        o=box('desk_lamp_shade',(.16,0,.46),(.27,.14,.06),'026',.03);o.rotation_euler.y=.25
        box('desk_lamp_diffuser',(.16,0,.425),(.22,.10,.008),emission('warm_bulb',(1,.73,.43),3),.008)
        cylinder('base_switch',(.06,0,.025),(.06,0,.035),.009,'black')
    elif i==132:
        box('bed_lamp_back',(0,0,.08),(.55,.06,.16),'028',.016)
        box('bed_lamp_diffuser',(0,-.05,.08),(.50,.05,.11),emission('phosphor',(.67,.9,1),2),.02)
        cylinder('pull_cord',(.20,-.04,.01),(.20,-.04,-.3),.0015,'white',8)
        cylinder('cord_handle',(.20,-.04,-.33),(.20,-.04,-.3),.008,'white')
    elif i in (133,135):
        box('floodlight_body',(0,0,.2),(.34,.14,.28),'031',.022)
        box('floodlight_reflector',(0,-.076,.2),(.30,.006,.24),'032',.015)
        box('floodlight_lens',(0,-.083,.2),(.28,.009,.22),'glass',.01)
        for x in [-.18,.18]:box('light_yoke',(x,0,.14),(.025,.04,.28),'031')
        box('light_yoke_base',(0,0,.015),(.38,.04,.03),'031')
        if i==133:
            for x in [-.2,.2]:curve('portable_stand',[(x,-.23,0),(x,.22,0),(x,.22,.1),(x,0,.15)],.012,'031')
            curve('carry_handle',[(-.1,0,.35),(-.1,0,.44),(.1,0,.44),(.1,0,.35)],.01,'032')
    elif i==134:
        cylinder('streetlight_pole',(0,0,0),(0,0,4.5),.055,'031')
        curve('streetlight_arm',[(0,0,4.5),(.1,0,4.8),(.45,0,4.9),(.8,0,4.9)],.035,'032')
        box('streetlight_head',(.8,0,4.88),(.6,.24,.13),'031',.06)
        box('streetlight_lens',(.8,0,4.81),(.47,.18,.018),'glass',.015)
        box('anchor_plate',(0,0,.025),(.28,.28,.05),'031',.008)
        for x in [-.1,.1]:
            for y in [-.1,.1]:cylinder('anchor_bolt',(x,y,.05),(x,y,.08),.012,'steel',6)

def electrical(i):
    if i in (136,137,138,139):
        count=4 if i==139 else 1;w=.082*count
        box('electrical_faceplate',(0,0,.043),(w,.016,.086),'028',.009)
        if i in (136,137):
            for x in ([0] if i==136 else [-.017,.017]):
                o=box('rocker_switch',(x,-.014,.045),(.055 if i==136 else .028,.012,.054),'white',.006);o.rotation_euler.x=.08
        else:
            for k in range(count):
                x=(k-(count-1)/2)*.082;cylinder('socket_recess',(x,-.01,.043),(x,-.018,.043),.029,'028')
                for sx in [-.0095,.0095]:cylinder('socket_contact',(x+sx,-.019,.043),(x+sx,-.021,.043),.003,'black',12)
                for z in [.019,.067]:box('earth_contact',(x,-.021,z),(.014,.003,.004),'steel',.001)
    elif i in (140,141):
        for s in [-1,1]:
            box('junction_side',(s*.075,0,.075),(.012,.07,.15),'031')
            box('junction_end',(0,0,.075+s*.069),(.14,.07,.012),'031')
        box('junction_back',(0,.035,.075),(.15,.01,.15),'031')
        if i==140:
            box('junction_lid',(0,-.043,.075),(.16,.018,.16),'032',.007)
            for x in [-.059,.059]:
                for z in [.016,.134]:bolt(x,-.054,z,.005)
        else:
            for k in range(4):curve('internal_wire',[(-.05+k*.03,.02,.03),(-.045+k*.03,-.01,.08),(-.05+k*.03,.02,.12)],.003,'red' if k==0 else 'black')
            o=box('open_junction_lid',(.14,-.04,.075),(.15,.01,.15),'032',.005);o.rotation_euler.z=.8
    elif i in (142,143):
        if i==142:box('cable_trunking',(0,0,.032),(3,.055,.065),'028',.005)
        else:
            for y in [-.14,.14]:box('tray_side',(0,y,.04),(3,.018,.08),'032')
            for k in range(40):box('tray_rung',(-1.46+k*.075,0,0),(.025,.29,.012),'032',.002)
    elif i==144:
        box('tray_bracket_back',(0,0,.15),(.06,.015,.30),'032')
        box('tray_bracket_arm',(0,-.18,.05),(.06,.36,.025),'032')
        curve('bracket_gusset',[(0,-.015,.25),(0,-.34,.05)],.008,'032')
        for z in [.05,.25]:bolt(0,-.01,z,.009)
    elif i in (145,146,147,148):
        if i==145:
            for j in range(7):curve('cable_bundle',[(x,(j-3)*.012,.08+.014*math.sin(x*2+j)) for x in [k*.08-1.5 for k in range(39)]],.004,'black')
            for x in [-1.2,-.4,.4,1.2]:box('cable_tie',(x,0,.08),(.012,.11,.02),'032')
        elif i==146:
            p=[(0,0,1.2),(.025,0,.95),(-.02,.01,.63),(.03,.02,.31),(.08,0,.04)]
            curve('dangling_wire',p,.008,'black')
            for j in range(5):curve('exposed_copper',[(.08+j*.003,0,.045),(.09+j*.006,0,.01)],.001,'wood')
        elif i==147:
            box('extension_strip',(0,0,.025),(.30,.065,.05),'028',.012)
            for x in [-.09,0,.09]:
                for sx in [-.0095,.0095]:cylinder('socket_hole',(x+sx,0,.051),(x+sx,0,.052),.003,'black',12)
            curve('extension_cord',[(.15,0,.02),(.28,.05,.02),(.42,.16,.02),(.55,.14,.02),(.63,.02,.02)],.005,'black')
            box('power_plug',(.64,0,.025),(.05,.035,.035),'black',.008)
        else:
            for y in [-.10,.10]:cylinder('cable_spool_flange',(0,y-.01,.22),(0,y+.01,.22),.20,'026')
            cylinder('spool_hub',(0,-.1,.22),(0,.1,.22),.08,'black')
            for j in range(30):torus('wound_cable',(0,-.087+j*.006,.22),.145,.005,'black')
            curve('spool_stand',[(-.18,-.15,0),(-.18,.15,0),(0,.16,.22),(.18,.15,0),(.18,-.15,0)],.012,'031')
    elif i in (149,150,151,152,153,154,155):
        if i in (149,154,155):
            w,h,d=(.55,.8,.18) if i==149 else (.9,1.9,.45)
            for s in [-1,1]:
                box('electrical_cabinet_side',(s*w/2,0,h/2),(.025,d,h),'031')
                box('electrical_cabinet_end',(0,0,h/2+s*h/2),(w,d,.025),'031')
            box('cabinet_back',(0,d/2,h/2),(w,.025,h),'032')
            box('internal_mounting_board',(0,0,h/2),(w-.06,.018,h-.08),'032')
            for j in range(3):
                z=h*.2+j*h*.28
                box('DIN_rail',(0,-.02,z),(w-.1,.025,.03),'032')
                for k in range(5):
                    x=(k-2)*w*.14;box('breaker',(x,-.055,z),(.055,.05,.10),'white',.004);box('breaker_toggle',(x,-.086,z),(.025,.018,.03),'black',.003)
            pivot=bpy.data.objects.new('ElectricalDoorPivot',None);bpy.context.collection.objects.link(pivot);pivot.location=(-w/2,-d/2,0);parts.append(pivot)
            before=len(parts);box('electrical_cabinet_door',(0,-d/2-.015,h/2),(w,.025,h),'026',.012);pull_handle(w*.35,-d/2-.08,h*.5,.13)
            for o in parts[before:]:o.parent=pivot;o.matrix_parent_inverse=pivot.matrix_world.inverted()
            pivot.rotation_euler.z=-.60
        elif i==150:
            box('breaker_body',(0,0,.055),(.018,.075,.11),'white',.003);box('breaker_toggle',(0,-.047,.06),(.012,.02,.025),'black',.003)
            for z in [.02,.09]:bolt(0,-.039,z,.003)
        elif i in (151,152):
            count=3 if i==151 else 1
            for k in range(count):
                x=k*.055;cylinder('ceramic_fuse',(x,0,.015),(x,0,.09),.015,'white')
                for z in [.015,.09]:cylinder('fuse_cap',(x,0,z-.007),(x,0,z+.007),.016,'032')
                if i==151:box('fuse_mount',(x,.025,.05),(.04,.025,.12),'black')
        else:
            box('isolator_back',(0,0,.16),(.19,.06,.32),'031',.012)
            cylinder('switch_spindle',(0,-.035,.16),(0,-.08,.16),.017,'steel')
            curve('isolator_lever',[(0,-.08,.16),(0,-.11,.31)],.014,'032')
            cylinder('isolator_grip',(0,-.11,.29),(0,-.11,.39),.023,'black')

def flange(center,axis='Z',radius=.08):
    c=Vector(center);d=Vector((0,0,.012) if axis=='Z' else ((.012,0,0) if axis=='X' else (0,.012,0)))
    cylinder('pipe_flange',c-d,c+d,radius,'031')
    for j in range(8):
        a=j*math.tau/8
        q=c+Vector((math.cos(a)*radius*.78,math.sin(a)*radius*.78,0) if axis=='Z' else ((0,math.cos(a)*radius*.78,math.sin(a)*radius*.78) if axis=='X' else (math.cos(a)*radius*.78,0,math.sin(a)*radius*.78)))
        cylinder('flange_bolt',q-d*1.4,q+d*1.4,.007,'steel',6)

def engineering(i):
    if i in (156,157,158):
        r={156:.035,157:.025,158:.065}[i];mat='031' if i!=157 else '032'
        cylinder('straight_pipe',(0,0,0),(0,0,3),r,mat)
        for z in [.12,2.88]:
            cylinder('socket_collar',(0,0,z-.035),(0,0,z+.035),r*1.22,mat)
            torus('collar_seam',(0,0,z+.03),r*1.23,.002,'black',(0,0,0))
    elif i==159:
        curve('pipe_elbow',[(.12*math.cos(a),0,.12*math.sin(a)) for a in [j*math.pi/2/24 for j in range(25)]],.035,'031')
        flange((.12,0,0),'Z',.065);flange((0,0,.12),'X',.065)
    elif i==160:
        cylinder('tee_trunk',(-.2,0,.2),(.2,0,.2),.035,'031');cylinder('tee_branch',(0,0,.2),(0,0,.4),.035,'031')
        for x in [-.2,.2]:flange((x,0,.2),'X',.065)
        flange((0,0,.4),'Z',.065)
    elif i==161:flange((0,0,.02),'Z',.11);cylinder('short_pipe',(0,0,-.08),(0,0,.1),.055,'031')
    elif i==162:
        torus('pipe_clamp',(0,0,.06),.041,.005,'032',(0,0,0));box('clamp_back',(0,.06,.06),(.08,.02,.09),'032')
        for x in [-.03,.03]:cylinder('clamp_bolt',(x,.04,.06),(x,.08,.06),.005,'steel',6)
    elif i in (163,164):
        if i==163:
            cylinder('valve_body',(-.18,0,.16),(.18,0,.16),.062,'031')
            for x in [-.18,.18]:flange((x,0,.16),'X',.09)
            cylinder('valve_bonnet',(0,0,.16),(0,0,.28),.04,'031')
            cylinder('valve_stem',(0,0,.28),(0,0,.39),.014,'steel')
        z=.40 if i==163 else .025
        torus('valve_handwheel',(0,0,z),.12,.012,'red',(0,0,0))
        for j in range(5):
            a=j*math.tau/5;cylinder('wheel_spoke',(0,0,z),(.115*math.cos(a),.115*math.sin(a),z),.007,'031')
        cylinder('wheel_hub',(0,0,z-.012),(0,0,z+.012),.022,'steel',6)
    elif i==165:
        cylinder('gauge_case',(0,.02,.10),(0,-.025,.10),.09,'032')
        cylinder('gauge_dial',(0,-.027,.1),(0,-.029,.1),.079,'white')
        for j in range(31):
            a=math.radians(-45+j*9);r=.066
            cylinder('gauge_tick',(math.cos(a)*r,-.031,.1+math.sin(a)*r),(math.cos(a)*(r-.008),-.031,.1+math.sin(a)*(r-.008)),.0008,'black',6)
        curve('gauge_needle',[(0,-.034,.1),(-.045,-.034,.135)],.0018,'red');text('bar',(0,-.035,.07),.018,'black')
        cylinder('gauge_connection',(0,0,0),(0,0,.03),.012,'steel',6)
    elif i in (166,167):
        if i==166:
            for j in range(12):
                x=-.55+j*.10;box('radiator_section',(x,0,.34),(.075,.16,.60),'028',.033)
                for z in [.09,.59]:cylinder('section_nipple',(x-.06,0,z),(x+.06,0,z),.021,'031')
            cylinder('bleed_valve',(.62,0,.59),(.66,0,.59),.012,'steel',6)
        else:
            for x in [-.65,.65]:box('radiator_cover_side',(x,0,.36),(.025,.23,.72),'028')
            box('radiator_cover_top',(0,0,.72),(1.32,.25,.025),'028')
            for j in range(50):box('cover_vertical_slat',(-.62+j*.025,-.12,.36),(.011,.014,.66),'028',.003)
    elif i in (168,169,170):
        if i==169:
            cylinder('round_duct',(0,0,0),(0,0,3),.18,'032')
            for j in range(12):torus('duct_seam',(0,0,j*.25),.181,.003,'032',(0,0,0))
        elif i==168:
            box('rectangular_duct',(0,0,1.5),(.5,.35,3),'032',.008)
            for z in [.04,1.5,2.96]:
                for s in [-1,1]:
                    box('duct_flange',(s*.26,0,z),(.025,.39,.035),'032')
                    box('duct_flange',(0,s*.185,z),(.54,.025,.035),'032')
        else:
            # elbow comprised of open sheet-metal bend, not a plugged cube
            v=[];f=[]
            for y in [-.175,.175]:
                for r in [.2,.7]:
                    for j in range(25):a=j*math.pi/2/24;v.append((r*math.cos(a),y,r*math.sin(a)))
            for ring in [0,2]:
                for j in range(24):a=ring*25+j;f.append((a,a+1,a+26,a+25))
            for r in [0,1]:
                for j in range(24):a=r*25+j;f.append((a,a+1,a+51,a+50))
            mesh('duct_elbow',v,f,'032')
    elif i in (171,172,173):
        for s in [-1,1]:
            box('vent_frame',(s*.22,0,.22),(.025,.06,.46),'032')
            box('vent_frame',(0,0,.22+s*.22),(.46,.06,.025),'032')
        if i in (171,172):
            for j in range(10):o=box('vent_louver',(0,0,.03+j*.043),(.41,.055,.014),'032',.002);o.rotation_euler.x=.5
            if i==172:
                cylinder('damper_spindle',(-.25,0,.22),(.25,0,.22),.008,'steel');curve('damper_lever',[(.25,0,.22),(.25,-.08,.32)],.009,'red')
        else:
            cylinder('fan_hub',(0,.01,.22),(0,-.04,.22),.04,'031')
            for j in range(6):
                a=j*math.tau/6
                ob=slab_polygon('fan_blade',[(.04,0),(.17,-.035),(.20,.035),(.07,.075)],-.01,.01,'032');ob.location=(0,0,.22);ob.rotation_euler.y=a
            for r in [.08,.12,.16,.20]:torus('fan_guard',(0,-.05,.22),r,.0025,'031')
    elif i==174:
        box('air_handler_base',(0,0,.08),(2.3,.95,.16),'031',.015)
        for x in [-.76,0,.76]:
            box('AHU_service_module',(x,0,.67),(.74,.9,1.06),'032',.018)
            box('AHU_access_panel',(x,-.465,.67),(.65,.025,.9),'026',.012)
            for hx in [x-.26,x+.26]:
                for z in [.29,1.05]:bolt(hx,-.482,z,.008)
            pull_handle(x+.22,-.52,.69,.14)
        for x in [-1.22,1.22]:cylinder('duct_connection',(x-.08,0,.7),(x+.08,0,.7),.23,'032')

def wheel(x,y,z=.085,r=.075):
    cylinder('castor_tire',(x,y-.018,z),(x,y+.018,z),r,'rubber')
    for s in [-1,1]:cylinder('wheel_hub',(x,y+s*.019,z),(x,y+s*.023,z),r*.44,'032')
    for s in [-1,1]:box('castor_fork',(x,y+s*.027,z+.035),(.035,.012,.09),'032',.007)
    cylinder('swivel_stem',(x,y,z+.075),(x,y,z+.13),.012,'steel')

def cart_base(w=.6,l=.45,z=.17):
    box('cart_chassis',(0,0,z),(w,l,.045),'031',.015)
    for x in [-w*.40,w*.40]:
        for y in [-l*.38,l*.38]:wheel(x,y)

def softbox(name,loc,size,mat='fabric_dirty',radius=.06):
    ob=box(name,loc,size,mat,min(radius,min(size)*.45))
    for mod in ob.modifiers:
        if mod.type=='BEVEL':mod.segments=8
    for p in ob.data.polygons:p.use_smooth=True
    return ob

def cloth(name,w,l,z,mat='fabric_dirty',drape=.0):
    v=[];f=[];nx,ny=64,86
    for j in range(ny+1):
        for k in range(nx+1):
            x=(k/nx-.5)*w;y=(j/ny-.5)*l
            sag=max(0,(abs(x)-w*.34)/(w*.16))*drape
            height=z-sag+.013*math.sin(x*33+y*15)+.007*math.sin(x*64-y*39)+.007*math.cos(y*12)
            v.append((x,y,height))
    for j in range(ny):
        for k in range(nx):a=j*(nx+1)+k;f.append((a,a+1,a+nx+2,a+nx+1))
    ob=mesh(name,v,f,mat)
    for p in ob.data.polygons:p.use_smooth=True
    mod=ob.modifiers.new('cloth_thickness','SOLIDIFY');mod.thickness=.002
    return ob

def hospital_bed(reinforced=False):
    w,l=.94,2.10
    for x in [-.45,.45]:
        cylinder('bed_side_rail',(x,-.98,.66),(x,.98,.66),.027,'028')
        for y in [-.93,.93]:
            cylinder('bed_leg',(x,y,.16),(x,y,1.05),.028,'028');wheel(x,y)
    for y in [-.90,.90]:cylinder('bed_end_crossmember',(-.45,y,.64),(.45,y,.64),.025,'031')
    # Open under-bed space is deliberately preserved.
    for j in range(19):box('mattress_support',(0,-.90+j*.10,.675),(.88,.024,.012),'031',.003)
    for y,top in [(-.98,.98),(.98,1.13)]:
        curve('rounded_bed_end',[(-.45,y,.68),(-.45,y,top-.1),(-.43,y,top-.03),(-.36,y,top),(.36,y,top),(.43,y,top-.03),(.45,y,top-.1),(.45,y,.68)],.026,'028')
        for x in [-.30,-.15,0,.15,.30]:cylinder('bed_end_spindle',(x,y,.73),(x,y,top-.03),.012,'028')
    if reinforced:
        for x in [-.45,.45]:
            box('restraint_mount',(x,.55,.70),(.05,.11,.025),'031')
            box('restraint_mount',(x,-.55,.70),(.05,.11,.025),'031')

def cabinet(w=.8,d=.42,h=1.8,glass=False,drawers=0):
    for s in [-1,1]:box('cabinet_side',(s*(w/2-.012),0,h/2),(.025,d,h),'028',.006)
    box('cabinet_back',(0,d/2-.01,h/2),(w,.02,h),'026')
    for z in [.1,h]:box('cabinet_horizontal',(0,0,z),(w,d,.025),'028')
    for x in [-w*.38,w*.38]:
        for y in [-d*.34,d*.34]:box('cabinet_foot',(x,y,.055),(.045,.045,.11),'031')
    if drawers:
        for j in range(drawers):
            z=.16+(j+.5)*(h-.2)/drawers
            box('drawer_front',(0,-d/2-.015,z),(w-.04,.03,(h-.2)/drawers-.012),'026',.007)
            curve('drawer_pull',[(-.08,-d/2-.03,z),(-.08,-d/2-.065,z),(.08,-d/2-.065,z),(.08,-d/2-.03,z)],.008,'steel')
    else:
        for s in [-1,1]:
            pivot=bpy.data.objects.new('CabinetDoorPivot',None);bpy.context.collection.objects.link(pivot);pivot.location=(s*(w/2-.03),-d/2,0);parts.append(pivot)
            before=len(parts);cx=s*w/4
            if glass:
                for x in [cx-w/4+.02,cx+w/4-.02]:box('glazed_door_stile',(x,-d/2,h*.55),(.035,.025,h*.86),'028')
                for z in [.13,h-.025]:box('glazed_door_rail',(cx,-d/2,z),(w/2-.02,.025,.035),'028')
                box('cabinet_glass',(cx,-d/2,h*.55),(w/2-.075,.008,h*.86-.05),'glass',.001)
            else:box('cabinet_door',(cx,-d/2,h*.55),(w/2-.025,.025,h*.86),'026',.008)
            pull_handle(s*.045,-d/2-.06,h*.55,.16)
            for ob in parts[before:]:ob.parent=pivot;ob.matrix_parent_inverse=pivot.matrix_world.inverted()
        for z in [h*.3,h*.52,h*.74]:box('cabinet_shelf',(0,0,z),(w-.04,d-.025,.012),'032' if glass else '028')

def medical(i):
    if i==175:hospital_bed()
    elif i==176:
        softbox('mattress',(0,0,.065),(.88,1.96,.13),radius=.06)
        for z in [.018,.112]:curve('mattress_piping',[(-.40,-.93,z),(.40,-.93,z),(.43,-.9,z),(.43,.9,z),(.4,.93,z),(-.4,.93,z),(-.43,.9,z),(-.43,-.9,z),(-.4,-.93,z)],.003,'028')
        for x in [-.25,.25]:
            for y in [-.65,0,.65]:cylinder('tuft_button',(x,y,.131),(x,y,.133),.009,'028',12)
    elif i==177:softbox('pillow',(0,0,.055),(.58,.38,.11),radius=.052)
    elif i in (178,179,180):cloth('bed_linen',1.28 if i==180 else .94,1.96,.28 if i==180 else .07,'fabric_blue' if i==179 else 'fabric_dirty',.26 if i==180 else .045)
    elif i==181:cabinet(.48,.43,.70,drawers=3)
    elif i==182:
        cart_base(.65,.42);cylinder('table_column',(-.25,0,.18),(-.25,0,.95),.025,'032')
        box('overbed_tabletop',(.1,0,.98),(.84,.43,.035),'wood',.018)
        box('table_height_lock',(-.25,-.04,.7),(.08,.05,.08),'black',.01)
    elif i in (183,300):
        cart_base(.70,1.8)
        for x in [-.30,.30]:
            for y in [-.73,.73]:cylinder('trolley_upright',(x,y,.16),(x,y,.8),.025,'032')
            cylinder('trolley_rail',(x,-.88,.82),(x,.88,.82),.02,'032')
        softbox('trolley_pad',(0,0,.80),(.65,1.90,.09),'fabric_blue',.04)
        for y in [-.96,.96]:curve('trolley_push_bar',[(-.32,y,.72),(-.32,y,.95),(.32,y,.95),(.32,y,.72)],.02,'032')
    elif i==184:
        for x in [-.32,.32]:
            torus('wheelchair_main_tire',(x,.12,.30),.278,.024,'rubber',(0,math.pi/2,0))
            torus('wheelchair_rim',(x,.12,.30),.255,.008,'032',(0,math.pi/2,0))
            cylinder('wheelchair_axle',(x-.025,.12,.3),(x+.025,.12,.3),.023,'steel')
            torus('wheel_push_rim',(x*1.12,.12,.30),.255,.009,'steel',(0,math.pi/2,0))
            for j in range(20):
                a=j*math.tau/20;cylinder('wheel_spoke',(x,.12,.3),(x,.12+math.cos(a)*.28,.3+math.sin(a)*.28),.0017,'steel',6)
            curve('wheelchair_frame',[(x,.18,.8),(x,.18,.36),(x,-.28,.20),(x,-.33,.10)],.018,'032')
            curve('wheelchair_push_handle',[(x,.18,.60),(x,.18,.91),(x,.35,.91)],.016,'032')
            cylinder('push_handle_grip',(x,.24,.91),(x,.35,.91),.022,'rubber')
            wheel(x,-.34,.07,.055)
            cylinder('armrest_support',(x,-.21,.45),(x,-.21,.70),.012,'032')
            softbox('armrest',(x,-.04,.71),(.055,.39,.04),'rubber',.02)
            box('footrest',(x,-.5,.10),(.18,.23,.018),'black',.012)
        softbox('wheelchair_seat',(0,-.04,.46),(.49,.45,.045),'rubber',.02)
        softbox('wheelchair_back',(0,.18,.69),(.49,.04,.45),'fabric_blue',.02)
    elif i in (185,208,209):
        cart_base(.65,1.5)
        for y in [-.5,.5]:cylinder('couch_lift_column',(0,y,.2),(0,y,.69),.045,'032')
        if i==185:
            softbox('medical_chair_seat',(0,-.12,.69),(.60,.56,.09),'fabric_blue',.04)
            ob=softbox('medical_chair_back',(0,.30,1.0),(.60,.10,.66),'fabric_blue',.04);ob.rotation_euler.x=-.22
            ob=softbox('leg_support',(0,-.6,.55),(.58,.62,.09),'fabric_blue',.04);ob.rotation_euler.x=.25
        else:
            for y,l,z,angle in [(-.48,.70,.75,0),(.12,.48,.75,0),(.62,.48,.82,-.20)]:
                ob=softbox('sectioned_medical_pad',(0,y,z),(.64,l,.10),'fabric_blue' if i==208 else 'rubber',.03);ob.rotation_euler.x=angle
        for x in [-.38,.38]:cylinder('accessory_rail',(x,-.65,.71),(x,.7,.71),.013,'032')
    elif i==186:
        for j in range(5):
            a=j*math.tau/5;x,y=math.cos(a)*.31,math.sin(a)*.31;cylinder('IV_base_leg',(0,0,.16),(x,y,.16),.015,'032');wheel(x,y,.055,.04)
        cylinder('IV_pole',(0,0,.15),(0,0,1.95),.012,'032')
        for s in [-1,1]:curve('IV_hook',[(0,0,1.95),(s*.16,0,1.95),(s*.19,0,1.91),(s*.16,0,1.88)],.005,'steel')
        cylinder('height_collar',(0,0,.98),(0,0,1.04),.019,'black')
    elif i==187:
        softbox('IV_bag',(0,0,.17),(.14,.035,.29),'glass',.015)
        box('IV_label',(0,-.02,.19),(.105,.002,.12),'028',.003);text('NaCl 0.9%',(0,-.023,.19),.016,'black')
        cylinder('IV_drip_chamber',(0,0,-.05),(0,0,.02),.012,'glass')
        curve('hanging_loop',[(-.03,0,.31),(-.03,0,.34),(.03,0,.34),(.03,0,.31)],.004,'white')
    elif i==188:curve('IV_tube',[(0,0,.9),(.02,0,.65),(-.03,.02,.30),(.07,.03,.05),(.21,.02,.01)],.002,'glass')
    elif i in (189,190,191):
        if i==191:
            curve('ceiling_curtain_track',[(-1,0,0),(1,0,0),(1.12,.05,0),(1.17,.17,0),(1.17,1.0,0)],.012,'032')
            for x in [-.8,0,.8]:cylinder('track_suspension',(x,0,0),(x,0,.25),.004,'steel')
        else:
            count=3 if i==189 else 1
            for panel in range(count):
                x=(panel-(count-1)/2)*.65
                before=len(parts);window_detail(120)
                for ob in parts[before:]:ob.scale.x=.37 if i==189 else 1;ob.location.x+=x
                if i==189:
                    for xx in [x-.31,x+.31]:
                        cylinder('screen_upright',(xx,0,.16),(xx,0,1.98),.014,'032')
                        cylinder('screen_foot',(xx,-.20,.15),(xx,.20,.15),.013,'032')
                        for yy in [-.18,.18]:wheel(xx,yy,.055,.04)
    elif i in (192,193,194,195,211):
        cart_base(.62,.43)
        if i==192:
            cabinet(.59,.4,.85,drawers=4)
        elif i in (194,195):
            for x in [-.28,.28]:
                for y in [-.18,.18]:cylinder('linen_cart_post',(x,y,.18),(x,y,1.0),.014,'032')
            for z in [.25,.9]:box('linen_cart_shelf',(0,0,z),(.59,.40,.018),'032')
            if i==194:softbox('linen_bag',(0,0,.57),(.54,.37,.60),'fabric_dirty',.08)
            else:
                for j in range(5):softbox('folded_linen',(0,0,.3+j*.055),(.46,.32,.05),'fabric_dirty',.015)
        else:
            for x in [-.28,.28]:
                for y in [-.18,.18]:cylinder('cart_post',(x,y,.17),(x,y,.88),.013,'032')
            for z in [.28,.87]:
                box('instrument_shelf',(0,0,z),(.61,.42,.016),'032')
                for y in [-.20,.20]:cylinder('shelf_guard',(-.30,y,z+.04),(.30,y,z+.04),.007,'032')
        curve('cart_push_handle',[(-.30,.22,.82),(-.30,.30,.93),(.30,.30,.93),(.30,.22,.82)],.013,'032')
    elif i in (196,197,198):
        if i==196:
            cylinder('oxygen_cylinder',(0,0,.13),(0,0,1.03),.115,'026',48)
            for z in [.13,1.03]:
                bpy.ops.mesh.primitive_uv_sphere_add(segments=32,ring_count=16,radius=.115,location=(0,0,z));ob=finish(bpy.context.object,'bottle_shoulder','026');ob.scale.z=.7
            cylinder('bottle_valve',(0,0,1.1),(0,0,1.23),.022,'steel',6)
            torus('oxygen_valve_wheel',(0,0,1.22),.044,.008,'032',(0,0,0))
            box('cylinder_label',(0,-.117,.64),(.14,.002,.22),'028');text('OXYGEN',(0,-.12,.72),.025,'black')
        elif i==197:
            for x in [-.14,.14]:
                wheel(x,.04,.10,.09);curve('bottle_trolley_frame',[(x,-.15,.13),(x,.08,.13),(x,.08,1.15),(x,.03,1.25)],.016,'031')
            box('cylinder_trolley_tray',(0,-.06,.13),(.36,.30,.02),'031')
            for z in [.45,.85]:curve('bottle_retaining_band',[(-.14,.05,z),(-.14,-.13,z),(0,-.18,z),(.14,-.13,z),(.14,.05,z)],.009,'032')
        else:
            cylinder('regulator_body',(-.06,0,.12),(.06,0,.12),.035,'steel',6)
            before=len(parts);engineering(165)
            for ob in parts[before:]:ob.scale*=.6;ob.location.z+=.15
            cylinder('regulator_outlet',(.06,0,.12),(.12,0,.12),.012,'steel',6)
    elif i==199:
        box('medical_gas_panel',(0,0,.12),(.85,.07,.24),'028',.015)
        for j in range(3):
            x=(j-1)*.24;cylinder('gas_socket',(x,-.04,.12),(x,-.06,.12),.037,'032')
            torus('coded_socket_ring',(x,-.061,.12),.031,.005,['026','red','amber'][j])
            text(['O2','VAC','AIR'][j],(x,-.038,.18),.022,'black')
    elif i in (200,201,202,203,204,205):
        dims={200:(.37,.16,.30),201:(.14,.14,.22),202:(.42,.32,.44),203:(.35,.26,.20),204:(.34,.27,.13),205:(.35,.24,.30)}[i];w,d,h=dims
        box('medical_device_case',(0,0,h/2),(w,d,h),'028',.025)
        if i in (200,202,203):
            box('display_bezel',(-w*.08,-d/2-.008,h*.62),(w*.67,.018,h*.54),'black',.008)
            box('display_glass',(-w*.08,-d/2-.020,h*.62),(w*.60,.006,h*.46),'glass',.005)
            if i==200:
                for z in [h*.54,h*.67]:curve('ECG_trace',[(x,-d/2-.025,z+.008*math.sin(x*60)+(.025 if .0<x<.012 else 0)) for x in [-w*.35+j*w*.52/90 for j in range(91)]],.0008,emission('monitor_trace',(.1,.8,.35),.6))
        for j in range(3):cylinder('control_knob',(w*.35,-d/2-.01,h*(.25+j*.22)),(w*.35,-d/2-.024,h*(.25+j*.22)),.012,'032')
        for j in range(7):box('device_vent',(w/2+.001,0,h*.2+j*.012),(.001,d*.6,.004),'black',0)
        if i==202:
            curve('ventilator_hose',[(w*.35,0,h*.25),(.35,-.15,.12),(.43,-.2,.02),(.50,-.17,.18),(.5,-.1,.35)],.018,'028')
            for j in range(12):torus('hose_corrugation',(.5,-.1,.18+j*.014),.019,.002,'032',(0,0,0))
        if i==203:
            for x in [-.22,.22]:box('defibrillator_paddle',(x,0,.08),(.09,.16,.05),'black',.018);curve('paddle_cable',[(x,.04,.1),(x,.16,.1),(0,.18,.07)],.004,'black')
        if i==204:box('paper_trace',(0,-.20,.04),(.18,.20,.002),'028',0)
        if i==205:
            for x in [-.1,.1]:cylinder('suction_jar',(x,-.02,h), (x,-.02,h+.20),.043,'glass');cylinder('jar_lid',(x,-.02,h+.20),(x,-.02,h+.22),.047,'028')
    elif i in (206,207):
        if i==206:
            box('scale_platform',(0,0,.04),(.38,.4,.08),'028',.025)
            box('scale_grip',(0,0,.084),(.32,.28,.01),'rubber',.015)
            cylinder('scale_column',(0,.15,.08),(0,.15,1.0),.025,'032')
            box('scale_display',(0,.12,1.04),(.22,.14,.11),'028',.02)
        else:
            box('height_measure',(0,0,1.05),(.055,.025,2.1),'028')
            for j in range(21):box('height_tick',(.01,-.015,j*.1),(.03,.002,.002),'black',0)
            box('head_slide',(0,-.15,1.7),(.22,.30,.018),'032')
    elif i==210:
        cylinder('surgical_lamp_stem',(0,0,1.0),(0,0,1.9),.025,'032')
        curve('lamp_articulated_arm',[(0,0,1.0),(.5,0,1.0),(.8,.25,.8)],.026,'028')
        cylinder('surgical_light_head',(.8,.25,.74),(.8,.25,.87),.31,'028',64)
        for j in range(6):
            a=j*math.tau/6;x=.8+math.cos(a)*.18;y=.25+math.sin(a)*.18;cylinder('surgical_reflector',(x,y,.729),(x,y,.742),.062,'glass')
        cylinder('sterile_handle',(.8,.25,.62),(.8,.25,.74),.018,'032')
    elif i==212:
        box('medical_tray_base',(0,0,.005),(.32,.20,.01),'032',.02)
        for y in [-.10,.10]:box('tray_lip',(0,y,.018),(.32,.008,.035),'032',.004)
        for x in [-.16,.16]:box('tray_lip',(x,0,.018),(.008,.20,.035),'032',.004)
    elif i in (213,214,215,216):
        if i in (213,215):
            for s in [-1,1]:
                torus('instrument_finger_ring',(s*.021,0,.026),.018,.003,'steel')
                cylinder('instrument_shank',(s*.021,0,.04),(-s*.012,0,.16),.003,'steel')
                cylinder('instrument_blade',(-s*.012,0,.16),(-s*.009,0,.22),.002,'steel')
            cylinder('instrument_pivot',(0,-.003,.10),(0,.003,.10),.004,'steel')
        elif i==214:
            for s in [-1,1]:curve('tweezer_arm',[(0,0,0),(s*.009,0,.06),(s*.003,0,.15)],.002,'steel')
        else:
            box('scalpel_handle',(0,0,.06),(.012,.006,.12),'steel',.004)
            slab_polygon('scalpel_blade',[(-.006,.12),(.014,.12),(.012,.16),(-.003,.18)],-.001,.001,'steel')
    elif i==217:
        cylinder('syringe_barrel',(0,0,.04),(0,0,.13),.009,'glass');cylinder('plunger',(0,0,.13),(0,0,.18),.004,'028');cylinder('needle',(0,0,0),(0,0,.04),.0006,'steel',8)
        box('finger_flange',(0,0,.13),(.04,.012,.003),'white');box('plunger_pad',(0,0,.18),(.025,.016,.003),'white')
        for j in range(9):box('graduation',(.004,-.009,.05+j*.008),(.008,.001,.0005),'black',0)
    elif i in (218,221,222):
        size=(.20,.1,.002) if i==218 else ((.12,.12,.012) if i==221 else (.22,.12,.08))
        box('medical_pack',(0,0,size[2]/2),size,'028',.005)
        text('STERILE' if i!=222 else 'GLOVES',(0,-size[1]/2-.001,size[2]*.4),min(.025,size[2]*.4),'026')
    elif i in (219,220):
        cylinder('bandage_roll',(0,0,0),(0,0,.06 if i==219 else .025),.04 if i==219 else .035,'fabric_dirty' if i==219 else '028')
        cylinder('roll_core',(0,0,.0601 if i==219 else .0251),(0,0,.061 if i==219 else .026),.018,'wood')
    elif i==223:
        softbox('medical_mask',(0,0,.045),(.17,.018,.09),'fabric_blue',.014)
        for j in range(3):box('mask_pleat',(0,-.012,.022+j*.022),(.15,.004,.003),'fabric_blue',.002)
        for s in [-1,1]:curve('ear_loop',[(s*.075,0,.08),(s*.12,.01,.075),(s*.125,.01,.015),(s*.075,0,.01)],.0015,'white')
    elif i==224:
        cloth('medical_gown_body',.72,1.05,.06,'fabric_blue',.05)
        for s in [-1,1]:
            ob=softbox('gown_sleeve',(s*.48,.33,.06),(.40,.22,.035),'fabric_blue',.015);ob.rotation_euler.z=s*.18
            curve('gown_tie',[(s*.12,.52,.07),(s*.18,.65,.06),(s*.25,.73,.02)],.003,'white')
    elif i in (225,226,270):
        w,d,h=(.18,.15,.25) if i==225 else ((.40,.34,.65) if i==226 else (.30,.27,.42))
        # hollow receptacle with separate lid
        for s in [-1,1]:
            box('bin_side',(s*w/2,0,h/2),(.01,d,h),'amber' if i==225 else '031')
            box('bin_end',(0,s*d/2,h/2),(w,.01,h),'amber' if i==225 else '031')
        box('bin_bottom',(0,0,.01),(w,d,.02),'031')
        box('bin_lid',(0,0,h+.012),(w+.025,d+.025,.025),'red' if i!=270 else '031',.012)
        if i!=270:text('BIOHAZARD',(0,-d/2-.008,h*.6),w*.10,'black')
    elif i in (227,228,229):
        w,h,d=(.12,.22,.10) if i!=229 else (.27,.25,.12)
        box('dispenser_back',(0,0,h/2),(w,d,h),'028',.025)
        if i!=229:
            box('dispenser_window',(0,-d/2-.006,h*.5),(.045,.008,h*.5),'glass',.008)
            box('dispenser_lever',(0,-d/2-.028,.04),(w*.65,.04,.06),'032',.012)
            cylinder('nozzle',(0,-.03,-.01),(0,-.03,.01),.006,'black')
        else:box('paper_towel',(0,-.03,-.03),(.18,.002,.12),'028',.001)
    elif i in (230,275,303,399):
        w,d=.60,.45
        box('sink_back',(0,.18,.18),(w,.06,.36),'028',.04)
        for s in [-1,1]:
            box('sink_rim_side',(s*.275,0,.14),(.05,d,.05),'028',.02)
            box('sink_rim_end',(0,s*.205,.14),(.52,.04,.05),'028',.015)
        # basin represented with a smooth concave surface, rather than a flat filled cube
        v=[(0,0,.025)];f=[]
        for k in range(1,17):
            r=k/16
            for j in range(64):a=j*math.tau/64;v.append((.24*r*math.cos(a),.16*r*math.sin(a),.025+.14*r*r))
        for j in range(64):f.append((0,j+1,(j+1)%64+1))
        for k in range(15):
            for j in range(64):a=1+k*64+j;b=1+k*64+(j+1)%64;f.append((a,b,b+64,a+64))
        ob=mesh('washbasin_interior',v,f,'white')
        for p in ob.data.polygons:p.use_smooth=True
        cylinder('sink_drain',(0,0,.026),(0,0,.028),.022,'steel')
        curve('sink_trap',[(0,0,.02),(0,0,-.16),(0,.1,-.22),(0,.17,-.15)],.025,'032')
    elif i==231:
        curve('gooseneck_faucet',[(0,0,0),(0,0,.2),(0,-.035,.25),(0,-.13,.25),(0,-.17,.2)],.015,'steel')
        cylinder('faucet_base',(0,0,0),(0,0,.025),.035,'steel')
        curve('elbow_tap_lever',[(.02,0,.05),(.16,0,.07),(.22,0,.10)],.008,'steel')

def office(i):
    if i in (232,233,234,274):
        w,d,h=(2.2,.7,1.12) if i==232 else (1.4,.65,.75)
        box('desk_top',(0,0,h),(w,d,.045),'wood' if i!=274 else '032',.016)
        if i==232:
            box('reception_front',(0,-d/2,h/2),(w,.05,h),'026',.012)
            box('counter_shelf',(0,-d*.20,.77),(w,d*.65,.04),'wood',.016)
        else:
            for x in [-w*.43,w*.43]:
                for y in [-d*.39,d*.39]:box('desk_leg',(x,y,h/2),(.04,.04,h),'031',.006)
            if i==234:
                before=len(parts);cabinet(.38,.5,.68,drawers=3)
                for o in parts[before:]:o.location.x+=w*.29
    elif i in (235,236,237,238):
        count=3 if i in (237,238) else 1
        for j in range(count):
            x=(j-(count-1)/2)*.56
            softbox('seat',(x,0,.46),(.48,.45,.065),'fabric_blue' if i==235 else '028',.027)
            ob=softbox('backrest',(x,.19,.74),(.46,.055,.50),'fabric_blue' if i==235 else '028',.025);ob.rotation_euler.x=-.10
            if i!=235:
                for xx in [x-.20,x+.20]:
                    for y in [-.17,.17]:cylinder('chair_leg',(xx,y,0),(xx,y,.45),.014,'031')
            else:
                cylinder('gas_lift',(0,0,.10),(0,0,.43),.027,'032')
                for k in range(5):
                    a=k*math.tau/5;xx,yy=math.cos(a)*.3,math.sin(a)*.3;cylinder('chair_star_leg',(0,0,.1),(xx,yy,.10),.016,'031');wheel(xx,yy,.04,.03)
                for s in [-1,1]:
                    cylinder('armrest_stem',(s*.28,0,.44),(s*.28,0,.66),.013,'031');softbox('chair_armrest',(s*.28,0,.68),(.065,.31,.04),'rubber',.018)
        if count>1:cylinder('bench_crossbar',(-.8,0,.3),(.8,0,.3),.035,'031')
    elif i in (239,240,241,242,243,244):
        if i in (239,241,242,244):cabinet(.9 if i!=242 else .40,.42 if i!=244 else .60,1.85,glass=i==241)
        elif i==243:
            box('locker_door',(0,0,.9),(.38,.025,1.8),'026',.006);pull_handle(.12,-.06,.95,.12)
            for z in [1.48,1.54,1.60]:box('locker_vent',(0,-.015,z),(.22,.003,.015),'black',.003)
        else:
            for x in [-.5,.5]:
                for y in [-.22,.22]:box('shelf_upright',(x,y,1),(.035,.035,2),'031')
            for z in [.12,.55,.98,1.41,1.84]:box('archive_shelf',(0,0,z),(1.04,.48,.025),'028',.005)
            for x in [-.45,.45]:curve('shelf_brace',[(x,.24,.12),(-x,.24,1.84)],.008,'031')
    elif i in (245,246):
        cabinet(.42,.12,.55) if i==245 else cabinet(.6,.5,.7)
        if i==245:
            for j in range(3):
                for k in range(4):curve('key_hook',[((k-1.5)*.08,-.07,.12+j*.15),((k-1.5)*.08,-.1,.12+j*.15),((k-1.5)*.08,-.105,.14+j*.15)],.002,'steel')
        else:
            torus('safe_handle',(0,-.32,.4),.08,.012,'steel')
            for j in range(3):
                a=j*math.tau/3;cylinder('safe_spoke',(0,-.32,.4),(.08*math.cos(a),-.32,.4+.08*math.sin(a)),.008,'steel')
    elif i in (247,248,249,250,251,252,253,254):
        if i==247:
            box('PC_tower',(0,0,.22),(.18,.42,.44),'028',.012)
            for z in [.30,.35]:box('drive_bay',(0,-.216,z),(.14,.012,.035),'black',.003)
            for j in range(12):box('PC_vent',(0,-.216,.04+j*.012),(.13,.003,.005),'black',0)
        elif i==248:
            box('CRT_housing',(0,.10,.22),(.43,.37,.35),'028',.045)
            box('CRT_bezel',(0,-.10,.22),(.40,.05,.32),'028',.026)
            box('CRT_glass',(0,-.13,.23),(.33,.022,.245),'glass',.028)
            cylinder('monitor_neck',(0,.03,.04),(0,.03,.09),.045,'028');box('monitor_foot',(0,.03,.025),(.25,.22,.04),'028',.02)
            for x in [.12,.15]:cylinder('CRT_control',(x,-.13,.09),(x,-.14,.09),.009,'black')
        elif i==249:
            box('keyboard_body',(0,0,.015),(.44,.15,.03),'028',.012)
            for j in range(5):
                for k in range(15):box('keyboard_key',(-.2+k*.028,-.06+j*.027,.035),(.023,.022,.009),'white',.003)
            box('spacebar',(0,-.06,.037),(.15,.022,.009),'028',.003)
        elif i==250:
            softbox('computer_mouse',(0,0,.018),(.06,.10,.035),'028',.016);box('mouse_seam',(0,-.02,.036),(.001,.06,.001),'black',0)
            torus('scroll_wheel',(0,-.018,.037),.006,.002,'black',(0,math.pi/2,0));curve('mouse_cable',[(0,-.05,.018),(0,-.12,.01),(.04,-.2,.004)],.0017,'black')
        elif i in (251,252):
            box('telephone_base',(0,0,.045),(.20,.21,.09),'028',.022)
            softbox('telephone_handset',(0,.04,.12),(.23,.055,.055),'black',.025)
            for j in range(4):
                for k in range(3):box('telephone_key',((k-1)*.03,-.065+j*.025,.096),(.023,.018,.006),'white',.003)
            curve('telephone_coil',[(.12+.012*math.cos(j*.7),.04+j*.001,.06+.012*math.sin(j*.7)) for j in range(130)],.0018,'black')
        elif i==253:
            box('printer_housing',(0,0,.13),(.42,.36,.26),'028',.025)
            box('paper_output',(0,-.185,.12),(.30,.012,.025),'black',.003)
            box('paper_tray',(0,-.24,.09),(.32,.18,.015),'028',.008)
            for j in range(5):box('printer_paper',(0,-.20,.102+j*.0007),(.21,.24,.0005),'white',0)
        else:
            cylinder('clock_rim',(0,0,.16),(0,-.035,.16),.16,'031',64);cylinder('clock_dial',(0,-.036,.16),(0,-.038,.16),.145,'028',64)
            for j in range(12):
                a=math.pi/2-j*math.tau/12
                text(str(j or 12),(.12*math.cos(a),-.04,.16+.12*math.sin(a)-.008),.022,'black')
            curve('clock_hour_hand',[(0,-.044,.16),(-.065,-.044,.18)],.004,'black');curve('clock_minute_hand',[(0,-.045,.16),(.02,-.045,.27)],.002,'black')
    elif i in (255,256,257,258,259,260,261,262,263):
        if i==255:
            box('whiteboard',(0,0,.4),(1.2,.025,.8),'028',.008)
            for s in [-1,1]:
                box('whiteboard_frame',(s*.6,-.01,.4),(.02,.035,.82),'032');box('whiteboard_frame',(0,-.01,.4+s*.4),(1.22,.035,.02),'032')
        else:
            count=30 if i==261 else 1
            w,h=(.22,.30);thickness=.025 if i in (256,257,258) else .001
            for j in range(count):box('document',(0,0,thickness/2+j*.0008),(w,h,thickness),'wood' if i in (256,257,258) else '028',.001)
            if i in (259,260):
                text('ЭХО / 04',(0,-.151,.001),.03,'black')
            if i==260:box('clipboard_clamp',(0,.14,.006),(.08,.03,.012),'032',.005)
            if i==263:
                mesh('envelope_flap',[(-w/2,h/2,.002),(w/2,h/2,.002),(0,0,.003)],[(0,1,2)],'white')
    elif i in (264,265):
        cylinder('pen_body',(0,0,.02),(0,0,.15),.004,'black' if i==264 else 'wood',12 if i==264 else 6)
        bpy.ops.mesh.primitive_cone_add(vertices=12,radius1=0,radius2=.004,depth=.02,location=(0,0,.01));finish(bpy.context.object,'writing_tip','steel' if i==264 else 'wood')
        if i==264:curve('pen_clip',[(0,0,.15),(.007,0,.15),(.007,0,.11)],.001,'steel')
    elif i==266:
        box('organizer_base',(0,0,.008),(.22,.13,.016),'031',.01)
        for x in [-.08,0,.08]:
            for y in [-.04,.04]:box('organizer_wall',(x,y,.06),(.07,.005,.10),'031')
    elif i in (267,268,269):
        r,h=(.04,.09) if i==267 else ((.045,.25) if i==268 else (.035,.23))
        # open wall with visible interior lip
        v=[];f=[]
        for radius,z in [(r,0),(r,h),(r-.004,h),(r-.004,.009)]:
            for j in range(48):a=j*math.tau/48;v.append((radius*math.cos(a),radius*math.sin(a),z))
        for k in range(3):
            for j in range(48):a=k*48+j;b=k*48+(j+1)%48;f.append((a,b,b+48,a+48))
        mesh('vessel_body',v,f,'glass' if i==269 else ('032' if i==268 else '028'))
        cylinder('vessel_base',(0,0,0),(0,0,.008),r,'028')
        if i==267:curve('mug_handle',[(r,0,.07),(.075,0,.07),(.078,0,.035),(r,0,.025)],.008,'028')
        else:cylinder('vessel_cap',(0,0,h),(0,0,h+.025),r*.85,'026')
    elif i==270:medical(i)
    elif i==271:
        cylinder('ashtray_stand',(0,0,0),(0,0,.7),.035,'031');cylinder('ashtray_foot',(0,0,0),(0,0,.025),.15,'031')
        cylinder('ashtray_bowl',(0,0,.70),(0,0,.73),.12,'031');cylinder('ash',(0,0,.731),(0,0,.733),.10,'037')
    elif i in (272,273):
        w,d,h=(.35,.34,.95) if i==272 else (.9,.65,1.8)
        box('appliance_case',(0,0,h/2),(w,d,h),'028' if i==272 else '026',.025)
        if i==272:
            cylinder('water_bottle',(0,0,h+.02),(0,0,h+.36),.14,'glass',48)
            box('cup_recess',(0,-d/2-.008,h*.65),(w*.75,.012,.30),'black',.018)
            for x in [-.07,.07]:cylinder('water_tap',(x,-d/2,.70),(x,-d/2-.045,.70),.012,'red' if x<0 else '032')
        else:
            box('vending_display',(-.12,-d/2-.014,1.03),(.52,.025,1.3),'glass',.015)
            for z in [.57,.87,1.17,1.47]:box('vending_shelf',(-.12,-.22,z),(.5,.15,.015),'032')
            box('vending_keypad',(.33,-d/2-.03,1.14),(.12,.03,.30),'black',.01)
            box('product_hatch',(0,-d/2-.025,.22),(.48,.035,.16),'black',.02)

def setup_scene():
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
    scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=24
    if any(d.use and d.type=='OPTIX' for d in bpy.context.preferences.addons['cycles'].preferences.devices):scene.cycles.device='GPU'
    scene.cycles.use_denoising=True
    scene.render.resolution_x=1000;scene.render.resolution_y=1000;scene.render.resolution_percentage=100
    scene.render.image_settings.file_format='PNG';scene.world.color=(.10,.10,.10)
    scene.view_settings.view_transform='AgX'

def stage():
    bpy.context.view_layer.update()
    points=[o.matrix_world@Vector(v) for o in parts if o.type=='MESH' for v in o.bound_box]
    low=Vector(tuple(min(v[j] for v in points) for j in range(3)));high=Vector(tuple(max(v[j] for v in points) for j in range(3)))
    center=(low+high)/2;size=max(high-low);size=max(size,.3)
    bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,low.z-.015));plane=bpy.context.object;plane.name='STUDIO_FLOOR';plane.data.materials.append(material('037'))
    bpy.ops.object.camera_add();cam=bpy.context.object;cam.name='STUDIO_CAMERA'
    if current in range(25,41) or current in range(41,61):
        direction=Vector((1,-1.5,1.7))
    else:direction=Vector((.95,-2.1,.65))
    cam.location=center+direction.normalized()*size*2.2;cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler()
    cam.data.type='ORTHO';cam.data.ortho_scale=size*1.50;cam.data.lens=55;bpy.context.scene.camera=cam
    for name,pos,power,color,area in [('key',(-3,-4,6),1800,(.80,.91,1),5),('fill',(4,-2,3),800,(1,.82,.62),4),('rim',(2,3,5),1600,(.48,.75,.80),3)]:
        bpy.ops.object.light_add(type='AREA',location=center+Vector(pos)*max(size/3,.25));l=bpy.context.object;l.name='STUDIO_'+name;l.data.energy=power*max((size/3)**2,.1);l.data.color=color;l.data.shape='DISK';l.data.size=area*max(size/3,.25);l.rotation_euler=(center-l.location).to_track_quat('-Z','Y').to_euler()
    return [list(low),list(high)]

def build(i):
    global parts,current
    current=i;parts=[];random.seed(71000+i);setup_scene();character_info=None
    if i<=24:wall(i)
    elif i<=40:box('material_sample',(0,0,.04),(3,3,.08),f'{i:03d}',.006)
    elif i<=60:floor_detail(i)
    elif i<=93:door(i)
    elif i<=105:sign(i)
    elif i<=120:window_detail(i)
    elif i<=135:fluorescent(i)
    elif i<=155:electrical(i)
    elif i<=174:engineering(i)
    elif i<=231:medical(i)
    elif i<=273:office(i)
    elif i<=318:
        import echo_extended_models as extended;extended.install(sys.modules[__name__]);extended.laboratory(i)
    elif i<=365:
        import echo_extended_models as extended;extended.install(sys.modules[__name__]);extended.utility(i)
    elif i<=394:
        import echo_extended_models as extended;extended.install(sys.modules[__name__]);extended.architecture(i)
    elif i<=407:
        import echo_extended_models as extended;extended.install(sys.modules[__name__]);extended.bathroom(i)
    elif i<=444:
        import echo_extended_models as extended;extended.install(sys.modules[__name__]);extended.commerce(i)
    elif i<=490:
        import echo_extended_models as extended;extended.install(sys.modules[__name__]);extended.outdoors(i)
    elif i<=505:
        import echo_extended_models as extended;extended.install(sys.modules[__name__]);extended.vegetation(i)
    elif i<=562:
        import echo_extended_models as extended;extended.install(sys.modules[__name__]);extended.clutter(i)
    elif i<=575:
        import echo_character_models as characters;characters.bind_install(sys.modules[__name__]);character_info=characters.build(i)
    else:raise ValueError('No authored builder for '+str(i))
    if not parts:raise RuntimeError('Empty asset '+str(i))
    folder=OUT/f'EH_{i:03d}';folder.mkdir(exist_ok=True)
    bounds=stage()
    # Export only asset parts, never preview floor, lights, or camera.
    bpy.ops.object.select_all(action='DESELECT')
    for o in parts:o.select_set(True)
    bpy.context.view_layer.objects.active=next(o for o in parts if o.type=='MESH')
    bpy.ops.export_scene.gltf(filepath=str(folder/f'EH_{i:03d}.glb'),export_format='GLB',use_selection=True,export_apply=True,export_yup=True,export_extras=True)
    externalize_images(folder/f'EH_{i:03d}.glb')
    for image in bpy.data.images:
        if image.source=='FILE' and image.filepath:
            absolute=bpy.path.abspath(image.filepath)
            image.filepath=bpy.path.relpath(absolute,start=str(folder))
    bpy.ops.wm.save_as_mainfile(filepath=str(folder/f'EH_{i:03d}.blend'),check_existing=False)
    if not opt.no_render:
        bpy.context.scene.render.filepath=str(folder/'preview.png');bpy.ops.render.render(write_still=True)
    deps=bpy.context.evaluated_depsgraph_get();triangles=0
    for o in parts:
        if o.type!='MESH':continue
        ev=o.evaluated_get(deps);me=ev.to_mesh();me.calc_loop_triangles();triangles+=len(me.loop_triangles);ev.to_mesh_clear()
    metadata={'id':i,'title':catalog.get(i),'file':f'EH_{i:03d}.glb','source':f'EH_{i:03d}.blend','units':'metres','blender_up':'Z','gltf_up':'Y','bounds_blender_m':bounds,'mesh_parts':sum(o.type=='MESH' for o in parts),'triangles':triangles,'has_preview':not opt.no_render,'status':'created_pending_independent_review','license':'Original project artwork; no third-party art','sha256':hashlib.sha256((folder/f'EH_{i:03d}.glb').read_bytes()).hexdigest(),'notes':'Image PBR. No engine collision or in-game placement authored. Studio is excluded from GLB.'}
    if character_info:metadata.update(character_info)
    (folder/'asset.json').write_text(json.dumps(metadata,ensure_ascii=False,indent=2),encoding='utf-8')
    print('ASSET_COMPLETE',i,triangles,flush=True)

if __name__=='__main__':
    for i in range(opt.start,opt.end+1):build(i)
    print('BATCH_COMPLETE',opt.start,opt.end,flush=True)
