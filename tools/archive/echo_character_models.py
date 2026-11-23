"""Reusable licensed humans and original rigged horror creature concepts.
Original creatures are segmented production concepts, not final sculpted humans.
"""
import bpy,math,json,random
from pathlib import Path
from mathutils import Vector,Matrix
K=None

def bind_install(kit):
    global K;K=kit

def skin_materials():
    for name,color,rough in [('creature_skin',(.19,.24,.21),.68),('necrotic_skin',(.10,.14,.13),.79),('exposed_flesh',(.18,.045,.035),.72),('ivory_bone',(.61,.58,.44),.61),('eye_amber',(.62,.32,.04),.25)]:
        mat=K.material(name);bs=mat.node_tree.nodes.get('Principled BSDF');bs.inputs['Base Color'].default_value=(*color,1);bs.inputs['Roughness'].default_value=rough
        if name=='eye_amber':bs.inputs['Emission Color'].default_value=(*color,1);bs.inputs['Emission Strength'].default_value=.6

def ellipsoid(name,loc,scale,mat='creature_skin'):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=32,ring_count=20,radius=1,location=loc)
    ob=bpy.context.object;ob.scale=scale;K.finish(ob,name,mat)
    for p in ob.data.polygons:p.use_smooth=True
    return ob

def segment(name,a,b,r1,r2,mat='creature_skin'):
    delta=Vector(b)-Vector(a)
    bpy.ops.mesh.primitive_cone_add(vertices=24,radius1=r1,radius2=r2,depth=delta.length,location=(Vector(a)+Vector(b))/2)
    ob=bpy.context.object;ob.rotation_mode='QUATERNION';ob.rotation_quaternion=delta.to_track_quat('Z','Y');K.finish(ob,name,mat,.008)
    for p in ob.data.polygons:p.use_smooth=True
    return ob

def skeleton(joints):
    data=bpy.data.armatures.new('ECHO_CreatureSkeleton');rig=bpy.data.objects.new('ECHO_Rig',data);bpy.context.collection.objects.link(rig);bpy.context.view_layer.objects.active=rig
    bpy.ops.object.select_all(action='DESELECT');rig.select_set(True);bpy.ops.object.mode_set(mode='EDIT')
    for name,(head,tail,parent) in joints.items():
        bone=data.edit_bones.new(name);bone.head=head;bone.tail=tail
        if parent:bone.parent=data.edit_bones[parent]
    bpy.ops.object.mode_set(mode='OBJECT');rig.show_in_front=True;K.parts.append(rig);return rig

def attach(ob,rig,bone):
    if ob.type!='MESH':return
    # Direct data transform avoids repeated scene evaluation on a dense skin.
    ob.data.transform(ob.matrix_world);ob.matrix_basis=Matrix.Identity(4)
    group=ob.vertex_groups.new(name=bone);group.add(list(range(len(ob.data.vertices))),1,'REPLACE')
    modifier=ob.modifiers.new('Skeleton deformation','ARMATURE');modifier.object=rig;ob.parent=rig;ob.matrix_parent_inverse=Matrix.Identity(4)

def idle(rig):
    scene=bpy.context.scene;scene.render.fps=24;scene.frame_start=1;scene.frame_end=73
    for bone in rig.pose.bones:
        bone.rotation_mode='XYZ'
        if bone.name not in ['spine','head','chest']:continue
        for frame,angle in [(1,0),(19,.018),(37,0),(55,-.018),(73,0)]:
            bone.rotation_euler.x=angle;bone.keyframe_insert(data_path='rotation_euler',frame=frame,group=bone.name)
    if rig.animation_data and rig.animation_data.action:rig.animation_data.action.name='ECHO_Idle_Breathing'
    scene.frame_set(1)

def fuse_organic_skin(weighted,rig):
    """Unify overlapping anatomy and assign smooth weights by bone distance."""
    import numpy as np
    skin=[(ob,bone) for ob,bone in weighted if ob.type=='MESH' and ob.data.materials and ob.data.materials[0].name in ('creature_skin','necrotic_skin') and not any(w in ob.name for w in ['finger','claw'])]
    if not skin:return weighted
    names={ob.name for ob,_ in skin};K.parts=[ob for ob in K.parts if ob.name not in names]
    remaining=[pair for pair in weighted if pair[0].name not in names]
    bpy.ops.object.select_all(action='DESELECT')
    for ob,_ in skin:ob.select_set(True)
    active=skin[0][0];bpy.context.view_layer.objects.active=active;bpy.ops.object.join();active.name='ECHO_Continuous_Organic_Anatomy'
    bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
    for mod in list(active.modifiers):active.modifiers.remove(mod)
    remesh=active.modifiers.new('Unified sculpt surface','REMESH');remesh.mode='VOXEL';remesh.voxel_size=.009;remesh.use_smooth_shade=True;bpy.ops.object.modifier_apply(modifier=remesh.name)
    print('ORGANIC_SURFACE',len(active.data.vertices),flush=True)
    smooth=active.modifiers.new('Anatomical blend','SMOOTH');smooth.factor=.75;smooth.iterations=7;bpy.ops.object.modifier_apply(modifier=smooth.name)
    dec=active.modifiers.new('Surface reduction','DECIMATE');dec.ratio=.65;bpy.ops.object.modifier_apply(modifier=dec.name)
    sculpt_normals=[v.normal.copy() for v in active.data.vertices]
    for vert,normal in zip(active.data.vertices,sculpt_normals):
        p=vert.co
        # shallow unevenness in the surface, with longer vertical tendons
        amount=.0018*math.sin(p.x*77+p.z*31)*math.sin(p.y*65-p.z*23)
        vert.co+=normal*amount
    K.uv(active)
    for poly in active.data.polygons:poly.use_smooth=True
    bones=[bone for bone in rig.data.bones if bone.name!='root'];coords=np.array([v.co[:] for v in active.data.vertices],dtype=np.float32)
    dist=[]
    for bone in bones:
        a=np.array(bone.head_local);b=np.array(bone.tail_local);v=b-a;t=np.clip(((coords-a)*v).sum(axis=1)/(v@v),0,1);delta=coords-a-t[:,None]*v;dist.append((delta*delta).sum(axis=1))
    d=np.stack(dist,1);nearest=np.argsort(d,axis=1)[:,:2]
    weights=1/(np.take_along_axis(d,nearest,1)+.002)**2;weights/=weights.sum(axis=1,keepdims=True)
    groups=[active.vertex_groups.new(name=bone.name) for bone in bones]
    # Quantized complementary weights allow bulk assignment instead of hundreds
    # of thousands of per-vertex dependency graph updates.
    quantized=np.round(weights[:,0]*32).astype('int32');weights=np.stack((quantized/32,1-quantized/32),axis=1)
    for bone_index,group in enumerate(groups):
        for column in range(2):
            candidates=nearest[:,column]==bone_index
            for step in range(1,33):
                indices=np.flatnonzero(candidates & (np.abs(weights[:,column]-step/32)<1e-8)).tolist()
                if indices:group.add(indices,step/32,'REPLACE')
    print('ORGANIC_WEIGHTS',len(coords),flush=True)
    mod=active.modifiers.new('Blended skeleton deformation','ARMATURE');mod.object=rig;active.parent=rig;K.parts.append(active)
    return remaining

def original_monster(i):
    skin_materials();random.seed(i*771)
    height={569:2.05,570:2.15,571:1.95,572:2.35,573:2.10,574:1.90,575:2.0}.get(i,1.95)
    hip=height*.49;shoulder=height*.78;neck=height*.86
    wide=.38 if i==570 else .26
    joints={'root':((0,0,0),(0,0,.18),None),'pelvis':((0,0,hip),(0,0,hip+.18),'root'),'spine':((0,0,hip+.18),(0,0,shoulder-.10),'pelvis'),'chest':((0,0,shoulder-.10),(0,0,neck),'spine'),'head':((0,0,neck),(0,0,height),'chest')}
    for s,suffix in [(-1,'L'),(1,'R')]:
        elbow=(s*(wide+.16),-.04,height*.59);wrist=(s*(wide+.20),-.11,height*.39 if i in (572,573) else height*.47)
        joints['upper_arm.'+suffix]=((s*wide,0,shoulder),elbow,'chest')
        joints['forearm.'+suffix]=(elbow,wrist,'upper_arm.'+suffix)
        joints['hand.'+suffix]=(wrist,(wrist[0],wrist[1]-.01,wrist[2]-.12),'forearm.'+suffix)
        knee=(s*.13,-.06,height*.25);ankle=(s*.14,0,.10)
        joints['thigh.'+suffix]=((s*.13,0,hip),knee,'pelvis');joints['shin.'+suffix]=(knee,ankle,'thigh.'+suffix);joints['foot.'+suffix]=(ankle,(s*.14,-.19,.06),'shin.'+suffix)
    rig=skeleton(joints);weighted=[]
    def add(ob,bone):weighted.append((ob,bone));return ob
    add(ellipsoid('pelvic_mass',(0,0,hip+.02),(.19,.13,.18)),'pelvis')
    add(ellipsoid('ribcage',(0,.015,height*.68),(wide,.14,height*.16),'necrotic_skin' if i==570 else 'creature_skin'),'chest')
    add(segment('exposed_spine',(0,.10,hip+.1),(0,.12,neck),.035,.03,'ivory_bone'),'spine')
    for j in range(9):add(ellipsoid('vertebra',(0,.15,hip+.17+j*.058),(.045,.035,.022),'ivory_bone'),'spine')
    for s,suffix in [(-1,'L'),(1,'R')]:
        for bone,r1,r2 in [('upper_arm',.07,.048),('forearm',.05,.025),('thigh',.10,.058),('shin',.057,.027)]:
            name=bone+'.'+suffix;a,b,_=joints[name];add(segment(bone,a,b,r1,r2),name)
            add(ellipsoid('joint',a,(r1,r1*.9,r1)) ,name)
        handbone='hand.'+suffix;wrist=joints[handbone][0]
        add(ellipsoid('palm',(wrist[0],wrist[1]-.005,wrist[2]-.045),(.043,.025,.075)),handbone)
        for finger in range(5):
            x=wrist[0]+(finger-2)*.018;z=wrist[2]-.08
            ob=K.curve('elongated_finger',[(x,wrist[1],z),(x+s*.014,wrist[1]-.014,z-.10),(x+s*.02,wrist[1]-.05,z-.15)],.006,'creature_skin');add(ob,handbone)
            add(segment('finger_claw',(x+s*.02,wrist[1]-.05,z-.15),(x+s*.025,wrist[1]-.08,z-.18),.007,.001,'ivory_bone'),handbone)
        add(ellipsoid('foot',(s*.14,-.07,.07),(.075,.15,.065)),'foot.'+suffix)
        for j in range(6):
            z=height*.63+j*.038
            ob=K.curve('exposed_rib',[(s*.025,-.095,z),(s*wide*.7,-.14,z+.02),(s*wide*.94,-.08,z+.04)],.009,'ivory_bone');add(ob,'chest')
    add(segment('neck',(0,0,neck-.04),(0,0,neck+.07),.055,.045),'head')
    headcenter=height*.937
    add(ellipsoid('cranium',(0,-.025,headcenter),(.105,.105,.14),'necrotic_skin' if i==570 else 'creature_skin'),'head')
    add(ellipsoid('jaw',(0,-.055,headcenter-.085),(.08,.075,.055)),'head')
    add(ellipsoid('sunken_mouth',(0,-.118,headcenter-.073),(.057,.013,.033),'black'),'head')
    for s in [-1,1]:
        add(ellipsoid('eye_socket',(s*.043,-.112,headcenter+.02),(.032,.015,.025),'black'),'head')
        if i not in (571,572):add(ellipsoid('eye',(s*.043,-.124,headcenter+.02),(.012,.008,.011),'eye_amber'),'head')
    for j in range(8):
        x=(j-3.5)*.013
        add(segment('upper_tooth',(x,-.135,headcenter-.05),(x,-.137,headcenter-.075),.004,.001,'ivory_bone'),'head')
    if i==569:
        # Suture bands cross the asymmetrical torso and head.
        for z in [height*.64,height*.73,headcenter]:
            for j in range(12):
                x=-.17+j*.031
                add(K.curve('suture_stitch',[(x,-.148,z-.012),(x+.015,-.151,z+.014)],.0018,'black'),'head' if z>neck else 'chest')
        add(ellipsoid('grafted_shoulder',(-wide,.015,shoulder),(.13,.13,.13),'exposed_flesh'),'upper_arm.L')
    elif i==570:
        for j in range(6):
            z=height*.54+j*.085
            add(ellipsoid('overlapping_carapace',(0,-.045,z),(.31-j*.006,.16,.065),'necrotic_skin'),'chest')
        for s in [-1,1]:
            for j in range(4):add(segment('shoulder_spike',(s*wide,0,shoulder-j*.045),(s*(wide+.14),.02,shoulder+.15-j*.045),.04,.001,'ivory_bone'),'upper_arm.L' if s<0 else 'upper_arm.R')
    elif i==571:
        # Dirty surgical apron, tied eye bandage, exaggerated instrument fingers.
        ob=K.slab_polygon('surgeon_apron',[(-.23,hip-.20),(.23,hip-.20),(.18,shoulder-.07),(-.18,shoulder-.07)],-.165,-.157,'fabric_dirty');add(ob,'chest')
        add(ellipsoid('blindfold',(0,-.04,headcenter+.025),(.11,.108,.030),'fabric_dirty'),'head')
        for s,suffix in [(-1,'L'),(1,'R')]:
            wrist=joints['hand.'+suffix][0];add(segment('surgical_blade',(wrist[0],wrist[1],wrist[2]-.13),(wrist[0],wrist[1]-.10,wrist[2]-.40),.025,.001,'steel'),'hand.'+suffix)
    elif i==572:
        for s in [-1,1]:
            ob=K.torus('hypertrophic_ear',(s*.11,-.015,headcenter+.015),.065,.014,'creature_skin',(0,math.pi/2,0));ob.scale.z=1.5;add(ob,'head')
        add(ellipsoid('sealed_eyes',(0,-.11,headcenter+.02),(.09,.02,.033),'fabric_dirty'),'head')
    elif i==573:
        for s in [-1,1]:
            for j in range(5):add(segment('dorsal_spike',(s*.08,.14,height*.62+j*.07),(s*.13,.35,height*.68+j*.07),.025,.001,'ivory_bone'),'spine')
    elif i==574:
        # Keeper's hanging lamp and key ring are attached to the wrist bone.
        wrist=Vector(joints['hand.R'][1]);center=wrist+Vector((0,0,-.13))
        start=len(K.parts)
        K.box('guide_lantern_core',center,(.08,.08,.13),K.emission('lantern_core',(.35,.65,.46),1.2),.01)
        for x in [-.05,.05]:
            for y in [-.05,.05]:K.cylinder('lantern_cage',center+Vector((x,y,-.09)),center+Vector((x,y,.09)),.004,'031')
        K.curve('lantern_handle',[center+Vector((-.04,0,.1)),center+Vector((-.04,0,.17)),center+Vector((.04,0,.17)),center+Vector((.04,0,.1))],.005,'031')
        for ob in K.parts[start:]:add(ob,'hand.R')
        for j in range(7):
            x=(j-3)*.022;add(K.curve('hanging_cloth_strip',[(x,-.15,height*.74),(x+.015,-.16,hip+.05),(x-.02,-.14,hip-.2-random.random()*.1)],.012,'fabric_dirty'),'chest')
    elif i==575:
        for z in [height*.57,height*.66,height*.75]:
            add(K.torus('restraint_band',(0,0,z),wide*.95,.016,'fabric_dirty',(0,0,0)),'chest')
        for j in range(18):
            x=-.32+j*.038;z=height*.64-.08*math.sin(j*math.pi/17);add(K.torus('binding_chain_link',(x,-.17,z),.015,.003,'031',(math.pi/2 if j%2 else 0,0,0)),'chest')
    weighted=fuse_organic_skin(weighted,rig)
    for ob,bone in weighted:attach(ob,rig,bone)
    idle(rig)
    return {'authorship':'Original creature concept with unified organic skin','rig_bones':len(joints),'animation':'ECHO_Idle_Breathing','limitation':'Procedural concept anatomy; requires detailed artistic sculpt/retopology and full locomotion/attack animations for final close-up gameplay.'}

def imported_human(i):
    root=K.ROOT;paths={563:'assets/models/rocketbox/Export/Male_Adult_04.fbx',565:'assets/models/menu_nurse/Medical_Female_01.fbx',566:'assets/models/horror_game_maker/Poses/Idle.fbx'}
    bpy.ops.import_scene.fbx(filepath=str(root/paths[i]),use_anim=True)
    meshes=[o for o in bpy.context.scene.objects if o.type=='MESH'];rigs=[o for o in bpy.context.scene.objects if o.type=='ARMATURE'];assert meshes and rigs
    # Normalize centimetre-origin source models to a consistent human scale.
    bpy.context.view_layer.update();points=[o.matrix_world@Vector(v) for o in meshes for v in o.bound_box];low=min(v.z for v in points);height=max(v.z for v in points)-low;factor=1.78/height
    parents=[o for o in bpy.context.scene.objects if o.parent is None]
    normalizer=bpy.data.objects.new('CharacterScaleRoot',None);bpy.context.collection.objects.link(normalizer)
    for o in parents:o.parent=normalizer
    normalizer.scale=(factor,)*3;normalizer.location.z=-low*factor;K.parts.extend([normalizer]+meshes+rigs)
    folder=root/('assets/models/rocketbox/textures' if i==563 else ('assets/models/menu_nurse' if i==565 else 'assets/models/menu_zombie/pixelhouse'))
    prefix='m006' if i==563 else 'f152'
    for ob in meshes:
        K.uv(ob) if not ob.data.uv_layers else None
        if i==566:
            skin_materials();ob.data.materials.clear();ob.data.materials.append(K.material('creature_skin'));continue
        for slot in ob.material_slots:
            mat=slot.material
            if not mat:continue
            mat.use_nodes=True;ns=mat.node_tree.nodes;links=mat.node_tree.links;bs=ns.get('Principled BSDF')
            for oldnode in list(ns):
                if oldnode.type not in ('BSDF_PRINCIPLED','OUTPUT_MATERIAL'):ns.remove(oldnode)
            if i==566:color=folder/'difusse.jpg';normal=folder/'normal.JPG'
            else:
                component='head' if any(s in mat.name.lower() for s in ['head','face']) else ('opacity' if any(s in mat.name.lower() for s in ['hair','opacity','eye']) else 'body')
                color=folder/f'{prefix}_{component}_color.tga';normal=folder/f'{prefix}_{component}_normal.tga'
            if color.exists():
                t=ns.new('ShaderNodeTexImage');t.image=bpy.data.images.load(str(color),check_existing=True);links.new(t.outputs['Color'],bs.inputs['Base Color'])
                if 'opacity' in color.name:links.new(t.outputs['Alpha'],bs.inputs['Alpha'])
            if normal.exists():
                t=ns.new('ShaderNodeTexImage');t.image=bpy.data.images.load(str(normal),check_existing=True);t.image.colorspace_settings.name='Non-Color';n=ns.new('ShaderNodeNormalMap');links.new(t.outputs['Color'],n.inputs['Color']);links.new(n.outputs['Normal'],bs.inputs['Normal'])
            bs.inputs['Roughness'].default_value=.75
    license_text=(root/('assets/models/horror_game_maker/SOURCE.json' if i==566 else 'assets/models/rocketbox/LICENSE.md')).read_text(encoding='utf-8')
    d=K.OUT/f'EH_{i:03d}';d.mkdir(exist_ok=True);(d/'LICENSE.md').write_text(license_text,encoding='utf-8')
    return {'authorship':'Adapted from existing licensed project source','upstream_source':paths[i],'license':'CC0, City Building Game Art / HorrorGameMaker.com' if i==566 else 'MIT, Microsoft Rocketbox','rig_bones':sum(len(r.data.bones) for r in rigs),'limitation':'Licensed medical human base; final zombie nurse face, skin and uniform art are still pending.' if i==565 else ('Existing monster adapted as a temporary hospital zombie base; dedicated patient clothing and human zombie face remain art tasks.' if i==566 else 'Source rig retained; animation compatibility in engine has not been tested.')}

def build(i):
    if i in (563,565,566):return imported_human(i)
    if i in (564,567,568):
        # Rigged original standalone clothing and event body-part concepts.
        skin_materials()
        if i==564:
            rig=skeleton({'root':((0,0,0),(0,0,.1),None),'pelvis':((0,0,.9),(0,0,1.05),'root'),'spine':((0,0,1.05),(0,0,1.5),'pelvis'),'chest':((0,0,1.5),(0,0,1.65),'spine')})
            ob=K.softbox('survivor_jacket',(0,0,1.36),(.51,.25,.56),'fabric_blue',.08);attach(ob,rig,'chest')
            for s in [-1,1]:
                ob=K.softbox('jacket_pocket',(s*.13,-.137,1.25),(.16,.018,.16),'fabric_blue',.015);attach(ob,rig,'chest')
                ob=segment('jacket_sleeve',(s*.25,0,1.56),(s*.40,0,1.12),.09,.055,'fabric_blue');attach(ob,rig,'chest')
                ob=segment('survivor_trouser_leg',(s*.12,0,.98),(s*.12,0,.10),.115,.065,'fabric_dirty');attach(ob,rig,'pelvis')
            idle(rig)
        elif i==567:
            rig=skeleton({'root':((0,0,0),(0,0,.1),None),'head':((0,0,.08),(0,0,.35),'root')})
            for ob in [ellipsoid('event_zombie_head',(0,0,.23),(.11,.11,.15)),ellipsoid('event_jaw',(0,-.045,.14),(.08,.07,.06))]:attach(ob,rig,'head')
            for s in [-1,1]:attach(ellipsoid('eye_socket',(s*.044,-.105,.25),(.032,.016,.026),'black'),rig,'head');attach(ellipsoid('eye',(s*.044,-.117,.25),(.011,.009,.01),'eye_amber'),rig,'head')
            attach(ellipsoid('open_mouth',(0,-.11,.15),(.052,.016,.027),'black'),rig,'head');idle(rig)
        else:
            joints={'root':((0,0,0),(0,0,.10),None),'forearm':((0,0,.10),(0,0,.45),'root'),'hand':((0,0,.45),(0,0,.58),'forearm')}
            rig=skeleton(joints);attach(segment('event_forearm',(0,0,.1),(0,0,.45),.055,.028),rig,'forearm');attach(ellipsoid('event_palm',(0,-.005,.50),(.065,.03,.085)),rig,'hand')
            for j in range(5):
                x=(j-2)*.027;attach(K.curve('event_finger',[(x,0,.55),(x*1.15,-.025,.66),(x*1.1,-.08,.69)],.009,'creature_skin'),rig,'hand')
            idle(rig)
        return {'authorship':'Original rigged concept','rig_bones':len(rig.data.bones),'limitation':'Segmented prototype, requires final sculpt and engine animation setup.'}
    return original_monster(i)
