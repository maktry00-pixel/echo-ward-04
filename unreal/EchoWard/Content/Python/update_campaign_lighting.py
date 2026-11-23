"""Update only generated campaign lighting, with no geometry/material rebuild."""
import unreal as u

levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
if not levels.load_level('/Game/Campaign/Maps/Campaign'):
    raise RuntimeError('Campaign map is missing')
for actor in actors.get_all_level_actors():
    if isinstance(actor,u.PostProcessVolume):
        s=actor.get_editor_property('settings')
        s.set_editor_property('ambient_cubemap',u.load_asset('/Engine/MapTemplates/Sky/DaylightAmbientCubemap'))
        s.set_editor_property('override_ambient_cubemap_intensity',True)
        s.set_editor_property('ambient_cubemap_intensity',.65)
        s.set_editor_property('override_ambient_cubemap_tint',True)
        s.set_editor_property('ambient_cubemap_tint',u.LinearColor(.28,.36,.30,1))
        actor.set_editor_property('settings',s)
if not levels.save_current_level():
    raise RuntimeError('Could not save lighting; close the running game/editor')
u.log('CAMPAIGN_LIGHTING_SAVED')
