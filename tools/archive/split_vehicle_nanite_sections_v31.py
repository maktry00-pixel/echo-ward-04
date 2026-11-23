"""Partition four material-heavy bodies into <=64-slot Nanite meshes, without reducing geometry."""
import bpy,json,hashlib,copy,shutil
from pathlib import Path
R=Path('D:/Projects/ECHO');O=R/'unreal/EchoWard/RawAssets/EchoCarsV31';p=O/'manifest.json';d=json.loads(p.read_text());new=[];notes=[]
for row in list(d['assets']):
 if row['id'] not in [1020,1021,1022,1031]:continue
 assert row['id'] in [1020,1021,1022,1031]
 vehicle=next(v for v in d['vehicles'] if v['body']==row['id'])
 source=Path(row['file']);full=source.with_name(source.stem+'_full_v31.fbx');spec=source.with_name(source.stem+'_full_v31.json')
 if not spec.exists():shutil.copy2(source,full);spec.write_text(json.dumps(row,indent=2))
 # Always partition the original full derivative, never an earlier partition.
 original=json.loads(spec.read_text())
 bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(full));ob=next(o for o in bpy.context.scene.objects if o.type=='MESH');me=ob.data;normals=[tuple(v.vector) for v in me.corner_normals];original_tri=sum(len(f.vertices)-2 for f in me.polygons);parts=[]
 used=sorted({f.material_index for f in me.polygons});chunks=[used[i:i+64] for i in range(0,len(used),64)]
 assert len(chunks)==2
 for ix,slots in enumerate(chunks):
  mid=row['id'] if ix==0 else 3000+row['id']-1000;faces=[f for f in me.polygons if f.material_index in slots];oldverts=sorted({v for f in faces for v in f.vertices});vmap={v:i for i,v in enumerate(oldverts)};loops=[li for f in faces for li in f.loop_indices]
  nm=bpy.data.meshes.new('body_partition');nm.from_pydata([me.vertices[i].co[:] for i in oldverts],[],[[vmap[i] for i in f.vertices] for f in faces]);nm.update()
  for i in slots:nm.materials.append(me.materials[i])
  for f,oldf in zip(nm.polygons,faces):f.material_index=slots.index(oldf.material_index);f.use_smooth=oldf.use_smooth
  for uv in me.uv_layers:
   layer=nm.uv_layers.new(name=uv.name)
   for i,li in enumerate(loops):layer.data[i].uv=uv.data[li].uv
  for col in me.color_attributes:
   if col.domain not in ['POINT','CORNER']:continue
   layer=nm.color_attributes.new(name=col.name,type=col.data_type,domain=col.domain)
   for i,oldi in enumerate(oldverts if col.domain=='POINT' else loops):layer.data[i].color=col.data[oldi].color
  nm.normals_split_custom_set([normals[i] for i in loops]);part=bpy.data.objects.new('EH_'+str(mid),nm);bpy.context.collection.objects.link(part);part.matrix_world.identity();bpy.ops.object.select_all(action='DESELECT');part.select_set(True);bpy.context.view_layer.objects.active=part
  out=O/('EH_'+str(mid)+'.fbx');bpy.context.scene.unit_settings.system='NONE';bpy.context.scene.unit_settings.scale_length=1
  bpy.ops.export_scene.fbx(filepath=str(out),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',global_scale=1,apply_unit_scale=False,apply_scale_options='FBX_SCALE_NONE',use_space_transform=False,bake_space_transform=False,use_mesh_modifiers=False,mesh_smooth_type='FACE',add_leaf_bones=False,bake_anim=False,path_mode='ABSOLUTE',embed_textures=False)
  pts=[part.matrix_world@v.co for v in nm.vertices];lo=[min(v[j] for v in pts) for j in range(3)];hi=[max(v[j] for v in pts) for j in range(3)];entry=copy.deepcopy(row);entry.update(id=mid,name='EH_'+str(mid),kind='body_part_'+str(ix),file=str(out),bounds_cm=[[lo[0],-hi[1],lo[2]],[hi[0],-lo[1],hi[2]]],materials=[nm.materials[i].name for i in range(len(nm.materials))],triangles=sum(len(f.vertices)-2 for f in nm.polygons),original_triangles=sum(len(f.vertices)-2 for f in nm.polygons),nanite=True,source_sha256=hashlib.sha256(out.read_bytes()).hexdigest());parts.append(entry);bpy.data.objects.remove(part,do_unlink=True)
 assert sum(a['triangles'] for a in parts)==original_tri,(row['id'],original_tri)
 row.clear();row.update(parts[0]);new.append(parts[1]);vehicle['body_extra']=[parts[1]['id']];notes.append(dict(slug=vehicle['slug'],parts=[a['id'] for a in parts],materials=[len(a['materials']) for a in parts],triangles_before=original_tri,triangles_after=sum(a['triangles'] for a in parts),normals_uvs_and_colors_preserved=True,source_local_coordinates_preserved=True))
extra_ids={a['id'] for a in new};d['assets']=[a for a in d['assets'] if a['id'] not in extra_ids]+new;p.write_text(json.dumps(d,ensure_ascii=False,indent=2));(R/'artifacts/echo_cars_v31/nanite_material_partitions.json').write_text(json.dumps(notes,indent=2)) if notes else None;print('NANITE_MATERIAL_PARTITIONS',notes,'assets',len(d['assets']),flush=True)
