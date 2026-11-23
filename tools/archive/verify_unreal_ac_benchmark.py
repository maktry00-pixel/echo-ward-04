"""Publish the repeat measurement with direct Windows AC confirmation."""
from pathlib import Path
import json
import shutil

ROOT = Path(__file__).resolve().parents[1]
SAVED = ROOT / "unreal/EchoWard/Saved/Prototype"
OUT = ROOT / "artifacts/unreal_optimization"


def main():
    names = ["ac_balanced1080", "ac_performance1080"]
    data = {name: json.loads((SAVED / f"benchmark_{name}.json").read_text(encoding="utf-8-sig")) for name in names}
    for name, report in data.items():
        assert report["profile"] == name
        assert (report["width"], report["height"]) == (1920, 1080)
        for key in ["ac_power_before", "ac_power_after"]:
            assert report[key]["success"] and report[key]["ac_line_status"] == 1
        assert report["render_settings"]["r.ScreenPercentage"] == (100 if name == names[0] else 75)
        assert report["render_settings"]["sg.GlobalIlluminationQuality"] == 2
        assert report["render_settings"]["sg.ReflectionQuality"] == 2
        assert report["render_settings"]["r.Shadow.Virtual.ResolutionLodBiasLocal"] == 0
        assert [row["scene"] for row in report["scenarios"]] == ["ward", "furniture", "corridor"]
        assert report["warmup_seconds_per_scene"] == 5 and report["sample_seconds_per_scene"] == 8
        assert all(row["frames"] > 100 and row["gpu_samples"] > 100 and row["gpu_ms"] > 0 for row in report["scenarios"])
        shutil.copy2(SAVED / f"benchmark_{name}.json", OUT)
        shutil.copy2(SAVED / f"gpu_{name}.csv", OUT)
        for scene in ["ward", "corridor"]:
            shutil.copy2(SAVED / f"benchmark_{name}_{scene}.png", OUT)
    summary = dict(success=True, ac_before_after_verified=True, viewport=[1920, 1080],
                   source_code_changed=False, full_campaign_tested=False,
                   profiles={name: report["scenarios"] for name, report in data.items()})
    (OUT / "ac_verification.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
    lines = ["# Повторный замер Unreal на сетевом питании", "",
             "19 октября 2026. Пользователь подтвердил подключение зарядки. Перед и после обоих запусков прямой Win32 GetSystemPowerStatus вернул ACLineStatus=1 (питание онлайн). При первичной проверке после ответа пользователя WMI ещё показывал BatteryStatus=1, хотя прямой API уже видел AC; в самих тестах WMI вернул2. Поэтому один BatteryStatus больше не используется как доказательство отключённого адаптера.", "",
             "Исходный код игры и графические профили между предыдущим и этим замером не менялись. Добавлена только точная запись AC в PowerShell-обвязку и флаг RequireAC, который отклоняет тест без подтверждённого питания в начале/конце. Код игры не пересобирался; прежние 23 проверки маршрута относятся к той же сборке.", "",
             "RTX5060 Laptop, UE5.8.3/DX12. Реальный viewport1920×1080, без VSync и лимита FPS. Три неподвижных вида, 5с прогрева +8с выборки каждый; screenshot на2с вне выборки. Противник в ожидании. Запуски последовательные. Lumen High, исходные текстуры, штатные тени VSM. Баланс — 100%, Производительность — TSR75% (внутренний рендер1440×810 с восстановлением до1920×1080).", "",
             "| Ракурс | Баланс: средний FPS | p95, мс | TSR75: средний FPS | p95, мс |",
             "|---|---:|---:|---:|---:|"]
    for i, label in enumerate(["Палата", "Мебель", "Коридор"]):
        a, b = (data[name]["scenarios"][i] for name in names)
        lines.append(f'| {label} | {a["average_fps"]:.1f} | {a["p95_ms"]:.2f} | {b["average_fps"]:.1f} | {b["p95_ms"]:.2f} |')
    lines += ["", "Это измерение маленькой тестовой комнаты, не всей будущей главы, не боя с группой врагов и не packaged build. Средний FPS и p95 не доказывают отсутствие редких рывков. Разница с прежним запуском не считается результатом новой оптимизации кода: изменились условия питания и могли измениться частоты/фоновая нагрузка. Сырые JSON, GPU CSV и PNG сохранены рядом; прежние результаты оставлены для сравнения.", "",
              "Обычная игра ограничена60FPS. Запуск PLAY_UNREAL.cmd; F4 переключает профиль с сохранением выбора; F3 включает FPS. Рекомендуемый стартовый профиль — Баланс, затем проверка полного игрового маршрута по мере переноса главы.", "",
              "Повтор из unreal/EchoWard:", "", "```powershell",
              ".\\Tools\\assessment.ps1 -Mode Balanced -Width 1920 -Height 1080 -Label ac_balanced1080 -RequireAC",
              ".\\Tools\\assessment.ps1 -Mode Performance -Width 1920 -Height 1080 -Label ac_performance1080 -RequireAC",
              "```", "",
              "Значения ACLineStatus подтверждены [документацией Microsoft](https://learn.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-system_power_status)."]
    (OUT / "AC_REPORT.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
