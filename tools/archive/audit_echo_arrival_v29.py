"""Audit the corrected canon and preservation of the existing hospital."""
from pathlib import Path
import hashlib,json
R=Path(__file__).resolve().parents[1];P=R/'unreal/EchoWard';O=R/'artifacts/echo_arrival_v29'
read=lambda f:json.loads(f.read_text(encoding='utf-8-sig'))
sha=lambda f:hashlib.sha256(f.read_bytes()).hexdigest()
d=read(P/'Content/CampaignData/echo_arrival_v29.json');before=read(O/'before_identity.json');src=(P/'Source/EchoWard/WardHospitalArrival29.cpp').read_text(encoding='utf-8-sig');h=read(P/'Content/CampaignData/hospital_world.json')
checks={
'past_accident_is_explicit_and_not_retriggered':d['accident_timing']=='long_before_hero_arrival' and d['new_laboratory_accident_on_arrival'] is False and d['rules']['no_new_laboratory_accident'],
'original_562_rooms_35_sites_unchanged':sum(len(l['rooms']) for l in h['locations'])==562 and len(h['locations'])==35 and sha(P/'Content/CampaignData/hospital_world.json')==before['map_sha256'],
'previous_art_and_story_data_preserved':all(sha(P/'Content/CampaignData'/n)==v for n,v in before['current_data_hashes'].items()),
'first_chapter_monsters_remain_forbidden':d['rules']['monsters_from_chapter']==2 and d['rules']['no_new_monsters_in_prologue'],
'hero_past_accident_not_spoiled':d['rules']['do_not_reveal_hero_past_accident'],
'no_staged_new_lab_explosion_or_blackout':'IncidentClock' not in src and 'lab_accident29' not in src.replace('old_lab_accident29','') and 'door_break' not in src and 'LampPower' not in src,
'old_saves_skip_prologue':d['rules']['legacy_saves_skip_arrival'] and 'TEXT("stage"))?FMath' in src,
'car_checkpoint_before_entry':d['car_to'][1]<-50 and d['checkpoint'][1]<-50 and d['entry_rect'][1]>-26,
'russian_notice_uses_runtime_ui_font':'SetSlateWidget' in src and 'NotoSans.ttf' in src and 'ArrivalDisplay' not in src,
'notice_is_on_outer_face_of_guard_booth':d['panel'][1]<-56.65 and d['checkpoint'][1]<d['panel'][1] and 'LineTraceSingleByChannel' in src,
}
report={'success':all(checks.values()),'checks':checks,'data_sha256':sha(P/'Content/CampaignData/echo_arrival_v29.json')};(O/'acceptance.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8');print(json.dumps(report,ensure_ascii=False));assert report['success']
