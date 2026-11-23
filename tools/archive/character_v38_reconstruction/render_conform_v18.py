from pathlib import Path
import unreal as u,json,traceback
O=Path('D:/Projects/ECHO/artifacts/echo_hero_v38/metahuman_conform_v18/review');O.mkdir(parents=True,exist_ok=True)
r={'asset':'/Game/Hero/MH_Beka_ConformV18','cloud_requests':False,'game_changed':False,'likeness_approved':False,'views':[],'grooms':[]}
state={'stage':'init','ticks':0,'elapsed':0.,'busy':False,'groom':0,'view':0}
styles=['M_SideSweptFringe','S_SideSweptFringe','S_Messy']
views=[('front',u.Vector(0,58,163)),('three_quarter',u.Vector(44,54,163)),('profile',u.Vector(64,0,163))]
def write():
 (O/'render_report.json').write_text(json.dumps(r,ensure_ascii=False,indent=2),encoding='utf8')
def select_hair(style):
 col=sub.get_preview_collection(c);item=u.load_asset('/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_'+style)
 if not item:raise RuntimeError('Groom asset missing '+style)
 key=col.try_add_item_from_wardrobe_item(slot_name='Hair',wardrobe_item=item);col.default_instance.set_single_slot_selection(slot_name='Hair',item_key=key)
 names=[]
 for p in col.default_instance.get_instance_parameters(item_path=u.MetaHumanPaletteItemPath(item_key=key)):
  names.append(str(p.name))
  if str(p.name)=='Melanin':p.set_float(value=.98)
  elif str(p.name)=='Redness':p.set_float(value=0.)
 sub.on_edit_preview_collection(c);sub.assemble_for_preview(character=c);r['grooms'].append({'style':style,'parameters':names});write()
def tick(dt):
 global sub,c,actor,world,capture,rt,ca
 if state['busy']:return
 state['busy']=True;state['ticks']+=1;state['elapsed']+=dt
 try:
  if state['stage']=='init':
   if state['ticks']<5:return
   u.EditorLevelLibrary.new_level('/Game/Hero/ConformV18ReviewStage')
   world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();c=u.load_asset(r['asset'])
   if not c:raise RuntimeError('Conformed character missing')
   sub=u.get_editor_subsystem(u.MetaHumanCharacterEditorSubsystem)
   if not sub.try_add_object_to_edit(c):raise RuntimeError('Cannot register the conformed character')
   state.update(stage='await_character',elapsed=0.);write();return
  if state['stage']=='await_character':
   if state['elapsed']<25 or u.AssetRegistryHelpers.get_asset_registry().is_loading_assets():return
   actor=sub.spawn_meta_human_actor(character=c)
   if not actor:raise RuntimeError('No actor for conformed character')
   actor.set_actor_location(u.Vector(0,0,0),False,False)
   sub.assemble_for_preview(character=c)
   for mc in actor.get_components_by_class(u.MeshComponent):mc.prestream_textures(180.,True,0)
   r['components']=[{'name':mc.get_name(),'class':mc.get_class().get_name()} for mc in actor.get_components_by_class(u.ActorComponent)]
   for pos,power,size in [(u.Vector(-70,100,205),2400,90),(u.Vector(80,80,175),1100,100),(u.Vector(0,-100,210),1600,80)]:
    ob=u.EditorLevelLibrary.spawn_actor_from_class(u.RectLight,pos,u.MathLibrary.find_look_at_rotation(pos,u.Vector(0,0,163)));lc=ob.get_component_by_class(u.RectLightComponent);lc.set_intensity(power);lc.set_editor_property('source_width',float(size));lc.set_editor_property('source_height',float(size))
   ca=u.EditorLevelLibrary.spawn_actor_from_class(u.SceneCapture2D,views[0][1]);capture=ca.get_component_by_class(u.SceneCaptureComponent2D)
   rt=u.RenderingLibrary.create_render_target2d(world,1080,1440,u.TextureRenderTargetFormat.RTF_RGBA8)
   capture.set_editor_property('texture_target',rt);capture.set_editor_property('capture_source',u.SceneCaptureSource.SCS_FINAL_COLOR_LDR);capture.set_editor_property('fov_angle',30.)
   pp=u.PostProcessSettings()
   for a,b in [('override_auto_exposure_method',True),('auto_exposure_method',u.AutoExposureMethod.AEM_MANUAL),('override_auto_exposure_bias',True),('auto_exposure_bias',-8.),('override_auto_exposure_apply_physical_camera_exposure',True),('auto_exposure_apply_physical_camera_exposure',False),('override_motion_blur_amount',True),('motion_blur_amount',0.),('override_bloom_intensity',True),('bloom_intensity',.02)]:pp.set_editor_property(a,b)
   capture.set_editor_property('post_process_settings',pp);capture.set_editor_property('post_process_blend_weight',1.);capture.set_editor_property('capture_every_frame',True);capture.set_editor_property('always_persist_rendering_state',True)
   u.SystemLibrary.execute_console_command(world,'r.Streaming.FullyLoadUsedTextures 1');u.SystemLibrary.execute_console_command(world,'r.ScreenPercentage 100')
   select_hair(styles[0]);state.update(stage='warm_groom',elapsed=0.);return
  if state['stage']=='warm_groom':
   if state['elapsed']<30.:return
   name,pos=views[state['view']];ca.set_actor_location(pos,False,False);ca.set_actor_rotation(u.MathLibrary.find_look_at_rotation(pos,u.Vector(0,0,163)),False);state.update(stage='capture',elapsed=0.);return
  if state['stage']=='capture':
   if state['elapsed']<5.:return
   name,_=views[state['view']];filename=styles[state['groom']]+'_'+name+'.png';capture.capture_scene();u.RenderingLibrary.export_render_target(world,rt,str(O),filename);r['views'].append(filename);write()
   state['view']+=1
   if state['view']<len(views):state.update(stage='warm_groom',elapsed=28.);return
   state['groom']+=1;state['view']=0
   if state['groom']<len(styles):select_hair(styles[state['groom']]);state.update(stage='warm_groom',elapsed=0.);return
   select_hair(styles[0]);u.EditorAssetLibrary.save_loaded_asset(c);r['success']=True;write();u.unregister_slate_post_tick_callback(handle);u.SystemLibrary.quit_editor()
 except Exception as e:
  r.update(success=False,error=str(e),traceback=traceback.format_exc());write();u.log_error(str(e));u.unregister_slate_post_tick_callback(handle);u.SystemLibrary.quit_editor()
 finally:state['busy']=False
handle=u.register_slate_post_tick_callback(tick)
