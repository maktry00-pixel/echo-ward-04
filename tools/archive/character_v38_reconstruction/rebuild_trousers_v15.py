from pathlib import Path
stage=Path(__file__).parent
s=(stage/'rebuild_trousers_v14.py').read_text(encoding='utf8').replace('character_candidate_v14','character_candidate_v15').replace('candidate_v14','candidate_v15').replace('v14','v15')
s=s.replace("[(.758,.045,.075,0),(.78,.115,.090,0),(.82,.165,.102,0),(.87,.178,.106,0),(.933,.174,.102,0)]", "[(.747,.006,.008,0),(.752,.030,.035,0),(.76,.065,.063,0),(.77,.090,.079,0),(.79,.130,.094,0),(.81,.156,.103,0),(.84,.178,.108,0),(.87,.181,.109,0),(.90,.180,.107,0),(.933,.174,.102,0)]")
s=s.replace("sm.iterations=5", "sm.iterations=12")
a=s.index('for side in [-1,1]:\n cx=side*.096;points=[]')
b=s.index('objects=[o for o in sc.objects',a)
pocket=r'''
for side in [-1,1]:
 cx=side*.096;points=[];faces=[];nx=8;nz=9
 for j in range(nz):
  t=j/(nz-1)
  for i in range(nx):
   u=-1+2*i/(nx-1);x=cx+u*.034;bottom=.785+.009*u*u;z=bottom+(.858-bottom)*t
   hit=pv.ray_cast(Vector((x,.5,z)),Vector((0,-1,0)),1);points.append(tuple(hit[0]+hit[1]*.0009))
 for j in range(nz-1):
  for i in range(nx-1):
   q=j*nx+i;faces.append((q,q+1,q+1+nx,q+nx))
 me=bpy.data.meshes.new('Surface fitted sewn pocket');me.from_pydata(points,[],faces);me.update();ob=bpy.data.objects.new('Rear fitted pocket '+str(side),me);bpy.context.collection.objects.link(ob);me.materials.append(pm)
 for p in me.polygons:p.use_smooth=True
'''
s=s[:a]+pocket+s[b:]
(stage/'resolved_character_v15.py').write_text(s,encoding='utf8')
exec(compile(s,str(stage/'resolved_character_v15.py'),'exec'))
