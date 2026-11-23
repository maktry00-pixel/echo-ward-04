extends RefCounted
## All exposed preferences have runtime consumers; saves keep campaign difficulty separately.
const DEFAULTS={"fullscreen":true,"effects":.8,"music":.7,"sensitivity":1.0,"camera_motion":true,
	"difficulty":"normal","language":"ru","texture_quality":"high","presentation_revision":2,
	"vsync":true,"max_fps":0,"msaa":2,"shadow_quality":"high","brightness":1.0,"contrast":1.0,
	"ambient":1.0,"fog":1.0,"flashlight_strength":1.0,"fov":60.0,"shake":1.0,"invert_y":false,
	"grain":true,"vignette":true,"show_noise":true,"objective_hint":true,"crosshair":true,
	"subtitles":true,"hud_opacity":1.0}
const RANGES={"effects":Vector2(0,1),"music":Vector2(0,1),"sensitivity":Vector2(.5,2),
	"brightness":Vector2(.65,1.35),"contrast":Vector2(.8,1.2),"ambient":Vector2(.6,1.4),
	"fog":Vector2(.5,1.4),"flashlight_strength":Vector2(.6,1.4),"fov":Vector2(55,85),
	"shake":Vector2(0,1),"hud_opacity":Vector2(.55,1)}
static func normalize(source:Dictionary)->Dictionary:
	var result:Dictionary=DEFAULTS.duplicate()
	for key:String in DEFAULTS:
		if DEFAULTS[key] is bool and source.get(key) is bool:result[key]=source[key]
	for key:String in RANGES:
		var n:Variant=source.get(key)
		if (n is int or n is float) and is_finite(float(n)):result[key]=clampf(float(n),RANGES[key].x,RANGES[key].y)
	for key:String in ["msaa","max_fps"]:
		var n:Variant=source.get(key)
		if (n is int or n is float) and is_finite(float(n)) and float(n)==floor(float(n)):
			if int(n) in ([0,1,2,3] if key=="msaa" else [0,30,60,90,120,144,165]):result[key]=int(n)
	for key:String in ["language","difficulty","texture_quality","shadow_quality"]:
		var allowed:Array=["ru","kk","en"] if key=="language" else (preload("res://scripts/difficulty_profile.gd").IDS if key=="difficulty" else ["high","standard"] if key=="texture_quality" else ["off","medium","high"])
		if source.get(key) is String and source[key] in allowed:result[key]=source[key]
	return result
