from pathlib import Path
import json,hashlib
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'assets/models/echo_custom'
for i in (563,565,566):
    p=OUT/f'EH_{i:03d}'/'asset.json';d=json.loads(p.read_text(encoding='utf-8'))
    if not d.get('source','').endswith('.blend'):d['upstream_source']=d['source']
    d['source']=f'EH_{i:03d}.blend'
    if i==565:d['limitation']='Licensed medical human base; final zombie nurse face, skin and uniform art are still pending.'
    p.write_text(json.dumps(d,ensure_ascii=False,indent=2),encoding='utf-8')
metadata=[]
for p in sorted(OUT.glob('EH_*/asset.json')):
    d=json.loads(p.read_text(encoding='utf-8'));i=d['id']
    d['art_status']='first_pass_for_scene_review' if i<563 else ('licensed_hero_base' if i==563 else ('zombie_art_pending' if i==565 else ('temporary_zombie_base' if i==566 else 'concept_art_and_animation_polish_pending')))
    p.write_text(json.dumps(d,ensure_ascii=False,indent=2),encoding='utf-8');metadata.append(d)
stats={'catalog_sha256':hashlib.sha256((OUT/'REQUESTED_CATALOG.txt').read_bytes()).hexdigest(),'requested':575,'files_authored':len(metadata),'scope':'First-pass 3D asset library. Not all artwork is final; see QUALITY_STATUS.md. Not integrated into the game.','glb_files':len(list(OUT.glob('EH_*/*.glb'))),'blend_sources':len(list(OUT.glob('EH_*/*.blend'))),'previews':len(list(OUT.glob('EH_*/preview.png'))),'triangles_total':sum(d['triangles'] for d in metadata),'unique_shared_images':len(list((OUT/'textures_shared').glob('*'))),'original_pbr_material_folders':len(list((OUT/'materials').glob('*/material.json'))),'library_bytes':sum(p.stat().st_size for p in OUT.rglob('*') if p.is_file()),'external_spending':0,'game_integration':False}
for name in ['progress','import_verification','rig_verification']:
    p=OUT/(name+'.json')
    if p.exists():
        report=json.loads(p.read_text(encoding='utf-8'));stats[name]={k:v for k,v in report.items() if k in ('passed','files_validated','remaining','failures')}
if (OUT/'GAME_INTEGRATION.json').exists():
    stats['game_integration']=json.loads((OUT/'GAME_INTEGRATION.json').read_text(encoding='utf8'))
    stats['scope']='First-pass authored library; static assets imported into Unreal, selected models actively placed and verified. Skeletal art/animation remains pending.'
(OUT/'LIBRARY_MANIFEST.json').write_text(json.dumps(stats,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(stats,ensure_ascii=False,indent=2))
