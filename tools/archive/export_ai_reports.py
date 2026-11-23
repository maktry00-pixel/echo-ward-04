"""Regenerate the syllabus comparison tables from the actual Godot test output."""
import csv
import json
from pathlib import Path

folder = Path(__file__).resolve().parents[1] / 'artifacts'
data = json.loads((folder / 'ai_experiments.json').read_text(encoding='utf-8'))
for name in ('astar_bfs', 'minimax', 'csp'):
    rows = data[name]
    with (folder / (name + '_experiments.csv')).open('w', encoding='utf-8', newline='') as output:
        writer = csv.DictWriter(output, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    print(name, len(rows), 'rows')
