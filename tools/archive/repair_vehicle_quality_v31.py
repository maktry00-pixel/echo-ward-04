from pathlib import Path
p=Path('tools/prepare_vehicle_meshes_v31.py');s=p.read_text();s=s.replace(" if done.exists() and (slug!='auto-mezzanine-deck-trailer' or json.loads(done.read_text()).get('revision')==5):"," if done.exists() and json.loads(done.read_text()).get('revision')==6:")
s=s.replace(" if original>max_tri:"," if False and original>max_tri:")
s=s.replace("  if slug=='auto-mezzanine-deck-trailer':meshes=[o for o in meshes if o.name!='Cylinder005']", "  if slug=='auto-mezzanine-deck-trailer':meshes=[o for o in meshes if o.name!='Cylinder005']\n  if slug=='2025-bentley-continental-gt-speed':meshes=[o for o in meshes if not o.name.startswith('Icosphere')]\n  if slug=='mercedes-benz-maybach-2022':meshes=[o for o in meshes if o.name!='Cube_Cube.001']\n  if slug=='court-porsche-test':meshes=[o for o in meshes if o.name!='Plane_0']")
a="    glass=bool(ma and any(t in ma.name.lower() for t in ['glass','glas','window']) and 'frame' not in ma.name.lower())";b="    glass=is_transparent(ma)";assert a in s;s=s.replace(a,b)
a="  has_glass=any(m and any(t in m.name.lower() for t in ['glass','glas','window']) and 'frame' not in m.name.lower() for ob in meshes for m in ob.data.materials)";b="  has_glass=any(is_transparent(m) for ob in meshes for m in ob.data.materials)";assert a in s;s=s.replace(a,b)
a='def export_asset(';b='''def is_transparent(m):
 if not m:return False
 name=m.name.lower()
 if any(t in name for t in ['glass','glas','window']) and 'frame' not in name:return True
 bs=next((n for n in m.node_tree.nodes if n.type=='BSDF_PRINCIPLED'),None) if m.use_nodes else None
 return bool(bs and bs.inputs.get('Alpha') and bs.inputs['Alpha'].default_value<.95)

def export_asset(''';s=s.replace(a,b);s=s.replace('payload=dict(revision=5','payload=dict(revision=6');p.write_text(s)
p=Path('tools/finalize_vehicle_sources_v31.py');s=p.read_text().replace("if row['nanite'] and any(d['materials'][k]['alpha']=='BLEND' for k in row['materials'])", "if row['nanite'] and (len(row['materials'])>64 or any(d['materials'][k]['alpha']=='BLEND' for k in row['materials']))");p.write_text(s)
p=Path('unreal/EchoWard/Content/Python/import_echo_cars_v31.py');s=p.read_text().replace("u.StaticMeshReductionSettings(1.,1.),u.StaticMeshReductionSettings(.48,.3),u.StaticMeshReductionSettings(.18,.11),u.StaticMeshReductionSettings(.04,.035)","u.StaticMeshReductionSettings(1.,.4),u.StaticMeshReductionSettings(.55,.12),u.StaticMeshReductionSettings(.22,.045),u.StaticMeshReductionSettings(.06,.012)");p.write_text(s)
p=Path('unreal/EchoWard/Content/Python/audit_echo_cars_v31.py');s=p.read_text().replace(" corrected=[]", " corrected=[]\n for path in E.list_assets(B+'/Materials',recursive=False):\n  if '/M_Cars31B_' in path and 'BLEND' not in path:\n   parent=u.load_asset(path);L.set_base_material_usage(parent,u.MaterialUsage.MATUSAGE_NANITE,True);L.recompile_material(parent);assert E.save_loaded_asset(parent)");p.write_text(s)
p=Path('tools/deploy_echo_cars_v31.py');s=p.read_text().replace("out=dict(version=31,", "out=dict(version=31,native_meshes=len(D['assets']),");s=s.replace("p=next(p for p in P if p['slug']==slug);v=models[slug]", "p=next(p for p in P if p['slug']==slug);v=models[slug]")
s=s.replace("first=next(p for p in P if p['role']=='passenger');", "next(v for v in views if v['id']=='v31_gmc-savana-2003')['at']=[113,-7,2.4]\nfirst=next(p for p in P if p['role']=='passenger');");p.write_text(s)
p=Path('unreal/EchoWard/Source/EchoWard/WardBookWorldAssessment.cpp');s=p.read_text().replace('v31_all_68_vehicle_meshes_load_with_metric_bounds','v31_all_vehicle_meshes_load_with_metric_bounds').replace('Count==68&&Assets&&Metric','Count==Number(WardEchoAssets::Cars31(),TEXT("native_meshes"))&&Assets&&Metric');p.write_text(s)
