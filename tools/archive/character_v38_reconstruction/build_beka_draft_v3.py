import pathlib
stage=pathlib.Path(__file__).parent
wrapper=(stage/'build_beka_draft_v2.py').read_text(encoding='utf8')
wrapper=wrapper[:wrapper.rfind("exec(compile(s,")]
ns={'__file__':str(stage/'build_beka_draft_v2.py')}
exec(compile(wrapper,'compose_v2','exec'),ns)
s=ns['s'].replace('beka_reconstruction_draft_v2','beka_reconstruction_draft_v3').replace('Beka_reconstruction_draft_v2','Beka_reconstruction_draft_v3').replace('manual_assembly_v2','manual_assembly_v3')
s=s.replace("arr[:,0]+=disp[:,0]*w;arr[:,2]+=disp[:,1]*w\n return arr","arr[:,0]+=disp[:,0]*w;arr[:,2]+=disp[:,1]*w\n correction=np.exp(-((np.abs(arr[:,0])-.030)/.028)**2-((arr[:,2]-1.655)/.026)**2)*np.clip((.035-arr[:,1])/.075,0,1)\n arr[:,2]-=.0105*correction\n return arr")
s=s.replace("for i in range(64):","for i in range(0):").replace("for i in range(460):","for i in range(200):")
captext=r"""
# Frontal hair color comes from the reference, applied to actual 3D scalp volume.
hairphoto=photo_mat('Photo matched black hair front',0,.82)
cap.data.materials.append(hairphoto)
uv=cap.data.uv_layers.new(name='HairPhotoUV')
for p in cap.data.polygons:
 p.material_index=1 if p.normal.y<-.25 else 0
 for li in p.loop_indices:
  co=cap.data.vertices[cap.data.loops[li].vertex_index].co
  uv.data[li].uv=((535+co.x*750)/1086,1-(1404-co.z*750)/1448)
"""
s=s.replace("bindhead(cap)",captext+"\nbindhead(cap)")
repair=r"""
# Close small accidental gaps in connected cloth, preserve waistband and cuffs.
pants=next(o for o in bpy.context.scene.objects if o.type=='MESH' and o.name=='Connected olive trousers')
bm=bmesh.new();bm.from_mesh(pants.data)
edges=[e for e in bm.edges if e.is_boundary and all(.22<v.co.z<.88 for v in e.verts)]
if edges:bmesh.ops.holes_fill(bm,edges=edges,sides=30)
for v in bm.verts:
 z=v.co.z;x=v.co.x
 if .18<z<.88:
  envelope=math.sin(math.pi*(z-.18)/.70)
  wave=.0017*math.sin(z*62+abs(x)*29)*envelope
  v.co+=v.normal*wave
bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(pants.data);bm.free()
"""
s=s.replace("# Capture deformed geometry",repair+"\n# Capture deformed geometry")
# Join static review objects by semantic group. Shared material slots are preserved.
join=r"""
groups={}
for o in export_objects:
 name=o.name
 key='Hair' if 'hair' in name.lower() else 'Head' if 'Head_Neck' in name else 'Hands' if 'Hands' in name else 'Eyes' if 'Iris' in name or 'Pupil' in name else 'Trousers' if any(k in name.lower() for k in ['trouser','belt loop','fly']) else 'Shoes' if any(k in name.lower() for k in ['sneaker','lace','sole']) else 'Hoodie'
 groups.setdefault(key,[]).append(o)
export_objects=[]
for key,items in groups.items():
 bpy.ops.object.select_all(action='DESELECT')
 for o in items:o.select_set(True)
 bpy.context.view_layer.objects.active=items[0]
 if len(items)>1:bpy.ops.object.join()
 obj=bpy.context.view_layer.objects.active;obj.name='Beka_'+key
 export_objects.append(obj)
"""
s=s.replace("# Bake procedural fabric values",join+"\n# Bake procedural fabric values")
s=s.replace("'stage':'manual_draft'","'stage':'manual_draft_v3'")
exec(compile(s,str(stage/'generated_build_v3.py'),'exec'))

