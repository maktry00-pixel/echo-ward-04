from pathlib import Path
p=Path(__file__).with_name('validate_character_v15.py')
s=p.read_text(encoding='utf8').replace("O=R/'artifacts/echo_hero_v38/character_candidate_v15'","O=R/'artifacts/echo_hero_v38/character_candidate_v16'").replace("O/'Beka_character_candidate_v15.glb'","O/'Beka_character_candidate_v16.glb'")
exec(compile(s,str(p),'exec'))
