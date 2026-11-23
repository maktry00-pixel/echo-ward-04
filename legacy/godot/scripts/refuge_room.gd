extends Node3D
## An authored annex, with a hunter-only threshold and no free supply generation.
const Builder = preload("res://scripts/world_builder.gd")
const Props = preload("res://scripts/imported_props.gd")
const SPAWN := Vector3(8,.1,19.7)
const LIMITS := Rect2(4.4,16.15,8.8,6.95)
var game: Node3D
var inside: bool = false
var discovered: bool = false
var stash: Dictionary = {"ammo":0,"medkit":0,"battery":0,"bottle":0,"shells":0,"boards":0}
var screen: CanvasLayer
var stations: Dictionary = {}
var threshold: StaticBody3D
var door: Node3D

func build(facility: Node3D) -> void:
	var floor_mat: Material = preload("res://scripts/environment_materials.gd").pbr("old_linoleum_flooring_01","b2a78f",.45)
	Builder.box(self,Vector3(9.6,.4,7.5),Vector3(8.8,-.2,19.75),floor_mat,true)
	Builder.box(self,Vector3(9.6,.2,7.5),Vector3(8.8,3.7,19.75),facility.ceiling_material,true)
	for part: Array in [[Vector3(4.0,0,19.75),Vector3(.4,3.6,7.5)],[Vector3(13.6,0,19.75),Vector3(.4,3.6,7.5)],[Vector3(8.8,0,23.5),Vector3(9.6,3.6,.4)]]:
		facility._wall(part[0],part[1])
	door = facility._add_door("refuge_door",Vector3(8,0,16),0,"",2.8,true)
	threshold = Props.body(self,Vector3(2.8,3.2,.16),Vector3(8,1.6,16.14))
	threshold.collision_layer = 16
	threshold.collision_mask = 4
	threshold.remove_from_group("solid_furniture")
	Props.face(Props.visual(self,"painted_wooden_cabinet_02",Vector3(1.7,1.9,.6),Vector3(12.5,0,22.8)),Vector3(12.5,0,22.8),PI)
	Props.body(self,Vector3(1.7,1.9,.6),Vector3(12.5,.95,22.8))
	facility._table(Vector3(6,0,22.5),Vector3(2.4,.85,1),facility.metal_material)
	facility._chair(Vector3(6,0,21.6),facility.metal_material,floor_mat)
	facility._desk_lamp(Vector3(5.5,.92,22.5),-.7)
	Props.detail(self,"sofa_03",2.4,Vector3(4.8,0,18.7),Vector3(0,PI/2,0))
	Props.body(self,Vector3(.82,.7,2.4),Vector3(4.8,.35,18.7))
	facility._lamp(Vector3(8.8,3.35,20),Color("e2c396"),.42,8)
	var fill := OmniLight3D.new()
	fill.position = Vector3(8.8,2.5,20)
	fill.light_color = Color("e6ba83")
	fill.light_energy = .48
	fill.omni_range = 7
	add_child(fill)
	facility._sign("ДЕЖУРНАЯ / ТИХАЯ ЗОНА",Vector3(8,2.7,15.72),PI,20)
	facility._sign("ЗДЕСЬ МОЖНО ПЕРЕВЕСТИ ДУХ",Vector3(8.5,2.6,23.25),PI,18)
	# Personal belongings make this an occupied staff room rather than an empty annex.
	facility._table(Vector3(6.0,0,18.9),Vector3(.9,.48,.7),facility.metal_material)
	Props.detail(self,"medical_box",.20,Vector3(6,.56,18.85))
	Builder.box(self,Vector3(.14,.018,.18),Vector3(6.2,.54,19),Builder.material("b4a58a"))
	var glass: Material = Builder.material("526c75",.12)
	Builder.box(self,Vector3(.03,1.0,2.3),Vector3(13.36,2.35,19.5),glass)
	for z: float in [-1.2,0,1.2]: Builder.box(self,Vector3(.08,1.14,.04),Vector3(13.31,2.35,19.5+z),facility.metal_material)
	for y: float in [-.55,.55]: Builder.box(self,Vector3(.08,.04,2.44),Vector3(13.31,2.35+y,19.5),facility.metal_material)
	var moon := OmniLight3D.new()
	moon.position = Vector3(12.9,2.6,19.5)
	moon.light_color = Color("94b9ca")
	moon.light_energy = .35
	moon.omni_range = 5
	add_child(moon)
	_station("save",Vector3(5.7,.20,22.25))
	_station("stash",Vector3(12.5,.20,22.15))
	_station("records",Vector3(7.0,.20,22.25))
	var paper := Builder.material("d4c6a4")
	Builder.box(self,Vector3(.36,.025,.25),Vector3(5.7,.94,22.25),paper)
	Builder.box(self,Vector3(.34,.012,.24),Vector3(7,.94,22.25),paper)
	Props.detail(self,"portable_cassette_player",.22,Vector3(7.4,.95,22.6))

func _station(kind: String, at: Vector3) -> void:
	var point := preload("res://scripts/refuge_station.gd").new()
	point.kind = kind
	point.refuge = self
	point.position = at
	point.rotation.y = PI
	add_child(point)
	stations[kind] = point

func configure(owner_game: Node3D) -> void:
	game = owner_game
	screen = preload("res://scripts/refuge_screen.gd").new()
	screen.refuge = self
	game.add_child(screen)

func contains(at: Vector3) -> bool:
	return at.y>-.6 and at.y<2.8 and LIMITS.has_point(Vector2(at.x,at.z))

func update() -> void:
	var entered: bool = contains(game.hero.position)
	game.hero.in_refuge = entered
	if entered and not inside:
		discovered = true
		game.hud.announce("Дежурная. Здесь безопасно. Журнал на столе сохраняет игру, шкаф хранит припасы.",6)
	inside = entered

func transfer(kind: String, deposit: bool) -> bool:
	if not stash.has(kind) or not contains(game.hero.position): return false
	var amount: int = mini(24 if kind=="ammo" else 8 if kind=="shells" else (3 if kind=="bottle" else 1),_carried(kind) if deposit else int(stash[kind]))
	if amount<=0: return false
	if deposit:
		stash[kind] += amount
		_set_carried(kind,_carried(kind)-amount)
	else:
		if kind=="ammo":
			amount = mini(amount,maxi(0,72-game.combat.reserve))
			while amount>0 and not game.inventory.can_take(kind,amount,false): amount-=1
		elif kind=="bottle": amount = mini(amount,maxi(0,3-game.survival_tools.bottles))
		elif kind in ["shells","boards"]:
			amount = mini(amount,maxi(0,(18 if kind=="shells" else 3)-_carried(kind)))
			while amount>0 and not game.inventory.can_take(kind,amount,false): amount-=1
		if amount<=0 or not game.inventory.take_supply(kind,amount): return false
		stash[kind] -= amount
	game.sound_bank.play_cue("click")
	return true

func _carried(kind: String) -> int:
	match kind:
		"shells": return game.combat.shells
		"boards": return game.inventory.boards
		"ammo": return game.combat.reserve
		"bottle": return game.survival_tools.bottles
		"medkit": return game.inventory.medkits
		"battery": return game.inventory.batteries
	return 0
func _set_carried(kind: String, amount: int) -> void:
	match kind:
		"shells": game.combat.shells = amount
		"boards": game.inventory.boards = amount
		"ammo": game.combat.reserve = amount
		"bottle": game.survival_tools.bottles = amount
		"medkit": game.inventory.medkits = amount
		"battery": game.inventory.batteries = amount
func reset_room() -> void:
	inside = false
	discovered = false
	for kind: String in stash: stash[kind] = 0
	if screen!=null: screen.dismiss(false)
func snapshot() -> Dictionary: return {"discovered":discovered,"stash":stash.duplicate()}
func restore(data: Dictionary) -> void:
	discovered = data.get("discovered",false)
	for kind: String in stash: stash[kind] = int(data.get("stash",{}).get(kind,0))
static func valid(data: Variant) -> bool:
	if not data is Dictionary or not data.get("discovered") is bool or not data.get("stash") is Dictionary: return false
	if data.stash.size() not in [4,5,6]: return false
	for kind: Variant in data.stash:
		if kind not in ["ammo","medkit","battery","bottle","shells","boards"]: return false
	for kind: String in ["ammo","medkit","battery","bottle"]:
		if not preload("res://scripts/field_inventory.gd")._integer(data.stash.get(kind),0,216 if kind=="ammo" else 32): return false
	for kind: String in ["shells","boards"]:
		if data.stash.has(kind) and not preload("res://scripts/field_inventory.gd")._integer(data.stash[kind],0,64): return false
	return true
