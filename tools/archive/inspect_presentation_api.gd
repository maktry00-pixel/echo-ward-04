extends SceneTree
func _initialize()->void:
	for kind:String in ["Viewport","Window","Environment"]:
		for item:Dictionary in ClassDB.class_get_property_list(kind):
			if item.name in ["scaling_3d_scale","content_scale_aspect","msaa_3d","screen_space_aa","positional_shadow_atlas_size","ambient_light_energy","fog_density"]:print(kind," ",item)
	quit()
