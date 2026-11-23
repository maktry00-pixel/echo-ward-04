"""Validate native optimization evidence and publish a reproducible comparison."""
from pathlib import Path
import hashlib
import json
import shutil

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / "unreal/EchoWard"
SAVED = PROJECT / "Saved/Prototype"
OUT = ROOT / "artifacts/unreal_optimization"


def read(name):
    return json.loads((SAVED / name).read_text(encoding="utf-8-sig"))


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    names = ["control1080", "balanced1080", "performance1080"]
    reports = {name: read("benchmark_" + name + ".json") for name in names}
    for name, report in reports.items():
        assert (report["width"], report["height"]) == (1920, 1080), name
        assert report["warmup_seconds_per_scene"] == 5 and report["sample_seconds_per_scene"] == 8
        assert "screenshot at 2s" in report["note"]
        assert [r["scene"] for r in report["scenarios"]] == ["ward", "furniture", "corridor"]
        for row in report["scenarios"]:
            assert row["frames"] > 100 and row["gpu_samples"] > 100
            assert row["gpu_ms"] > 0 and row["p95_ms"] >= row["median_ms"] > 0
        settings = report["render_settings"]
        assert settings["r.AntiAliasingMethod"] == 4
        assert settings["sg.GlobalIlluminationQuality"] == 2 and settings["sg.ReflectionQuality"] == 2
        assert settings["r.ScreenPercentage"] == (75 if name == "performance1080" else 100)
    assert reports["balanced1080"]["render_settings"] == reports["control1080"]["render_settings"]
    power_modes = {entry["BatteryStatus"] for report in reports.values() for key in ["power_before", "power_after"] for entry in report.get(key, [])}
    smoke = read("smoke_result.json")
    assert smoke["won"] and len(smoke["checks"]) >= 23 and all(c["passed"] for c in smoke["checks"])
    assets = json.loads((OUT / "experiment_nanite_assets.json").read_text(encoding="utf-8"))
    assert assets["success"] and len(assets["meshes"]) == 2
    assert all(r["nanite"] and r["keep_percent_triangles"] == 1 and r["trim_relative_error"] == 0 for r in assets["meshes"])
    assert digest(PROJECT / "Content/Prototype/Maps/Ward04.umap") == digest(OUT / "before/Ward04.umap")
    for name in ["SM_Bed", "SM_Lamp"]:
        assert digest(PROJECT / f"Content/Prototype/Meshes/{name}.uasset") == digest(OUT / f"before/{name}.uasset")
    OUT.mkdir(parents=True, exist_ok=True)
    for name in names:
        shutil.copy2(SAVED / ("benchmark_" + name + ".json"), OUT)
        shutil.copy2(SAVED / ("gpu_" + name + ".csv"), OUT)
        for scene in ["ward", "furniture"]:
            shutil.copy2(SAVED / f"benchmark_{name}_{scene}.png", OUT)
    for name in ["smoke_result.json", "benchmark_baseline1080.json", "benchmark_nanite1080.json"]:
        shutil.copy2(SAVED / name, OUT)
    rows = []
    for i, before in enumerate(reports["control1080"]["scenarios"]):
        after = reports["balanced1080"]["scenarios"][i]
        rows.append(dict(scene=before["scene"], before_fps=before["average_fps"],
                         balanced_fps=after["average_fps"],
                         fps_gain_percent=100*(after["average_fps"]/before["average_fps"]-1),
                         gpu_time_reduction_percent=100*(1-after["gpu_ms"]/before["gpu_ms"])))
    verification = dict(success=True, viewport=[1920, 1080], runtime_checks=len(smoke["checks"]),
                        map_unchanged=True, source_meshes_restored=True, nanite_experiment_accepted=False,
                        power_modes=sorted(power_modes), stable_frame_rate_proven=False, comparisons=rows,
                        scope="Three short idle views and physical room route, not a full campaign or a packaged build")
    (OUT / "verification.json").write_text(json.dumps(verification, indent=2), encoding="utf-8")
    lines = ["# Unreal: оптимизация текущей комнаты — 19 октября 2026", "",
             "Пользователь выбрал Unreal основным движком. Godot сохранён как исходная рабочая кампания. Повтор после подключения зарядки: [AC_REPORT.md](AC_REPORT.md). Прежняя запись BatteryStatus=1 сама по себе не доказывает отсутствие сетевого питания; в новом замере записан прямой ACLineStatus.", "",
             "## Что сделано", "",
             "- Проверен Nanite у кровати (49 990 треугольников) и лампы (24 102). В этой маленькой комнате прирост не подтверждён: эксперимент отменён, исходные uasset возвращены побайтово. Автоматический Setup его не включает. Карта, коллизии, материалы, текстуры и источники света сохранены.",
             "- Ящик и дверь обновляются только во время движения. После завершения анимации Tick отключается; повторное взаимодействие снова его включает.",
             "- Все 10 звуков предварительно загружаются до начала игры. Во время взаимодействий/попаданий используется сохранённая ссылка вместо синхронного LoadObject.",
             "- F4 переключает и сохраняет профиль. Качество: Epic, 100%; Баланс: High, 100%; Производительность: High, TSR 75%. Во всех профилях остаются Lumen и текстуры исходного разрешения. Уменьшение плотности VSM отдельно проверено, но выгода не подтверждена, поэтому стандартные тени возвращены.",
             "- Обычный запуск ограничен 60 FPS. На паузе показаны текущий профиль и подсказки F4/F3. Это временный доступ к графике до переноса полноценного меню.", "",
             "## Измерение", "",
             "Core 5 210H / RTX 5060 Laptop / 16 ГБ. Unreal 5.8.3, DX12, окно 1920×1080 с ForceRes. Без лимита FPS и VSync. Три фиксированных вида, 5 с прогрева и 8 с выборки каждый, AI в ожидании. PNG снимается на второй секунде, вне выборки. Одинаковая методика. Прежние отчёты 1600×900 с захватом внутри выборки сюда не подмешиваются.", "",
             "Во время работы обнаружено питание от батареи. Первый эксперимент с тенями дал нестабильные 34–46 FPS и исключён из сравнения (unstable_power_balanced1080.json сохранён); причина скачка не установлена. После него контроль повторён: power_before/power_after и частоты/мощность/температура GPU записываются для каждого запуска. Повтор теней дал 62–67 FPS против контроля 64–69: выгода не доказана, изменение отменено. Это последовательные короткие замеры, не лабораторная гарантия прироста. Первые результаты baseline1080 / nanite1080 сохранены как история эксперимента, без обещания статистической значимости.", "",
             "| Ракурс | Контроль High / 100% | Баланс / 100% | Производительность / 75% |", "|---|---:|---:|---:|"]
    for i, scene in enumerate(["Палата", "Мебель", "Коридор"]):
        vals = [f'{reports[n]["scenarios"][i]["average_fps"]:.1f}' for n in names]
        lines.append("| " + scene + " | " + " | ".join(vals) + " |")
    lines += ["", "Значения — средний FPS. GPU, медиана и p95 кадров, число выборок и фактические CVar сохранены в JSON рядом. Потоки CPU и GPU перекрываются, их времена нельзя складывать. Результаты коротких запусков зависят от питания, температуры и фоновых программ. Профиль 75% отдельно показывает эффект апскейлинга и не считается приростом при исходном разрешении.", "",
              "**Стабильные 60/90 FPS не доказаны.** При 100% заметного прироста FPS не подтверждено; сохранены качество и новые средства выбора графики. В двух первых видах TSR75 достигает примерно93FPS, но в последнем коридорном виде был скачок времени кадра: CSV фиксирует падение частоты GPU до180МГц и затем около1100МГц. Причина переключения частот не установлена; нельзя считать это ни доказанной проблемой игры, ни доказанной причиной в батарее. Для итоговой приёмки нужен повтор на подключённом питании без параллельной нагрузки и длительный проход. Показатель предзагрузки звуков проверяет наличие ссылок до взаимодействия, а не отсутствие всех возможных звуковых задержек.", "",
              f'Физический маршрут комнаты: проверок — **{len(smoke["checks"])}, все пройдены**, {smoke["seconds"]:.1f} с. Проверены стрельба, поиск, установка, дверь, выход, предварительная загрузка звуков, переходы графики и остановка Tick. Это автоматизированная проверка известного решения, а не длительность человеческого прохождения. Художественное качество анимаций этим тестом не доказывается.', "",
              'Дополнительно через окно игры проверены F4 (все три профиля), F3 на паузе, запись Preset=2 / масштаба75 в GameUserSettings.ini и восстановление режима Производительность после закрытия и нового запуска. В конце через F4 возвращён и сохранён Баланс (Preset=1). Для проверки окна использован computer-use; это отдельная ручная UI-проверка, не часть 23 автоматических проверок.', "",
              "## Запуск и повторение", "", "`PLAY_UNREAL.cmd` в корне — отдельное окно игры, `OPEN_UNREAL_EDITOR.cmd` — редактор. F4 — графика, F3 — FPS, Esc — пауза, F10 — выход. Профиль хранится через UGameUserSettings, секция EchoWard.Graphics в Saved/Config/WindowsEditor/GameUserSettings.ini. Выбор по умолчанию — Баланс.", "",
              "Из папки unreal/EchoWard:", "", "```powershell", ".\\Tools\\assessment.ps1 -Mode Balanced -Width 1920 -Height 1080 -Label balanced1080", ".\\Tools\\assessment.ps1 -Mode Performance -Width 1920 -Height 1080 -Label performance1080", ".\\Tools\\assessment.ps1 -Mode Smoke", "```", "",
              "Исходные карта/меши/код до этого этапа сохранены в before/. Меши в рабочей карте уже восстановлены. experiment_nanite.py требует явного флага -WardTestNanite и не вызывается обычной сборкой. Не запускать несколько GPU-тестов одновременно.", "",
              "## Следующий этап разработки", "", "Переносить полноценную главу в Unreal по частям: нормальные AnimBP/прицеливание и оружие → модульные палаты/коридоры и потоковая подгрузка → сохранения/меню → A*, CSP и Minimax по силабусу → измеренное прохождение. Сейчас в Unreal остаётся одна проверочная комната; большой Godot-кампании и её сохранений здесь пока нет. Производительность 25-минутной карты ещё не измерена.", "",
              "Основания настроек: [Nanite](https://dev.epicgames.com/documentation/en-us/unreal-engine/nanite-virtualized-geometry-in-unreal-engine), [Virtual Shadow Maps](https://dev.epicgames.com/documentation/en-us/unreal-engine/virtual-shadow-maps-in-unreal-engine), [Lumen Performance Guide](https://dev.epicgames.com/documentation/en-us/unreal-engine/lumen-performance-guide-for-unreal-engine)."]
    (OUT / "REPORT.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(json.dumps(verification, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
