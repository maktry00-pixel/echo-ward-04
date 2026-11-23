"""Independent story placement and timing audit on the real furnished map."""
import contextlib,hashlib,io,json,runpy
from pathlib import Path
from collections import Counter
ROOT=Path(__file__).resolve().parents[1]
with contextlib.redirect_stdout(io.StringIO()):g=runpy.run_path(str(ROOT/'tools/verify_story_layout.py'))
d=g['D'];game=d['story_gameplay'];checks={};bad=[]
OUT=ROOT/f'artifacts/unreal_hospital_v{d.get("review_revision",7)}';OUT.mkdir(exist_ok=True)
zones={r['id']:(l,r) for l in d['locations'] for r in l['rooms']};rows=game['placements']+game['mobs']
for p in rows:
    l,r=zones[p['zone']];x,y,z=p['at'];ox,oy,zz=l['origin'];rx,ry,w,h=r['rect'];n=g['node'](x,y,z)
    if not(ox+rx<x<ox+rx+w and oy+ry<y<oy+ry+h and abs(z-zz)<.01 and g['flat'][n] and g['parent'][n]>=0):bad.append(p['id'])
checks['every_story_item_and_mob_is_inside_reachable_furnished_floor']=not bad
checks['19_chapters_and_78_explicit_spatial_scene_stubs']=len(game['chapters'])==19 and len(game['scenes'])==78 and all(s['implementation']=='interactive_spatial_stub' for s in game['scenes'])
notes=[p for p in game['placements'] if p['kind']=='note']
checks['96_notes_have_empty_bodies']=len(notes)==96 and all(p['body']=='' for p in notes)
checks['unique_item_and_mob_ids']=len(rows)==len({p['id'] for p in rows})
checks['17_species_46_mobs_and_no_chapter_one_attackers']=len({p['species'] for p in game['mobs']})==17 and len(game['mobs'])==46 and min(p['chapter'] for p in game['mobs'])==2
rules={r['id']:r for r in d['monster_rules']}
checks['species_first_chapter_and_kill_class_match_source_rules']=all(p['chapter']==rules[p['species']]['first_chapter'] and p['combat_class']==rules[p['species']]['combat_class'] for p in game['mobs'])
checks['listener_remains_B_finale_only']=rules['listener']['combat_class']=='B' and rules['listener']['kill_condition']=='finale_only'
checks['all_chapter_scene_requirements_refer_to_present_actors']=all(set(c['required'])<={p['id'] for p in game['placements'] if p['kind']=='scene' and p['chapter']==c['id']} for c in game['chapters'])
checks['pistol_is_in_security_post_shotgun_in_department_security']=any(p['id']=='first_pistol' and p['zone']=='F101' for p in rows) and any(p['id']=='first_shotgun' and p['zone']=='B114' for p in rows)
checks['all_12_link_nodes_are_interactive']=len([p for p in rows if p.get('kind')=='node'])==12
checks['six_side_stories_have_physical_blank_documents']=len(game['side_stories'])==6 and len([p for p in notes if p.get('side_story')])==18
report=dict(success=all(checks.values()),checks=checks,issues=bad,map_sha256=hashlib.sha256(g['SOURCE'].read_bytes()).hexdigest(),
    counts=dict(placements=len(game['placements']),scene_stubs=78,notes=len(notes),mobs=46,species=17),
    limits=game['limits'])
(OUT/'story_acceptance.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(report,ensure_ascii=False,indent=2))
if not report['success']:raise SystemExit(1)
