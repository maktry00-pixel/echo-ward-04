import pathlib
stage=pathlib.Path(__file__).parent
wrapper=(stage/'build_beka_draft_v3.py').read_text(encoding='utf8')
wrapper=wrapper[:wrapper.rfind("exec(compile(s,")]
ns={'__file__':str(stage/'build_beka_draft_v3.py')}
exec(compile(wrapper,'compose_v3','exec'),ns)
s=ns['s'].replace('beka_reconstruction_draft_v3','beka_reconstruction_draft_v4').replace('Beka_reconstruction_draft_v3','Beka_reconstruction_draft_v4').replace('manual_assembly_v3','manual_assembly_v4')
s=s.replace("p.material_index=1 if p.normal.y<-.25 else 0","p.material_index=1 if p.center.y<-.028 else 0")
s=s.replace("bindhead(cap)","bm=bmesh.new();bm.from_mesh(cap.data)\nfor f in bm.faces:\n radial=f.calc_center_median()-Vector((0,.005,1.664))\n if f.normal.dot(radial)<0:f.normal_flip()\nbm.to_mesh(cap.data);bm.free();cap.data.update()\nbindhead(cap)")
s=s.replace("(face.matrix_world@v.co).z<1.455 or","(face.matrix_world@v.co).z<1.490 or")
s=s.replace("edges=[e for e in bm.edges if e.is_boundary and all(.22<v.co.z<.88 for v in e.verts)]\nif edges:bmesh.ops.holes_fill(bm,edges=edges,sides=30)","edges=[e for e in bm.edges if e.is_boundary]\nif edges:bmesh.ops.holes_fill(bm,edges=edges,sides=0)")
trim=r"""
# Place raised V trim directly on the curved hoodie surface at its final height.
profiles_fit=[(.925,.175,.118),(.94,.184,.127),(.98,.189,.132),(1.04,.191,.132),(1.10,.193,.136),(1.16,.199,.143),(1.22,.211,.150),(1.28,.222,.148),(1.33,.218,.140),(1.38,.205,.125),(1.42,.173,.108),(1.455,.104,.081),(1.47,.080,.076)]
for o in bpy.context.scene.objects:
 if o.type=='MESH' and o.name.startswith('V chest seam'):
  for v in o.data.vertices:
   z=v.co.z
   for j in range(len(profiles_fit)-1):
    z0,rx0,ry0=profiles_fit[j];z1,rx1,ry1=profiles_fit[j+1]
    if z0<=z<=z1:
     t=(z-z0)/(z1-z0);rx=rx0+(rx1-rx0)*t;ry=ry0+(ry1-ry0)*t
     v.co.y=-ry*math.sqrt(max(.001,1-(v.co.x/rx)**2))-.0018
     break
"""
s=s.replace("# Hair: a real volumetric cap",trim+"\n# Hair: a real volumetric cap")
s=s.replace("'stage':'manual_draft_v3'","'stage':'manual_draft_v4'")
exec(compile(s,str(stage/'generated_build_v4.py'),'exec'))

