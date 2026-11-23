"""Individually authored specialty equipment, utility and outdoor kit recipes."""
import math,random
from mathutils import Vector

def install(k):
    globals().update({n:getattr(k,n) for n in ['box','cylinder','curve','mesh','slab_polygon','torus','bolt','text','pull_handle','lever_handle','wheel','cart_base','softbox','cloth','cabinet','hospital_bed','material','emission','flange']})
    globals()['K']=k

def move_since(before,loc,scale=None):
    for ob in K.parts[before:]:
        if ob.parent is None:
            ob.location+=Vector(loc)
            if scale:ob.scale=scale

def tank(r=.18,h=.65,mat='032'):
    cylinder('pressure_vessel',(0,0,r*.5),(0,0,h-r*.5),r,mat,48)
    import bpy
    for z in [r*.5,h-r*.5]:
        bpy.ops.mesh.primitive_uv_sphere_add(segments=32,ring_count=16,radius=r,location=(0,0,z));o=K.finish(bpy.context.object,'vessel_end',mat);o.scale.z=.5
    for z in [r,h-r]:torus('weld_seam',(0,0,z),r,.003,'032',(0,0,0))
    cylinder('vessel_valve',(0,0,h),(0,0,h+.06),.018,'steel',6)

def frame(w,d,h,mat='032'):
    for x in [-w/2,w/2]:
        for y in [-d/2,d/2]:box('frame_leg',(x,y,h/2),(.035,.035,h),mat,.004)
    for y in [-d/2,d/2]:box('frame_top_rail',(0,y,h),(w+.035,.035,.035),mat)
    for x in [-w/2,w/2]:box('frame_top_rail',(x,0,h),(.035,d,.035),mat)

def table(w=1.5,d=.7,h=.85,mat='032'):
    frame(w-.10,d-.10,h-.03);box('worktop',(0,0,h),(w,d,.04),mat,.014)

def open_bin(w=.4,d=.3,h=.3,mat='026'):
    for s in [-1,1]:
        box('bin_wall',(s*w/2,0,h/2),(.015,d,h),mat,.006)
        box('bin_wall',(0,s*d/2,h/2),(w,.015,h),mat,.006)
    box('bin_floor',(0,0,.01),(w,d,.02),mat,.006)

def appliance(w=.6,d=.6,h=1.7,front='028'):
    box('appliance_shell',(0,0,h/2),(w,d,h),'028',.022)
    pivot=K.bpy.data.objects.new('ApplianceDoorPivot',None);K.bpy.context.collection.objects.link(pivot);pivot.location=(-w/2,-d/2,0);K.parts.append(pivot)
    start=len(K.parts);box('appliance_door',(0,-d/2-.02,h/2),(w-.015,.035,h-.03),front,.017);pull_handle(w*.33,-d/2-.09,h*.6,.26)
    for ob in K.parts[start:]:ob.parent=pivot;ob.matrix_parent_inverse=pivot.matrix_world.inverted()
    for z in [h*.08,h*.12,h*.16]:box('compressor_vent',(0,d/2+.001,z),(w*.8,.002,.012),'black',.001)

def laboratory(i):
    if i in (274,299,303):
        table(1.9,.75,.85)
        if i==299:
            for s in [-1,1]:box('dissection_table_rim',(0,s*.37,.90),(1.92,.02,.08),'032',.009)
            cylinder('table_drain',(.65,0,.873),(.65,0,.875),.032,'black')
            for j in range(5):box('table_drain_channel',(.4+j*.08,0,.873),(.003,.58,.001),'black',0)
        if i==303:
            before=len(K.parts);K.medical(230);move_since(before,(0,0,.78))
    elif i==275:K.medical(230)
    elif i in (276,277):
        table(1.4,.8,.82);box('hood_back',(0,.34,1.35),(1.4,.10,1.1),'028',.012)
        for x in [-.67,.67]:box('hood_side',(x,0,1.35),(.06,.8,1.1),'028')
        box('hood_upper_plenum',(0,0,1.92),(1.4,.8,.32),'026',.016)
        box('hood_sliding_sash',(0,-.37,1.60),(1.25,.012,.50),'glass',.003)
        cylinder('sash_handle',(-.57,-.40,1.34),(.57,-.40,1.34),.012,'032')
        for j in range(12):box('hood_air_slot',(-.55+j*.10,-.22,.85),(.05,.28,.003),'black',.001)
        if i==276:cylinder('extraction_stack',(0,.1,2.08),(0,.1,2.5),.15,'032')
        else:box('HEPA_filter_bank',(0,.05,2.04),(1.1,.60,.15),'032',.015)
    elif i==278:
        softbox('microscope_foot',(0,0,.025),(.18,.24,.05),'028',.022)
        curve('microscope_arm',[(0,.07,.04),(0,.07,.20),(0,.03,.32),(0,-.03,.35)],.024,'028')
        cylinder('ocular_tube',(0,-.03,.35),(0,-.10,.40),.017,'black')
        cylinder('eyepiece',(0,-.10,.40),(0,-.125,.42),.021,'rubber')
        box('specimen_stage',(0,-.03,.15),(.16,.14,.016),'black',.004)
        for x in [-.05,.05]:box('slide_clip',(x,-.035,.162),(.012,.08,.003),'032')
        for j in range(3):
            a=j*math.tau/3;cylinder('objective_lens',(.025*math.cos(a),-.03+.025*math.sin(a),.25),(.025*math.cos(a),-.03+.025*math.sin(a),.19),.009,'steel')
        cylinder('focus_knob',(-.06,.05,.20),(.06,.05,.20),.025,'black')
    elif i==279:
        softbox('centrifuge_case',(0,0,.13),(.36,.40,.26),'028',.045)
        cylinder('centrifuge_lid',(0,.02,.26),(0,.02,.28),.16,'026',48)
        box('centrifuge_display',(0,-.205,.14),(.12,.012,.055),'black',.006)
        for x in [-.10,.10]:cylinder('control_knob',(x,-.205,.06),(x,-.22,.06),.012,'032')
    elif i in (280,281,415,443):appliance(.65,.65,1.8 if i!=281 else .95)
    elif i in (282,283):
        box('autoclave_case',(0,0,.25),(.6,.6,.5),'032',.025)
        cylinder('pressure_door',(0,-.31,.25),(0,-.34,.25),.20,'032',64)
        torus('pressure_door_seal',(0,-.344,.25),.18,.009,'rubber')
        pull_handle(0,-.40,.25,.14)
        for j in range(6):a=j*math.tau/6;bolt(.17*math.cos(a),-.35,.25+.17*math.sin(a),.009)
    elif i in (284,285,286,287,288,289,290,291):
        if i==284:
            for z in [.015,.07]:
                for x in [-.12,.12]:box('rack_side',(x,0,.04),(.012,.12,.09),'028')
                box('rack_grid',(0,0,z),(.25,.12,.008),'028')
                for x in [-.09,-.045,0,.045,.09]:
                    for y in [-.03,.03]:cylinder('rack_hole',(x,y,z+.004),(x,y,z+.005),.012,'black')
        elif i==285:
            cylinder('test_tube',(0,0,.01),(0,0,.13),.009,'glass');softbox('rounded_tube_bottom',(0,0,.012),(.018,.018,.024),'glass',.008)
            cylinder('test_tube_cap',(0,0,.13),(0,0,.145),.011,'red')
        elif i==286:
            K.bpy.ops.mesh.primitive_cone_add(vertices=48,radius1=.06,radius2=.018,depth=.13,location=(0,0,.065));K.finish(K.bpy.context.object,'erlenmeyer_body','glass')
            cylinder('flask_neck',(0,0,.13),(0,0,.20),.018,'glass');torus('flask_lip',(0,0,.20),.019,.003,'glass',(0,0,0))
        elif i in (287,288):
            r,h=(.045,.11) if i==287 else (.045,.015)
            # open translucent vessel
            for rr,zz in [(r,0),(r,h)]:torus('glass_rim',(0,0,zz),rr,.0015,'glass',(0,0,0))
            v=[];f=[]
            for z in [0,h]:
                for j in range(48):a=j*math.tau/48;v.append((r*math.cos(a),r*math.sin(a),z))
            for j in range(48):f.append((j,(j+1)%48,(j+1)%48+48,j+48))
            mesh('glass_wall',v,f,'glass');cylinder('glass_base',(0,0,0),(0,0,.002),r,'glass')
            if i==287:
                for j in range(8):box('beaker_graduation',(.01,-r-.001,.02+j*.01),(.018,.001,.0008),'white',0)
        elif i==289:
            cylinder('pipette_body',(0,0,.04),(0,0,.19),.006,'028');cylinder('pipette_tip',(0,0,0),(0,0,.04),.0015,'glass');cylinder('pipette_plunger',(0,0,.19),(0,0,.215),.004,'032')
        else:
            tank(.05,.18,'glass' if i==290 else '028');cylinder('specimen_lid',(0,0,.18),(0,0,.205),.047,'red');box('specimen_label',(0,-.051,.11),(.075,.001,.06),'028',.002);text('LAB / 04',(0,-.053,.11),.013,'black')
    elif i in (292,296):
        open_bin(.4,.3,.25,'032');box('sealed_container_lid',(0,0,.26),(.43,.33,.03),'031',.008)
        for x in [-.12,.12]:box('transport_latch',(x,-.17,.24),(.04,.02,.07),'steel',.004)
        if i==296:text('BIOHAZARD',(0,-.158,.13),.035,'red')
    elif i==293:
        for x in [-.033,.033]:softbox('goggle_lens',(x,0,.025),(.062,.018,.05),'glass',.015)
        curve('goggle_bridge',[(-.01,0,.04),(0,-.004,.045),(.01,0,.04)],.004,'rubber')
        curve('goggle_strap',[(-.067,0,.025),(-.08,.08,.025),(0,.15,.025),(.08,.08,.025),(.067,0,.025)],.005,'rubber')
    elif i in (294,295):
        cylinder('emergency_water_riser',(0,0,0),(0,0,2.1 if i==294 else 1.05),.024,'032')
        if i==294:
            curve('shower_arm',[(0,0,2.1),(0,-.12,2.3),(0,-.4,2.3)],.024,'032');cylinder('shower_head',(0,-.4,2.27),(0,-.4,2.30),.13,'026')
            curve('pull_rod',[(.02,-.1,2.2),(.02,-.1,1.7),(.12,-.1,1.6),(-.08,-.1,1.6),(.02,-.1,1.7)],.006,'032')
        else:
            cylinder('eyewash_bowl',(0,0,1.0),(0,0,1.025),.20,'032');curve('eyewash_heads',[(-.1,0,1.05),(-.1,0,1.17),(.1,0,1.17),(.1,0,1.05)],.009,'032')
    elif i in (297,298):
        if i==297:
            box('morgue_cold_store',(0,0,1.05),(2.0,1.0,2.1),'032',.02)
            for j in range(3):
                for s in [-1,1]:
                    x=s*.49;z=.36+j*.68;box('cold_store_door',(x,-.52,z),(.95,.05,.63),'032',.018);pull_handle(x+.32,-.61,z,.20);text(f'04-{j*2+(1 if s<0 else 2):02d}',(x,-.55,z+.16),.055,'black')
        else:
            box('body_tray',(0,0,.025),(.65,2.1,.05),'032',.025)
            for x in [-.32,.32]:box('body_tray_lip',(x,0,.07),(.02,2.1,.09),'032')
            for x in [-.23,.23]:cylinder('tray_runner',(x,-1.05,-.03),(x,1.05,-.03),.017,'032')
    elif i==300:K.medical(i)
    elif i==301:
        softbox('body_bag',(0,0,.12),(.65,2.0,.24),'rubber',.10)
        curve('body_bag_zipper',[(0,-.9,.245),(0,0,.25),(0,.9,.245)],.003,'032')
        for y in [-.65,.65]:
            for s in [-1,1]:curve('body_bag_handle',[(s*.29,y-.07,.12),(s*.39,y-.07,.12),(s*.39,y+.07,.12),(s*.29,y+.07,.12)],.01,'rubber')
    elif i==302:
        box('body_tag',(0,0,.001),(.09,.045,.002),'028',.002)
        curve('tag_string',[(-.04,0,.003),(-.10,0,.003),(-.13,.04,.003),(-.09,.08,.003),(-.04,0,.003)],.001,'white')
    elif i==304:K.floor_detail(44)
    elif i==305:K.window_detail(106);K.window_detail(113)
    elif i==306:hospital_bed(True)
    elif i==307:
        for x in [-.3,.3]:
            for z in [.3,.9]:softbox('padded_wall_cell',(x,0,z),(.59,.09,.59),'fabric_dirty',.04)
    elif i in (308,309):
        softbox('armchair_seat',(0,-.04,.42),(.66,.65,.16),'fabric_blue',.07);softbox('armchair_back',(0,.28,.75),(.75,.18,.70),'fabric_blue',.08)
        for x in [-.37,.37]:softbox('armchair_arm',(x,0,.56),(.15,.74,.38),'fabric_blue',.065)
        for x in [-.28,.28]:
            for y in [-.23,.23]:box('armchair_foot',(x,y,.13),(.06,.06,.26),'wood',.008)
    elif i==310:table(.8,.55,.43,'wood')
    elif i in (311,314):
        for x in ([-.4,.4] if i==311 else [0]):
            cylinder('rehab_parallel_rail',(x,-1.1,.95),(x,1.1,.95),.025,'032')
            for y in [-.85,.85]:cylinder('rail_support',(x,y,0),(x,y,.95),.022,'032');box('support_base',(x,y,.02),(.35,.20,.04),'031')
    elif i==312:
        for x in [-.40,.40]:box('wall_bar_side',(x,0,1.15),(.06,.12,2.3),'wood',.02)
        for j in range(12):cylinder('wall_bar_rung',(-.40,-.03,.18+j*.18),(.40,-.03,.18+j*.18),.018,'wood')
    elif i==313:
        for x in [-.28,.28]:cylinder('exercise_bike_foot',(x,-.45,.06),(x,.45,.06),.024,'031')
        curve('bike_frame',[(0,-.35,.06),(0,0,.42),(0,.35,.06),(0,-.35,.06)],.035,'032')
        cylinder('seat_post',(0,0,.4),(0,0,.85),.024,'032');softbox('bike_saddle',(0,0,.89),(.26,.24,.07),'rubber',.035)
        curve('exercise_handlebar',[(0,.32,.08),(0,.4,.90),(-.2,.37,1.0),(-.2,.2,.95)],.022,'032')
        torus('bike_flywheel',(0,.1,.3),.20,.04,'black',(0,math.pi/2,0))
        cylinder('crank_axle',(-.18,0,.4),(.18,0,.4),.018,'steel')
        for s in [-1,1]:cylinder('pedal_crank',(s*.17,0,.4),(s*.17,s*.12,.4),.012,'steel');box('pedal',(s*.22,s*.12,.4),(.15,.07,.025),'rubber')
    elif i in (315,316,317):
        if i==315:
            for x in [-.27,.27]:
                curve('walker_side',[(x,-.22,0),(x,-.18,.8),(x,.18,.8),(x,.22,0)],.016,'032')
                cylinder('walker_grip',(x,-.1,.8),(x,.10,.8),.022,'rubber')
            for z in [.3,.65]:cylinder('walker_crossbrace',(-.27,.18,z),(.27,.18,z),.013,'032')
        elif i==316:
            curve('crutch_frame',[(0,0,0),(-.08,0,1.12),(.08,0,1.12),(0,0,0)],.014,'032');softbox('crutch_underarm_pad',(0,0,1.14),(.22,.06,.05),'rubber',.024);cylinder('crutch_grip',(-.045,0,.76),(.045,0,.76),.019,'rubber')
        else:curve('cane',[(0,0,0),(0,0,.86),(.03,0,.90),(.12,0,.90)],.013,'wood')
    elif i==318:softbox('rehab_mat',(0,0,.035),(.8,1.8,.07),'fabric_blue',.025)

def utility(i):
    if i==319:
        frame(1.9,.85,.8,'031');box('generator_sump',(0,0,.15),(1.95,.9,.20),'031',.025)
        box('engine_block',(-.35,0,.55),(.7,.6,.56),'026',.03);cylinder('alternator',(.08,0,.56),(.77,0,.56),.28,'032',48)
        for x in [-.52,-.28,-.04]:box('cylinder_head',(x,0,.86),(.18,.48,.13),'031',.012)
        cylinder('exhaust',(-.55,.22,.85),(-.55,.22,1.20),.035,'031');cylinder('muffler',(-.55,.22,1.12),(.25,.22,1.12),.08,'031')
        for j in range(8):torus('alternator_cooling_fin',(.18+j*.065,0,.56),.282,.008,'032',(0,math.pi/2,0))
        box('generator_control_panel',(-.45,-.32,.62),(.38,.04,.26),'black',.012)
    elif i==320:
        box('generator_console',(0,0,.58),(.65,.36,1.16),'026',.025)
        for x in [-.20,0,.20]:cylinder('analogue_dial',(x,-.19,.88),(x,-.20,.88),.055,'white')
        for j in range(4):cylinder('console_button',(-.20+j*.13,-.19,.60),(-.20+j*.13,-.22,.60),.02,'red' if j==0 else 'black')
    elif i in (321,323,324,325):
        if i==321:
            tank(.42,1.35,'031')
            for x in [-.3,.3]:box('fuel_tank_foot',(x,0,.10),(.10,.5,.20),'031')
            cylinder('filler_neck',(0,0,1.35),(0,0,1.48),.05,'032')
        elif i in (323,324):
            tank(.35 if i==323 else .22,1.4 if i==323 else .65,'028' if i==323 else 'red')
            if i==323:
                for z in [.2,.9]:curve('boiler_connection',[(.35,0,z),(.48,0,z),(.48,0,z-.15)],.025,'032')
        else:
            tank(.23,.80,'026');cylinder('compressor_motor',(-.25,0,.75),(.25,0,.75),.13,'031')
            for x in [-.18,.18]:box('compressor_head',(x,0,.95),(.18,.22,.22),'032',.012)
    elif i==322:
        box('pump_skid',(0,0,.06),(1.1,.55,.12),'031',.008)
        cylinder('pump_motor',(-.38,0,.32),(.18,0,.32),.18,'026',48)
        cylinder('pump_volute',(.30,-.12,.32),(.30,.12,.32),.23,'031',48)
        for x in [-.30,-.18,-.06,.06]:torus('motor_fin',(x,0,.32),.181,.006,'032',(0,math.pi/2,0))
        cylinder('pump_outlet',(.30,0,.45),(.30,0,.68),.05,'031');flange((.30,0,.68),'Z',.085)
    elif i==326:
        tank(.075,.48,'red');box('extinguisher_handle',(0,0,.56),(.12,.03,.025),'black',.008)
        curve('extinguisher_hose',[(.04,0,.53),(.12,0,.48),(.12,0,.2),(.08,0,.12)],.009,'rubber')
        box('extinguisher_label',(0,-.077,.28),(.10,.002,.20),'028',.002);text('FIRE',(0,-.08,.34),.025,'red')
    elif i==327:
        box('extinguisher_mount',(0,0,.2),(.08,.015,.4),'031')
        for z in [.12,.30]:curve('bottle_band',[(-.08,0,z),(-.1,-.06,z),(0,-.13,z),(.1,-.06,z),(.08,0,z)],.005,'032')
    elif i==328:
        cabinet(.70,.25,.9,glass=True)
        for ob in K.parts:
            if ob.type=='MESH' and 'cabinet_side' in ob.name:ob.data.materials.clear();ob.data.materials.append(material('red'))
        text('ПОЖАРНЫЙ КРАН',(0,-.14,.80),.045,'white')
    elif i==329:
        p=[]
        for j in range(1000):
            a=j/999*math.tau*8;r=.06+.18*j/999;p.append((r*math.cos(a),0,.28+r*math.sin(a)))
        curve('rolled_fire_hose',p,.013,'fabric_dirty')
        cylinder('hose_coupling',(.25,-.02,.26),(.25,-.10,.26),.025,'steel')
    elif i==330:K.engineering(163)
    elif i==331:
        box('fire_alarm_button',(0,0,.065),(.13,.04,.13),'red',.009);box('break_glass',(0,-.024,.065),(.08,.006,.065),'glass',.002);text('НАЖАТЬ',(0,-.03,.063),.015,'white')
    elif i in (332,333):
        cylinder('ceiling_mount',(0,0,.015),(0,0,.035),.065 if i==332 else .035,'028')
        if i==332:
            cylinder('smoke_detector',(0,0,-.02),(0,0,.015),.055,'028')
            for j in range(20):a=j*math.tau/20;box('detector_vent',(.054*math.cos(a),.054*math.sin(a),0),(.003,.012,.008),'black',.001)
        else:
            cylinder('sprinkler_stem',(0,0,-.05),(0,0,.02),.008,'steel');cylinder('sprinkler_deflector',(0,0,-.055),(0,0,-.05),.026,'steel');cylinder('glass_bulb',(0,0,-.04),(0,0,-.015),.003,'red')
    elif i==334:
        open_bin(.28,.24,.28,'026');curve('bucket_handle',[(-.14,0,.25),(-.14,0,.45),(.14,0,.45),(.14,0,.25)],.004,'steel')
    elif i==335:
        cart_base(.55,.9);frame(.48,.8,.85)
        for y in [-.24,.24]:
            before=len(K.parts);open_bin(.40,.32,.35,'026');move_since(before,(0,y,.2))
        box('janitor_shelf',(0,0,.86),(.54,.86,.025),'032');pull_handle(.28,-.32,.90,.25)
    elif i in (336,337,338,339):
        if i!=338:cylinder('cleaning_handle',(0,0,.05),(0,0,1.35),.012,'wood')
        if i in (336,339):
            box('cleaning_head',(0,0,.045),(.36,.10,.05),'wood',.012)
            for j in range(55):
                x=random.uniform(-.17,.17);y=random.uniform(-.04,.04);curve('mop_strand' if i==336 else 'brush_bristle',[(x,y,.04),(x+random.uniform(-.04,.04),y+random.uniform(-.04,.04),.006)],.003,'fabric_dirty' if i==336 else 'black')
        elif i==337:
            for j in range(60):
                x=random.uniform(-.12,.12);curve('broom_straw',[(x*.15,0,.22),(x,random.uniform(-.03,.03),.005)],.002,'wood')
        else:
            box('dustpan',(0,0,.01),(.28,.25,.02),'026',.012);box('dustpan_back',(0,.12,.05),(.28,.02,.1),'026');cylinder('dustpan_handle',(0,.12,.07),(0,.30,.07),.015,'black')
    elif i==340:cloth('cleaning_rag',.38,.32,.02,'fabric_dirty',.015)
    elif i in (341,342):
        softbox('bottle_body',(0,0,.14),(.12,.07,.27),'026',.025) if i==341 else softbox('jerrycan_body',(0,0,.20),(.30,.16,.40),'026',.035)
        if i==341:cylinder('detergent_cap',(0,0,.28),(0,0,.32),.028,'red')
        else:
            curve('can_handle',[(-.08,0,.40),(-.08,0,.48),(.06,0,.48),(.06,0,.40)],.019,'026');cylinder('can_cap',(.10,0,.40),(.10,0,.44),.032,'032')
    elif i==343:
        softbox('garbage_bag',(0,0,.25),(.45,.4,.50),'rubber',.16)
        curve('bag_tie',[(-.08,0,.46),(0,0,.59),(.07,.02,.49)],.015,'rubber')
        for j in range(12):
            a=j*math.tau/12;curve('bag_crease',[(.19*math.cos(a),.17*math.sin(a),.12),(.2*math.cos(a),.18*math.sin(a),.35),(0,0,.57)],.002,'black')
    elif i==344:open_bin(.55,.37,.30)
    elif i==345:table(1.65,.72,.87,'wood');box('workbench_backboard',(0,.32,1.2),(1.65,.04,.65),'031')
    elif i==346:
        box('vice_base',(0,0,.025),(.22,.16,.05),'031',.015);box('vice_body',(0,0,.1),(.12,.18,.14),'026',.015)
        for y in [-.075,.075]:box('vice_jaw',(0,y,.19),(.18,.03,.06),'steel',.004)
        cylinder('vice_screw',(0,-.20,.1),(0,.08,.1),.014,'steel');cylinder('vice_handle',(-.1,-.20,.1),(.1,-.20,.1),.006,'steel')
    elif i==347:
        box('toolbox',(0,0,.1),(.45,.22,.20),'031',.018);box('toolbox_lid',(0,0,.21),(.46,.23,.025),'026',.014);curve('toolbox_handle',[(-.08,0,.23),(-.08,0,.29),(.08,0,.29),(.08,0,.23)],.01,'black')
        for x in [-.14,.14]:box('toolbox_latch',(x,-.12,.19),(.035,.018,.06),'steel')
    elif i==348:
        cylinder('hammer_handle',(0,0,0),(0,0,.31),.015,'wood');box('hammer_head',(0,0,.30),(.14,.04,.05),'031',.008)
    elif i==349:
        cylinder('screwdriver_grip',(0,0,0),(0,0,.10),.018,'red');cylinder('screwdriver_shaft',(0,0,.10),(0,0,.24),.004,'steel');box('flat_tip',(0,0,.245),(.009,.002,.012),'steel',.001)
    elif i in (350,351):
        if i==350:
            box('wrench_shank',(0,0,.12),(.025,.008,.24),'steel',.008)
            for z in [0,.24]:
                curve('wrench_jaw',[(-.027,0,z+.025),(-.033,0,z-.01),(0,0,z-.026),(.033,0,z-.01),(.027,0,z+.025)],.008,'steel')
        else:
            for s in [-1,1]:
                curve('pliers_handle',[(s*.035,0,0),(s*.025,0,.11),(-s*.012,0,.16),(-s*.012,0,.20)],.007,'steel')
                cylinder('pliers_grip',(s*.035,0,0),(s*.027,0,.09),.011,'red')
            cylinder('pliers_pivot',(0,-.005,.12),(0,.005,.12),.011,'steel')
    elif i==352:
        softbox('drill_body',(0,0,.17),(.22,.08,.10),'026',.025);ob=softbox('drill_grip',(-.06,0,.07),(.06,.065,.16),'rubber',.018);ob.rotation_euler.y=-.2
        cylinder('drill_chuck',(.11,0,.17),(.17,0,.17),.03,'steel');cylinder('drill_bit',(.17,0,.17),(.28,0,.17),.003,'steel')
    elif i==353:
        for s in [-1,1]:
            curve('ladder_side',[(s*.25,-.30,0),(s*.25,0,1.8),(s*.25,.30,0)],.022,'032')
        for j in range(7):cylinder('ladder_rung',(-.25,-.3+(j+1)*.037,.22*(j+1)),(.25,-.3+(j+1)*.037,.22*(j+1)),.016,'032')
    elif i in (354,355):
        mat='wood' if i==354 else '026'
        for x in [-.45,0,.45]:
            for y in [-.40,0,.40]:box('pallet_block',(x,y,.07),(.15,.14,.12),mat,.006)
        for z in [.015,.14]:
            for j in range(7):box('pallet_slat',(0,-.45+j*.15,z),(1.2,.10,.025),mat,.003)
    elif i in (356,357,358):
        mat='wood' if i==358 else '028';w,d,h=.60,.44,.42
        open_bin(w,d,h,mat)
        if i==357:
            for s in [-1,1]:ob=box('open_box_flap',(s*.37,0,h+.05),(.22,d,.006),mat,.001);ob.rotation_euler.y=s*.8
        else:box('box_lid',(0,0,h+.01),(w,d,.02),mat,.003)
        if i==358:
            for x in [-.24,.24]:box('crate_reinforcement',(x,-d/2-.02,h/2),(.06,.035,h),mat,.003)
    elif i in (359,360):
        tank(.29,.88,'031' if i==359 else '026')
        for z in [.1,.3,.6,.8]:torus('barrel_hoop',(0,0,z),.292,.012,'031' if i==359 else '026',(0,0,0))
    elif i==361:
        frame(2.4,.9,2.5,'031')
        for z in [.15,1.15,2.15]:
            box('pallet_shelf',(0,0,z),(2.45,.95,.05),'032')
            for y in [-.45,.45]:box('shelf_beam',(0,y,z-.05),(2.45,.06,.12),'red')
    elif i in (362,363):
        if i==362:
            for x in [-.22,.22]:
                wheel(x,0,.13,.12);curve('handtruck_rail',[(x,-.22,.10),(x,0,.10),(x,0,1.2),(x,-.08,1.3)],.02,'031')
            box('handtruck_toe',(0,-.15,.08),(.52,.38,.035),'031');cylinder('handtruck_crossbar',(-.22,0,.65),(.22,0,.65),.016,'031')
        else:
            for x in [-.22,.22]:
                box('pallet_jack_fork',(x,-.15,.10),(.15,1.2,.10),'026',.025);wheel(x,-.65,.05,.04)
            box('jack_body',(0,.38,.18),(.55,.25,.25),'026',.025);curve('jack_handle',[(0,.4,.2),(0,.6,1.05),(-.18,.6,1.16),(.18,.6,1.16),(0,.6,1.05)],.023,'031')
    elif i==364:
        box('forklift_chassis',(0,0,.35),(1.05,1.8,.4),'026',.06)
        for x in [-.51,.51]:
            for y in [-.55,.55]:cylinder('forklift_tire',(x-.09,y,.25),(x+.09,y,.25),.25,'rubber',48)
        frame(.90,1.0,2.0,'031');box('forklift_roof',(0,.15,2.03),(1.1,1.3,.08),'031',.025)
        softbox('driver_seat',(0,.18,.82),(.5,.5,.12),'rubber');softbox('driver_seat_back',(0,.4,1.1),(.5,.10,.55),'rubber')
        for x in [-.34,.34]:box('forklift_mast',(x,-1.0,1.2),(.10,.12,2.4),'031');box('fork',(x,-1.55,.16),(.12,1.2,.10),'031',.015)
        torus('steering_wheel',(0,-.30,1.12),.14,.015,'black',(0,0,0))
    elif i==365:cylinder('wrap_film',(0,0,0),(0,0,.5),.07,'glass');cylinder('film_core',(0,0,-.02),(0,0,.52),.025,'wood')

def rail(length=3,h=1.05):
    for x in [-length/2,0,length/2]:cylinder('railing_post',(x,0,0),(x,0,h),.022,'031')
    for z in [.48,h]:cylinder('railing_horizontal',(-length/2,0,z),(length/2,0,z),.022,'032')
    for j in range(int(length/.15)):
        x=-length/2+.075+j*.15;cylinder('baluster',(x,0,.1),(x,0,h-.02),.009,'031')

def stair(n=12,width=1.4,rise=.16,run=.28):
    # Solid, closed stepped profile with exact rise/run.
    profile=[(0,0),(n*run,0),(n*run,n*rise)]
    for j in range(n-1,-1,-1):profile.extend([(j*run,(j+1)*rise),(j*run,j*rise)])
    slab_polygon('stair_flight',profile,-width/2,width/2,'029')
    for j in range(n):box('stair_nosing',((j+.05)*run,0,(j+1)*rise+.003),(.025,width,.006),'032',.002)

def architecture(i):
    if i==366:stair()
    elif i==367:box('stair_landing',(0,0,.08),(2.8,1.5,.16),'029',.006)
    elif i==368:rail()
    elif i==369:K.wall(20)
    elif i==370:
        stair(16,1.0,.17,.27)
        for y in [-.52,.52]:
            for x in [0,2.1,4.3]:cylinder('fire_stair_post',(x,y,x*.17/.27+.17),(x,y,x*.17/.27+1.2),.02,'031')
            cylinder('fire_stair_handrail',(0,y,1.2),(4.32,y,3.92),.025,'031')
    elif i==371:
        for x in [-.30,.30]:cylinder('roof_ladder_side',(x,0,0),(x,0,4.5),.025,'031')
        for j in range(16):cylinder('roof_ladder_rung',(-.3,0,j*.28),(.3,0,j*.28),.016,'032')
        for z in [2.1,2.8,3.5,4.2]:
            curve('safety_cage_hoop',[(.48*math.cos(a),-.12-.48*math.sin(a),z) for a in [j*math.pi/32 for j in range(33)]],.012,'031')
        for a in [math.pi*.2,math.pi*.5,math.pi*.8]:cylinder('cage_vertical',(.48*math.cos(a),-.12-.48*math.sin(a),2.1),(.48*math.cos(a),-.12-.48*math.sin(a),4.2),.009,'031')
    elif i==372:
        box('lift_floor',(0,0,.04),(1.8,2.4,.08),'038',.003)
        for s in [-1,1]:box('lift_side',(s*.90,0,1.2),(.04,2.4,2.4),'032',.008)
        box('lift_back',(0,1.20,1.2),(1.8,.04,2.4),'032',.008);box('lift_ceiling',(0,0,2.4),(1.8,2.4,.04),'028')
        for s in [-1,1]:cylinder('lift_grab_rail',(s*.85,-.9,.9),(s*.85,.9,.9),.018,'steel')
        box('lift_diffuser',(0,0,2.36),(.85,1.5,.015),emission('phosphor',(.67,.9,1),2),.01)
    elif i in (373,374):
        for x in [-.45,.45]:
            box('lift_sliding_leaf',(x,0,1.15),(.89,.045,2.3),'032',.012)
            box('door_safety_edge',(x+(.44 if x<0 else -.44),-.03,1.15),(.01,.015,2.25),'rubber',.001)
        if i==374:
            for x in [-.96,.96]:box('lift_jamb',(x,0,1.2),(.12,.18,2.4),'031')
            box('lift_header',(0,0,2.42),(2.04,.18,.15),'031')
    elif i in (375,376,377):
        w,h=(.15,.6) if i==375 else ((.10,.2) if i==376 else (.28,.13))
        box('lift_control_plate',(0,0,h/2),(w,.02,h),'032',.01)
        if i==377:
            box('lift_display',(0,-.015,h/2),(w-.02,.008,h-.02),'black',.004);text('04 ↑',(0,-.022,.04),.07,'red')
        else:
            count=6 if i==375 else 2
            for j in range(count):
                z=.06+j*(h-.12)/max(count-1,1);cylinder('lift_button',(0,-.012,z),(0,-.02,z),.019,'steel');text(str(j+1) if i==375 else ('↑' if j else '↓'),(0,-.023,z-.006),.017,'black')
    elif i==378:
        for x in [-.07,-.035,0,.035,.07]:cylinder('lift_wire_rope',(x,0,0),(x,0,4),.009,'031')
    elif i==379:
        cylinder('lift_motor',(-.45,0,.3),(.25,0,.3),.25,'026',48)
        for j in range(7):torus('motor_cooling_fin',(-.4+j*.09,0,.3),.252,.01,'032',(0,math.pi/2,0))
        for x in [.36,.42,.48]:torus('traction_sheave',(x,0,.3),.36,.024,'031',(0,math.pi/2,0))
        box('lift_motor_bed',(0,0,.04),(1.25,.75,.08),'031')
    elif i==380:K.wall(17)
    elif i in (381,382):
        if i==382:K.engineering(174)
        else:
            cylinder('roof_vent_riser',(0,0,0),(0,0,.9),.23,'032',48);cylinder('roof_vent_cap',(0,0,.98),(0,0,1.02),.35,'032',48)
            for j in range(8):a=j*math.tau/8;cylinder('cap_support',(.22*math.cos(a),.22*math.sin(a),.82),(.22*math.cos(a),.22*math.sin(a),1),.008,'032')
            box('roof_vent_flashing',(0,0,.015),(.85,.85,.03),'032',.01)
    elif i==383:K.door(71)
    elif i==384:K.floor_detail(59)
    elif i==385:
        box('roof_parapet',(0,0,.5),(3,.25,1),'029',.008);box('parapet_coping',(0,0,1.025),(3,.34,.05),'032',.008)
    elif i==386:rail(3,1.1)
    elif i==387:
        cylinder('roof_drain_body',(0,0,-.1),(0,0,0),.08,'031');torus('roof_drain_flange',(0,0,.003),.13,.015,'032',(0,0,0))
        for j in range(10):a=j*math.tau/10;curve('drain_leaf_guard',[(.09*math.cos(a),.09*math.sin(a),0),(.09*math.cos(a),.09*math.sin(a),.12),(0,0,.15)],.005,'031')
    elif i==388:K.engineering(158)
    elif i==389:
        frame(2.8,.9,.25,'031')
        for j in range(60):box('walkway_grate',(-1.45+j*.049,0,.27),(.015,1,.03),'032')
        for y in [-.52,.52]:
            start=len(K.parts);rail(3,1);move_since(start,(0,y,.27))
    elif i==390:
        cylinder('helipad_base',(0,0,.08),(0,0,.14),5,'029',96)
        for x in [-.7,.7]:box('helipad_H',(x,0,.142),(.22,2.7,.003),'white',0)
        box('helipad_H_crossbar',(0,0,.143),(1.5,.22,.003),'white',0);torus('helipad_marking',(0,0,.145),3.8,.055,'white',(0,0,0))
    elif i==391:
        cylinder('helipad_light_base',(0,0,0),(0,0,.06),.06,'031');cylinder('landing_lens',(0,0,.06),(0,0,.12),.04,emission('landing_lens',(.1,.8,.3),2))
    elif i==392:
        cylinder('windsock_pole',(0,0,0),(0,0,3.2),.03,'032')
        # tapered wind cone in actual 3D, with alternating bands
        v=[];f=[]
        for j in range(9):
            for a in range(32):t=a*math.tau/32;r=.22-j*.022;v.append((j*.15,r*math.cos(t),3.2+r*math.sin(t)-j*.012))
        for j in range(8):
            for a in range(32):q=j*32+a;f.append((q,j*32+(a+1)%32,(j+1)*32+(a+1)%32,q+32))
        ob=mesh('windsock_cloth',v,f,'red');ob.data.materials.append(material('white'))
        for j,p in enumerate(ob.data.polygons):p.material_index=(j//32)%2
    elif i==393:
        cylinder('antenna_mast',(0,0,0),(0,0,3),.03,'032')
        for z,length in [(1.2,1.3),(1.65,1.1),(2.1,.9),(2.55,.7)]:cylinder('antenna_director',(-length/2,0,z),(length/2,0,z),.008,'032')
        for a in [0,math.tau/3,math.tau*2/3]:cylinder('antenna_guy',(0,0,2.5),(math.cos(a),math.sin(a),0),.003,'steel')
    elif i==394:
        v=[(0,0,.55)];f=[]
        for rj in range(1,17):
            r=rj*.5/16
            for j in range(64):a=j*math.tau/64;v.append((r*math.cos(a),r*math.sin(a),.55+.28*(r/.5)**2))
        for j in range(64):f.append((0,j+1,(j+1)%64+1))
        for rj in range(15):
            for j in range(64):q=1+rj*64+j;f.append((q,1+rj*64+(j+1)%64,1+(rj+1)*64+(j+1)%64,q+64))
        ob=mesh('satellite_reflector',v,f,'028');ob.rotation_euler.x=.7
        cylinder('dish_mount',(0,0,0),(0,0,.6),.035,'032');curve('LNB_support',[(0,-.45,.65),(0,-.6,.85),(0,-.2,1.05)],.012,'032');box('LNB',(0,-.2,1.05),(.07,.09,.06),'white')

def bathroom(i):
    if i in (395,396,406):
        if i==395:
            for s in [-1,1]:box('cubicle_side',(s*.5,.1,1.05),(.025,1.4,1.8),'026',.008)
            for x in [-.53,.53]:cylinder('cubicle_foot',(x,-.6,0),(x,-.6,.2),.015,'032')
        if i in (395,396):
            box('cubicle_door',(0,-.6,1.05),(.86,.025,1.65),'026',.008);lever_handle(.30,-.62,.95)
            for z in [.45,1.65]:K.hinge(None,-.44,z)
        if i==406:
            box('shower_glass',(0,0,1),(.85,.012,2),'glass',.006)
            for x in [-.43,.43]:box('shower_channel',(x,0,1),(.025,.035,2.02),'032')
    elif i in (397,398):
        if i==397:
            softbox('toilet_pedestal',(0,.05,.18),(.25,.38,.36),'white',.1)
            # Oval rim and hollow bowl visible from above.
            v=[];f=[]
            for r,z in [(1,.43),(.76,.43),(.22,.24)]:
                for j in range(64):a=j*math.tau/64;v.append((.19*r*math.cos(a),.27*r*math.sin(a)-.08,z))
            for ring in range(2):
                for j in range(64):q=ring*64+j;f.append((q,ring*64+(j+1)%64,(ring+1)*64+(j+1)%64,q+64))
            ob=mesh('toilet_bowl',v,f,'white')
            for p in ob.data.polygons:p.use_smooth=True
            o=torus('toilet_seat',(0,-.08,.445),.18,.022,'028',(0,0,0));o.scale.y=1.40
            softbox('toilet_cistern',(0,.25,.56),(.39,.16,.43),'white',.04);box('cistern_lid',(0,.25,.79),(.42,.19,.035),'white',.015);cylinder('flush_button',(0,.25,.809),(0,.25,.814),.023,'steel')
        else:
            softbox('urinal_back',(0,.05,.45),(.33,.10,.62),'white',.08);softbox('urinal_lower_bowl',(0,-.07,.24),(.33,.30,.16),'white',.07)
            cylinder('urinal_flush_pipe',(0,.05,.7),(0,.05,.93),.015,'steel');cylinder('urinal_flush_button',(0,.05,.89),(0,-.005,.89),.026,'steel')
    elif i==399:K.medical(230)
    elif i==400:K.medical(231)
    elif i in (401,402):
        box('mirror_backing',(0,0,.4),(.6,.022,.8),'031',.012)
        box('mirror',(0,-.015,.4),(.57,.006,.77),'steel',.006)
        if i==402:
            for j in range(9):a=j*math.tau/9;curve('mirror_crack',[(.06,-.019,.37),(.07+math.cos(a)*.16,-.019,.38+math.sin(a)*.16),(max(-.28,min(.28,.07+math.cos(a)*.5)),-.019,max(.02,min(.78,.38+math.sin(a)*.5)))],.0009,'black')
    elif i==403:
        for x in [-.07,.07]:curve('roll_holder',[(x,0,.06),(x,-.10,.06)],.008,'steel')
        cylinder('toilet_roll',(-.055,-.08,.06),(.055,-.08,.06),.055,'fabric_dirty');cylinder('toilet_roll_core',(-.057,-.08,.06),(.057,-.08,.06),.02,'wood')
    elif i==404:
        for s in [-1,1]:
            box('shower_tray_rim',(s*.43,0,.05),(.05,.9,.10),'white',.018)
            box('shower_tray_rim',(0,s*.43,.05),(.86,.05,.10),'white',.018)
        box('shower_tray_base',(0,0,.02),(.86,.86,.04),'white',.018);cylinder('shower_drain',(.27,.27,.041),(.27,.27,.043),.04,'steel')
    elif i==405:
        curve('shower_neck',[(0,0,0),(0,0,.35),(0,-.07,.44),(0,-.20,.44)],.016,'steel');cylinder('shower_head',(0,-.20,.41),(0,-.20,.44),.08,'steel')
        for j in range(30):a=j*2.4;r=.06*math.sqrt(j/30);cylinder('water_jet_hole',(r*math.cos(a),-.20+r*math.sin(a),.408),(r*math.cos(a),-.20+r*math.sin(a),.410),.0015,'black',6)
    elif i==407:open_bin(.45,.45,.65,'028');box('laundry_bin_lid',(0,0,.67),(.49,.49,.035),'028',.015)

def commerce(i):
    if i in (408,419):
        K.office(240)
        if i==419:
            for ob in K.parts:
                if 'shelf' in ob.name and ob.type=='MESH':ob.data.materials.clear();ob.data.materials.append(material('028'))
    elif i in (409,410,420,421,422,423):
        w,d,h=1.5,.65,.9
        box('counter_plinth',(0,0,.08),(w,d,.16),'031',.012)
        box('counter_carcass',(0,0,.45),(w,d,.75),'028',.012)
        if i in (409,420):
            for x in [-w/2,w/2]:box('display_sideglass',(x,0,.94),(.012,d,.64),'glass',.003)
            box('display_frontglass',(0,-d/2,.94),(w,.012,.64),'glass',.003);box('display_topglass',(0,0,1.27),(w,d,.012),'glass',.003)
            for z in [.70,.95,1.20]:box('display_glass_shelf',(0,0,z),(w-.04,d-.04,.008),'glass',.002)
        elif i==421:
            for x in [-.37,.37]:box('freezer_sliding_lid',(x,0,.92),(.73,.61,.025),'glass',.009)
            for j in range(10):box('freezer_vent',(-.5,-.33,.18+j*.025),(.5,.004,.008),'black',0)
        elif i==423:
            box('checkout_belt',(0,0,.86),(1.8,.60,.05),'rubber',.015)
            for x in [-.90,.90]:cylinder('belt_roller',(x,-.3,.86),(x,.3,.86),.03,'032')
        else:box('countertop',(0,0,.88),(w+.06,d+.06,.04),'wood',.015)
    elif i in (411,412,413,414,426,427,428,429,430):
        if i in (411,427,428):
            w,d,h=(.10,.035,.07) if i==411 else (.13,.06,.2)
            box('retail_package',(0,0,h/2),(w,d,h),'028',.003);box('printed_band',(0,-d/2-.001,h*.55),(w*.9,.002,h*.3),'026',.001);text('04 / MED' if i==411 else 'ПРОДУКТ',(0,-d/2-.003,h*.56),.013,'black')
        elif i==412:
            box('blister_foil',(0,0,.002),(.06,.11,.004),'032',.002)
            for x in [-.015,.015]:
                for j in range(4):softbox('pill_bubble',(x,-.04+j*.027,.007),(.015,.021,.01),'glass',.005)
        elif i==414:
            box('ampoule_tray',(0,0,.005),(.10,.07,.01),'028')
            for x in [-.035,-.0175,0,.0175,.035]:cylinder('ampoule_body',(x,0,.01),(x,0,.06),.005,'glass');cylinder('ampoule_neck',(x,0,.06),(x,0,.085),.002,'glass')
        elif i==426:
            for x in [-.04,.04]:
                for y in [-.04,.04]:cylinder('food_tin',(x,y,0),(x,y,.10),.035,'032');torus('tin_rim',(x,y,.1),.035,.002,'032',(0,0,0))
        elif i==429:
            cylinder('drink_can',(0,0,0),(0,0,.115),.033,'026');torus('can_rim',(0,0,.116),.032,.002,'032',(0,0,0));torus('pull_tab',(0,0,.119),.008,.002,'steel',(0,0,0))
        else:
            r,h=(.022,.07) if i==413 else (.035,.22)
            tank(r,h,'glass');cylinder('bottle_cap',(0,0,h),(0,0,h+.02),r*.6,'028')
    elif i==415:appliance()
    elif i in (416,417,418):
        if i==416:
            box('till_base',(0,0,.05),(.4,.34,.10),'028',.02);softbox('till_upper',(0,.08,.18),(.35,.20,.2),'028',.025);box('till_display',(0,-.026,.23),(.25,.01,.07),'black',.005)
            for j in range(3):
                for x in [-.08,-.04,0,.04,.08]:box('till_key',(x,-.10+j*.03,.12),(.027,.023,.012),'white',.003)
        elif i==417:
            softbox('scanner_handle',(0,0,.07),(.04,.05,.14),'black',.015);softbox('scanner_head',(0,-.03,.17),(.075,.10,.07),'black',.018);box('scanner_window',(0,-.085,.17),(.05,.005,.04),'red',.005)
        else:
            softbox('bank_terminal',(0,0,.025),(.085,.16,.05),'black',.012);box('terminal_display',(0,.035,.053),(.065,.055,.004),'glass',.003)
            for j in range(4):
                for k in range(3):box('terminal_key',((k-1)*.021,-.055+j*.02,.054),(.016,.014,.004),'white',.002)
    elif i in (424,425):
        w,d,h=(.55,.75,.45) if i==424 else (.35,.25,.25)
        z=.5 if i==424 else 0
        # wire basket has actual openings
        for j in range(16):
            x=-w/2+j*w/15;curve('basket_wire',[(x,-d/2,z+h),(x,-d/2,z),(x,d/2,z),(x,d/2,z+h)],.003,'032')
        for k in range(8):
            zz=z+k*h/7;curve('basket_band',[(-w/2,-d/2,zz),(w/2,-d/2,zz),(w/2,d/2,zz),(-w/2,d/2,zz),(-w/2,-d/2,zz)],.004,'032')
        if i==424:
            cart_base(w,.8);curve('shopping_cart_handle',[(-w/2,d/2,z+h),( -w/2,d/2+.15,z+h), (w/2,d/2+.15,z+h),(w/2,d/2,z+h)],.016,'026')
            for x in [-w/2,w/2]:cylinder('basket_support',(x,.2,.17),(x,.2,z+h),.015,'032')
        else:curve('basket_handle',[(-w/2,0,h),(-w/2,0,h+.18),(w/2,0,h+.18),(w/2,0,h)],.007,'black')
    elif i in (431,432):
        open_bin(.48,.34,.22,'wood')
        for j in range(12):
            x=random.uniform(-.18,.18);y=random.uniform(-.12,.12)
            softbox('produce' if i==431 else 'bread',(x,y,.16),(.085,.08,.08) if i==431 else (.12,.065,.065),'026' if i==431 else 'wood',.03)
    elif i==433:table(.8,.8,.74,'wood')
    elif i==434:K.office(236)
    elif i==435:K.medical(212)
    elif i in (436,437):
        r,h=(.12,.018) if i==436 else (.035,.11)
        if i==436:
            cylinder('plate_center',(0,0,0),(0,0,.006),.09,'white');torus('plate_rim',(0,0,.01),.105,.012,'white',(0,0,0))
        else:
            v=[];f=[]
            for rr,z in [(r*.8,0),(r,h),(r-.002,h),(r*.8-.002,.004)]:
                for j in range(48):a=j*math.tau/48;v.append((rr*math.cos(a),rr*math.sin(a),z))
            for k in range(3):
                for j in range(48):a=k*48+j;f.append((a,k*48+(j+1)%48,(k+1)*48+(j+1)%48,a+48))
            mesh('drinking_glass',v,f,'glass')
    elif i==438:
        for x in [-.04,0,.04]:box('cutlery_handle',(x,0,.003),(.009,.15,.006),'steel',.004)
        softbox('spoon_bowl',(-.04,.10,.003),(.035,.055,.009),'steel',.004)
        box('knife_blade',(.04,.10,.003),(.018,.08,.003),'steel',.003)
        for x in [-.009,-.003,.003,.009]:box('fork_tine',(x,.11,.003),(.002,.05,.003),'steel',.001)
    elif i==439:
        box('coffee_machine',(0,0,.22),(.48,.33,.44),'032',.025);box('coffee_front',(0,-.18,.24),(.44,.025,.30),'black',.012)
        for x in [-.12,.12]:cylinder('coffee_head',(x,-.21,.23),(x,-.26,.23),.04,'steel');cylinder('portafilter_handle',(x,-.26,.23),(x,-.40,.23),.016,'black')
        box('coffee_drip_tray',(0,-.24,.04),(.44,.18,.03),'032',.01)
    elif i==440:table(1.6,.75,.85);start=len(K.parts);K.medical(230);move_since(start,(0,0,.77))
    elif i==441:
        box('kitchen_range',(0,0,.43),(.9,.7,.86),'032',.018)
        for x in [-.24,.24]:
            for y in [-.18,.18]:torus('burner_ring',(x,y,.88),.095,.008,'black',(0,0,0));box('burner_support',(x,y,.884),(.24,.015,.015),'031');box('burner_support',(x,y,.884),(.015,.24,.015),'031')
        box('oven_door',(0,-.365,.43),(.78,.025,.50),'031',.015);pull_handle(.30,-.42,.60,.3)
    elif i==442:
        box('hood_lower',(0,0,.1),(1.5,.85,.20),'032',.025);box('hood_stack',(0,.10,.55),(.55,.45,.75),'032',.018)
        for j in range(16):box('grease_filter',(0,-.35+j*.045,-.005),(1.35,.018,.008),'031')
    elif i==443:appliance(.7,.7,1.9,'032')
    elif i==444:table()

def roadwheel(x,y,z,r=.31):
    cylinder('vehicle_tire',(x-.09,y,z),(x+.09,y,z),r,'rubber',48)
    for s in [-1,1]:
        cylinder('wheel_rim',(x+s*.095,y,z),(x+s*.10,y,z),r*.64,'032',32)
        for j in range(5):
            a=j*math.tau/5;cylinder('wheel_lug',(x+s*.10,y+math.cos(a)*r*.35,z+math.sin(a)*r*.35),(x+s*.105,y+math.cos(a)*r*.35,z+math.sin(a)*r*.35),.009,'steel',6)
    for j in range(36):
        a=j*math.tau/36;ob=box('tread_block',(x,y+math.sin(a)*r,z+math.cos(a)*r),(.18,.035,.017),'rubber',.003);ob.rotation_euler.x=-a

def vehicle(i):
    if i==489:roadwheel(0,0,.32);return
    if i==490:
        # elongated fuselage, cabin glass, tail boom, landing skids and main/tail rotors
        softbox('helicopter_fuselage',(0,0,1.5),(1.65,3.5,1.55),'028',.6)
        softbox('cockpit_canopy',(0,-1.35,1.68),(1.5,1.1,1.15),'glass',.45)
        for x in [-.45,.45]:box('cockpit_frame',(x,-1.73,1.72),(.045,.035,1.0),'032',.008)
        cylinder('tail_boom',(0,1.3,1.7),(0,5.2,2.15),.15,'028')
        slab_polygon('tail_fin',[(-.15,0),(.15,0),(.2,1.15),(-.22,.9)],-.06,.06,'red');fin=K.parts[-1];fin.location=(0,5.0,2.1)
        for x in [-.75,.75]:
            curve('landing_skid',[(x,-1.9,.25),(x,-1.65,.15),(x,1.55,.15)],.045,'031')
            for y in [-.9,.9]:cylinder('skid_strut',(x,y,.15),(x*.65,y,.88),.032,'032')
        cylinder('rotor_mast',(0,0,2.15),(0,0,2.95),.065,'031')
        for a in [0,math.pi/2,math.pi,math.pi*1.5]:
            ob=box('main_rotor',(math.cos(a)*1.8,math.sin(a)*1.8,3.0),(3.5,.18,.04),'031',.018);ob.rotation_euler.z=a
        cylinder('tail_rotor_axle',(-.16,5.2,2.45),(.16,5.2,2.45),.03,'031')
        for a in [0,math.pi/2]:ob=box('tail_rotor',(-.18,5.2,2.45),(.025,1.1,.09),'031');ob.rotation_euler.x=a
        for s in [-1,1]:
            box('medical_cross',(s*.835,.4,1.65),(.005,.65,.18),'red',.005);box('medical_cross',(s*.84,.4,1.65),(.005,.18,.65),'red',.005)
        return
    van=i in (484,486,487);truck=i==487
    w,l,h=(1.9,4.8,2.25) if van else (1.75,4.2,1.45)
    if truck:w,l,h=2.25,6.3,2.8
    box('vehicle_chassis',(0,0,.46),(w*.88,l*.88,.18),'031',.05)
    # separate upper cabin and lower sill permit clean window geometry
    box('lower_body',(0,0,.77),(w,l,.55),'028' if i in (484,486) else '026',.12)
    if truck:
        box('truck_cargo_box',(0,.75,1.75),(w,3.9,2.0),'028',.06)
        box('truck_cab',(0,-2.1,1.34),(w,1.8,1.5),'026',.16)
    elif van:box('van_upper_body',(0,.35,1.62),(w,l-.7,1.3),'028',.16)
    else:softbox('car_cabin',(0,.18,1.26),(w*.88,2.25,.72),'026',.18)
    # dark panes are separate from painted pillars and door panels
    fronty=-l/2+.65
    box('windscreen',(0,fronty,1.48 if van else 1.28),(w*.83,.035,.70 if van else .49),'glass',.06)
    for side in [-1,1]:
        for y in ([-1.15,.1,1.25] if van else [-.65,.50]):
            box('side_window',(side*(w/2+.005),y,1.60 if van else 1.35),(.018,.9,.62 if van else .40),'glass',.04)
            box('side_door_seam',(side*(w/2+.012),y+.49,.92),(.003,.005,.75),'black',0)
            box('door_handle',(side*(w/2+.025),y-.20,1.03),(.035,.14,.025),'steel',.009)
        for y in [-l*.31,l*.31]:roadwheel(side*w*.49,y,.35,.34 if not truck else .45)
        box('side_mirror',(side*(w/2+.16),fronty+.2,1.4),(.18,.24,.14),'031',.05)
    for y in [-l/2-.025,l/2+.025]:
        box('vehicle_bumper',(0,y,.52),(w+.05,.12,.15),'031',.04)
        box('license_plate',(0,y+(-.065 if y<0 else .065),.66),(.40,.006,.09),'028',.006)
        for x in [-w*.36,w*.36]:box('headlamp' if y<0 else 'tail_lamp',(x,y,.91),(.30,.018,.13),'glass' if y<0 else 'red',.025)
    if i==484:
        box('ambulance_lightbar',(0,-.55,2.32),(1.1,.30,.14),'032',.035)
        for x in [-.38,.38]:box('blue_beacon',(x,-.55,2.37),(.30,.27,.16),'glass',.035)
        for side in [-1,1]:
            box('ambulance_stripe',(side*(w/2+.015),.4,.94),(.005,l-.5,.12),'red',.002)
            box('medical_cross',(side*(w/2+.018),1.2,1.75),(.008,.55,.15),'red');box('medical_cross',(side*(w/2+.018),1.2,1.75),(.008,.15,.55),'red')
    if i==488:
        ob=box('crumpled_bonnet',(0,-1.6,1.13),(1.4,.9,.12),'031',.035);ob.rotation_euler.y=.20
        for side in [-1,1]:
            for j in range(6):curve('body_gouge',[(side*.88,-1+j*.15,.90),(side*.89,-.60+j*.15,1.0)],.004,'031')

def outdoors(i):
    if i==445:
        box('perimeter_concrete',(0,0,1.6),(3,.22,3.2),'029',.008)
        for z in [.5,1.5,2.5]:box('perimeter_casting_rib',(0,-.13,z),(3,.04,.09),'029',.005)
    elif i==446:box('perimeter_post',(0,0,1.8),(.32,.32,3.6),'029',.012)
    elif i==447:K.wall(17)
    elif i==448:
        p=[]
        for j in range(1500):a=j/1499*math.tau*12;p.append((-1.5+j/1499*3,.23*math.cos(a),.3+.23*math.sin(a)))
        curve('razor_wire_coil',p,.003,'031')
        for j in range(96):
            a=j*math.tau/8;x=-1.5+j/95*3;y=.23*math.cos(a);z=.3+.23*math.sin(a)
            cylinder('barb',(x-.025,y-.02,z-.02),(x+.025,y+.02,z+.02),.0015,'steel',6)
    elif i==449:
        curve('wire_bracket',[(0,0,0),(0,0,.5),(.28,0,.8)],.018,'031')
        for z in [.58,.72]:torus('wire_eye',(.1+(z-.58),0,z),.012,.003,'steel')
    elif i in (450,451):
        w=3 if i==450 else 1
        for x in [-w/2,w/2]:box('gate_post',(x,0,1.25),(.09,.09,2.5),'031')
        for z in [.15,2.25]:box('gate_rail',(0,0,z),(w,.07,.06),'031')
        for j in range(int(w/.12)):cylinder('gate_picket',(-w/2+.06+j*.12,0,.15),(-w/2+.06+j*.12,0,2.25),.011,'031')
        lever_handle(w*.35,-.04,1)
    elif i in (452,453):
        box('barrier_pedestal',(0,0,.52),(.28,.32,1.04),'028',.025)
        if i==452:
            box('barrier_arm',(1.6,0,.88),(3.4,.065,.095),'white',.014)
            for j in range(8):ob=box('barrier_red_band',(.25+j*.4,-.035,.88),(.17,.003,.09),'red',.001);ob.rotation_euler.y=.2
        else:
            box('barrier_card_reader',(0,-.17,.8),(.16,.025,.2),'black',.008);cylinder('call_button',(0,-.19,.55),(0,-.205,.55),.027,'steel')
    elif i in (454,457):
        if i==457:
            frame(2.4,2.4,4.2,'031')
            for y in [-1.2,1.2]:
                for x in [-1.2,1.2]:cylinder('tower_brace',(x,y,0),(-x,y,4.2),.035,'031')
        z=4.2 if i==457 else 0
        box('guard_booth_floor',(0,0,z+.06),(2.5,2.5,.12),'029');box('guard_booth_roof',(0,0,z+2.5),(2.8,2.8,.10),'031')
        for x in [-1.20,1.20]:
            for y in [-1.20,1.20]:box('booth_corner',(x,y,z+1.25),(.08,.08,2.5),'032')
        for side in [-1,1]:
            box('booth_side_lower',(side*1.20,0,z+.65),(.05,2.4,1.2),'026')
            box('booth_side_glass',(side*1.20,0,z+1.8),(.012,2.3,1.0),'glass')
        box('booth_back',(0,1.20,z+1.25),(2.4,.05,2.5),'026')
        for x in [-.9,.9]:box('booth_front',(x,-1.2,z+1.25),(.6,.05,2.5),'026')
        before=len(K.parts);K.door(64);move_since(before,(0,-1.2,z))
    elif i==455:K.wall(12)
    elif i==456:
        for x in [-.45,.45]:box('turnstile_pedestal',(x,0,.55),(.20,.45,1.1),'032',.035)
        cylinder('turnstile_axis',(-.32,0,.85),(-.18,0,.85),.05,'031')
        for j in range(3):a=j*math.tau/3;cylinder('turnstile_arm',(-.22,0,.85),(.28,.3*math.cos(a),.85+.3*math.sin(a)),.025,'032')
    elif i==458:K.fluorescent(135)
    elif i in (459,460):
        box('CCTV_wall_plate',(0,.1,.2),(.12,.025,.24),'031',.008)
        curve('CCTV_bracket',[(0,.1,.2),(0,-.1,.2),(0,-.1,.29)],.02,'032')
        if i==459:
            box('camera_housing',(0,-.15,.34),(.13,.30,.11),'028',.02);cylinder('camera_lens',(0,-.31,.34),(0,-.33,.34),.035,'black');box('camera_sunshield',(0,-.17,.41),(.17,.35,.018),'032',.009)
        else:
            cylinder('dome_camera_mount',(0,-.15,.34),(0,-.15,.40),.09,'028');softbox('camera_dome',(0,-.15,.29),(.17,.17,.14),'glass',.065)
    elif i==461:K.office(248)
    elif i==462:
        box('radio_base',(0,0,.06),(.28,.19,.12),'031',.016)
        for x in [-.10,.10]:cylinder('radio_knob',(x,-.105,.06),(x,-.125,.06),.018,'black')
        box('radio_display',(0,-.10,.07),(.12,.008,.035),'glass');box('radio_mic',(.18,0,.08),(.055,.035,.09),'black',.012)
        curve('radio_coil',[(.15+.012*math.cos(j*.7),.02+j*.001,.03+.012*math.sin(j*.7)) for j in range(100)],.0018,'black')
    elif i==463:cabinet(.65,.45,1.85)
    elif i==464:
        box('weapon_rack_back',(0,0,.12),(.8,.025,.24),'031')
        for x in [-.3,-.1,.1,.3]:curve('weapon_hook',[(x,-.02,.12),(x,-.15,.12),(x,-.15,.17)],.012,'rubber')
    elif i==465:
        cylinder('siren_body',(0,0,.15),(0,0,.35),.11,'red')
        for j in range(12):a=j*math.tau/12;box('siren_vent',(.11*math.cos(a),.11*math.sin(a),.24),(.018,.018,.08),'black',.004)
        box('siren_mount',(0,0,.02),(.3,.25,.04),'031')
    elif i==466:
        for x in [-.65,.65]:
            for y in [-.22,.22]:box('bench_leg',(x,y,.22),(.07,.07,.44),'031')
        for j in range(5):box('bench_seat_slat',(0,-.22+j*.11,.46),(1.8,.09,.035),'wood',.008)
        for j in range(4):box('bench_back_slat',(0,.25,.64+j*.1),(1.8,.035,.08),'wood',.008)
    elif i==467:open_bin(.4,.4,.7,'031')
    elif i==468:
        box('dumpster_body',(0,0,.6),(1.5,.85,1.0),'026',.055);box('dumpster_lid',(0,0,1.13),(1.6,.95,.07),'031',.025)
        for x in [-.62,.62]:
            for y in [-.3,.3]:wheel(x,y,.075,.06)
        for x in [-.5,.5]:pull_handle(x,-.49,1.13,.15)
    elif i==469:
        for x in [-.8,-.4,0,.4,.8]:curve('bike_rack',[(x,-.35,0),(x,-.35,.65),(x,0,.85),(x,.35,.65),(x,.35,0)],.025,'032')
        for y in [-.35,.35]:cylinder('rack_base',(-.95,y,.03),(.95,y,.03),.025,'031')
    elif i in (470,471,472):
        w,d,h=(3.2,1.5,2.6) if i==470 else ((5,5.5,3.5) if i==471 else (5.5,5.5,2.8))
        for x in [-w/2,w/2]:
            for y in [-d/2,d/2]:box('canopy_support',(x,y,h/2),(.10,.10,h),'031')
        ob=box('canopy_roof',(0,0,h),(w+.4,d+.4,.10),'032',.02);ob.rotation_euler.x=.045
        for x in [-w/2,0,w/2]:box('canopy_roof_purlin',(x,0,h-.1),(.07,d,.14),'031')
    elif i==473:box('road_kerb',(0,0,.12),(1.0,.18,.24),'029',.018)
    elif i==474:
        for j in range(5):
            for k in range(5):box('paving_block',(-.4+k*.2+(.1 if j%2 else 0),-.4+j*.2,.03),(.194,.194,.06),'029',.007)
    elif i==475:
        mesh('access_ramp',[(-.7,0,0),(.7,0,0),(-.7,3,0),(.7,3,0),(-.7,3,.3),(.7,3,.3)],[(0,2,3,1),(0,1,5,4),(0,4,2),(1,3,5),(2,4,5,3)],'029',.005)
    elif i==476:
        for y in [0,1.5,3]:cylinder('ramp_post',(0,y,y*.1),(0,y,.9+y*.1),.022,'032')
        cylinder('ramp_toprail',(0,0,.9),(0,3,1.2),.025,'032')
    elif i==477:
        cylinder('bollard',(0,0,0),(0,0,.85),.065,'031');cylinder('reflective_band',(0,0,.65),(0,0,.72),.066,'white');cylinder('bollard_cap',(0,0,.85),(0,0,.87),.07,'032')
    elif i==478:
        cylinder('sign_post',(0,0,0),(0,0,2.4),.03,'032');cylinder('road_sign',(0,0,2.05),(0,-.015,2.05),.30,'red',64);text('STOP',(0,-.02,1.99),.12,'white')
    elif i==479:
        box('wheel_stop',(0,0,.065),(1.75,.14,.13),'rubber',.025)
        for x in [-.6,.6]:box('wheel_stop_reflector',(x,-.072,.06),(.18,.005,.055),'white',.004)
    elif i==480:
        K.bpy.ops.mesh.primitive_cone_add(vertices=48,radius1=.15,radius2=.025,depth=.5,location=(0,0,.29));K.finish(K.bpy.context.object,'traffic_cone','red')
        box('cone_base',(0,0,.02),(.36,.36,.04),'rubber',.025)
        cylinder('reflective_cone_band',(0,0,.28),(0,0,.32),.085,'white',48)
    elif i==481:
        for x in [-.70,.70]:
            cylinder('barrier_leg',(x,0,0),(x,0,1.0),.024,'031');cylinder('barrier_foot',(x,-.30,.03),(x,.30,.03),.025,'031')
        box('barrier_board',(0,0,.75),(1.8,.05,.25),'white',.012)
        for x in [-.65,-.25,.15,.55]:ob=box('barrier_stripe',(x,-.028,.75),(.16,.004,.24),'red',.001);ob.rotation_euler.y=-.3
    elif i in (482,483):
        if i==482:
            cylinder('manhole_cover',(0,0,0),(0,0,.025),.34,'031',64)
            for j in range(-5,6):
                yy=j*.05;length=2*math.sqrt(max(0,.30**2-yy**2));box('manhole_tread',(0,yy,.027),(length,.018,.004),'032',.002)
            torus('cover_frame',(0,0,.01),.35,.018,'031',(0,0,0))
        else:K.floor_detail(48)
    elif 484<=i<=490:vehicle(i)

def branch(a,b,r1,r2,mat='wood',sides=9):
    d=Vector(b)-Vector(a);K.bpy.ops.mesh.primitive_cone_add(vertices=sides,radius1=r1,radius2=r2,depth=d.length,location=(Vector(a)+Vector(b))/2)
    o=K.bpy.context.object;o.rotation_mode='QUATERNION';o.rotation_quaternion=d.to_track_quat('Z','Y');return K.finish(o,'tapered_branch',mat)

def foliage_material():
    mat=material('foliage');bs=mat.node_tree.nodes.get('Principled BSDF');bs.inputs['Base Color'].default_value=(.06,.12,.065,1);bs.inputs['Roughness'].default_value=.85
    return 'foliage'

def leaf_cluster(loc,scale=.3):
    mat=foliage_material();verts=[];faces=[]
    for j in range(32):
        a=random.random()*math.tau;r=random.random()*scale;z=random.uniform(-scale*.5,scale*.5)
        c=Vector(loc)+Vector((math.cos(a)*r,math.sin(a)*r,z));u=Vector((math.cos(a)*.10,math.sin(a)*.10,.025))*scale*3;v=Vector((-math.sin(a)*.035,math.cos(a)*.035,.01))*scale*3
        start=len(verts);verts.extend([c-u,c+v,c+u,c-v]);faces.extend([(start,start+1,start+2),(start,start+2,start+3)])
    return mesh('individual_leaves',verts,faces,mat)

def vegetation(i):
    green=foliage_material()
    if i in (491,492):
        h=7.5 if i==491 else 6.5;branch((0,0,0),(0,0,h),.19,.025)
        for level in range(14):
            z=1.3+level*.39;reach=(1-z/h)*(2.1 if i==492 else 1.6)+.12
            for j in range(8):
                a=j*math.tau/8+level*.7;end=(math.cos(a)*reach,math.sin(a)*reach,z-.15);branch((0,0,z),end,.025,.006)
                # radial conifer needles in true geometry, not opaque cones
                verts=[];faces=[]
                for q in range(70):
                    t=random.uniform(.18,1);x,y=end[0]*t,end[1]*t;zz=z-.15*t
                    angle=a+random.uniform(-1.6,1.6);span=random.uniform(.09,.24)*(1-.4*t)
                    p=len(verts);verts.extend([(x,y,zz),(x+math.cos(angle)*span,y+math.sin(angle)*span,zz+.03),(x+math.cos(angle)*span*.6,y+math.sin(angle)*span*.6,zz+.08)]);faces.append((p,p+1,p+2))
                mesh('conifer_needles',verts,faces,green)
    elif i in (493,494):
        branch((0,0,0),(.08,0,4.5),.22,.04)
        for j in range(22):
            z=1.6+j*.12;a=j*2.4;r=random.uniform(.9,1.7);end=Vector((math.cos(a)*r,math.sin(a)*r,z+.7));branch((0,0,z),end,.045,.009)
            for s in [-1,1]:
                tip=end+Vector((math.cos(a+s*.6)*.6,math.sin(a+s*.6)*.6,.45));branch(end,tip,.014,.003)
                if i==493:leaf_cluster(tip,.55);leaf_cluster(end,.55)
    elif i==495:
        branch((0,0,0),(0,0,.55),.34,.27)
        cylinder('cut_stump_top',(0,0,.55),(0,0,.553),.26,'wood')
        for j in range(8):a=j*math.tau/8;branch((0,0,.2),(.55*math.cos(a),.55*math.sin(a),.02),.10,.025)
    elif i in (496,497):
        length=2.7 if i==496 else 1.3;r=.22 if i==496 else .025
        branch((-length/2,0,r),(length/2,.1,r),r,r*.7)
        for j in range(6):
            x=-length*.4+j*length*.14;branch((x,0,r),(x+.15,(-1)**j*(.4 if i==496 else .2),r+.13),r*.25,.005)
    elif i in (498,499):
        h=.7 if i==498 else 1.4
        for j in range(14):
            a=j*2.4;end=(math.cos(a)*h*.45,math.sin(a)*h*.45,random.uniform(.45,1)*h);branch((0,0,.01),end,.018,.003);leaf_cluster(end,h*.3)
    elif i in (500,501):
        h=.7 if i==500 else .18;v=[];f=[]
        for j in range(180):
            a=random.uniform(0,math.tau);r=.25*math.sqrt(random.random());x,y=r*math.cos(a),r*math.sin(a);height=h*random.uniform(.45,1);lean=Vector((math.cos(a)*height*.24,math.sin(a)*height*.24,0));width=.006
            p=len(v);v.extend([(x-width,y,0),(x+width,y,0),(x+lean.x+width*.4,y+lean.y,height*.55),(x+lean.x*1.7,y+lean.y*1.7,height)]);f.extend([(p,p+1,p+2),(p,p+2,p+3)])
        mesh('grass_blades',v,f,green)
    elif i==502:
        for j in range(12):
            a=j*2.4;length=random.uniform(.35,.65)
            points=[(math.cos(a)*length*t,math.sin(a)*length*t,.35*math.sin(t*math.pi*.8)) for t in [q/18 for q in range(19)]];curve('fern_frond',points,.002,green)
            for q in range(2,17):
                t=q/18;c=Vector(points[q]);span=.11*(1-t)
                for s in [-1,1]:
                    tip=c+Vector((math.cos(a+s*1.0)*span,math.sin(a+s*1.0)*span,.025));u=Vector((math.cos(a)*.02,math.sin(a)*.02,0));mesh('fern_pinna',[c-u,tip,c+u],[(0,1,2)],green)
    elif i==503:
        for j in range(60):
            x,y=random.uniform(-.5,.5),random.uniform(-.5,.5);a=random.uniform(0,math.tau);u=Vector((math.cos(a)*.04,math.sin(a)*.04,0));v=Vector((-math.sin(a)*.02,math.cos(a)*.02,.006));c=Vector((x,y,.003));mesh('fallen_leaf',[c-u,c+v,c+u,c-v],[(0,1,2),(0,2,3)],'wood')
    elif i in (504,505):
        K.bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=3,radius=.32 if i==504 else 1.4,location=(0,0,.17 if i==504 else .70));ob=K.bpy.context.object
        for v in ob.data.vertices:v.co*=random.uniform(.86,1.14);v.co.z*=.72
        K.finish(ob,'weathered_rock','029',.008)

def clutter(i):
    if 506<=i<=509:box('ground_material_sample',(0,0,.02),(3,3,.04),f'{i:03d}',.005)
    elif 510<=i<=530:
        # Decal card with explicit 0..1 UV; PNG alpha remains portable to engines.
        ob=mesh('decal_projection_card',[(-.5,-.5,.002),(.5,-.5,.002),(.5,.5,.002),(-.5,.5,.002)],[(0,1,2,3)],f'{i:03d}')
        for p in ob.data.polygons:
            for li,coords in zip(p.loop_indices,[(0,0),(1,0),(1,1),(0,1)]):ob.data.uv_layers.active.data[li].uv=coords
    elif i in (531,532,533,534):
        mat={531:'025',532:'ceiling_fibre',533:'glass',534:'030'}[i]
        for j in range(9):
            x,y=random.uniform(-.25,.25),random.uniform(-.25,.25);r=random.uniform(.025,.09)
            outline=[(x+math.cos(a)*r*random.uniform(.7,1.2),y+math.sin(a)*r*random.uniform(.7,1.2)) for a in [q*math.tau/5 for q in range(5)]]
            v=[(xx,yy,random.uniform(.005,.012)) for xx,yy in outline]+[(xx,yy,random.uniform(.02,.035)) for xx,yy in outline]
            mesh('debris_piece',v,[(4,3,2,1,0),(5,6,7,8,9)]+[(q,(q+1)%5,(q+1)%5+5,q+5) for q in range(5)],mat,.001)
    elif i==535:
        K.bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2,radius=.075,location=(0,0,.06));ob=K.bpy.context.object
        for v in ob.data.vertices:v.co*=random.uniform(.65,1.2)
        K.finish(ob,'crumpled_paper','028')
    elif i in (536,537):
        if i==536:
            tank(.035,.20,'glass');cylinder('bottle_neck',(0,0,.2),(0,0,.24),.012,'glass')
            for z in [.035,.075,.115,.155]:torus('bottle_rib',(0,0,z),.035,.002,'glass',(0,0,0))
        else:
            cylinder('empty_can',(0,0,0),(0,0,.115),.033,'032');torus('can_rim',(0,0,.116),.032,.002,'032',(0,0,0))
    elif i==538:
        v=[];f=[];n=32
        for j in range(n+1):
            for q in range(n+1):x=(q/n-.5)*.65;y=(j/n-.5)*.4;v.append((x,y,.025+.015*math.sin(x*50+y*20)))
        for j in range(n):
            for q in range(n):
                if (q<2 or j<2) and random.random()<.45:continue
                a=j*(n+1)+q;f.append((a,a+1,a+n+2,a+n+1))
        ob=mesh('torn_cloth',v,f,'fabric_dirty');mod=ob.modifiers.new('cloth_thickness','SOLIDIFY');mod.thickness=.001
    elif i==539:
        softbox('abandoned_shoe',(0,0,.055),(.11,.27,.11),'rubber',.045);box('shoe_sole',(0,0,.012),(.115,.28,.025),'black',.02)
        softbox('shoe_collar',(0,.065,.12),(.10,.11,.10),'wood',.04)
        for y in [-.05,-.02,.01,.04]:curve('shoelace',[(-.035,y,.115),(.035,y+.02,.115)],.0015,'fabric_dirty')
    elif i in (540,541):
        w,d,h=(.48,.25,.30) if i==540 else (.48,.18,.65)
        softbox('patient_luggage',(0,0,h/2),(w,d,h),'fabric_blue' if i==540 else 'wood',.055)
        for x in [-.14,.14]:curve('bag_handle',[(x,0,h), (x,0,h+.12),(-x,0,h+.12),(-x,0,h)],.012,'rubber')
        if i==541:
            for x in [-.16,.16]:wheel(x,.05,.04,.025)
            for x in [-.09,.09]:cylinder('suitcase_pull',(x,.055,h),(x,.055,h+.35),.009,'032')
            cylinder('suitcase_grip',(-.09,.055,h+.35),(.09,.055,h+.35),.015,'black')
    elif i==542:
        # Small abandoned stuffed rabbit, not a living creature.
        for name,loc,size in [('body',(0,0,.14),(.15,.12,.22)),('head',(0,0,.29),(.15,.14,.15)),('left_ear',(-.04,0,.42),(.04,.04,.19)),('right_ear',(.04,0,.42),(.04,.04,.19)),('left_paw',(-.10,0,.13),(.07,.07,.13)),('right_paw',(.10,0,.13),(.07,.07,.13))]:softbox(name,loc,size,'fabric_dirty',min(size)*.4)
        for x in [-.035,.035]:cylinder('toy_button_eye',(x,-.072,.31),(x,-.076,.31),.009,'black')
        curve('toy_mouth',[(-.02,-.075,.265),(0,-.08,.255),(.02,-.075,.265)],.001,'wood')
    elif i==543:
        K.bpy.ops.mesh.primitive_uv_sphere_add(segments=48,ring_count=24,radius=.13,location=(0,0,.13));ob=K.finish(K.bpy.context.object,'child_ball','red')
        for p in ob.data.polygons:p.use_smooth=True
        for a in [0,math.pi/2]:torus('ball_seam',(0,0,.13),.1305,.0015,'rubber',(0,a,0))
    elif i in (544,545,546,547):
        w,h=(.30,.40) if i==544 else (.45,.65)
        box('paper_backing',(0,0,h/2),(w,.012,h),'wood' if i==544 else '028',.002)
        if i==544:
            box('photo',(0,-.008,h/2),(w-.03,.002,h-.03),'025',.001)
            for x in [-w/2,w/2]:box('photo_frame',(x,-.01,h/2),(.018,.02,h+.018),'wood')
            for z in [0,h]:box('photo_frame',(0,-.01,z),(w+.018,.02,.018),'wood')
            text('ОТДЕЛЕНИЕ 04',(0,-.012,.05),.021,'black')
        elif i==545:
            for j in range(5):curve('poster_tear',[(-w/2+j*.08,-.008,.02),(-w/2+j*.08+.02,-.008,.13)],.003,'029')
            text('ТИШИНА',(0,-.009,h*.70),.065,'026')
        elif i==546:
            text('ГИГИЕНА РУК',(0,-.009,h*.85),.045,'026')
            for j in range(6):
                x=(j%3-1)*.13;z=.16+(j//3)*.20
                cylinder('hand_diagram',(x,-.009,z),(x,-.012,z),.038,'026');text(str(j+1),(x,-.014,z-.015),.026,'white')
        else:
            text('ОКТЯБРЬ 2026',(0,-.009,.56),.038,'black')
            for j in range(31):text(str(j+1),((j%7-3)*.055,-.009,.44-(j//7)*.08),.028,'red' if j%7==6 else 'black')
    elif i in (548,549,550):
        if i==550:
            box('access_card',(0,0,.001),(.086,.054,.002),'028',.004)
            box('magstripe',(0,.015,.0025),(.08,.009,.0005),'black',0)
            box('ID_photo',(-.025,-.004,.0025),(.02,.025,.0005),'025',0)
        else:
            if i==548:
                box('key_tag',(0,0,.002),(.055,.025,.004),'026',.005);torus('tag_eye',(-.02,0,.004),.004,.001,'steel',(0,0,0))
            else:
                torus('key_ring',(0,0,.02),.025,.002,'steel',(0,0,0))
                for j in range(4):
                    x=(j-1.5)*.016;torus('key_bow',(x,.04,.018),.012,.003,'steel',(0,0,0));box('key_shank',(x,.075,.018),(.007,.055,.004),'steel',.001)
                    for q in range(3):box('key_tooth',(x+.005,.09+q*.007,.018),(.009,.004,.004),'steel',.001)
    elif i in (551,552,553,554):
        if i==551:
            cylinder('flashlight_body',(0,0,.02),(0,0,.19),.024,'031',32);cylinder('flashlight_head',(0,0,.19),(0,0,.24),.039,'031');cylinder('reflector',(0,0,.2401),(0,0,.242),.032,'steel');cylinder('flashlight_glass',(0,0,.243),(0,0,.245),.031,'glass')
            for j in range(8):torus('flashlight_knurl',(0,0,.04+j*.015),.024,.0015,'rubber',(0,0,0))
            box('torch_switch',(0,-.025,.15),(.016,.008,.025),'rubber',.006)
        elif i==552:
            cylinder('battery',(0,0,0),(0,0,.05),.007,'026');cylinder('positive_terminal',(0,0,.05),(0,0,.052),.003,'steel');text('+',(0,-.007,.037),.008,'white')
        elif i==553:
            softbox('handheld_radio',(0,0,.075),(.065,.035,.15),'031',.01);cylinder('radio_antenna',(-.022,0,.15),(-.022,0,.31),.004,'rubber')
            box('radio_screen',(0,-.02,.10),(.045,.005,.025),'glass',.003)
            for j in range(7):box('speaker_slot',(0,-.02,.026+j*.006),(.042,.002,.002),'black',0)
            cylinder('radio_volume',(.018,0,.15),(.018,0,.17),.009,'black')
        else:
            softbox('first_aid_case',(0,0,.10),(.28,.15,.20),'026',.02)
            curve('medical_case_handle',[(-.05,0,.20),(-.05,0,.25),(.05,0,.25),(.05,0,.20)],.009,'black')
            box('first_aid_cross',(0,-.077,.11),(.11,.003,.035),'white');box('first_aid_cross',(0,-.079,.11),(.035,.003,.11),'white')
    elif i in (555,556,557,558,559,560,561,562):
        if i==555:
            # Fictional service pistol: separate slide, frame, magazine, trigger.
            box('pistol_slide',(0,0,.15),(.035,.19,.035),'031',.005)
            box('pistol_frame',(0,.015,.12),(.033,.16,.025),'031',.005)
            ob=box('pistol_grip',(0,.064,.065),(.032,.055,.11),'rubber',.008);ob.rotation_euler.x=-.18
            curve('trigger_guard',[(-.014,.025,.12),(-.014,-.005,.08),(-.014,-.045,.08),(-.014,-.045,.12)],.003,'031')
            cylinder('pistol_barrel',(0,-.096,.15),(0,-.112,.15),.008,'steel');cylinder('muzzle_opening',(0,-.113,.15),(0,-.114,.15),.005,'black')
            for z in [.02,.035,.05,.065,.08]:box('grip_texture',(0,.095,z),(.029,.002,.004),'031',.001)
            for y in [.045,.055,.065,.075,.085]:box('slide_serration',(.018,y,.15),(.001,.003,.023),'steel',.0005)
            for y in [-.08,.08]:box('pistol_sight',(0,y,.172),(.008,.008,.008),'black',.001)
        elif i==556:box('pistol_magazine',(0,0,.055),(.025,.032,.11),'031',.003);box('magazine_floorplate',(0,0,.003),(.030,.037,.006),'black',.002)
        elif i in (557,558,560):
            r,h=(.006,.05) if i==560 else (.0048,.019)
            cylinder('cartridge_case',(0,0,0),(0,0,h),r,'wood' if i!=560 else 'red');cylinder('case_rim',(0,0,0),(0,0,.002),r*1.10,'steel')
            if i==557:
                K.bpy.ops.mesh.primitive_uv_sphere_add(segments=16,ring_count=12,radius=r,location=(0,0,h+.002));o=K.finish(K.bpy.context.object,'bullet_tip','steel');o.scale.z=1.6
            if i==558:cylinder('empty_case_interior',(0,0,h),(0,0,h+.0003),r*.8,'black')
        elif i==559:
            cylinder('shotgun_barrel',(0,-.35,.14),(0,.32,.14),.012,'031');box('receiver',(0,.38,.13),(.045,.18,.055),'031',.009)
            softbox('shotgun_foreend',(0,-.02,.10),(.055,.22,.06),'wood',.014)
            slab_polygon('shotgun_stock',[(-.035,.08),(.035,.08),(.055,-.10),(.10,-.18),(-.10,-.18),(-.055,-.04)],-.022,.022,'wood')
            ob=K.parts[-1];ob.location=(0,.49,.12);ob.rotation_euler.x=math.pi/2
            for j in range(8):box('pump_grip_groove',(0,-.10+j*.026,.071),(.052,.005,.003),'black',.001)
        elif i==561:
            softbox('knife_grip',(0,0,.055),(.027,.018,.11),'rubber',.008);box('knife_guard',(0,0,.113),(.055,.025,.007),'steel',.003)
            slab_polygon('knife_blade',[(-.013,.116),(.013,.116),(.013,.235),(-.002,.27),(-.013,.23)],-.0015,.0015,'steel')
        else:K.office(269)
