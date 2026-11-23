from pathlib import Path
stage=Path(__file__).parent
s=(stage/'resolved_head_v10.py').read_text(encoding='utf8').replace('likeness_head_v10','likeness_head_v11').replace('candidate_v10','candidate_v11').replace('v10_albedo','v11_albedo')
a=s.index('# Portrait hair coloration');b=s.index('# Keep the scalp and strands',a)
s=s[:a]+s[b:]
s=s.replace("cv.bevel_depth=.00011","cv.bevel_depth=.000075")
s=s.replace("(.009,.006,.0038,1)","(.0038,.0025,.0015,1)")
s=s.replace("hp.inputs['Roughness'].default_value=.48","hp.inputs['Roughness'].default_value=.66")
s=s.replace("(.0035,.0024,.0018,1)","(.002,.0014,.001,1)")
mark="strands=bpy.data.objects.new('Beka hair individual strands',cv);bpy.context.collection.objects.link(strands)"
extra=r'''
# Long continuous curved fringe guides originate around the reference's off-centre part.
# They cover the lower front cap instead of ending halfway down the skull.
edgecounts0={}
for p in hairme.polygons:
 for e in p.edge_keys:edgecounts0[e]=edgecounts0.get(e,0)+1
ends=[]
for (a,b),count in edgecounts0.items():
 if count==1:
  va,vb=hairme.vertices[a],hairme.vertices[b]
  if (va.co.y+vb.co.y)<-.06 and 1.64<(va.co.z+vb.co.z)/2<1.71:ends.append((va.co.copy(),vb.co.copy()))
for i in range(7800):
 va,vb=rng.choice(ends);end=va.lerp(vb,rng.random());side=-1 if end.x<.018 else 1
 root=Vector((rng.uniform(.015,.04),rng.uniform(-.015,.02),rng.uniform(1.745,1.765)))
 c1=root+Vector((side*rng.uniform(.020,.040),-.030,.008))
 c2=end+Vector((-side*rng.uniform(.018,.035),.024,.025))
 spl=cv.splines.new('POLY');spl.points.add(25)
 for j in range(26):
  t=j/25;u=1-t;q=u*u*u*root+3*u*u*t*c1+3*u*t*t*c2+t*t*t*end
  hit=capbvh.find_nearest(q);p=hit[0].copy();n=hit[1]
  p+=n*(.0005+.002*math.sin(t*math.pi)+.0005*math.sin(t*10+i*.13))
  if t>.94:p+=Vector((side*.001,0,-.001))*((t-.94)/.06)
  spl.points[j].co=(*p,1);spl.points[j].radius=max(.06,math.sin((t*.87+.08)*math.pi))
# Smooth only the polygonal cap boundary; the visible silhouette comes from the strand tips.
sub=hair.modifiers.new('Soft scalp boundary','SUBSURF');sub.levels=1
bpy.ops.object.select_all(action='DESELECT');hair.select_set(True);bpy.context.view_layer.objects.active=hair;bpy.ops.object.modifier_apply(modifier=sub.name)
'''
s=s.replace(mark,extra+'\n'+mark)
s=s.replace("sc.cycles.samples=24","sc.cycles.samples=24")
(stage/'resolved_head_v11.py').write_text(s,encoding='utf8')
exec(compile(s,str(stage/'resolved_head_v11.py'),'exec'))
