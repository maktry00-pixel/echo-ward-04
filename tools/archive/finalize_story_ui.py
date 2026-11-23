"""Small idempotent UI-label updates for the two additional atlas sheets."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]/'unreal/EchoWard/Source/EchoWard'
p=root/'WardFieldMenuUI.cpp';s=p.read_text(encoding='utf-8')
s=s.replace('{TEXT("B2"),TEXT("Гелиос")}};', '{TEXT("B2"),TEXT("Гелиос")},{TEXT("DG2"),TEXT("Старый корпус 2")},{TEXT("UT"),TEXT("Тоннели")}};')
p.write_text(s,encoding='utf-8')
p=root/'WardBookMap.cpp';s=p.read_text(encoding='utf-8')
s=s.replace('Json Room=R->AsObject();const auto& A=Array(Room,TEXT("rect"));if(Q.X', 'Json Room=R->AsObject();if(Boolean(Room,TEXT("hidden"))&&Mode.IsValid()&&!Mode->Flag(TEXT("discovered_RH00")))continue;const auto& A=Array(Room,TEXT("rect"));if(Q.X')
p.write_text(s,encoding='utf-8')
