extends SceneTree
## Read-only snapshot of the complete authored Godot campaign for a native UE port.
## No Godot scene/script/save is changed. Runtime behavior is ported separately.
const DEST = "res://unreal/EchoWard/RawAssets/Campaign"
const AXIS = Basis(Vector3(0,1,0),Vector3(0,0,1),Vector3(-1,0,0))
var game:Node3D
var meshes:Dictionary={}
var materials:Dictionary={}
var textures:Dictionary={}
var instances:Array=[]
var collision:Array=[]
var objects:Array=[]
var lights:Array=[]
var signs:Array=[]
var owners:Dictionary={}
var mesh_cache:Dictionary={}
var unit_box:BoxMesh=BoxMesh.new()
var scanning_enemies:bool=false
var serial:int=0

func _initialize()->void: call_deferred("run")
func vec(v:Vector3)->Array: return [v.x,v.y,v.z]
func transform(t:Transform3D)->Dictionary:
	var b:Basis=AXIS*t.basis*AXIS.inverse()
	var scale:Vector3=b.get_scale()
	var rotation_basis:=Basis(b.x/(scale.x if absf(scale.x)>.000001 else 1),b.y/(scale.y if absf(scale.y)>.000001 else 1),b.z/(scale.z if absf(scale.z)>.000001 else 1))
	var q:Quaternion=rotation_basis.orthonormalized().get_rotation_quaternion()
	return {"position":vec(AXIS*t.origin*100),"rotation":[q.x,q.y,q.z,q.w],"scale":vec(scale)}
func clean(v:Variant,depth:int=0)->Variant:
	if depth>8:return null
	if v==null or v is bool or v is String or v is int:return v
	if v is float:return v if is_finite(v) else 0
	if v is Vector3:return vec(v)
	if v is Vector2:return [v.x,v.y]
	if v is Rect2:return [v.position.x,v.position.y,v.size.x,v.size.y]
	if v is Color:return [v.r,v.g,v.b,v.a]
	if v is Node:return {"node":str(game.get_path_to(v))}
	if v is Resource:return {"resource":v.resource_path}
	if v is Dictionary:
		var d:Dictionary={}
		for key:Variant in v:d[str(key)]=clean(v[key],depth+1)
		return d
	if v is Array or v is PackedStringArray or v is PackedInt32Array:
		var a:Array=[]
		for x:Variant in v:a.append(clean(x,depth+1))
		return a
	return null
func props(n:Object)->Dictionary:
	var d:Dictionary={}
	for p:Dictionary in n.get_property_list():
		if p.usage&PROPERTY_USAGE_SCRIPT_VARIABLE and p.name not in ["game","facility","chapter","screen","hud","motion","opening_tween","world"]:
			d[p.name]=clean(n.get(p.name))
	return d
func register(id:String,n:Node3D,kind:String)->void:
	if n==null:return
	owners[n.get_instance_id()]=id
	var s:Script=n.get_script()
	var row:Dictionary={"id":id,"path":str(game.get_path_to(n)),"kind":kind,"script":s.resource_path if s else "","transform":transform(n.global_transform),"properties":props(n)}
	if n.get("storage") is Node:
		row["storage"]=props(n.get("storage"))
	for key:String in ["hinge","drawer"]:
		if n.get(key) is Node3D:row[key+"_transform"]=transform(n.get(key).global_transform)
	if n.has_method("interaction_point"):row["stance"]=vec(AXIS*n.interaction_point()*100)
	else:row["stance"]=vec(AXIS*(n.global_position+n.global_basis*Vector3(0,.1,1.4))*100)
	objects.append(row)
func texture_id(tex:Texture2D)->String:
	if tex==null:return ""
	var source:String=tex.resource_path
	var id:String=source.sha256_text().substr(0,16) if not source.is_empty() else str(tex.get_instance_id())
	if textures.has(id):return id
	var existing:String="Textures/T_"+id+".png"
	if FileAccess.file_exists(DEST+"/"+existing):
		textures[id]={"id":id,"source":source,"file":existing,"width":tex.get_width(),"height":tex.get_height()}
		return id
	var image:Image=tex.get_image()
	if image==null:return ""
	if image.is_compressed():image.decompress()
	var path:String="Textures/T_"+id+".png"
	if image.save_png(DEST+"/"+path)!=OK:push_error("Texture export failed "+source)
	textures[id]={"id":id,"source":source,"file":path,"width":image.get_width(),"height":image.get_height()}
	return id
func material_id(mat:Material)->String:
	var d:Dictionary={"color":[.4,.4,.4,1],"roughness":.8,"metallic":0.,"uv":[1,1],"albedo":"","normal":"","roughness_texture":"","normal_scale":1.,"roughness_channel":0,"masked":false,"emission":[0,0,0],"emission_energy":0.,"organic":false,"wetness":.12}
	if mat is StandardMaterial3D:
		d.color=clean(mat.albedo_color);d.roughness=mat.roughness;d.metallic=mat.metallic
		d.uv=[mat.uv1_scale.x,mat.uv1_scale.y];d.albedo=texture_id(mat.albedo_texture)
		if mat.normal_enabled:d.normal=texture_id(mat.normal_texture);d.normal_scale=mat.normal_scale
		d.roughness_texture=texture_id(mat.roughness_texture);d.roughness_channel=mat.roughness_texture_channel
		d.masked=mat.transparency!=BaseMaterial3D.TRANSPARENCY_DISABLED
		if mat.emission_enabled:d.emission=clean(mat.emission);d.emission_energy=mat.emission_energy_multiplier
	elif mat is ShaderMaterial and mat.shader!=null and "organic_skin" in mat.shader.resource_path:
		d.organic=true
		var tissue:Variant=mat.get_shader_parameter("tissue_color")
		var wet:Variant=mat.get_shader_parameter("wetness")
		d.color=clean(tissue if tissue is Color else Color(.21,.15,.12,1))
		d.wetness=float(wet) if wet!=null else .12
		d.roughness=lerpf(.88,.44,d.wetness)
	elif mat is ShaderMaterial and mat.shader!=null and "wood_grain" in mat.shader.resource_path:
		d["wood"]=true;d.color=clean(mat.get_shader_parameter("wood_color"));d["grain"]=clean(mat.get_shader_parameter("grain_scale"));d.roughness=.7
	var id:String=JSON.stringify(d).sha256_text().substr(0,16)
	if not materials.has(id):d["id"]=id;materials[id]=d
	return id
func mesh_id(mesh:Mesh)->String:
	var cache:String=str(mesh.get_instance_id())
	if mesh_cache.has(cache):return mesh_cache[cache]
	var hash_context:=HashingContext.new();hash_context.start(HashingContext.HASH_SHA256)
	for si:int in range(mesh.get_surface_count()):
		var arrays:Array=mesh.surface_get_arrays(si)
		for channel:int in [Mesh.ARRAY_VERTEX,Mesh.ARRAY_NORMAL,Mesh.ARRAY_TEX_UV,Mesh.ARRAY_INDEX]:
			if arrays[channel]!=null:hash_context.update(arrays[channel].to_byte_array())
	var key:String=hash_context.finish().hex_encode().substr(0,16)
	mesh_cache[cache]=key
	if meshes.has(key):return key
	var file:String="Meshes/SM_"+key+".obj"
	if FileAccess.file_exists(DEST+"/"+file) and FileAccess.get_file_as_bytes(DEST+"/"+file).size()>100:
		var count:int=0
		for si:int in range(mesh.get_surface_count()):
			var a:Array=mesh.surface_get_arrays(si)
			count+=(a[Mesh.ARRAY_INDEX].size() if a[Mesh.ARRAY_INDEX]!=null and not a[Mesh.ARRAY_INDEX].is_empty() else a[Mesh.ARRAY_VERTEX].size())/3
		meshes[key]={"id":key,"file":file,"source":mesh.resource_path,"surfaces":mesh.get_surface_count(),"triangles":count}
		return key
	var out:FileAccess=FileAccess.open(DEST+"/"+file,FileAccess.WRITE)
	if out==null:push_error("Cannot create "+file);return ""
	var offset:int=1
	var triangles:int=0
	for si:int in range(mesh.get_surface_count()):
		if mesh is ArrayMesh and mesh.surface_get_primitive_type(si)!=Mesh.PRIMITIVE_TRIANGLES:continue
		var a:Array=mesh.surface_get_arrays(si)
		if a.is_empty():continue
		var vertices:PackedVector3Array=a[Mesh.ARRAY_VERTEX]
		var normals:PackedVector3Array=a[Mesh.ARRAY_NORMAL] if a[Mesh.ARRAY_NORMAL]!=null else PackedVector3Array()
		var uv:PackedVector2Array=a[Mesh.ARRAY_TEX_UV] if a[Mesh.ARRAY_TEX_UV]!=null else PackedVector2Array()
		var indices:PackedInt32Array=a[Mesh.ARRAY_INDEX] if a[Mesh.ARRAY_INDEX]!=null else PackedInt32Array()
		out.store_line("o surface_"+str(si)+"\nusemtl slot_"+str(si))
		for v:Vector3 in vertices:
			var p:Vector3=AXIS*v*100;out.store_line("v %.7f %.7f %.7f"%[p.x,p.y,p.z])
		for i:int in range(vertices.size()):
			var n:Vector3=AXIS*normals[i] if i<normals.size() else Vector3.UP
			out.store_line("vn %.7f %.7f %.7f"%[n.x,n.y,n.z])
		for i:int in range(vertices.size()):
			var t:Vector2=uv[i] if i<uv.size() else Vector2.ZERO;out.store_line("vt %.7f %.7f"%[t.x,1-t.y])
		var count:int=indices.size() if not indices.is_empty() else vertices.size()
		for i:int in range(0,count-2,3):
			var face:Array=[]
			for j:int in [0,2,1]:
				var vi:int=offset+(indices[i+j] if not indices.is_empty() else i+j);face.append("%d/%d/%d"%[vi,vi,vi])
			out.store_line("f "+" ".join(face));triangles+=1
		offset+=vertices.size()
	out.close()
	meshes[key]={"id":key,"file":file,"source":mesh.resource_path,"surfaces":mesh.get_surface_count(),"triangles":triangles}
	return key
func walk(n:Node,owner_id:String="")->void:
	if owners.has(n.get_instance_id()):owner_id=owners[n.get_instance_id()]
	if n==game.hero or (not scanning_enemies and n is CharacterBody3D and n in game.hunters):return
	if n is MeshInstance3D and n.mesh!=null and not n is Label3D:
		# Keep initially hidden movable contents; record the effective visibility.
		var mats:Array=[]
		for si:int in range(n.mesh.get_surface_count()):mats.append(material_id(n.get_active_material(si)))
		var source_mesh:Mesh=n.mesh
		var placement:Transform3D=n.global_transform
		if source_mesh is BoxMesh:
			placement=placement.scaled_local(source_mesh.size)
			source_mesh=unit_box
		instances.append({"path":str(game.get_path_to(n)),"owner":owner_id,"mesh":mesh_id(source_mesh),"materials":mats,"transform":transform(placement),"visible":n.is_visible_in_tree(),"shadow":n.cast_shadow!=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF})
	elif n is CollisionShape3D and n.shape!=null and n.get_parent() is CollisionObject3D:
		var s:Shape3D=n.shape
		var d:Dictionary={"path":str(game.get_path_to(n)),"owner":owner_id,"transform":transform(n.global_transform),"disabled":n.disabled,"layer":n.get_parent().collision_layer,"type":s.get_class()}
		if s is BoxShape3D:d["size"]=vec(s.size)
		elif s is CapsuleShape3D or s is CylinderShape3D:d["radius"]=s.radius;d["height"]=s.height
		elif s is SphereShape3D:d["radius"]=s.radius
		else:d["aabb"]=true
		collision.append(d)
	elif n is Label3D and owner_id.is_empty() and n.is_visible_in_tree() and n.billboard==BaseMaterial3D.BILLBOARD_DISABLED and not n.text.is_empty():
		signs.append({"path":str(game.get_path_to(n)),"text":n.text,"font_size":n.font_size,"pixel_size":n.pixel_size,"color":clean(n.modulate),"transform":transform(n.global_transform)})
	elif n is Light3D:
		lights.append({"path":str(game.get_path_to(n)),"owner":owner_id,"type":n.get_class(),"transform":transform(n.global_transform),"color":clean(n.light_color),"energy":n.light_energy,"range":n.omni_range if n is OmniLight3D else (n.spot_range if n is SpotLight3D else 10),"shadow":n.shadow_enabled,"visible":n.is_visible_in_tree()})
	for child:Node in n.get_children():walk(child,owner_id)
func run()->void:
	unit_box.size=Vector3.ONE
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(DEST+"/Meshes"))
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(DEST+"/Textures"))
	game=load("res://scenes/third_person.tscn").instantiate();game.set_meta("automated_test",true);root.add_child(game)
	for i:int in range(35):await process_frame
	game.apply_settings(game.normalise_settings({"texture_quality":"high","language":"ru"}),false)
	game.set_physics_process(false);game.hero.set_physics_process(false)
	for h:Node in game.hunters:h.set_physics_process(false)
	for id:String in game.mission.objects:register(id,game.mission.objects[id],"mission")
	for door:Node3D in game.facility.doors:register(door.door_id,door,"door")
	for pair:Array in [[game.annex,"annex_"],[game.isolation,"isolation_"],[game.diagnostics,"chapter_"],[game.upgrades,"upgrade_"],[game.refuge,"refuge_"]]:
		for id:String in pair[0].stations:register(pair[1]+id,pair[0].stations[id],"station")
	for i:int in range(game.cores.size()):register("record_"+str(i),game.cores[i],"record")
	for i:int in range(game.medkits.size()):register("field_medkit_"+str(i),game.medkits[i],"medkit")
	for i:int in range(game.batteries.size()):register("field_battery_"+str(i),game.batteries[i],"battery")
	var beds:Array=get_nodes_in_group("hide_beds")
	for i:int in range(beds.size()):register("bed_"+str(i),beds[i],"bed")
	for pair:Array in [[game.diagnostics,"chapter_"],[game.isolation,"isolation_"]]:
		for id:String in pair[0].partitions:register(pair[1]+id,pair[0].partitions[id],"partition")
	register("residence_lock",game.residential.lock_target,"partition")
	for i:int in range(game.saves.points.size()):register("save_"+str(i),game.saves.points[i],"save")
	walk(game.get_node("World"))
	var enemies:Array=[]
	scanning_enemies=true
	for i:int in range(game.hunters.size()):
		var h:Node3D=game.hunters[i]
		var rig:Dictionary={"properties":props(h.model),"bones":{}}
		for key:String in ["head","jaw","torso","limbs","legs","arms","sacs","limb_segments"]:
			var val:Variant=h.model.get(key)
			var nodes:Array=[]
			if val is Node3D:nodes.append(val)
			elif val is Array:
				for v:Variant in val:
					if v is Node3D:nodes.append(v)
					elif v is Array:
						for vv:Variant in v:
							if vv is Node3D:nodes.append(vv)
			for node:Node3D in nodes:rig.bones[str(game.get_path_to(node))]=transform(node.global_transform)
		enemies.append({"id":"hunter_"+str(i),"transform":transform(h.global_transform),"properties":props(h),"rig":rig})
		var was_visible:bool=h.visible;h.show();walk(h,"hunter_"+str(i));h.visible=was_visible
	var equipment:Dictionary={}
	game.combat.weapon.global_transform=Transform3D.IDENTITY
	game.combat.weapon.show()
	for id:String in ["pistol","shotgun"]:
		var gun:Node3D=game.combat.get(id);gun.show();walk(gun,"equipped_"+id);equipment[id]=props(gun)
	for id:String in ["ammo","shells","boards","battery","medkit","bottle"]:
		var supply:=Node3D.new();game.get_node("World").add_child(supply)
		preload("res://scripts/loot_visual.gd").build(supply,"ammo_dropped" if id=="ammo" else id,id);walk(supply,"loose_"+id)
	var data:Dictionary={"version":1,"units":"UE centimetres; (-GodotZ,GodotX,GodotY)","start":vec(AXIS*game.facility.START*100),"exit":vec(AXIS*game.facility.EXIT*100),"objects":objects,"enemies":enemies,"instances":instances,"collisions":collision,"lights":lights,"meshes":meshes.values(),"materials":materials.values(),"textures":textures.values(),"rooms":clean(game.exploration.ROOMS),"stories":clean(game.STORIES),"circuits":clean(game.mission.CIRCUITS),"annex_requirements":clean(game.annex.REQUIREMENTS),"annex_labels":clean(game.annex.LABELS),"difficulty":clean(preload("res://scripts/difficulty_profile.gd").VALUES),"settings":clean(preload("res://scripts/presentation_settings.gd").DEFAULTS),"training":clean(preload("res://scripts/field_guide.gd").TRAINING),"controls":clean(preload("res://scripts/field_guide.gd").CONTROLS),"about":clean(preload("res://scripts/field_guide.gd").ABOUT),"locale_tsv":preload("res://assets/localization/catalog.gd").DATA,"chapters":{}}
	for id:String in ["mission","residential","diagnostics","isolation","annex","upgrades","inventory","combat","survival_tools","refuge"]:data.chapters[id]=props(game.get(id))
	var bot:Node=preload("res://scripts/game_autopilot.gd").new();bot.configure(game)
	data["agent_routes"]={}
	var original_circuit:String=game.isolation.circuit
	for route:String in ["light","dark"]:
		game.isolation.circuit=route;bot._plan();data.agent_routes[route]=clean(bot.itinerary)
	game.isolation.circuit=original_circuit;bot.free()
	data["upgrade_recordings"]={}
	data["equipment"]=equipment
	for id:String in game.upgrades.RECORDS:data.upgrade_recordings[id]={"text":game.upgrades.RECORDS[id]}
	# Sample a structural navigation grid with dynamic gates removed; the native
	# A* reapplies their current blocking state. This affects only this export clone.
	for door:Node3D in game.facility.doors:door.restore_state(true,false)
	for id:String in ["ammo_morgue","ammo_lab","ammo_theatre"]:
		for body:Node in game.mission.objects[id].find_children("*","CollisionObject3D",true,false):body.collision_layer=0
	for id:int in [1,2]:
		for body:Node in game.medkits[id].find_children("*","CollisionObject3D",true,false):body.collision_layer=0
	for partition:Dictionary in [game.diagnostics.partitions,game.isolation.partitions]:
		for p:Node3D in partition.values():
			for body:Node in p.find_children("*","CollisionObject3D",true,false):body.collision_layer=0
	if game.residential.lock_target is CollisionObject3D:game.residential.lock_target.collision_layer=0
	for body:Node in game.residential.lock_target.find_children("*","CollisionObject3D",true,false):body.collision_layer=0
	await physics_frame
	game.navigation.build(game.get_world_3d().direct_space_state)
	var cells:Array=[]
	for cell:Vector2i in game.navigation.walkable:
		var point:=Vector3(cell.x,game.navigation.floor_height(Vector3(cell.x,0,cell.y)),cell.y)
		cells.append(vec(AXIS*point*100))
	data["navigation_cells"]=cells
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(DEST+"/Audio"))
	var bank:Node=preload("res://scripts/sound_bank.gd").new();root.add_child(bank)
	var audio:Array=[]
	for id:String in bank.sounds:
		var path:String="Audio/"+id+".wav"
		if not FileAccess.file_exists(DEST+"/"+path):bank.sounds[id].save_to_wav(DEST+"/"+path)
		audio.append({"id":id,"file":path,"loop":false})
	for id:String in ["ambience","pursuit","refuge_music"]:
		var stream:AudioStreamWAV=bank.get(id).stream
		var path:String="Audio/"+id+".wav"
		if not FileAccess.file_exists(DEST+"/"+path):stream.save_to_wav(DEST+"/"+path)
		audio.append({"id":id,"file":path,"loop":true})
	for id:String in ["shift","evidence","ceiling","farewell"]:
		var path:String="Audio/story_"+id+".wav"
		DirAccess.copy_absolute("res://assets/audio/stories/"+id+".wav",DEST+"/"+path)
		audio.append({"id":"story_"+id,"file":path,"loop":false})
	if FileAccess.file_exists(DEST+"/Audio/score.wav"):audio.append({"id":"score","file":"Audio/score.wav","loop":true})
	data["audio"]=audio
	data["signs"]=signs
	bank.queue_free()
	var file:FileAccess=FileAccess.open(DEST+"/campaign.json",FileAccess.WRITE);file.store_string(JSON.stringify(data,"\t"));file.close()
	print("CAMPAIGN_EXPORT_OK ",JSON.stringify({"objects":objects.size(),"enemies":enemies.size(),"instances":instances.size(),"collisions":collision.size(),"meshes":meshes.size(),"materials":materials.size(),"textures":textures.size(),"rooms":data.rooms.size()}))
	game.queue_free();await process_frame;quit()
