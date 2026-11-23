extends Node
## Text-only localization; narrative, labels, prompts and formatted HUD share a catalog.
signal language_changed(language:String)
var language:String="ru"
var entries:Dictionary={}
var folded:Dictionary={}
var fragments:Array[String]=[]
var patterns:Array[Dictionary]=[]
var targets:Array[WeakRef]=[]
var cache:Dictionary={}
func _ready() -> void:
	process_mode=Node.PROCESS_MODE_ALWAYS
	process_priority=1000
	_build_catalog()
	get_tree().node_added.connect(_track)
	for node:Node in get_tree().root.find_children("*","",true,false):_track(node)
func _build_catalog() -> void:
	for line:String in preload("res://assets/localization/catalog.gd").DATA.split("\n",false):
		var cells:PackedStringArray=line.split("\t")
		if cells.size()!=3:continue
		for i:int in range(3):cells[i]=cells[i].replace("\\n","\n")
		entries[cells[0]]={"en":cells[1],"kk":cells[2]}
		folded[cells[0].to_lower()]=cells[0]
		if cells[0].length()>=3 and not cells[0].contains("%"):
			fragments.append(cells[0])
		elif cells[0].contains("%"):
			var spec:=RegEx.new()
			spec.compile("%[0-9.]*[dfs]")
			var matches:Array[RegExMatch]=spec.search_all(cells[0])
			var expression:String=""
			var cursor:int=0
			for match:RegExMatch in matches:
				expression+=_escape(cells[0].substr(cursor,match.get_start()-cursor).replace("%%","%"))
				expression+="([+-]?[0-9]+(?:\\.[0-9]+)?)" if not match.get_string().ends_with("s") else "(.+?)"
				cursor=match.get_end()
			expression+=_escape(cells[0].substr(cursor).replace("%%","%"))
			if cells[0].ends_with("%s"):expression+="$"
			var regex:=RegEx.new()
			if regex.compile(expression)==OK:patterns.append({"source":cells[0],"regex":regex,"spec":spec})
	fragments.sort_custom(func(a:String,b:String)->bool:return a.length()>b.length())
	patterns.sort_custom(func(a:Dictionary,b:Dictionary)->bool:return a.source.length()>b.source.length())
func _escape(text:String) -> String:
	var out:String=""
	for ch:String in text:
		out+=("\\" if ch in ["\\",".","+","*","?","[","]","(",")","{","}","^","$","|"] else "")+ch
	return out
func set_language(value:String) -> void:
	if value not in ["ru","kk","en"]:value="ru"
	if value==language:return
	language=value
	cache.clear()
	TranslationServer.set_locale(value)
	language_changed.emit(value)
	refresh.call_deferred()
func translate(source:String,depth:int=0) -> String:
	if language=="ru" or source.is_empty() or depth>4:return source
	if cache.has(source):return cache[source]
	var result:String=source
	if entries.has(source):result=entries[source][language]
	elif folded.has(source.to_lower()):
		result=entries[folded[source.to_lower()]][language]
		if source==source.to_upper():result=result.to_upper()
	else:
		for pattern:Dictionary in patterns:
			var found:RegExMatch=pattern.regex.search(result)
			if found==null:continue
			var target:String=entries[pattern.source][language]
			var specs:Array[RegExMatch]=pattern.spec.search_all(target)
			var built:String=""
			var cursor:int=0
			for i:int in range(specs.size()):
				built+=target.substr(cursor,specs[i].get_start()-cursor)+translate(found.get_string(i+1),depth+1)
				cursor=specs[i].get_end()
			built+=target.substr(cursor)
			built=built.replace("%%","%")
			result=result.substr(0,found.get_start())+built+result.substr(found.get_end())
		for fragment:String in fragments:
			if result.contains(fragment):result=result.replace(fragment,entries[fragment][language])
	if cache.size()>4096:cache.clear()
	cache[source]=result
	return result
func _track(node:Node) -> void:
	if node is Label or node is Label3D or node is BaseButton or node is RichTextLabel:targets.append(weakref(node))
func _process(_delta:float) -> void:
	# Refresh after gameplay writes labels, before rendering: no source-language flicker.
	if language!="ru":refresh()
func refresh() -> void:
	var live:Array[WeakRef]=[]
	for ref:WeakRef in targets:
		var node:Node=ref.get_ref()
		if node==null:continue
		live.append(ref)
		if node.get_meta("loc_manual",false):continue
		if node is OptionButton:
			for i:int in range(node.item_count):
				var key:String="item_%d"%i
				var text:String=node.get_item_text(i)
				var original:String=_source(node,key,text)
				var localized:String=translate(original)
				node.set_item_text(i,localized)
				node.set_meta("loc_last_"+key,localized)
		elif node.get("text") is String:
			var source:String=_source(node,"text",node.text)
			var localized:String=translate(source)
			if node.text!=localized:node.text=localized
			node.set_meta("loc_last_text",localized)
		if node is Control and not node.tooltip_text.is_empty():
			var source:String=_source(node,"tooltip",node.tooltip_text)
			node.tooltip_text=translate(source)
			node.set_meta("loc_last_tooltip",node.tooltip_text)
	targets=live
func _source(node:Node,key:String,current:String) -> String:
	if not node.has_meta("loc_source_"+key) or current!=node.get_meta("loc_last_"+key,""):node.set_meta("loc_source_"+key,current)
	return node.get_meta("loc_source_"+key,current)
