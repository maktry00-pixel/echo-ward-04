"""Local, offline catalog of real assets and explicit pending entries."""
from pathlib import Path
import json,html
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'assets/models/echo_custom'
p=json.loads((OUT/'progress.json').read_text(encoding='utf-8'))
cards=[]
for a in p['assets']:
    if a['status']=='not_started':continue
    i=a['id'];d=f'EH_{i:03d}';title=html.escape(a['title'])
    note='<p style="color:#d2ad7d">Черновой дизайн: см. QUALITY_STATUS.md</p>' if i>=564 else ''
    cards.append(f'<article data-id="{i}" data-title="{title.lower()}"><a href="{d}/preview.png"><img loading="lazy" src="{d}/preview.png" alt="{title}"></a><div class="body"><small>№ {i:03d}</small><h2>{title}</h2>{note}<p><a href="{d}/{d}.blend">Исходник Blender</a> · <a href="{d}/{d}.glb">3D-модель</a></p></div></article>')
pending=' · '.join(f'{a["id"]:03d}' for a in p['assets'] if a['status']=='not_started')
page='''<!doctype html><html lang="ru"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>ЭХО — библиотека моделей</title><style>
*{box-sizing:border-box}body{margin:0;background:#101a1b;color:#dee7df;font:16px/1.5 Segoe UI,Arial,sans-serif}header,main,footer{max-width:1500px;margin:auto;padding:32px}header{border-bottom:1px solid #37514e}header b{letter-spacing:.35em;color:#c29460}h1{font-size:42px;line-height:1.1;margin:14px 0}header p{max-width:850px;color:#a5bbb3}.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(235px,1fr));gap:18px}article{background:#1b2b2c;border:1px solid #36504c;border-radius:9px;overflow:hidden}img{display:block;width:100%;aspect-ratio:1;object-fit:cover}.body{padding:16px}h2{font-size:17px;min-height:52px;margin:7px 0}small{color:#b5c2a7}a{color:#acd1c7;text-decoration:none}a:hover{text-decoration:underline}article p{font-size:13px}footer{color:#9cb0a9}details{margin:25px 0;padding:18px;border:1px solid #36504c}summary{cursor:pointer}.hero{width:100%;max-height:640px;object-fit:cover;border-radius:10px;margin:20px 0}</style>
<header><b>ЭХО / ОТДЕЛЕНИЕ 04</b><h1>Библиотека 3D-моделей</h1><p>Заброшенная больница. Холодная краска, старая штукатурка, тусклый металл и следы времени. Нумерация соответствует вашему списку из 575 пунктов.</p>'''
page+=f'<p><strong>Проверены файлы {p["files_validated"]} из {p["total_requested"]} позиций.</strong> Первый проход моделей и материалов. Светлые превью показывают детали. Часть сложных предметов и персонажей — черновые модели; интеграция в игру пока не выполнена. <a href="QUALITY_STATUS.md">Ограничения и художественная готовность</a>.</p>'
if (OUT/'showcase/ECHO_corridor.png').exists():page+='<a href="showcase/ECHO_corridor.png"><img class="hero" src="showcase/ECHO_corridor.png" alt="Сцена коридора из новых 3D-моделей"></a>'
page+='</header><main><p><label>Найти по номеру или названию <input id="search" type="search" placeholder="Например: 175, дверь, лампа" style="width:340px;max-width:100%;padding:12px;margin:10px;background:#203435;color:#e1ebe5;border:1px solid #48665c;border-radius:6px"></label> <span id="visible"></span></p><div class="grid">'+''.join(cards)+'</div>'
page+='<details><summary>Пока не созданы: '+str(p['remaining'])+' позиций</summary><p>'+pending+'</p></details></main>'
page+='<footer>GLB использует общую папку textures_shared. Для переноса сохраните всю библиотеку целиком. Исходники Blender используют папку materials. Подробности — ART_DIRECTION.md.</footer><script>const search=document.querySelector("#search");search.addEventListener("input",()=>{const q=search.value.trim().toLowerCase();let n=0;document.querySelectorAll("article").forEach(card=>{const yes=!q||card.dataset.title.includes(q)||String(card.dataset.id).padStart(3,"0").includes(q);card.hidden=!yes;if(yes)n++;});document.querySelector("#visible").textContent="Показано: "+n;});</script></html>'
(OUT/'index.html').write_text(page,encoding='utf-8')
print('GALLERY',p['files_validated'])
