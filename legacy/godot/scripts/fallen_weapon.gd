extends RigidBody3D
## Gameplay stays paused on defeat; only this presentation fall keeps advancing.
var presentation_velocity: Vector3 = Vector3(.5,.5,-.4)
var settled: bool = false
func _process(delta: float) -> void:
	if not get_tree().paused or settled: return
	freeze = true
	presentation_velocity.y -= delta*9.8
	var shape := SphereShape3D.new()
	shape.radius = .08
	var query := PhysicsShapeQueryParameters3D.new()
	query.shape = shape
	query.transform = Transform3D(Basis.IDENTITY,global_position)
	query.motion = presentation_velocity*delta
	query.collision_mask = 1
	query.exclude = [get_rid()]
	var result: PackedFloat32Array = get_world_3d().direct_space_state.cast_motion(query)
	var fraction: float = result[0] if result.size()==2 else 0
	global_position += query.motion*fraction
	if fraction<.99:
		settled = true
		rotation = Vector3(0,rotation.y,PI/2)
		var floor_query := PhysicsRayQueryParameters3D.create(global_position,global_position+Vector3.DOWN,1)
		floor_query.exclude = [get_rid()]
		var floor_hit: Dictionary = get_world_3d().direct_space_state.intersect_ray(floor_query)
		if not floor_hit.is_empty():
			var lowest: float = INF
			for mesh: MeshInstance3D in find_children("*","MeshInstance3D",true,false):
				lowest = minf(lowest,(mesh.global_transform*mesh.get_aabb()).position.y)
			global_position.y += floor_hit["position"].y+.003-lowest
	else: rotation += Vector3(2,3,1)*delta
