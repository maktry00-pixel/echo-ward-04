from pathlib import Path
stage=Path(__file__).parent
s=(stage/'resolved_head_v9.py').read_text(encoding='utf8').replace('likeness_head_v9','likeness_head_v10').replace('candidate_v9','candidate_v10').replace('v9_albedo','v10_albedo')
s=s.replace('w*=1-.90*nosefade','w*=1-.45*nosefade')
s=s.replace('p+=n*lift;vs.append(tuple(p))',"ph=math.atan2(p.x,-p.y+.015)\n lift+=.0035*math.sin(ph*10+p.z*45)+.0018*math.sin(ph*19+p.z*80)\n p+=n*lift\n p.z-=.005*float(np.clip((p.z-1.70)/.075,0,1))\n vs.append(tuple(p))")
s=s.replace('range(6200)','range(4700)')
s=s.replace('ph=phi-(.95*t+.18*math.sin(t*math.pi))','ph=phi-(.95*t+.18*math.sin(t*math.pi)+.12*math.sin(t*math.pi*2))')
s=s.replace('le=rng.uniform(.003,.008)','le=rng.uniform(.0015,.004)')
s=s.replace("hairline[:,1]))-.008","hairline[:,1]))-.045")
s=s.replace("sc.cycles.samples=32","sc.cycles.samples=24")
(stage/'resolved_head_v10.py').write_text(s,encoding='utf8')
exec(compile(s,str(stage/'resolved_head_v10.py'),'exec'))
