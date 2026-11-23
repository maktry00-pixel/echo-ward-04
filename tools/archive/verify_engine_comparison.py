"""Verify the local native-room and staged-performance evidence, without rerunning it."""
import json
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ART = ROOT/'artifacts'
NATIVE = ROOT/'unreal/EchoWard/Saved/Prototype'
def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))
smoke = read(NATIVE/'smoke_result.json')
assert smoke['won'] and len(smoke['checks'])==17 and all(row['passed'] for row in smoke['checks'])
assert 'Result: Succeeded' in (NATIVE/'assessment_build.log').read_text(encoding='utf-8-sig')
assert read(NATIVE/'import_result.json')['success']
assert (ROOT/'unreal/EchoWard/Content/Prototype/Maps/Ward04.umap').stat().st_size>10000
reports = {'godot':read(ART/'comparison_godot.json')}
for profile in ['high','epic']:
    source=NATIVE/f'benchmark_{profile}.json'
    data=read(source)
    log=(NATIVE/f'assessment_{profile}.log').read_text(encoding='utf-8-sig')
    assert 'Chosen D3D12 Adapter Id = 0' in log and 'adapter 0: NVIDIA GeForce RTX 5060 Laptop GPU' in log
    assert 't.MaxFPS = "0"' in log and 'r.VSync = "0"' in log and 'r.ScreenPercentage = "100"' in log
    assert 'WARD_ASSESSMENT' in log
    for image in ['ward','furniture','corridor']:
        assert (NATIVE/f'benchmark_{profile}_{image}.png').stat().st_size>10000
    reports[f'unreal_{profile}']=data
    shutil.copy2(source,ART/f'comparison_unreal_{profile}.json')
shutil.copy2(NATIVE/'smoke_result.json',ART/'comparison_unreal_smoke.json')
shutil.copy2(NATIVE/'benchmark_high_ward.png',ART/'comparison_unreal_high.png')
shutil.copy2(NATIVE/'benchmark_epic_furniture.png',ART/'comparison_unreal_epic.png')
assert 'GODOT COMPARISON COMPLETE' in (ART/'comparison_godot.log').read_text(encoding='utf-8')
assert 'ERROR:' not in (ART/'comparison_godot.log').read_text(encoding='utf-8')
for name,data in reports.items():
    assert (data['width'],data['height'])==(1600,900)
    assert data['warmup_seconds_per_scene']==5 and data['sample_seconds_per_scene']==8
    assert len(data['scenarios'])==3
    for scene in data['scenarios']:
        assert scene['frames']>100 and 1<scene['mean_ms']<1000
        assert abs(scene['average_fps']-1000/scene['mean_ms'])<.01
        assert scene['p95_ms']>=scene['median_ms']>0
report={'native_checks':len(smoke['checks']),'native_failures':0,'native_room_won':smoke['won'],
        'measurements':reports,'sequential_gpu_runs':True,'packaged_build':False,
        'same_scene_comparison':False,'human_playtest':False,'full_campaign_ported':False,
        'limitations':['Different geometry, lighting and antialiasing; cannot rank engines by this FPS.',
                       'Staged idle/frozen AI; no full gameplay performance measurement.',
                       'Single brief run, no power/thermal stress test.',
                       'Native room uses ordinary character methods, not full keyboard-input automation.']}
(ART/'comparison_verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps({'native_checks':17,'failures':0,'fps':{name:[round(scene['average_fps'],1) for scene in data['scenarios']] for name,data in reports.items()}},ensure_ascii=False))
