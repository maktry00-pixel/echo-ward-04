"""Validate evidence from the current Godot service-wing release."""
from pathlib import Path
import json
import re

ROOT = Path(__file__).resolve().parents[1]
ART = ROOT / 'artifacts'
suites = json.loads((ART / 'annex_regressions.json').read_text(encoding='utf-8'))
for name in suites:
    text = (ART / f'{name}_annex.log').read_text(encoding='utf-8')
    match = re.search(r'(\d+) checks, 0 failures', text)
    assert match and 'ERROR:' not in text, name
    assert suites[name] == {'exit': 0, 'checks': int(match[1]), 'failures': 0, 'error': False}, name
for name, path in [('annex', 'annex_test.log'), ('admin', 'admin_mode_annex.log')]:
    text = (ART / path).read_text(encoding='utf-8')
    match = re.search(r'(\d+) checks, 0 failures', text)
    assert match and 'ERROR:' not in text, name
    suites[name] = {'exit': 0, 'checks': int(match[1]), 'failures': 0, 'error': False}
campaign = json.loads((ART / 'annex_agent_playthrough.json').read_text(encoding='utf-8'))
log = (ART / 'annex_agent_live.log').read_text(encoding='utf-8')
assert campaign['won'] and campaign['annex']['complete'] and campaign['diagnostics'] and campaign['isolation']
assert campaign['steps'][-1] == 'exit' and campaign['records'] == ['shift','evidence','ceiling','farewell']
assert 'ERROR:' not in log and 'AUTOPILOT WAIT' not in log and 'RUNTIME AGENT COMPLETE' in log
assert 1000 < campaign['seconds'] < 1800
route = json.loads((ART / 'annex_route.json').read_text(encoding='utf-8'))
assert route['completed'] and route['annex']['complete']
assert 'ERROR:' not in (ART / 'annex_route.log').read_text(encoding='utf-8')
coverage = json.loads((ART / 'localization_coverage.json').read_text(encoding='utf-8'))
assert not coverage['untranslated'] and set(coverage['languages']) == {'ru','kk','en'}
previews = ['gallery','pump','boiler','archive','radio','gameplay','map','guide']
assert 'ANNEX PREVIEW COMPLETE' in (ART / 'annex_preview.log').read_text(encoding='utf-8')
for name in previews:
    assert (ART / f'annex_{name}.png').stat().st_size > 10000, name
baseline = json.loads((ART / 'admin_agent_playthrough.json').read_text(encoding='utf-8'))
steps = {name: float(time) for name, time in re.findall(r'AGENT STEP (\w+) at ([\d.]+) s', log)}
report = {
    'checks': sum(v['checks'] for v in suites.values()), 'failures': 0, 'suites': suites,
    'campaign': {k: campaign[k] for k in ['won','seconds','health','shots','status']},
    'step_timing': steps, 'old_agent_seconds': baseline['seconds'],
    'added_agent_seconds': round(campaign['seconds']-baseline['seconds'],1),
    'annex_walked_metres': round(campaign['annex']['distance'],1),
    'new_regions': 13, 'new_enemies': 4, 'new_save_stations': 2,
    'localization': coverage, 'preview_images': previews,
    'human_duration_target_minutes': 25, 'human_duration_measured': False,
    'user_save_files_touched': False,
    'limitations': ['Campaign test covers normal/light and deterministic seed 1907.',
                    'Agent knows the solutions; its simulated game time is not human playtest time.',
                    'Independent older balance probe adapted but not rerun in this release.',
                    'No new packaged export or FPS benchmark; existing START_GAME.cmd runs current sources.']
}
(ART / 'annex_verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps({k: report[k] for k in ['checks','failures','campaign','added_agent_seconds','annex_walked_metres']},ensure_ascii=False))
