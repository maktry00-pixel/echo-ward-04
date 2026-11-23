extends Node3D
## Visible finite projectile, blocked by real world geometry.
const Builder = preload("res://scripts/world_builder.gd")
var game: Node3D
var heading: Vector3
var remaining: float = 3.5
func _ready() -> void:
	Builder.sphere(self, 0.12, Vector3.ZERO, Builder.material("b1a55d", 1.1))
	var light := OmniLight3D.new()
	light.light_color = Color("b1a55d")
	light.light_energy = 0.5
	light.omni_range = 2
	add_child(light)
func _physics_process(delta: float) -> void:
	remaining -= delta
	var next: Vector3 = global_position + heading * delta * 9
	var ray := PhysicsRayQueryParameters3D.create(global_position, next, 3|16)
	var hit: Dictionary = get_world_3d().direct_space_state.intersect_ray(ray)
	if not hit.is_empty():
		game.feedback.burst(hit["position"], "a79a60")
		if hit["collider"] == game.hero and game.hero.invulnerable_remaining <= 0 and not game.has_lost:
			game.take_projectile_hit()
		queue_free()
		return
	global_position = next
	if remaining <= 0:
		queue_free()
