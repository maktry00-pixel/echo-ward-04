extends RefCounted
## Short stance locks against the physical floor, released before a leg stretches.
var anchors: Array[Vector3] = [Vector3.INF,Vector3.INF]
var weights: Array[float] = [0.0,0.0]
var ages: Array[float] = [0.0,0.0]
var lock_events: int = 0
func reset() -> void:
	anchors.assign([Vector3.INF,Vector3.INF])
	weights.assign([0.0,0.0])
	ages.assign([0.0,0.0])
func apply(model: Node3D, delta: float) -> void:
	var hero: CharacterBody3D = model.get_parent()
	var rig: Skeleton3D = model.skeleton
	for i: int in range(2):
		var prefix: String = "Bip01 "+("R" if i==0 else "L")+" "
		var foot: int = rig.find_bone(prefix+"Foot")
		var pose: Transform3D = rig.global_transform*rig.get_bone_global_pose(foot)
		var ray := PhysicsRayQueryParameters3D.create(pose.origin+Vector3.UP*.25,pose.origin-Vector3.UP*.45,1)
		var hit: Dictionary = hero.get_world_3d().direct_space_state.intersect_ray(ray)
		var planted: bool = hero.is_on_floor() and hero.dash_remaining<=0 and not hit.is_empty() and hit.normal.y>.7
		if planted:
			var floor_y: float = hit.position.y+.075
			ages[i] += delta
			if anchors[i].is_finite() and (pose.origin.y>floor_y+.14 or ages[i]>.46 or pose.origin.distance_to(anchors[i])>.34): anchors[i] = Vector3.INF
			if not anchors[i].is_finite() and pose.origin.y<floor_y+.055:
				anchors[i] = Vector3(pose.origin.x,floor_y,pose.origin.z)
				ages[i] = 0
				lock_events += 1
		else: anchors[i] = Vector3.INF
		weights[i] = move_toward(weights[i],1.0 if anchors[i].is_finite() else 0.0,delta*18)
		if weights[i]>.01 and anchors[i].is_finite():
			var target: Vector3 = pose.origin.lerp(anchors[i],weights[i])
			model._pose_chain(rig.find_bone(prefix+"Thigh"),rig.find_bone(prefix+"Calf"),foot,target,-hero.model.global_basis.z)
			pose.origin = target
			rig.set_bone_global_pose_override(foot,rig.global_transform.affine_inverse()*pose,1,true)
