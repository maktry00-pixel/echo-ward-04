"""Fetch human-authored CC0 PBR assets from their official distribution.

Powered by Poly Haven. This is an offline game import, not a live API client
in the shipped game. Cache metadata and verify every publisher MD5.
"""
import concurrent.futures,hashlib,json,time,urllib.request
from pathlib import Path

R=Path(__file__).resolve().parents[1];OUT=R/'assets/external/quality_v23';OUT.mkdir(parents=True,exist_ok=True)
HEAD={'User-Agent':'EchoWardAssetIntegration/23','Referer':'https://polyhaven.com/'}
SLUGS='''metal_office_desk SchoolChair_01 sofa_03 modern_arm_chair_01 painted_wooden_cabinet painted_wooden_cabinet_02 drawer_cabinet wooden_bookshelf_worn steel_frame_shelves_01 steel_frame_shelves_02 steel_frame_shelves_03 tool_cart industrial_storage_cart modular_street_seating old_bed_frame painted_wooden_nightstand vintage_wooden_drawer_01 metal_stool_02 metal_stool_03 plastic_monobloc_chair_01 painted_wooden_bench worn_metal_rack wooden_picnic_table plastic_crate_01 plastic_crate_02 wooden_crate_01 cardboard_box_01 industrial_pastic_container metal_tool_chest metal_toolbox metal_trash_can Barrel_01 Barrel_02 barrel_03 metal_jerrycan_green plastic_jerrycan propane_tank old_tyre hand_truck covered_car classic_laptop Television_01 television_02 CashRegister_01 boombox portable_cassette_player desk_lamp_arm_01 industrial_microscope vintage_microscope chemistry_set security_camera_01 security_camera_02 power_box_01 exterior_aircon_unit portable_generator mounted_fluorescent_lights industrial_caged_sconce industrial_wall_lamp street_lamp_02 security_light utility_box_01 fire_alarm korean_fire_extinguisher_01 medical_box medical_tape clipboard office_notepads stationery_supplies vintage_stapler all_purpose_cleaner bleach_bottle multi_cleaner_5_litre multi_cleaner_bottle plastic_broom dustpan plunger drain_cleaner modified_thermos plastic_thermos jug_01 plastic_container long_life_food russian_food_cans_01 food_apple_01 bananas hamburger_buns lemon wooden_cutting_board adjustable_wrench combination_wrench flathead_screwdriver screwdriver pliers tongue_groove_pliers cross_pein_hammer crowbar_01 bolt_cutters_01 Drill_01 circuit_board retro_multimeter lubricant_spray measuring_tape_01 ammo_box small_plastic_torch vintage_flashlight old_gas_mask metal_jug bunsen_burner potted_plant_04 planter_box_01 planter_box_02 planter_pot_clay fern_02 shrub_03 shrub_04 shrub_sorrel_01 nettle_plant weed_plant_02 grass_bermuda_01 fir_sapling jacaranda_tree concrete_road_barrier_02 water_manhole_cover'''.split()

def get(url):
 for attempt in range(3):
  try:
   with urllib.request.urlopen(urllib.request.Request(url,headers=HEAD),timeout=90) as r:return r.read()
  except OSError:
   if attempt==2:raise
   time.sleep(attempt+1)

def download(job):
 path,spec=job
 if path.exists() and hashlib.md5(path.read_bytes()).hexdigest()==spec['md5']:return
 data=get(spec['url'])
 if len(data)!=spec['size'] or hashlib.md5(data).hexdigest()!=spec['md5']:raise RuntimeError('Publisher hash mismatch: '+str(path))
 path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)

def metadata(slug):
 path=OUT/('files_'+slug+'.json')
 if path.exists():return json.loads(path.read_text(encoding='utf8'))
 data=json.loads(get('https://api.polyhaven.com/files/'+slug));path.write_text(json.dumps(data,indent=2),encoding='utf8');return data

if __name__=='__main__':
 catalog=json.loads((OUT/'polyhaven_catalog.json').read_text(encoding='utf-8-sig'))
 entries=[];jobs=[]
 with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:
  specs=dict(zip(SLUGS,pool.map(metadata,SLUGS)))
 for index,slug in enumerate(SLUGS):
  source=specs[slug];res='2k' if '2k' in source.get('gltf',{}) else '1k';file=source['gltf'][res]['gltf'];folder=OUT/'models'/slug
  main=folder/Path(urllib.request.urlparse(file['url']).path).name if False else folder/(slug+'_'+res+'.gltf')
  jobs.append((main,file))
  for name,spec in file['include'].items():
   path=(folder/name).resolve()
   if not path.is_relative_to(folder.resolve()):raise RuntimeError('Unsafe download path')
   jobs.append((path,spec))
  cat=catalog[slug];entries.append(dict(id=700+index,slug=slug,file=str(main),source='https://polyhaven.com/a/'+slug,license='CC0-1.0',authors=cat['authors'],title=cat['name'],resolution=res,publisher_files=file,catalog_polycount=cat.get('polycount')))
 total=sum(s['size'] for p,s in jobs);print(json.dumps(dict(models=len(entries),files=len(jobs),download_mb=round(total/1e6,1))),flush=True)
 with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:
  for i,_ in enumerate(pool.map(download,jobs)):
   if i%30==0:print('VERIFIED',i,'/',len(jobs),flush=True)
 old=json.loads((R/'assets/models/polyhaven/SOURCE_MANIFEST.json').read_text(encoding='utf8'))
 wheelchair=next(x for x in old if x['id']=='wheelchair_01');src=R/'assets/models/polyhaven/wheelchair_01/wheelchair_01_1k.gltf'
 assert src.exists() and hashlib.md5(src.read_bytes()).hexdigest()==wheelchair['files']['md5']
 entries.append(dict(id=700+len(entries),slug='wheelchair_01',file=str(src),source=wheelchair['source'],license=wheelchair['license'],authors=wheelchair['authors'],title=wheelchair['title'],resolution='1k',publisher_files=wheelchair['files'],existing_verified_source=True))
 (OUT/'SOURCE_MANIFEST.json').write_text(json.dumps(dict(version=23,assets=entries,credit='Powered by Poly Haven',license_url='https://polyhaven.com/license'),ensure_ascii=False,indent=2),encoding='utf8')
 (OUT/'CREDITS.md').write_text('# Third-party art / ЭХО v23\n\nPowered by [Poly Haven](https://polyhaven.com/). These assets are human-authored CC0-1.0 assets; they are not original Echo artwork. Source metadata and publisher file hashes are preserved in SOURCE_MANIFEST.json. The game loads local cooked assets and makes no API calls.\n\n'+''.join('- ['+a['title']+']('+a['source']+') — '+', '.join(a['authors'])+' — '+a['license']+'\n' for a in entries),encoding='utf8')
 print('SOURCE_DOWNLOAD_COMPLETE',len(entries),flush=True)
