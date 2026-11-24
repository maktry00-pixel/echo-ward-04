<p align="center">
  <img src="docs/media/exo-ward-04.png" alt="ЭХО · Отделение 04" width="860">
</p>

<h1 align="center">EXO · Отделение 04</h1>

<p align="center">
  <img src="unreal/EchoWard/Qaisar114/Studio.png" alt="Логотип QAISAR STUDIO" width="185">
</p>

<p align="center"><strong>QAISAR STUDIO · «Қайсар»</strong><br>Survival horror · Unreal Engine 5.8 · Бета 0.2.0-b117</p>

<p align="center">
    <a href="#скачать-игру">Скачать игру</a> ·
<a href="#об-игре">Об игре</a> ·
  <a href="#видео">Видео</a> ·
  <a href="#исходники">Исходники</a> ·
  <a href="#запуск-проекта-unreal-на-windows">Запуск</a> ·
  <a href="#демонстрация-ии">Демонстрация ИИ</a>
</p>

## Скачать игру

[Скачать EXO 0.2.0-b117 для Windows (ZIP, 4,93 ГБ)](https://drive.google.com/file/d/12pBU0-EM5sDaYZOcDBNvwihbRHvaVC0t/view?usp=sharing)

Распакуйте архив целиком и запустите `EXO.exe` из распакованной папки. Если Windows запросит Visual C++ x64, установите `Prerequisites\vc_redist.x64.exe`.

## Об игре

После ночного звонка Бека приезжает к больнице. На КПП он узнаёт о давней аварии в лаборатории. Чтобы выяснить, что произошло в Отделении 04, ему предстоит исследовать закрытые корпуса, сопоставлять улики и уходить от существ, которые идут на шум.

В проекте есть сюжетная кампания, кооператив и интерактивная демонстрация алгоритмов. Над игрой работает команда из четырёх человек. Демонстрация подготовлена для курса **QAIRU AI Fundamentals**.

## Видео

- [Трейлер EXO · Отделение 04](https://youtu.be/2jJua9lB9G8) — трейлер QAISAR STUDIO с обновлёнными голосами персонажей.
- [Обзор игры на казахском](https://youtu.be/DWqx8-J_PYg) — прохождение вступления и демонстрация игровых механик.

## Исходники

| Путь | Содержимое |
|---|---|
| [`unreal/EchoWard`](unreal/EchoWard) | Проект Unreal Engine 5.8, C++ кампании, кооператива и интерфейса, карта, настройки и данные игрового мира |
| `unreal/EchoWard/Content/Python` | Скрипты редактора Unreal для подготовки игрового контента |
| `legacy/godot` | Исходники прежней версии на Godot с её сценами, логикой, тестами и данными |
| [`qa/ai-demo-b117.json`](qa/ai-demo-b117.json) | Результат автоматической проверки демонстрации в собранной игре |

Для полной сборки игры нужны исходные материалы проекта Unreal. Они включают модели, текстуры, озвучку, музыку и анимации. Возьмите соответствующий комплект `Content` у команды и добавьте отсутствующие материалы в `unreal/EchoWard/Content`. При копировании сохраните файлы репозитория с настройками, кодом и данными. Большие медиа и готовая Windows-сборка хранятся отдельно.

## Запуск проекта Unreal на Windows

1. Установите Unreal Engine 5.8 и Visual Studio 2022 с компонентами C++ для Unreal Engine.
2. Выполните `powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\restore-world.ps1`. Скрипт распакует данные больницы и сверит SHA-256.
3. Добавьте комплект исходных материалов игры. Откройте `unreal/EchoWard/EchoWard.uproject`, соберите игровой модуль и нажмите **Play** на карте `Campaign`.

Для сборки модуля редактора из PowerShell укажите путь к установленному Unreal Engine.

```powershell
$ue = 'C:\Program Files\Epic Games\UE_5.8'
$project = (Resolve-Path -LiteralPath '.\unreal\EchoWard\EchoWard.uproject').Path
& "$ue\Engine\Build\BatchFiles\Build.bat" EchoWardEditor Win64 Development "-Project=$project" -WaitMutex
```

Скрипты в `tools/archive` и старые вспомогательные сценарии в `unreal/EchoWard/Tools` относятся к производству карты и ресурсов. Для их повторного запуска проверьте пути к установленным программам и исходным материалам.

## Демонстрация ИИ

В готовой игре откройте **Главное меню → Демонстрация ИИ**. В каждой вкладке можно изменить входные данные, повторить шаги алгоритма и нажать **Проверить результат**.

| Метод | Вход | Проверка |
|---|---|---|
| A* · неделя 2 | Старт, цель и состояние дверей | Поиск в ширину сверяет длину и доступность маршрута. При закрытии D2 обход занимает 13 шагов. |
| CSP · неделя 3 | Возможные позиции припасов и минимальная дистанция | Полный перебор проверяет 8 расстановок. При 4 м подходящих вариантов 0. |
| Minimax · неделя 4 | Позиции охотника и игрока, глубина поиска | Полный перебор и alpha-beta выбирают C4 с оценкой −58. Пройдено 697 и 158 узлов соответственно. |

<details>
<summary>Посмотреть проверку маршрута A* в игре</summary>

![Демонстрация ИИ в EXO. A* и BFS независимо находят обход закрытой двери за 13 шагов.](docs/media/ai-demo-astar.png)

</details>

Реализация алгоритмов и интерфейс проверки доступны в исходниках. [WardCampaignAI.cpp](unreal/EchoWard/Source/EchoWard/WardCampaignAI.cpp) · [WardAIDemo115.inl](unreal/EchoWard/Source/EchoWard/WardAIDemo115.inl)

Для автоматической проверки откройте актуальную собранную игру с аргументами `-WardCampaignSmoke -WardAboutMenuCheck -WardAIDemoCheck115 -EchoRelay=disabled`. Запуск создаёт `Saved/Prototype/ai_demo_check.json`. В b117 прошли все 29 проверок.
