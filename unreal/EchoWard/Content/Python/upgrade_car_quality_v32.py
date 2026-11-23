"""Restore supplied vehicle texture detail; do not upscale small source images."""
import unreal as u,json,hashlib,traceback
from pathlib import Path
P=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));R=P.parents[1];report=R/'artifacts/echo_car_quality_v32';src=json.loads((report/'source_textures.json').read_text());B='/Game/Campaign/EchoCustom/Textures/';E=u.EditorAssetLibrary
try:
 jobs={};L=u.MaterialEditingLibrary;A=u.AssetToolsHelpers.get_asset_tools()
 for row in src['all']:
  kind='color' if row['channel']=='albedo' else 'normal' if row['channel']=='normal' else 'mask';parameter={'albedo':'Albedo','normal':'Normal','roughness_map':'RoughnessMap','metallic_map':'MetallicMap','alpha_file':'Alpha'}[row['channel']]
  mat=u.load_asset('/Game/Campaign/EchoCustom/Materials/MI_'+row['key']);assert mat,row['key'];tex=L.get_material_instance_texture_parameter_value(mat,parameter)
  if not tex or not tex.get_path_name().startswith(B):
   key=hashlib.sha256((row['file']+kind).encode()).hexdigest()[:16];name='T_V32_'+key;path=B+name;tex=u.load_asset(path)
   if not tex:
    task=u.AssetImportTask();task.filename=row['file'];task.destination_path=B.rstrip('/');task.destination_name=name;task.automated=True;task.save=True;A.import_asset_tasks([task]);tex=u.load_asset(path)
   assert tex,row['file'];tex.set_editor_property('srgb',kind=='color')
   if kind=='normal':tex.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_NORMALMAP);tex.set_editor_property('flip_green_channel',True)
   if kind=='mask':tex.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_MASKS)
   L.set_material_instance_texture_parameter_value(mat,parameter,tex);assert E.save_loaded_asset(mat);assert E.save_loaded_asset(tex)
  path=tex.get_path_name().split('.')[0]
  item=jobs.setdefault(path,dict(path=path,hero=False,kind=kind,source=row['file'],size=row['size'],names=[]));item['hero']|=row['hero'];item['names'].append(row['name'])
 results=[]
 for i,(path,row) in enumerate(jobs.items()):
  tex=u.load_asset(path);assert tex,path;limit=4096 if row['hero'] else 2048;before=dict(max_size=tex.get_editor_property('max_texture_size'),lod_bias=tex.get_editor_property('lod_bias'),group=str(tex.get_editor_property('lod_group')),compression=str(tex.get_editor_property('compression_settings')))
  settings=dict(max_texture_size=limit,lod_bias=0,lod_group=u.TextureGroup.TEXTUREGROUP_VEHICLE_NORMAL_MAP if row['kind']=='normal' else u.TextureGroup.TEXTUREGROUP_VEHICLE_SPECULAR if row['kind']=='mask' else u.TextureGroup.TEXTUREGROUP_VEHICLE)
  if row['kind']=='color':settings['mip_gen_settings']=u.TextureMipGenSettings.TMGS_SHARPEN1
  # Keep the detailed hero albedo out of BC1 block compression.
  if row['hero'] and row['kind']=='color':settings['compression_settings']=u.TextureCompressionSettings.TC_BC7
  changed=False
  for name,value in settings.items():
   if tex.get_editor_property(name)!=value:tex.set_editor_property(name,value);changed=True
  if changed:assert E.save_loaded_asset(tex)
  assert tex.get_editor_property('max_texture_size')==limit and tex.get_editor_property('lod_bias')==0
  row.update(before=before,max_size=limit,after_size=[min(v,limit) for v in row['size']],changed=changed,compression=str(tex.get_editor_property('compression_settings')));results.append(row)
  if i%20==0:u.log('QUALITY32_TEXTURE '+str(i));u.SystemLibrary.collect_garbage()
 out=dict(success=True,revision=32,hero_budget=4096,parked_budget=2048,source_images_unchanged=True,upscaled_images=0,textures=len(results),hero_textures=sum(v['hero'] for v in results),updated=sum(v['changed'] for v in results),assets=results)
 (P/'Content/CampaignData/echo_car_quality_v32.json').write_text(json.dumps(out,ensure_ascii=False,indent=2));(report/'native_texture_audit.json').write_text(json.dumps(out,ensure_ascii=False,indent=2));u.log('CAR_QUALITY32_PASS '+str(len(results)))
except Exception:
 (report/'quality32_failed.txt').write_text(traceback.format_exc());raise
finally:u.SystemLibrary.quit_editor()
