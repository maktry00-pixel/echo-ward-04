import bpy,json,argparse,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'assets/models/echo_custom'
ids=json.loads((OUT/'metal_retouch.json').read_text())['affected']
parser=argparse.ArgumentParser();parser.add_argument('--shard',type=int,default=0);parser.add_argument('--shards',type=int,default=1)
opt=parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
ids=ids[opt.shard::opt.shards]
for i in ids:
    d=OUT/f'EH_{i:03d}'
    bpy.ops.wm.open_mainfile(filepath=str(d/f'EH_{i:03d}.blend'))
    for image in bpy.data.images:
        if image.source=='FILE':image.reload()
    cp=bpy.context.preferences.addons['cycles'].preferences;cp.compute_device_type='OPTIX';cp.get_devices()
    for device in cp.devices:device.use=device.type=='OPTIX'
    bpy.context.scene.cycles.device='GPU';bpy.context.scene.render.filepath=str(d/'preview.png');bpy.ops.render.render(write_still=True)
    print('METAL_PREVIEW_COMPLETE',i,flush=True)
