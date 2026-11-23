"""Create an outward-facing Nanite cube for the complete empty complex."""
import unreal as u
asset = '/Game/Campaign/BookWorld/SM_BookCube'
mesh = u.load_asset(asset)
if not mesh:
    u.EditorAssetLibrary.make_directory('/Game/Campaign/BookWorld')
    mesh = u.EditorAssetLibrary.duplicate_asset('/Engine/BasicShapes/Cube', asset)
if not mesh:
    raise RuntimeError('Could not copy the standard outward-facing cube')
subsystem = u.get_editor_subsystem(u.StaticMeshEditorSubsystem)
settings = subsystem.get_nanite_settings(mesh)
if not settings.get_editor_property('enabled'):
    settings.set_editor_property('enabled', True)
    subsystem.set_nanite_settings(mesh, settings, True)
if subsystem.get_simple_collision_count(mesh) == 0:
    subsystem.add_simple_collisions(mesh, u.ScriptCollisionShapeType.BOX)
if not u.EditorAssetLibrary.save_loaded_asset(mesh):
    raise RuntimeError('Could not save room-shell collision')
u.log('BOOK_WORLD_CUBE_READY Nanite=true simple_colliders=' + str(subsystem.get_simple_collision_count(mesh)))
