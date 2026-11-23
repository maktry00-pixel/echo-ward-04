extends Area3D
## Bullet query only; these areas never obstruct movement or the camera.
var hunter: CharacterBody3D
var index: int = 0
var weakpoint: bool = false
var dead: bool:
	get: return hunter.dead
func is_head_hit(_at: Vector3) -> bool: return false
func receive_bullet(source: Vector3, _damage: int, _headshot: bool, _impact: Vector3 = Vector3.INF) -> bool:
	if weakpoint: return hunter.receive_weakspot(source)
	return hunter.receive_limbshot(source,index)
