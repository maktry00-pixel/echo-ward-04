extends Node3D

signal changed

const Builder = preload("res://scripts/world_builder.gd")
const Materials = preload("res://scripts/environment_materials.gd")
var is_open: bool = true
var busy: bool = false
var hinge: Node3D
var body: StaticBody3D
var closed_material: Material
var motion: Tween
var door_id: String = ""
var lock_kind: String = ""
var locked: bool = false
var initial_open: bool = true
var initial_locked: bool = false
var width: float = 2.4
var opened_once: bool = false
var breached: bool = false
var barricade_hp: int = 0
var barricade_side: int = 1
var barricade_mesh: Node3D


func _ready() -> void:
	var frame := Builder.material("313a38")
	closed_material = Materials.surface("concrete", "746957", 0.55)
	var half: float = width / 2.0
	for x: float in [-half - 0.12, half + 0.12]:
		Builder.box(self, Vector3(0.15, 3.15, 0.3), Vector3(x, 1.57, 0), frame)
	Builder.box(self, Vector3(width + 0.4, 0.16, 0.3), Vector3(0, 3.1, 0), frame)
	hinge = Node3D.new()
	hinge.position.x = -half
	add_child(hinge)
	body = Builder.box(hinge, Vector3(width, 2.95, 0.13), Vector3(half, 1.47, 0), closed_material, true) as StaticBody3D
	body.set_meta("impact_surface","wood")
	var handle := Builder.material("b0a68c")
	Builder.box(hinge, Vector3(0.22, 0.06, 0.08), Vector3(width - 0.27, 1.15, 0.12), handle)
	Builder.box(hinge, Vector3(0.6, 0.3, 0.02), Vector3(half, 2.3, 0.08), frame)
	if door_id=="quarantine_gate":
		var panel_at := Vector3(half+.38,1.4,.23)
		Builder.box(self,Vector3(.29,.44,.07),panel_at,frame)
		Builder.box(self,Vector3(.23,.08,.015),panel_at+Vector3(0,.13,.045),Builder.material("789b83",.2))
		for i: int in range(9):
			Builder.box(self,Vector3(.055,.055,.015),panel_at+Vector3((i%3-1)*.075,.035-(i/3)*.075,.045),handle)
	reset_door()


func toggle_door(hero_position: Vector3) -> bool:
	if barricade_hp>0: return false
	if breached: return false
	if busy or locked:
		return false
	if is_open:
		var local: Vector3 = to_local(hero_position)
		if (absf(local.z) < 0.8 and absf(local.x) < width / 2.0 + 0.4) or Vector2(local.x, local.z).length() < 1.45:
			return false
	busy = true
	is_open = not is_open
	if is_open:
		opened_once = true
	body.collision_layer = 0
	motion = create_tween()
	motion.tween_property(hinge, "rotation:y", -PI / 2.0 if is_open else 0.0, 0.45).set_trans(Tween.TRANS_SINE)
	motion.tween_callback(_finish_motion)
	return true


func _finish_motion() -> void:
	body.collision_layer = 0 if is_open else 1
	busy = false
	changed.emit()


func reset_door() -> void:
	set_barricade(0,1)
	breached = false
	hinge.show()
	for part: Node in get_children():
		if part.is_in_group("door_debris"): part.free()
	if motion != null and motion.is_valid():
		motion.kill()
	is_open = initial_open
	locked = initial_locked
	opened_once = false
	busy = false
	hinge.rotation.y = -PI / 2.0 if is_open else 0.0
	body.collision_layer = 0 if is_open else 1


func prompt() -> String:
	if barricade_hp>0: return "Укреплено [%d/3] · удерживай K — разобрать / выбить" % barricade_hp
	if breached: return "Разбитая дверь · проход свободен"
	if lock_kind=="guardian" and locked: return "Дверь заклинило · за ней что-то есть"
	if door_id=="quarantine_gate" and locked: return "F — кодовый замок карантина"
	if locked:
		match lock_kind:
			"power": return "F — проверить обесточенную дверь"
			"badge": return "F — приложить пропуск"
			"inside": return "F — проверить засов"
	return "F — закрыть дверь" if is_open else "F — открыть дверь"


func restore_state(open: bool, lock: bool) -> void:
	if motion != null and motion.is_valid():
		motion.kill()
	is_open = open
	locked = lock
	busy = false
	hinge.rotation.y = -PI / 2 if is_open else 0.0
	body.collision_layer = 0 if is_open else 1

func breach() -> void:
	set_barricade(0,1)
	if breached: return
	if motion!=null and motion.is_valid(): motion.kill()
	breached = true
	is_open = true
	locked = false
	busy = false
	opened_once = true
	body.collision_layer = 0
	hinge.hide()
	for i: int in range(5):
		var fragment: Node3D = Builder.box(self,Vector3(width/5-.04,1.0,.09),Vector3((i-2)*width/5,1.3,0),closed_material)
		fragment.add_to_group("door_debris")
		var tween := fragment.create_tween()
		tween.set_parallel(true)
		tween.tween_property(fragment,"position",Vector3((i-2)*.43,.07,.6+absf(i-2)*.2),.7)
		tween.tween_property(fragment,"rotation",Vector3(PI/2,0,(i-2)*.2),.7)
	changed.emit()

func set_barricade(hp: int, side: int) -> void:
	barricade_hp = clampi(hp,0,3)
	barricade_side = -1 if side<0 else 1
	if barricade_mesh!=null:
		barricade_mesh.free()
		barricade_mesh = null
	if barricade_hp>0: barricade_mesh = preload("res://scripts/door_barricade.gd").visual(self)

func damage_barricade() -> bool:
	if barricade_hp<=0: return false
	barricade_hp -= 1
	if barricade_hp<=0: set_barricade(0,barricade_side)
	elif barricade_mesh!=null:
		barricade_mesh.rotation.z += .025
	return true
