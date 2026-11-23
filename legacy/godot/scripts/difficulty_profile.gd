extends RefCounted
## A campaign choice. Normal preserves the original scarce-resource balance.
const IDS: Array[String] = ["story","normal","nightmare","hardcore"]
const NAMES := {"story":"Легко","normal":"Нормально","nightmare":"Сложно","hardcore":"Хардкор"}
const VALUES := {
	"story":{"speed":.82,"windup":1.25,"ammo":12,"shells":4,"invulnerability":2.8,"heal":2,"assist":.065,"escape":210.0},
	"normal":{"speed":1.0,"windup":1.0,"ammo":8,"shells":3,"invulnerability":1.8,"heal":1,"assist":0.0,"escape":180.0},
	"nightmare":{"speed":1.13,"windup":.85,"ammo":6,"shells":2,"invulnerability":1.35,"heal":1,"assist":0.0,"escape":165.0},
	"hardcore":{"speed":1.25,"windup":.72,"ammo":4,"shells":1,"invulnerability":1.05,"heal":1,"assist":0.0,"escape":150.0}}
static func value(id: String,key: String) -> Variant: return VALUES.get(id,VALUES.normal)[key]
static func description(id: String) -> String:
	return {"story":"Для первого знакомства: больше припасов и помощь при прицеливании.","normal":"Для знакомых с жанром: ограниченные припасы и ручной прицел.","nightmare":"Для опытных: меньше припасов, быстрее враги и опаснее атаки.","hardcore":"Для мастеров: минимум припасов, быстрые атаки и 150 секунд на финальный побег."}.get(id,"")
