extends RefCounted
const INK = Color("071012")
const PAPER = Color("dce5e2")
const MUTED = Color("8ea7a6")
const GOLD = Color("d5a653")
const TEAL = Color("58c1b6")
const CORAL = Color("ed684f")
const SANS = preload("res://assets/ui/fonts/NotoSans.ttf")
const SERIF = preload("res://assets/ui/fonts/Literata.ttf")
static func panel(alpha: float = .96) -> StyleBoxFlat:
	var style := StyleBoxFlat.new()
	style.bg_color = Color(INK,alpha)
	style.border_color = Color("263f43")
	style.set_border_width_all(1)
	style.set_corner_radius_all(5)
	style.set_content_margin_all(22)
	return style
static func button(caption: String, primary: bool = false) -> Button:
	var b := Button.new()
	b.text = caption
	b.custom_minimum_size = Vector2(0,44)
	b.alignment = HORIZONTAL_ALIGNMENT_LEFT
	b.add_theme_font_size_override("font_size",17)
	b.add_theme_font_override("font",SANS)
	b.add_theme_color_override("font_color",PAPER)
	b.add_theme_color_override("font_disabled_color",Color("748383"))
	var normal := panel(.92)
	normal.set_content_margin_all(12)
	normal.bg_color = CORAL if primary else Color("0c181b")
	normal.border_color = CORAL if primary else Color("263f43")
	if primary:
		for state:String in ["font_color","font_focus_color","font_hover_color","font_pressed_color"]:b.add_theme_color_override(state,INK)
	b.add_theme_stylebox_override("normal",normal)
	var hover: StyleBoxFlat = normal.duplicate()
	hover.bg_color = Color("ff8268") if primary else Color("153033")
	hover.border_color = CORAL if primary else TEAL
	b.add_theme_stylebox_override("hover",hover)
	b.add_theme_stylebox_override("focus",hover)
	var pressed: StyleBoxFlat = hover.duplicate()
	pressed.bg_color = Color("bd4f3d") if primary else Color("244549")
	b.add_theme_stylebox_override("pressed",pressed)
	b.add_theme_stylebox_override("disabled",normal)
	return b
static func label(caption: String, size: int = 17, color: Color = PAPER) -> Label:
	var l := Label.new()
	l.text = caption
	l.add_theme_font_size_override("font_size",size)
	l.add_theme_font_override("font",SANS)
	l.add_theme_color_override("font_color",color)
	return l
static func heading(caption:String,size:int=36)->Label:
	var l:=label(caption,size)
	l.add_theme_font_override("font",SERIF)
	return l
static func photo(id:String,height:float=260)->TextureRect:
	var picture:=TextureRect.new()
	picture.texture=load("res://assets/ui/guide/"+id+".jpg")
	picture.expand_mode=TextureRect.EXPAND_IGNORE_SIZE
	picture.stretch_mode=TextureRect.STRETCH_KEEP_ASPECT_COVERED
	picture.custom_minimum_size.y=height
	picture.size_flags_horizontal=Control.SIZE_EXPAND_FILL
	picture.mouse_filter=Control.MOUSE_FILTER_IGNORE
	return picture
