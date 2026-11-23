extends Node
## Eight real slots. Documents are separate; essential tools cannot be discarded.
const CAPACITY: int = 8
var game: Node3D
var medkits: int = 0
var batteries: int = 0
var badge_examined: bool = false
var quarantine_code: bool = false
var supplies_installed: bool = false
var boards: int = 0
var screen: CanvasLayer

func configure(owner_game: Node3D) -> void:
	game = owner_game
	screen = preload("res://scripts/inventory_screen.gd").new()
	screen.inventory = self
	game.add_child(screen)

func entries(extra: String = "", amount: int = 0) -> Array[Dictionary]:
	var result: Array[Dictionary] = []
	if game.combat.equipped or extra=="weapon": result.append({"id":"weapon","name":"Служебный пистолет","count":1})
	if game.combat.shotgun_owned or extra=="shotgun": result.append({"id":"shotgun","name":"Помповый дробовик","count":1})
	var shells: int = game.combat.shells+(amount if extra=="shells" else 0)
	for i: int in range(ceili(float(shells)/8)): result.append({"id":"shells","name":"Патроны 12 калибра","count":mini(8,shells-i*8)})
	var wood: int = boards+(amount if extra=="boards" else 0)
	if wood>0: result.append({"id":"boards","name":"Доски для баррикад","count":wood})
	var reserve: int = game.combat.reserve+(amount if extra=="ammo" else 0)
	for i: int in range(ceili(float(reserve)/24)):
		result.append({"id":"ammo","name":"Патроны 9 мм","count":mini(24,reserve-i*24)})
	var bottles: int = game.survival_tools.bottles+(amount if extra=="bottle" else 0)
	if bottles>0: result.append({"id":"bottle","name":"Бутылки","count":bottles})
	for item: Array in [["medkit","Аптечка",medkits],["battery","Батарея вспышки",batteries]]:
		for i: int in range(int(item[2])+(amount if extra==item[0] else 0)):
			result.append({"id":item[0],"name":item[1],"count":1})
	var quest: Dictionary = {"fuse":game.mission.has_fuse,"badge":game.mission.has_badge,
		"fuel":game.mission.has_fuel and not game.mission.purge_started and not supplies_installed,
		"coolant":game.mission.has_coolant and not game.mission.purge_started and not supplies_installed,
		"residence_prybar":game.residential.has_prybar and not game.residential.complete,
		"residence_core":game.residential.core_extracted and not game.residential.complete,
		"isolation_coil":game.isolation.coil and not game.isolation.generator_started,
		"isolation_tag":game.isolation.tag and not game.isolation.log_taken,
		"annex_wheel":game.annex!=null and "wheel" in game.annex.steps and "pump" not in game.annex.steps}
	var names := {"fuse":"Предохранитель","badge":"Пропуск сотрудника","fuel":"Топливо","coolant":"Хладагент","residence_prybar":"Монтажный инструмент","residence_core":"Ремонтный модуль","isolation_coil":"Контактный блок","isolation_tag":"Браслет пациента17","annex_wheel":"Маховик насоса"}
	for id: String in quest:
		if quest[id] or extra==id: result.append({"id":id,"name":names[id],"count":1})
	return result

func can_take(kind: String, amount: int = 1, announce: bool = true) -> bool:
	var fits: bool = entries(kind,amount).size()<=CAPACITY
	if not fits and announce: game.hud.announce("Инвентарь заполнен (8 ячеек). I — применить или оставить припасы. Предмет остаётся здесь.",5)
	return fits

func take_supply(kind: String, amount: int = 1) -> bool:
	if (kind=="shells" and game.combat.shells+amount>18) or (kind=="boards" and boards+amount>3):
		game.hud.announce("Запас полон. Припасы остаются здесь.",3)
		return false
	if (kind=="ammo" and game.combat.reserve+amount>72) or (kind=="bottle" and game.survival_tools.bottles+amount>3):
		game.hud.announce("Запас этого предмета полон. Припасы остаются здесь.",3)
		return false
	if kind not in ["medkit","battery","ammo","bottle","shells","boards"] or amount<=0 or not can_take(kind,amount): return false
	match kind:
		"medkit": medkits += amount
		"battery": batteries += amount
		"ammo": game.combat.reserve += amount
		"bottle": game.survival_tools.bottles += amount
		"shells": game.combat.shells += amount
		"boards": boards += amount
	return true

func use_item(kind: String) -> bool:
	if game.has_lost or game.has_won or game.hero.hiding_bed!=null or game.hero.performing_action: return false
	if kind in ["weapon","shotgun"]: return game.combat.select_weapon("shotgun" if kind=="shotgun" else "pistol")
	if kind=="medkit" and medkits>0:
		if game.health>=3:
			game.hud.announce("Ран нет. Аптечка остаётся в инвентаре.",3)
			return false
		medkits -= 1
		game.health = mini(3,game.health+int(game.difficulty_value("heal")))
		game.hud.announce("Рана перевязана. −1 аптечка.",3)
	elif kind=="battery" and batteries>0:
		if game.flash_charges>=3:
			game.hud.announce("Вспышки заряжены. Батарея остаётся в инвентаре.",3)
			return false
		batteries -= 1
		game.flash_charges += 1
		game.hud.announce("Вспышка заряжена. −1 батарея.",3)
	else: return false
	game.sound_bank.play_cue("core",.8)
	return true

func drop_item(kind: String, requested: int = 0) -> bool:
	var amount: int = 1
	match kind:
		"shells":
			if game.combat.shells<=0: return false
			amount = mini(requested if requested>0 else 8,game.combat.shells)
			game.combat.shells -= amount
		"boards":
			if boards<=0: return false
			amount = boards
			boards = 0
		"medkit":
			if medkits<=0: return false
			medkits -= 1
		"battery":
			if batteries<=0: return false
			batteries -= 1
		"ammo":
			if game.combat.reserve<=0: return false
			amount = mini(requested if requested>0 else 24,game.combat.reserve)
			game.combat.reserve -= amount
		"bottle":
			if game.survival_tools.bottles<=0: return false
			amount = game.survival_tools.bottles
			game.survival_tools.bottles = 0
		_: return false
	var at: Vector3 = game.hero.global_position
	at.y = game.navigation.floor_height(at)+.035
	spawn_supply(kind,amount,at)
	game.hud.announce("Припасы оставлены у ног. Их можно подобрать снова клавишей F.",4)
	return true

func spawn_supply(kind: String, amount: int, at: Vector3) -> void:
	var item: Area3D = preload("res://scripts/dropped_supply.gd").new()
	item.inventory = self
	item.kind = kind
	item.amount = amount
	game.get_node("World").add_child(item)
	item.global_position = at

func reset_inventory() -> void:
	boards = 0
	medkits = 0
	batteries = 0
	badge_examined = false
	quarantine_code = false
	supplies_installed = false
	if screen!=null: screen.dismiss(false)
	for item: Node in get_tree().get_nodes_in_group("dropped_supplies"): item.free()

func snapshot() -> Dictionary:
	var drops: Array[Dictionary] = []
	for item: Node3D in get_tree().get_nodes_in_group("dropped_supplies"):
		drops.append({"kind":item.kind,"amount":item.amount,"at":[item.position.x,item.position.y,item.position.z]})
	return {"medkits":medkits,"batteries":batteries,"boards":boards,"examined":badge_examined,"code":quarantine_code,"installed":supplies_installed,"drops":drops}

func restore(data: Dictionary) -> void:
	medkits = int(data.get("medkits",0))
	boards = int(data.get("boards",0))
	batteries = int(data.get("batteries",0))
	badge_examined = data.get("examined",false)
	quarantine_code = data.get("code",false)
	supplies_installed = data.get("installed",false)
	for item: Dictionary in data.get("drops",[]): spawn_supply(item.kind,int(item.amount),Vector3(item.at[0],item.at[1],item.at[2]))

static func valid(data: Variant) -> bool:
	if not data is Dictionary: return false
	if data.has("boards") and not _integer(data.boards,0,3): return false
	for key: String in ["medkits","batteries"]:
		if not _integer(data.get(key),0,8): return false
	if not data.get("examined") is bool or not data.get("code") is bool: return false
	if data.has("installed") and not data.installed is bool: return false
	if not data.get("drops") is Array or data.drops.size()>32: return false
	for item: Variant in data.drops:
		if not item is Dictionary or item.get("kind") not in ["medkit","battery","ammo","bottle","shells","boards"]: return false
		if not _integer(item.get("amount"),1,24 if item.kind=="ammo" else 8 if item.kind=="shells" else (3 if item.kind in ["bottle","boards"] else 1)): return false
		if not item.get("at") is Array or item.at.size()!=3: return false
		for i: int in range(3):
			var value: Variant = item.at[i]
			if not (value is float or value is int) or not is_finite(float(value)): return false
		if item.at[0]<-30 or item.at[0]>104 or item.at[1]<-5.2 or item.at[1]>4 or item.at[2]<-192 or item.at[2]>82: return false
	return true

static func _integer(value: Variant, low: int, high: int) -> bool:
	return (value is int or value is float) and is_finite(float(value)) and float(value)==floor(float(value)) and value>=low and value<=high
