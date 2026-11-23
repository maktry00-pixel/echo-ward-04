extends RefCounted
## Additional sculpted anatomy; original geometry and materials, no humanoid asset swap.
const B = preload("res://scripts/world_builder.gd")
static func dress(actor: Node3D,skin: Material) -> void:
	var bone := B.material("b5aa8a")
	bone.roughness = .78
	var tissue := B.material("622e2d")
	tissue.roughness = .32
	var dark := B.material("1b1618")
	dark.roughness = .25
	var height: float = .72 if actor.crawler else 1.35
	# Asymmetric tissue, exposed tendon and overlapping organic armour.
	for side: float in [-1,1]:
		actor._form(actor.torso,Vector3(.15,.22,.30),Vector3(side*.26,height+.03,.12),skin,43+int(side))
		for i: int in range(4):
			actor._strand(actor.torso,Vector3(side*.09,height+.15-i*.07,-.20),Vector3(side*.30,height+.12-i*.07,-.02),.016,bone)
		actor._form(actor.head,Vector3(.07,.13,.13),Vector3(side*.19,-.04,-.14),skin,52)
		for i: int in range(4):
			actor._fang(actor.head,Vector3(side*(.045+i*.033),-.015,-.255),Vector3(side*(.04+i*.03),-.09,-.27),.012,bone)
		actor._strand(actor.head,Vector3(side*.16,.08,-.18),Vector3(side*.20,-.02,-.21),.014,dark)
	actor._form(actor.head,Vector3(.11,.08,.035),Vector3(0,-.10,-.256),tissue,21)
	for i: int in range(5):
		actor._form(actor.torso,Vector3(.07,.045,.065),Vector3(.018*sin(i),height+.28,.22+i*.09),bone,10+i)
	actor.set_meta("anatomy_detail",true)
