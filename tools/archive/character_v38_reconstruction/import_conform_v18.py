from pathlib import Path
import unreal as u,json,traceback
R=Path('D:/Projects/ECHO');O=R/'artifacts/echo_hero_v38/metahuman_conform_v18';O.mkdir(exist_ok=True)
r={'cloud_requests':False,'game_changed':False,'likeness_approved':False,'asset':'/Game/Hero/MH_Beka_ConformV18'}
s=u.get_editor_subsystem(u.MetaHumanCharacterEditorSubsystem);c=None;registered=False
try:
 def importmesh(name):
  path='/Game/Hero/PhotoConformV18/SM_BekaTarget'+name
  if u.EditorAssetLibrary.does_asset_exist(path):return u.load_asset(path)
  t=u.AssetImportTask();t.filename=str(O/('BekaTarget_'+name+'.fbx'));t.destination_path='/Game/Hero/PhotoConformV18';t.destination_name='SM_BekaTarget'+name;t.automated=True;t.replace_existing=False;t.save=True;t.factory=u.FbxFactory()
  opt=u.FbxImportUI();opt.automated_import_should_detect_type=False;opt.mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH;opt.import_as_skeletal=False;opt.import_mesh=True;opt.import_animations=False;opt.import_materials=False;opt.import_textures=False
  data=opt.static_mesh_import_data;data.set_editor_property('combine_meshes',True);data.set_editor_property('generate_lightmap_u_vs',False);data.set_editor_property('remove_degenerates',False);t.options=opt
  u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t]);mesh=u.load_asset(path)
  if not mesh:raise RuntimeError('Missing imported target '+name+str(t.imported_object_paths))
  return mesh
 targets={n:importmesh(n) for n in ['Head','LeftEye','RightEye']};r['targets']={k:v.get_path_name() for k,v in targets.items()}
 c=u.load_asset(r['asset']) if u.EditorAssetLibrary.does_asset_exist(r['asset']) else u.EditorAssetLibrary.duplicate_asset('/Game/Hero/MH_Beka_FaceCandidate',r['asset'])
 if not c:raise RuntimeError('Could not duplicate preserved 4K source')
 registered=s.try_add_object_to_edit(c)
 if not registered:raise RuntimeError('Cannot edit the isolated conform candidate')
 r['landmarks_before']=[[p.x,p.y,p.z] for p in s.get_face_landmarks(c)]
 params=u.ImportFromTemplateParams();params.isolate_head_from_body=False;params.match_vertices_by_u_vs=True;params.use_eye_meshes=True;params.use_teeth_mesh=False
 params.alignment_options=u.MetaHumanAlignmentOptions.SCALING_ROTATION_TRANSLATION
 code=s.import_from_template(c,targets['Head'],targets['LeftEye'],targets['RightEye'],None,params);r['conform_result']=str(code)
 if 'SUCCESS' not in str(code).upper():raise RuntimeError('Template conform failed: '+str(code))
 r['landmarks_after']=[[p.x,p.y,p.z] for p in s.get_face_landmarks(c)]
 u.EditorAssetLibrary.save_loaded_asset(c)
 texpath='/Game/Hero/PhotoConformV18/T_BekaPhotoAlbedo'
 if not u.EditorAssetLibrary.does_asset_exist(texpath):
  t=u.AssetImportTask();t.filename=str(O/'Beka_FaceColor_nativeUV_4096.png');t.destination_path='/Game/Hero/PhotoConformV18';t.destination_name='T_BekaPhotoAlbedo';t.automated=True;t.replace_existing=False;t.save=True;u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
 tex=u.load_asset(texpath)
 if not tex:raise RuntimeError('Photo albedo did not import')
 tex.set_editor_property('srgb',True);tex.set_editor_property('max_texture_size',4096);tex.set_editor_property('lod_bias',0);tex.set_editor_property('never_stream',True);u.EditorAssetLibrary.save_loaded_asset(tex)
 settings=c.get_editor_property('skin_settings');over=settings.get_editor_property('texture_material_overrides');over.set_editor_property('enable_texture_overrides',True)
 texture_set=over.get_editor_property('texture_overrides');texture_set.set_editor_property('face',{u.FaceTextureType.BASECOLOR:tex});over.set_editor_property('texture_overrides',texture_set);settings.set_editor_property('texture_material_overrides',over);s.commit_skin_settings(c,settings)
 c.preview_material_type=u.MetaHumanCharacterSkinPreviewMaterial.EDITABLE
 collection=s.get_preview_collection(c)
 for slot,path in [('Hair','/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_M_SideSweptFringe'),('Eyebrows','/MetaHumanCharacter/Optional/Grooms/Bindings/Eyebrows/WI_Eyebrows_M_Natural'),('Eyelashes','/MetaHumanCharacter/Optional/Grooms/Bindings/Eyelashes/WI_Eyelashes_S_Sparse')]:
  item=u.load_asset(path);key=collection.try_add_item_from_wardrobe_item(slot_name=slot,wardrobe_item=item);collection.default_instance.set_single_slot_selection(slot_name=slot,item_key=key)
  for p in collection.default_instance.get_instance_parameters(item_path=u.MetaHumanPaletteItemPath(item_key=key)):
   if str(p.name)=='Melanin':p.set_float(value=.98)
   elif str(p.name)=='Redness':p.set_float(value=0.)
 s.on_edit_preview_collection(c)
 u.EditorAssetLibrary.set_metadata_tag(c,'ECHO.LikenessStatus','Native topology conform from photo-fitted mesh, photo-albedo override. Visual likeness pending, never treat successful import as approval.')
 r['saved']=bool(u.EditorAssetLibrary.save_loaded_asset(c));r['has_full_facial_rig']=bool(c.get_editor_property('has_face_dna_blendshapes'));r['success']=r['saved']
except Exception as e:
 r.update(success=False,error=str(e),traceback=traceback.format_exc());u.log_error(str(e))
finally:
 if registered:s.remove_object_to_edit(c)
 (O/'unreal_import_result.json').write_text(json.dumps(r,ensure_ascii=False,indent=2),encoding='utf8')
 u.log('ECHO_CONFORM_V18 '+str(r.get('success')))
