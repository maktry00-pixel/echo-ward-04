from pathlib import Path
stage=Path(__file__).parent
s8=(stage/'build_head_likeness_v8.py').read_text(encoding='utf8')
s8=s8[:s8.rfind('exec(compile(s,')];n8={'__file__':str(stage/'build_head_likeness_v8.py')};exec(compile(s8,'compose8','exec'),n8)
s7=n8['s'];s7=s7[:s7.rfind('exec(compile(src,')];n7={'__file__':str(stage/'build_head_likeness_v7.py')};exec(compile(s7,'compose7','exec'),n7)
s=n7['src'].replace('likeness_head_v8','likeness_head_v9').replace('candidate_v8','candidate_v9').replace('v8_albedo','v9_albedo')
s=s.replace("blend.data[v.index].color=(w,w,w,1)","nosefade=math.exp(-((p.z-1.601)/.010)**4-(p.x/.023)**4)*float(np.clip((-p.y-.075)/.035,0,1))\n w*=1-.90*nosefade\n blend.data[v.index].color=(w,w,w,1)")
at="strands=bpy.data.objects.new('Beka hair individual strands',cv);bpy.context.collection.objects.link(strands)"
add=r'''
# Loosen the edge silhouette with fine, irregular tapered fringe and sideburn strands.
edgecounts={}
for p in hairme.polygons:
 for e in p.edge_keys:edgecounts[e]=edgecounts.get(e,0)+1
for (a,b),count in edgecounts.items():
 if count!=1:continue
 va,vb=hairme.vertices[a],hairme.vertices[b]
 if (va.co.z+vb.co.z)/2>1.705:continue
 for k in range(8):
  t=rng.random();root=va.co.lerp(vb.co,t);n=va.normal.lerp(vb.normal,t).normalized()
  down=Vector((-.45 if root.y<-.02 else 0,-.18 if root.y<-.02 else .05,-1)).normalized()
  spl=cv.splines.new('POLY');spl.points.add(8)
  le=rng.uniform(.003,.008)
  for j in range(9):
   q=j/8;p=root+down*(le*q)+n*(.0007+.0005*math.sin(q*math.pi));p.x+=.0008*math.sin(q*4+k)
   spl.points[j].co=(*p,1);spl.points[j].radius=max(.04,1-q)
# Portrait hair coloration is mapped onto the continuous scalp. Clamp to the actual hair region,
# so the mall background cannot be projected onto the top of the head.
huv=hairme.uv_layers.new(name='PortraitProjection')
hblend=hairme.color_attributes.new(name='PortraitBlend',type='FLOAT_COLOR',domain='POINT')
topline=np.array([[.1,.24],[.18,.14],[.3,.075],[.5,.045],[.68,.063],[.80,.115],[.90,.245]])
for i,idx in enumerate(used):
 n=hairme.vertices[i].normal;w=float(np.clip((-n.y+.05)/.55,0,1));hblend.data[i].color=(w,w,w,1)
for p in hairme.polygons:
 for li in p.loop_indices:
  idx=used[hairme.loops[li].vertex_index];xy=projected[idx].copy();xy[0]=float(np.clip(xy[0],.14,.87))
  lo=float(np.interp(xy[0],topline[:,0],topline[:,1]))+.015
  hi=float(np.interp(xy[0],hairline[:,0],hairline[:,1]))-.008
  xy[1]=float(np.clip(xy[1],lo,max(lo+.01,hi)));huv.data[li].uv=(xy[0],1-xy[1])
hairme.materials.clear();hairme.materials.append(projection_material('Portrait scalp baking',(.004,.0025,.0018)))
bpy.ops.object.select_all(action='DESELECT');hair.select_set(True);bpy.context.view_layer.objects.active=hair
sub=hair.modifiers.new('Soft irregular hair boundary','SUBSURF');sub.levels=1;bpy.ops.object.modifier_apply(modifier=sub.name)
hairuv=hair.data.uv_layers.new(name='HairBakedUV');hair.data.uv_layers.active_index=len(hair.data.uv_layers)-1
bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.smart_project(angle_limit=1.12,island_margin=.006);bpy.ops.object.mode_set(mode='OBJECT')
ha=bpy.data.images.new('Beka hair albedo',width=2048,height=2048,alpha=False)
hm=hair.data.materials[0];targetim=hm.node_tree.nodes.new('ShaderNodeTexImage');targetim.image=ha;hm.node_tree.nodes.active=targetim
bpy.context.scene.render.engine='CYCLES';bpy.context.scene.cycles.samples=1;bpy.context.scene.render.bake.margin=10;bpy.ops.object.bake(type='EMIT')
ha.filepath_raw=str(O/'Beka_hair_albedo_2048.png');ha.file_format='PNG';ha.save();ha.pack()
hmat=bpy.data.materials.new('Beka portrait colored hair');hmat.use_nodes=True;hn=hmat.node_tree.nodes;hl=hmat.node_tree.links
hi=hn.new('ShaderNodeTexImage');hi.image=ha;hu=hn.new('ShaderNodeUVMap');hu.uv_map='HairBakedUV';hl.new(hu.outputs['UV'],hi.inputs['Vector']);hl.new(hi.outputs['Color'],hn.get('Principled BSDF').inputs['Base Color'])
hn.get('Principled BSDF').inputs['Roughness'].default_value=.73;hn.get('Principled BSDF').inputs['Specular IOR Level'].default_value=.10
hair.data.materials.clear();hair.data.materials.append(hmat)
for layer in list(hair.data.uv_layers):
 if layer.name!='HairBakedUV':hair.data.uv_layers.remove(layer)
'''
s=s.replace(at,at+add)
s=s.replace("sc.cycles.samples=40","sc.cycles.samples=32")
# Save the resolved source for reproducibility instead of keeping only nested composer scripts.
(stage/'resolved_head_v9.py').write_text(s,encoding='utf8')
exec(compile(s,str(stage/'resolved_head_v9.py'),'exec'))
