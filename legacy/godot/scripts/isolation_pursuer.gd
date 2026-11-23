extends "res://scripts/stalker.gd"
## A single persistent actor; release never restores health or injured legs.
var chapter: Node3D
var breath_clock: float = 0
func _ready() -> void:
	max_health = 12
	use_minimax = true
	super._ready()
	var metal := preload("res://scripts/world_builder.gd").material("534c42")
	metal.metallic = .35
	# Original restraint collar and uneven hospital straps distinguish this actor.
	for i: int in range(4):
		var strap := preload("res://scripts/world_builder.gd").box(model,Vector3(.49,.055,.13),Vector3(0,.74+i*.23,.17),metal)
		strap.rotation.z = -.12+i*.055
func _physics_process(delta: float) -> void:
	if chapter==null or not chapter.released:
		if active and not dead: model.animate(0)
		return
	super._physics_process(delta)
	breath_clock -= delta
	if active and not dead and breath_clock<=0:
		var game: Node = get_tree().get_first_node_in_group("game")
		if position.distance_to(game.hero.position)<20: game.sound_bank.play_spatial("restraint_drag",position,.8)
		breath_clock = 4.6
