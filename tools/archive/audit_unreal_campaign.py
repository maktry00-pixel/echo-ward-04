"""Static export/parity audit. Runtime results are reported separately, not inferred."""
from pathlib import Path
import json,re
ROOT=Path(__file__).resolve().parents[1]
data=json.loads((ROOT/"unreal/EchoWard/RawAssets/Campaign/campaign.json").read_text(encoding="utf-8"))
keys={line.split("\t")[0] for line in data["locale_tsv"].splitlines()}
keys.update(line.split("\t")[0] for line in (ROOT/"unreal/EchoWard/Content/CampaignData/native_locale.tsv").read_text(encoding="utf-8").splitlines())
strings=set()
for source in (ROOT/"unreal/EchoWard/Source/EchoWard").glob("WardCampaign*.cpp"):
    strings.update(s for s in re.findall(r'TEXT\("([^"]*)"\)',source.read_text(encoding="utf-8-sig")) if re.search("[А-Яа-я]",s))
missing=sorted(strings-keys)
(ROOT/"artifacts/campaign_missing_locale.txt").write_text("\n".join(missing),encoding="utf-8")
ids={x["id"] for x in data["objects"]}
print(json.dumps({"rooms":len(data["rooms"]),"objects":len(ids),"enemies":len(data["enemies"]),"signs":len(data.get("signs",[])),"audio":len(data["audio"]),"untranslated_native_strings":len(missing),"agent_aliases":[x for x in data["agent_routes"]["light"] if x not in ids]},ensure_ascii=False))
