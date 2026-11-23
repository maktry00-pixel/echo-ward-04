import bpy,bmesh,math,json
from pathlib import Path
from mathutils import Vector
R=Path('D:/Projects/ECHO');O=R/'artifacts/echo_hero_v38/character_candidate_v13';O.mkdir(exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(R/'artifacts/echo_hero_v38/character_candidate_v12/Beka_character_candidate_v12.blend'))
bpy.context.preferences.filepaths.save_version=0
pants=bpy.data.objects['Beka_Trousers'];head=next(o for o in bpy.context.scene.objects if 'likeness_candidate' in o.name)
bm=bmesh.new();bm.from_mesh(head.data)
bmesh.ops.delete(bm,geom=[v for v in bm.verts if v.co.z<1.475 or (v.co.z<1.535 and abs(v.co.x)>.050)],context='VERTS');bm.to_mesh(head.data);bm.free()
for layer in list(pants.data.uv_layers):pants.data.uv_layers.remove(layer)
uv=pants.data.uv_layers.new(name='TrouserCylindricalUV');uv.active_render=True
for p in pants.data.polygons:
 side=1 if p.center.x>=0 else -1
 vals=[]
 for i in p.vertices:
  co=pants.data.vertices[i].co;cx=side*(.12-.025*max(0,min(1,(co.z-.18)/.65)))
  angle=(math.atan2(co.y,co.x-cx)+math.pi)/(2*math.pi);vals.append(angle)
 if max(vals)-min(vals)>.5:vals=[u+1 if u<.5 else u for u in vals]
 for li,u in zip(p.loop_indices,vals):
  co=pants.data.vertices[pants.data.loops[li].vertex_index].co;uv.data[li].uv=(.012+(.5 if side==1 else 0)+u*.475,.025+(co.z-.10)/.86*.95)
m=bpy.data.materials.new('Repaired olive woven trousers');m.use_nodes=True;n=m.node_tree.nodes;l=m.node_tree.links;bs=n.get('Principled BSDF')
bs.inputs['Base Color'].default_value=(.028,.029,.021,1);bs.inputs['Roughness'].default_value=.88
tc=n.new('ShaderNodeTexCoord');noise=n.new('ShaderNodeTexNoise');noise.inputs['Scale'].default_value=400;noise.inputs['Detail'].default_value=2;l.new(tc.outputs['Generated'],noise.inputs['Vector'])
bump=n.new('ShaderNodeBump');bump.inputs['Distance'].default_value=.00012;bump.inputs['Strength'].default_value=.20;l.new(noise.outputs['Fac'],bump.inputs['Height']);l.new(bump.outputs['Normal'],bs.inputs['Normal'])
mix=n.new('ShaderNodeMixRGB');mix.inputs[1].default_value=(.024,.025,.018,1);mix.inputs[2].default_value=(.032,.033,.023,1);l.new(noise.outputs['Fac'],mix.inputs[0]);l.new(mix.outputs[0],bs.inputs['Base Color'])
pants.data.materials.clear();pants.data.materials.append(m)
for p in pants.data.polygons:p.material_index=0
bpy.ops.object.select_all(action='DESELECT');pants.select_set(True);bpy.context.view_layer.objects.active=pants
sc=bpy.context.scene;sc.cycles.samples=1
al=bpy.data.images.new('Trouser_albedo_v13',width=2048,height=2048,alpha=False);no=bpy.data.images.new('Trouser_normal_v13',width=2048,height=2048,alpha=False);no.colorspace_settings.name='Non-Color'
out=n.get('Material Output');em=n.new('ShaderNodeEmission');l.new(mix.outputs[0],em.inputs[0]);l.new(em.outputs[0],out.inputs[0]);ti=n.new('ShaderNodeTexImage');ti.image=al;n.active=ti
bpy.ops.object.bake(type='EMIT');al.filepath_raw=str(O/'Trouser_albedo_2048.png');al.file_format='PNG';al.save();al.pack()
l.new(bs.outputs[0],out.inputs[0]);n.remove(em);ni=n.new('ShaderNodeTexImage');ni.image=no;n.active=ni;bpy.ops.object.bake(type='NORMAL');no.filepath_raw=str(O/'Trouser_normal_2048.png');no.file_format='PNG';no.save();no.pack()
u=n.new('ShaderNodeUVMap');u.uv_map='TrouserCylindricalUV';ai=n.new('ShaderNodeTexImage');ai.image=al;l.new(u.outputs[0],ai.inputs[0]);l.new(ai.outputs[0],bs.inputs['Base Color']);l.new(u.outputs[0],ni.inputs[0]);nm=n.new('ShaderNodeNormalMap');l.new(ni.outputs[0],nm.inputs['Color']);l.new(nm.outputs[0],bs.inputs['Normal'])
# Cuffs must not share a material that was baked onto the trouser atlas in the previous study.
hood=bpy.data.objects['Beka_Hoodie']
for i,ma in enumerate(hood.data.materials):
 if 'rib' in ma.name.lower():
  new=bpy.data.materials.new('Separate black rib cuffs');new.use_nodes=True;p=new.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(.007,.008,.009,1);p.inputs['Roughness'].default_value=.9;hood.data.materials[i]=new
objects=[o for o in sc.objects if o.type=='MESH']
bpy.ops.object.select_all(action='DESELECT')
for o in objects:o.select_set(True)
bpy.context.view_layer.objects.active=head
bpy.ops.wm.save_as_mainfile(filepath=str(O/'Beka_character_candidate_v13.blend'))
bpy.ops.export_scene.gltf(filepath=str(O/'Beka_character_candidate_v13.glb'),use_selection=True,export_format='GLB',export_apply=True,export_animations=False)
sc.render.resolution_x=900;sc.render.resolution_y=1200;sc.cycles.samples=24
for name,pos,z,scale in [('front',(0,-3,1.0),.88,2.02),('three_quarter',(-2,-3,1.0),.88,2.02),('back',(0,3,1.0),.88,2.02),('face',(0,-1,1.62),1.62,.34)]:
 cam=sc.camera;cam.data.ortho_scale=scale;cam.location=pos;cam.rotation_euler=(Vector((0,0,z))-cam.location).to_track_quat('-Z','Y').to_euler();sc.render.filepath=str(O/(name+'.png'));bpy.ops.render.render(write_still=True)
(O/'status.json').write_text(json.dumps({'status':'candidate_not_approved','likeness_verified':False,'game_changed':False,'rigged':False,'repairs':['Cylindrical trouser UV instead of failed voxel smart projection','Separate cuff material','Removed residual neck shoulder surfaces']}),encoding='utf8')
print('V13_REPAIR_COMPLETE')
