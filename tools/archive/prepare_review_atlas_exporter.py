"""Fork the reviewed v3 atlas renderer without overwriting the historical atlas."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
s=(root/'tools/export_story_hospital_atlas.py').read_text(encoding='utf-8')
s=s.replace("output/pdf/ECHO_Hospital_v3","output/pdf/ECHO_Hospital_v4")
s=s.replace("tmp/pdfs/hospital_story_v3","tmp/pdfs/hospital_review_v4")
s=s.replace('ЭХО_карта_v3_геометрия_и_сюжет.pdf','ЭХО_полная_карта_со_всеми_исправлениями.pdf')
s=s.replace('карта v3, размеры и сюжетные привязки','карта с исправлениями v4, размеры и инженерия')
s=s.replace('22.10.2026','23.10.2026')
s=s.replace('hospital_story_shell_v3 / метры / 23.10.2026','revision 4 / метры / экспорт 23.10.2026')
s=s.replace("assert DRAFT or MARKER['build_identity'].split(':')[-1].lower()==hashlib.sha256(SOURCE.read_bytes()).hexdigest()", "assert not DRAFT and DATA['review_revision']==4\nassert MARKER['build_identity'].split(':')[-1].lower()==hashlib.sha256(SOURCE.read_bytes()).hexdigest()\nVERIFIED=json.loads((ROOT/'artifacts/unreal_hospital_v4/verification.json').read_text(encoding='utf-8'))\nassert VERIFIED['success'] and VERIFIED['map_sha256']==hashlib.sha256(SOURCE.read_bytes()).hexdigest()")
needle='# Exact wall centrelines'
pos=s.index(needle)
s=s[:pos]+"for n in DATA['escape_niches']:PORTALS.append((*n['entry'][:2],n['entry'][2],n['side'],2))\nPORTAL_BINS=defaultdict(list)\nfor px,py,pz,side,pw in PORTALS:PORTAL_BINS[(pz,'NS' if side in ('north','south') else 'EW',py if side in ('north','south') else px)].append((px,py,pz,side,pw))\n"+s[pos:]
s=s.replace('for px,py,pz,side,pw in PORTALS)',"for px,py,pz,side,pw in PORTAL_BINS.get((z,'NS' if dy else 'EW',my if dy else mx),[]))")
pos=s.index("PDF=OUT/")
s=s[:pos]+"exec((ROOT/'tools/hospital_atlas_v4_layers.py').read_text(encoding='utf-8'),globals())\n\n"+s[pos:]
s=s.replace("('Что входит в архив','PDF, отдельные PNG всех схем, CSV всех 334 зон, покрытий, стен, лестничных ступеней, лифта, фонарей и инженерии, а также исходный JSON карты. Идентификаторы на плане совпадают с игрой.')", "('Полнота данных','В конце PDF добавлены схемы укрытий, выходов, опор, тоннелей и состояний по главам. Все 334 зоны включены в реестры. Полные CSV и JSON встроены в PDF как вложения; их можно извлечь через панель вложений в настольном PDF-просмотрщике.')")
s=s.replace('Геометрия снимка: 23.10.2026; SHA-256 источника указан в manifest.json.','Карта проверена в игре 22.10.2026; экспорт 23.10.2026. SHA-256 указан в приложении PDF и manifest.json.')
s=s.replace('Подписи X/Y на краю схемы - мировые координаты.', 'Дополнение с выходами и нишами: листы 40-54. Подписи X/Y - мировые координаты.')
s=s.replace('for r in block:', 'for r in block:')
s=s.replace('c.save()', 'append_review_pages(c)\nc.save()')
s=s.replace('assert len(reader.pages)==len(PAGE_INDEX)==35','assert len(reader.pages)==len(PAGE_INDEX)==57')
s=s.replace("manifest=dict(draft=DRAFT,world_id=DATA['map_id']", "manifest=dict(revision=4,draft=DRAFT,world_id=DATA['map_id']")
s=s.replace("print(json.dumps({'pdf'", "embed_review_attachments(PDF)\nprint(json.dumps({'pdf'")
(root/'tools/export_review_hospital_atlas.py').write_text(s,encoding='utf-8')
print('Prepared tools/export_review_hospital_atlas.py')
