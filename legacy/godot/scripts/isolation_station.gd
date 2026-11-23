extends "res://scripts/chapter_station.gd"
## Cabinets have a visible opening step before their contents can be inspected.
var storage: Node3D
var opened: bool = false
var drawer: Node3D
var contents: Node3D
var motion: Tween
func use_station() -> void:
	if drawer!=null and not opened:
		opened = true
		if contents!=null: contents.visible = not chapter.done(station_id)
		motion = create_tween()
		motion.tween_property(drawer,"position:z",.60,.35)
		chapter.game.sound_bank.play_spatial("creak",global_position)
		chapter.game.hud.announce("Шкаф открыт. F — осмотреть содержимое.",3)
		return
	chapter.use_station(station_id)
func reset_station() -> void:
	if motion!=null and motion.is_valid(): motion.kill()
	opened = false
	used = false
	if drawer!=null: drawer.position.z = .32
	if contents!=null: contents.hide()
