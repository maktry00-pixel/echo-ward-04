from pathlib import Path
s=Path(__file__).with_name('bake_clean_skin_v21.py').read_text(encoding='utf8')
s=s.replace('metahuman_conform_v21','metahuman_conform_v23')
s=s.replace("s=s.replace('blend.data[v.index].color=(w,w,w,1)', 'w*=float(np.clip((p.z-1.54)/.035,0,1))\\n blend.data[v.index].color=(w,w,w,1)')",'')
s=s.replace('(.46,.285,.19)','(.34,.20,.13)')
exec(compile(s,str(Path(__file__).with_name('bake_clean_skin_v21.py')),'exec'))
