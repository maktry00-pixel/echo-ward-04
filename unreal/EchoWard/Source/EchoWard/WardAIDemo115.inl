#include "WardCampaignAI.h"
#include "WardTitleMenuStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Input/Reply.h"
#include "Rendering/DrawElements.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SLeafWidget.h"

namespace WardAIDemo115
{
constexpr int32 Width = 8, Height = 7;
const FLinearColor Paper(.87f, .88f, .85f), Faded(.49f, .55f, .59f), Copper(.82f, .43f, .25f), AuditInk(.42f, .76f, .68f);
const FLinearColor Tile(.034f, .059f, .069f), Wall(.011f, .018f, .025f), Rim(.063f, .103f, .119f);
const FLinearColor Route(.35f, .23f, .10f), Start(.07f, .30f, .26f), Goal(.36f, .052f, .063f);
const FLinearColor Frontier(.066f, .139f, .175f), Expanded(.086f, .217f, .249f);
const FSlateColorBrush Fill(FLinearColor::White);
const FSlateRoundedBoxBrush KeyShape(FLinearColor::White, 4.f);
const FSlateRoundedBoxBrush Frame(FLinearColor(.011f, .020f, .027f, .97f), 8.f, Rim, 1.f);
const FSlateRoundedBoxBrush OutcomeFrame(FLinearColor(.032f, .027f, .027f), 6.f, FLinearColor(.217f, .089f, .060f), 1.f);
const FButtonStyle KeyStyle = []
{
    FButtonStyle Style;
    Style.SetNormal(KeyShape);
    Style.SetHovered(FSlateRoundedBoxBrush(FLinearColor(1.34f, 1.30f, 1.26f), 4.f));
    Style.SetPressed(FSlateRoundedBoxBrush(FLinearColor(.78f, .81f, .82f), 4.f));
    Style.SetDisabled(KeyShape);
    return Style;
}();
const FProgressBarStyle BarStyle = []
{
    FProgressBarStyle Style;
    Style.SetBackgroundImage(FSlateRoundedBoxBrush(Wall, 2.f));
    Style.SetFillImage(FSlateRoundedBoxBrush(FLinearColor::White, 2.f));
    return Style;
}();

enum class ETopic { Path, Supplies, Hunt };

struct FVerification
{
    bool HasValidInput = true;
    bool Matches = false;
    int32 ReferenceSteps = INDEX_NONE;
    int32 Combinations = 0;
    int32 Allowed = 0;
    double PlacedMin = 0.0;
    double BestMin = 0.0;
    FIntPoint FullMove = FIntPoint::ZeroValue;
    float PrunedScore = 0.f;
    float FullScore = 0.f;
};

int32 ReferenceDistance(const FWardCampaignGrid& Grid, FIntPoint From, FIntPoint To)
{
    const auto Walkable = [&Grid](FIntPoint Cell) { return Grid.Ground.Contains(Cell) && !Grid.Blocked.Contains(Cell); };
    if (!Walkable(From) || !Walkable(To)) return INDEX_NONE;
    const FIntPoint Directions[] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
    TArray<FIntPoint> Queue{From};
    TMap<FIntPoint, int32> Distance{{From, 0}};
    for (int32 Read = 0; Read < Queue.Num(); ++Read)
    {
        const FIntPoint At = Queue[Read];
        const int32 Steps = Distance.FindChecked(At);
        if (At == To) return Steps;
        for (const FIntPoint Direction : Directions)
        {
            const FIntPoint Next = At + Direction;
            if (!Walkable(Next) || Distance.Contains(Next)) continue;
            Distance.Add(Next, Steps + 1);
            Queue.Add(Next);
        }
    }
    return INDEX_NONE;
}

double ClosestPair(FVector Medkit, FVector Ammo, FVector Battery)
{
    return FMath::Min3(FVector::Dist(Medkit, Ammo), FVector::Dist(Medkit, Battery), FVector::Dist(Ammo, Battery));
}

class SRouteArrow : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SRouteArrow) {} SLATE_ATTRIBUTE(FIntPoint, Direction) SLATE_END_ARGS()
    void Construct(const FArguments& Args)
    {
        Direction = Args._Direction;
        SetVisibility(EVisibility::HitTestInvisible);
        ForceVolatile(true);
    }
    FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }
    int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&, FSlateWindowElementList& Draw,
        int32 Layer, const FWidgetStyle&, bool) const override
    {
        const FIntPoint Offset = Direction.Get(FIntPoint::ZeroValue);
        if (Offset == FIntPoint::ZeroValue) return Layer;
        const FVector2D Axis(Offset.X, Offset.Y), Across(-Offset.Y, Offset.X);
        const FVector2D Mid = Geometry.GetLocalSize() / 2.0;
        const FVector2D Tip = Mid + Axis * 10.0;
        const FVector2D Chevrons = Tip - Axis * 7.0;
        const auto Area = Geometry.ToPaintGeometry();
        const TArray<FVector2D> Stem{Mid - Axis * 11.0, Tip};
        const TArray<FVector2D> Head{Chevrons + Across * 6.0, Tip, Chevrons - Across * 6.0};
        FSlateDrawElement::MakeLines(Draw, Layer, Area, Stem, ESlateDrawEffect::None, Paper, true, 2.f);
        FSlateDrawElement::MakeLines(Draw, Layer, Area, Head, ESlateDrawEffect::None, Paper, true, 2.f);
        return Layer;
    }

private:
    TAttribute<FIntPoint> Direction;
};

class SDemo : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SDemo) {} SLATE_ARGUMENT(TWeakObjectPtr<AEWardCampaignMode>, Game) SLATE_END_ARGS()

    void Construct(const FArguments& Args)
    {
        Game = Args._Game;
        Serif = FPaths::ProjectContentDir() / TEXT("CampaignData/Fonts/Literata.ttf");
        Sans = FPaths::ProjectContentDir() / TEXT("CampaignData/Fonts/NotoSans.ttf");
        Select(ETopic::Path);
    }

    bool SupportsKeyboardFocus() const override { return true; }
    FReply OnKeyDown(const FGeometry&, const FKeyEvent& Event) override
    {
        const FKey Key = Event.GetKey();
        if (Key == EKeys::Escape) Back();
        else if (Key == EKeys::Left) Scrub(Cursor - 1);
        else if (Key == EKeys::Right) Scrub(Cursor + 1);
        else return FReply::Unhandled();
        return FReply::Handled();
    }

    void Tick(const FGeometry& Geometry, double Now, float Delta) override
    {
        SCompoundWidget::Tick(Geometry, Now, Delta);
        if (!HasInitialFocus && FSlateApplication::IsInitialized())
        {
            HasInitialFocus = FSlateApplication::Get().SetKeyboardFocus(SharedThis(this), EFocusCause::SetDirectly);
        }
        if (Playing && Now >= NextTick)
        {
            SetStep(Cursor + 1);
            Playing = Cursor < Trace.Num();
            NextTick = Now + .56;
        }
    }

    bool Click(FName Id)
    {
        const TSharedPtr<SButton> Button = Controls.FindRef(Id).Pin();
        if (!Button || !Button->IsEnabled()) return false;
        Button->SimulateClick();
        return true;
    }

    ETopic Topic = ETopic::Path;
    FIntPoint From{1, 1}, To{6, 1}, Hunter{1, 3}, Runner{5, 3}, NextMove;
    bool UpperOpen = true, LowerOpen = true, Solved = false, SameMove = false;
    int32 Separation = 3, Depth = 2, FullNodes = 0, PrunedNodes = 0, Cutoffs = 0;
    FWardCampaignGrid Grid;
    TSet<FIntPoint> Path;
    TMap<FString, FVector> Placement;
    TArray<FWardAITraceEvent> Trace;
    int32 Cursor = 0;
    bool Playing = false;
    TSet<FIntPoint> Visited;
    FVerification Verification;
    bool ShowingVerification = false;

private:
    TWeakObjectPtr<AEWardCampaignMode> Game;
    TMap<FName, TWeakPtr<SButton>> Controls;
    TMap<FIntPoint, FString> Candidates;
    TMap<FIntPoint, FIntPoint> Arrows;
    TSet<FIntPoint> Open;
    TMap<FString, FIntPoint> TrialPlacement;
    FString Serif, Sans;
    bool PickFirst = false, HasInitialFocus = false;
    double NextTick = 0.0;

    FString Local(const TCHAR* Ru, const TCHAR* Kk, const TCHAR* En) const
    {
        return Game.IsValid() && Game->Language == TEXT("kk") ? Kk
            : Game.IsValid() && Game->Language == TEXT("en") ? En : Ru;
    }

    TSharedRef<STextBlock> Text(const FString& Value, int32 Size, FLinearColor Color = Paper, bool Heading = false) const
    {
        return SNew(STextBlock).Text(FText::FromString(Value)).Font(FSlateFontInfo(Heading ? Serif : Sans, Size))
            .ColorAndOpacity(Color);
    }

    FString TileName(FIntPoint Cell) const { return FString::Printf(TEXT("%c%d"), TCHAR('A' + Cell.X), Cell.Y + 1); }
    bool AtResult() const { return Cursor == Trace.Num(); }
    const FWardAITraceEvent* Current() const { return Cursor > 0 ? &Trace[Cursor - 1] : nullptr; }

    void SetStep(int32 Step)
    {
        Cursor = FMath::Clamp(Step, 0, Trace.Num());
        Open.Reset();
        Visited.Reset();
        TrialPlacement.Reset();
        for (int32 Index = 0; Index < Cursor; ++Index)
        {
            const auto& Event = Trace[Index];
            switch (Event.Kind)
            {
                case EWardAITraceKind::Expand: Open.Remove(Event.Cell); Visited.Add(Event.Cell); break;
                case EWardAITraceKind::Queue: Open.Add(Event.Cell); break;
                case EWardAITraceKind::PlaceItem: TrialPlacement.Add(Event.Item, Event.Cell); break;
                case EWardAITraceKind::Backtrack: TrialPlacement.Remove(Event.Item); break;
                default: break;
            }
        }
    }

    void Scrub(int32 Step) { Playing = false; SetStep(Step); }

    void ToggleReplay()
    {
        if (Trace.IsEmpty()) return;
        if (Playing) { Playing = false; return; }
        if (AtResult()) SetStep(0);
        Playing = true;
        NextTick = FSlateApplication::Get().GetCurrentTime() + .35;
    }

    void ResetTopic()
    {
        if (Topic == ETopic::Path) { From = {1, 1}; To = {6, 1}; UpperOpen = LowerOpen = true; }
        else if (Topic == ETopic::Supplies) Separation = 3;
        else { Hunter = {1, 3}; Runner = {5, 3}; Depth = 2; }
        PickFirst = false;
        Calculate();
    }

    TSharedRef<SButton> Key(FName Id, TSharedRef<SWidget> Content, TFunction<void()> Clicked,
        TAttribute<FSlateColor> Tint, FMargin Padding = FMargin(14, 9), TAttribute<bool> Enabled = true)
    {
        auto Control = SNew(SButton).ButtonStyle(&KeyStyle).ButtonColorAndOpacity(Tint).ContentPadding(Padding)
            .IsEnabled(Enabled).HAlign(HAlign_Center).VAlign(VAlign_Center).OnClicked_Lambda([Callback = MoveTemp(Clicked)]
            { Callback(); return FReply::Handled(); })[Content];
        Controls.Add(Id, Control);
        return Control;
    }

    void Back() const { if (Game.IsValid()) Game->Command(TEXT("back")); }

    void Select(ETopic NewTopic)
    {
        Topic = NewTopic;
        PickFirst = false;
        Calculate();
        Rebuild();
    }

    static bool IsWall(FIntPoint Cell) { return Cell.X == 3 && Cell.Y != 1 && Cell.Y != 5; }
    bool IsClosed(FIntPoint Cell) const { return Grid.Blocked.Contains(Cell); }

    void CheckRoute(const TArray<FVector>& Points)
    {
        Verification.ReferenceSteps = ReferenceDistance(Grid, From, To);
        if (Verification.ReferenceSteps == INDEX_NONE)
        {
            Verification.Matches = Points.IsEmpty();
            return;
        }
        FIntPoint Previous = From;
        bool Legal = true;
        for (const FVector& Point : Points)
        {
            const FIntPoint At = Grid.Cell(Point);
            const FIntPoint Delta = At - Previous;
            Legal &= Grid.Ground.Contains(At) && !IsClosed(At) && FMath::Abs(Delta.X) + FMath::Abs(Delta.Y) == 1;
            Previous = At;
        }
        Verification.Matches = Legal && Previous == To && Points.Num() == Verification.ReferenceSteps;
    }

    void CheckPlacement(const TMap<FString, TArray<FVector>>& Domains)
    {
        const auto& Medkit = Domains.FindChecked(TEXT("M"));
        const auto& Ammo = Domains.FindChecked(TEXT("A"));
        const auto& Battery = Domains.FindChecked(TEXT("B"));
        const double Required = Separation * 100.0;
        for (const FVector& M : Medkit)
            for (const FVector& A : Ammo)
                for (const FVector& B : Battery)
                {
                    const double Gap = ClosestPair(M, A, B);
                    ++Verification.Combinations;
                    if (Gap >= Required) ++Verification.Allowed;
                    Verification.BestMin = FMath::Max(Verification.BestMin, Gap);
                }
        if (!Solved)
        {
            Verification.Matches = Placement.IsEmpty() && Verification.Allowed == 0;
            return;
        }
        const auto* M = Placement.Find(TEXT("M"));
        const auto* A = Placement.Find(TEXT("A"));
        const auto* B = Placement.Find(TEXT("B"));
        if (!M || !A || !B || Placement.Num() != Domains.Num()) return;
        Verification.PlacedMin = ClosestPair(*M, *A, *B);
        Verification.Matches = Verification.Allowed > 0 && Medkit.Contains(*M) && Ammo.Contains(*A) && Battery.Contains(*B)
            && Verification.PlacedMin >= Required;
    }

    void Calculate()
    {
        Grid = FWardCampaignGrid{};
        Verification = FVerification{};
        Trace.Reset();
        for (int32 Y = 0; Y < Height; ++Y)
            for (int32 X = 0; X < Width; ++X)
                if (!IsWall({X, Y})) Grid.Ground.Add({X, Y}, 0.f);
        if (Topic == ETopic::Path)
        {
            if (!UpperOpen) Grid.Blocked.Add({3, 1});
            if (!LowerOpen) Grid.Blocked.Add({3, 5});
            Path.Reset();
            Arrows.Reset();
            Verification.HasValidInput = !IsClosed(From) && !IsClosed(To);
            if (Verification.HasValidInput)
            {
                const auto Points = Grid.FindPath(Grid.Point(From), Grid.Point(To), &Trace);
                CheckRoute(Points);
                for (int32 Index = 0; Index < Points.Num(); ++Index)
                {
                    const FIntPoint Cell = Grid.Cell(Points[Index]);
                    Path.Add(Cell);
                    if (Index + 1 < Points.Num()) Arrows.Add(Cell, Grid.Cell(Points[Index + 1]) - Cell);
                }
            }
        }
        else if (Topic == ETopic::Supplies)
        {
            const TMap<FString, TArray<FVector>> Domains = {
                {TEXT("M"), {Grid.Point({0, 0}), Grid.Point({0, 6})}},
                {TEXT("A"), {Grid.Point({7, 0}), Grid.Point({7, 6})}},
                {TEXT("B"), {Grid.Point({2, 3}), Grid.Point({5, 3})}}
            };
            Candidates.Reset();
            for (const auto& Item : Domains)
                for (const FVector& Point : Item.Value) Candidates.Add(Grid.Cell(Point), Item.Key);
            Solved = Grid.Allocate(Domains, Placement, Separation * 100.f, &Trace);
            CheckPlacement(Domains);
        }
        else
        {
            NextMove = Grid.Tactical(Hunter, Runner, Depth, true, &Trace);
            PrunedNodes = Grid.TacticalNodes;
            Cutoffs = Grid.TacticalPrunes;
            TArray<FWardAITraceEvent> Unpruned;
            Verification.FullMove = Grid.Tactical(Hunter, Runner, Depth, false, &Unpruned);
            SameMove = NextMove == Verification.FullMove;
            FullNodes = Grid.TacticalNodes;
            Verification.PrunedScore = Trace.Last().Value;
            Verification.FullScore = Unpruned.Last().Value;
            Verification.Matches = SameMove && FMath::IsNearlyEqual(Verification.PrunedScore, Verification.FullScore) && PrunedNodes <= FullNodes;
        }
        Scrub(Trace.Num());
    }

    void Pick(FIntPoint Cell)
    {
        if (IsWall(Cell) || IsClosed(Cell) || Topic == ETopic::Supplies) return;
        if (Topic == ETopic::Path) (PickFirst ? From : To) = Cell;
        else
        {
            if (Cell == (PickFirst ? Runner : Hunter)) return;
            (PickFirst ? Hunter : Runner) = Cell;
        }
        Calculate();
    }

    FString Mark(FIntPoint Cell) const
    {
        if (Topic == ETopic::Supplies) return Candidates.FindRef(Cell);
        if (Topic == ETopic::Path)
        {
            if (Cell == From && Cell == To) return TEXT("S/G");
            if (Cell == From) return TEXT("S");
            if (Cell == To) return TEXT("G");
            if (IsClosed(Cell)) return TEXT("×");
            if (AtResult() && Arrows.Contains(Cell)) return {};
            if (Visited.Contains(Cell)) return TEXT("·");
            if (Open.Contains(Cell)) return TEXT("+");
            if (Cell.X == 3 && !IsWall(Cell)) return TEXT("·");
            return {};
        }
        if (Cell == Hunter) return TEXT("H");
        if (Cell == Runner) return TEXT("P");
        return AtResult() && Cell == NextMove ? TEXT("+") : FString();
    }

    FLinearColor Color(FIntPoint Cell) const
    {
        if (IsWall(Cell)) return Wall;
        if (IsClosed(Cell)) return Goal;
        if (Topic == ETopic::Supplies)
        {
            const FString* Item = Candidates.Find(Cell);
            if (!Item) return Tile;
            if (!AtResult())
            {
                if (const auto* Event = Current(); Event && Event->Kind == EWardAITraceKind::RejectLocation && Event->Cell == Cell) return Goal;
                const FIntPoint* Trial = TrialPlacement.Find(*Item);
                return Trial && *Trial == Cell ? Start : Frontier;
            }
            const FVector* Where = Placement.Find(*Item);
            return Where && Grid.Cell(*Where) == Cell ? Start : Frontier;
        }
        if (Topic == ETopic::Hunt)
        {
            const auto* Event = Current();
            if (AtResult() ? Cell == NextMove : Event && Event->Cell == Cell) return Route;
            return Cell == Hunter ? Goal : Cell == Runner ? Start : Tile;
        }
        if (Cell == From) return Start;
        if (Cell == To) return Goal;
        if (AtResult() && Path.Contains(Cell)) return Route;
        if (!AtResult() && Current() && Current()->Cell == Cell) return Route;
        if (Visited.Contains(Cell)) return Expanded;
        if (Open.Contains(Cell)) return Frontier;
        return Cell.X == 3 ? Rim : Tile;
    }

    FString Outcome() const
    {
        if (Topic == ETopic::Path)
        {
            if (IsClosed(From) || IsClosed(To)) return Local(TEXT("Точка стоит на закрытой двери. Выберите открытую клетку."), TEXT("Нүкте жабық есікте тұр. Ашық ұяшықты таңдаңыз."), TEXT("An endpoint is on a closed door. Pick an open tile."));
            if (Path.IsEmpty() && From != To) return Local(TEXT("Маршрут недоступен. Откройте дверь или смените точку."), TEXT("Жол жоқ. Есікті ашыңыз немесе нүктені өзгертіңіз."), TEXT("No route. Open a door or move an endpoint."));
            return FString::Format(*Local(TEXT("Кратчайший маршрут — {0} шагов"), TEXT("Ең қысқа жол — {0} қадам"), TEXT("Shortest route — {0} steps")), {Path.Num()});
        }
        if (Topic == ETopic::Supplies) return Solved
            ? Local(TEXT("Все три припаса размещены"), TEXT("Үш зат та орналастырылды"), TEXT("All three supplies placed"))
            : Local(TEXT("При такой дистанции решения нет"), TEXT("Бұл арақашықтықта шешім жоқ"), TEXT("No valid placement at this distance"));
        if (NextMove == Hunter) return Local(TEXT("Решение охотника — ждать"), TEXT("Аңшының шешімі — күту"), TEXT("Hunter's decision — wait"));
        return FString::Format(*Local(TEXT("Ход охотника — клетка {0}"), TEXT("Аңшының жүрісі — {0} ұяшығы"), TEXT("Hunter moves to tile {0}")), {TileName(NextMove)});
    }

    FString Metrics() const
    {
        if (Topic == ETopic::Path) return FString::Format(*Local(TEXT("Раскрыто клеток  {0}"), TEXT("Ашылған ұяшықтар  {0}"), TEXT("Expanded tiles  {0}")), {Grid.Expanded});
        if (Topic == ETopic::Supplies) return FString::Format(*Local(TEXT("Назначения  {0}   ·   Проверки  {1}   ·   Возвраты  {2}"), TEXT("Тағайындау  {0}   ·   Тексеру  {1}   ·   Қайту  {2}"), TEXT("Assignments  {0}   ·   Checks  {1}   ·   Backtracks  {2}")),
            {Grid.Assignments, Grid.ConstraintChecks, Grid.Backtracks});
        const FString Agreement = SameMove ? Local(TEXT("да"), TEXT("иә"), TEXT("yes")) : Local(TEXT("нет"), TEXT("жоқ"), TEXT("no"));
        return FString::Format(*Local(TEXT("Отсечения  {0}   ·   Совпадение хода  {1}"), TEXT("Қысқартулар  {0}   ·   Жүрістің сәйкестігі  {1}"), TEXT("Cutoffs  {0}   ·   Same move  {1}")), {Cutoffs, Agreement});
    }

    FString VerificationInputs() const
    {
        if (Topic == ETopic::Path)
        {
            const auto Door = [this](bool Opened) { return Opened
                ? Local(TEXT("открыта"), TEXT("ашық"), TEXT("open")) : Local(TEXT("закрыта"), TEXT("жабық"), TEXT("closed")); };
            return FString::Format(*Local(TEXT("Старт {0}, цель {1}. Дверь D2 {2}, дверь D6 {3}."), TEXT("Бастау {0}, мақсат {1}. D2 есігі {2}, D6 есігі {3}."), TEXT("Start {0}, goal {1}. Door D2 is {2}, door D6 is {3}.")),
                {TileName(From), TileName(To), Door(UpperOpen), Door(LowerOpen)});
        }
        if (Topic == ETopic::Supplies)
            return FString::Format(*Local(TEXT("Минимум {0} м между предметами. У аптечки, патронов и батареи по два возможных места."), TEXT("Заттар арасы кемінде {0} м. Дәрі қобдишасына, оқ-дәріге және батареяға екі орыннан берілген."), TEXT("At least {0} m between items. The medkit, ammo and battery have two candidate locations each.")), {Separation});
        return FString::Format(*Local(TEXT("Охотник {0}, игрок {1}. Глубина поиска в парах ходов — {2}."), TEXT("Аңшы {0}, ойыншы {1}. Іздеу тереңдігі, жүріс жұбы — {2}."), TEXT("Hunter {0}, player {1}. Search depth in move pairs — {2}.")),
            {TileName(Hunter), TileName(Runner), Depth});
    }

    FString VerificationOutput() const
    {
        if (Topic == ETopic::Path) return Local(TEXT("A* ищет кратчайший путь по f = g + h."), TEXT("A* ең қысқа жолды f = g + h арқылы іздейді."), TEXT("A* searches for the shortest route using f = g + h.")) + TEXT("\n") + Outcome();
        if (Topic == ETopic::Supplies)
        {
            const auto* M = Placement.Find(TEXT("M"));
            const auto* A = Placement.Find(TEXT("A"));
            const auto* B = Placement.Find(TEXT("B"));
            if (!Solved || !M || !A || !B) return Local(TEXT("CSP подбирает места с заданной дистанцией."), TEXT("CSP берілген арақашықтыққа сай орын іздейді."), TEXT("CSP searches for placements at the required distance.")) + TEXT("\n") + Outcome();
            return FString::Format(*Local(TEXT("CSP подбирает по одному месту для каждого предмета. Аптечка {0}, патроны {1}, батарея {2}."), TEXT("CSP әр затқа бір орын таңдайды. Дәрі қобдишасы {0}, оқ-дәрі {1}, батарея {2}."), TEXT("CSP selects one location per item. Medkit {0}, ammo {1}, battery {2}.")),
                {TileName(Grid.Cell(*M)), TileName(Grid.Cell(*A)), TileName(Grid.Cell(*B))});
        }
        return Outcome() + TEXT("\n") + FString::Format(*Local(TEXT("Оценка хода {0}, просмотрено узлов {1}."), TEXT("Жүріс бағасы {0}, қаралған түйіндер {1}."), TEXT("Move score {0}, nodes visited {1}.")),
            {FMath::RoundToInt(Verification.PrunedScore), PrunedNodes});
    }

    FString VerificationEvidence() const
    {
        if (Topic == ETopic::Path)
        {
            if (!Verification.HasValidInput)
                return Local(TEXT("Одна из выбранных точек закрыта. Открой дверь или выбери другую клетку."), TEXT("Таңдалған нүктелердің бірі жабық. Есікті ашыңыз немесе басқа ұяшық таңдаңыз."), TEXT("An endpoint is blocked. Open the door or pick a different tile."));
            if (Verification.ReferenceSteps == INDEX_NONE)
                return Local(TEXT("Поиск в ширину (BFS) не нашёл прохода между этими точками."), TEXT("Ені бойынша іздеу (BFS) осы нүктелердің арасынан жол таппады."), TEXT("Breadth-first search (BFS) found no route between these tiles."));
            if (!Verification.Matches)
                return FString::Format(*Local(TEXT("BFS нашёл {0} шагов. Проверь длину и проходимость маршрута A*."), TEXT("BFS {0} қадам тапты. A* жолының ұзындығы мен өту мүмкіндігін тексеріңіз."), TEXT("BFS found {0} steps. Check the A* route length and passability.")), {Verification.ReferenceSteps});
            return FString::Format(*Local(TEXT("BFS независимо нашёл {0} шагов. Проверен каждый шаг A*. Все клетки проходимы, длина совпала."), TEXT("BFS бөлек есептеп, {0} қадам тапты. A* жолының әр қадамы тексерілді. Ұяшықтар ашық, ұзындығы сәйкес."), TEXT("BFS independently found {0} steps. Every step of the A* route is legal and the length matches.")), {Verification.ReferenceSteps});
        }
        if (Topic == ETopic::Supplies)
        {
            const FString Count = FString::Format(*Local(TEXT("Из {0} комбинаций полный перебор подтвердил {1}."), TEXT("Толық іздеу {0} нұсқаның {1} жарамды екенін растады."), TEXT("Exhaustive search found {1} valid placements out of {0}.")), {Verification.Combinations, Verification.Allowed});
            FString Distance = FString::Printf(TEXT("%.2f"), (Solved ? Verification.PlacedMin : Verification.BestMin) / 100.0);
            if (Game.IsValid() && Game->Language != TEXT("en")) Distance.ReplaceInline(TEXT("."), TEXT(","));
            return Count + TEXT("\n") + FString::Format(*(Solved
                ? Local(TEXT("В найденной расстановке ближайшая пара на расстоянии {0} м."), TEXT("Табылған нұсқадағы ең жақын жұптың арасы {0} м."), TEXT("The closest pair in the returned placement is {0} m apart."))
                : Local(TEXT("Лучший вариант даёт {0} м между ближайшими предметами."), TEXT("Ең жақсы нұсқада ең жақын заттардың арасы {0} м."), TEXT("Even the best placement has a closest pair only {0} m apart."))), {Distance});
        }
        return FString::Format(*Local(TEXT("Полный перебор выбрал {0}, оценка {1}. Просмотрено {2} узлов. Alpha-beta просмотрел {3}."), TEXT("Толық іздеу {0} жүрісін таңдады, бағасы {1}. {2} түйін қаралды. Alpha-beta {3} түйінді қарады."), TEXT("Full search chose {0}, score {1}, in {2} nodes. Alpha-beta visited {3}.")),
            {TileName(Verification.FullMove), FMath::RoundToInt(Verification.FullScore), FullNodes, PrunedNodes});
    }

    TSharedRef<SWidget> VerificationBlock(const FString& Heading, TFunction<FString()> Body) const
    {
        auto Paragraph = Text(FString(), 20);
        Paragraph->SetWrapTextAt(818.f);
        Paragraph->SetText(TAttribute<FText>::CreateLambda([Body = MoveTemp(Body)] { return FText::FromString(Body()); }));
        return SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()[Text(Heading, 17, AuditInk)]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 5, 0, 0)[Paragraph];
    }

    TSharedRef<SWidget> BuildVerification() const
    {
        auto State = Text(FString(), 21, Paper, true);
        State->SetText(TAttribute<FText>::CreateLambda([this]
        {
            return FText::FromString(!Verification.HasValidInput ? Local(TEXT("Проверь входные данные"), TEXT("Кіріс деректерін тексеріңіз"), TEXT("Review the inputs"))
                : Verification.Matches ? Local(TEXT("Результат подтверждён"), TEXT("Нәтиже расталды"), TEXT("Result verified"))
                : Local(TEXT("Результаты расходятся"), TEXT("Нәтижелер сәйкес емес"), TEXT("Results differ")));
        }));
        auto Content = SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 16)[Text(Local(TEXT("Проверка результата"), TEXT("Нәтижені тексеру"), TEXT("Verify the result")), 35, Paper, true)]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)[VerificationBlock(Local(TEXT("Входные данные"), TEXT("Кіріс деректері"), TEXT("Inputs")), [this] { return VerificationInputs(); })]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)[VerificationBlock(Local(TEXT("Результат алгоритма"), TEXT("Алгоритмнің нәтижесі"), TEXT("Algorithm output")), [this] { return VerificationOutput(); })]
            + SVerticalBox::Slot().AutoHeight()[VerificationBlock(Local(TEXT("Контрольный расчёт"), TEXT("Бақылау есебі"), TEXT("Reference check")), [this] { return VerificationEvidence(); })]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 0)
                [SNew(SBorder).BorderImage(&KeyShape).BorderBackgroundColor_Lambda([this] { return Verification.Matches ? Start : Goal; })
                    .Padding(FMargin(13, 9))[State]];
        return SNew(SBox).MaxDesiredHeight(554)
            [SNew(SScrollBox) + SScrollBox::Slot()[Content]];
    }

    FString StepDescription() const
    {
        const auto* Event = Current();
        if (!Event)
        {
            return Trace.IsEmpty()
                ? Local(TEXT("Выберите доступную клетку, чтобы начать поиск."), TEXT("Іздеуді бастау үшін ашық ұяшықты таңдаңыз."), TEXT("Pick a free tile to start the search."))
                : Local(TEXT("Нажмите «Пуск» или перейдите к следующему шагу."), TEXT("«Бастау» түймесін басыңыз немесе келесі қадамға өтіңіз."), TEXT("Press Play or go to the next step."));
        }
        const FString Cell = TileName(Event->Cell);
        switch (Event->Kind)
        {
            case EWardAITraceKind::Expand:
                return FString::Format(*Local(TEXT("Раскрываем {0}   ·   g = {1}   ·   h = {2}   ·   f = {3}"), TEXT("{0} ұяшығын ашамыз   ·   g = {1}   ·   h = {2}   ·   f = {3}"), TEXT("Expand {0}   ·   g = {1}   ·   h = {2}   ·   f = {3}")), {Cell, Event->Count, Event->Estimate, Event->Count + Event->Estimate});
            case EWardAITraceKind::Queue:
                if (Event->Cell == Event->Other)
                    return FString::Format(*Local(TEXT("Кладём старт {0} в очередь   ·   f = {1}"), TEXT("Бастау {0} кезекке қосылды   ·   f = {1}"), TEXT("Queue the start tile {0}   ·   f = {1}")), {Cell, Event->Estimate});
                return FString::Format(*Local(TEXT("Сосед {0} добавлен в очередь из {1}   ·   f = {2}"), TEXT("{1} көршісі {0} кезекке қосылды   ·   f = {2}"), TEXT("Queue neighbor {0} from {1}   ·   f = {2}")), {Cell, TileName(Event->Other), Event->Count + Event->Estimate});
            case EWardAITraceKind::ChooseItem:
                return FString::Format(*Local(TEXT("Выбираем {0}   ·   доступных мест: {1}"), TEXT("{0} заты таңдалды   ·   жарамды орындар: {1}"), TEXT("Choose {0}   ·   legal locations: {1}")), {Event->Item, Event->Count});
            case EWardAITraceKind::RejectLocation:
                return FString::Format(*Local(TEXT("{0} в {1} отклонён: до {2} всего {3} м, минимум {4} м."), TEXT("{1} орнындағы {0} жарамсыз: {2} нүктесіне дейін {3} м, кемінде {4} м керек."), TEXT("Reject {0} at {1}: only {3} m from {2}, minimum {4} m.")),
                    {Event->Item, Cell, TileName(Event->Other), FString::Printf(TEXT("%.2f"), Event->Value / 100.f), Event->Count / 100});
            case EWardAITraceKind::PlaceItem:
                return FString::Format(*Local(TEXT("Пробуем поставить {0} в {1}."), TEXT("{0} затын {1} ұяшығына қойып көреміз."), TEXT("Try placing {0} at {1}.")), {Event->Item, Cell});
            case EWardAITraceKind::Backtrack:
                return FString::Format(*Local(TEXT("Откат: снимаем {0} с {1} и ищем другой вариант."), TEXT("Кері қадам: {0} затын {1} ұяшығынан алып, басқа нұсқаны іздейміз."), TEXT("Backtrack: remove {0} from {1} and try another option.")), {Event->Item, Cell});
            case EWardAITraceKind::ScoreMove:
                return FString::Format(*Local(TEXT("Ход {0}   ·   оценка {1}   ·   просмотрено {2}, отсечений {3}"), TEXT("{0} жүрісі   ·   баға {1}   ·   қаралған түйіндер {2}, қысқартулар {3}"), TEXT("Move {0}   ·   score {1}   ·   explored {2}, cutoffs {3}")),
                    {Cell, (Event->Bound ? FString(TEXT("<= ")) : FString()) + FString::FromInt(FMath::RoundToInt(Event->Value)), Event->Count, Event->Estimate});
            default: return Outcome();
        }
    }

    TSharedRef<SWidget> Player()
    {
        auto Row = SNew(SHorizontalBox);
        Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 22, 0)
            [Text(Local(TEXT("Ход алгоритма"), TEXT("Алгоритм қадамдары"), TEXT("Execution trace")), 25, Paper, true)];
        const TAttribute<bool> Available = TAttribute<bool>::CreateLambda([this] { return !Trace.IsEmpty(); });
        auto First = Key(TEXT("start"), Text(TEXT("|‹"), 19), [this] { Scrub(0); }, FSlateColor(Tile), FMargin(12, 6), Available);
        First->SetToolTipText(FText::FromString(Local(TEXT("В начало"), TEXT("Басына"), TEXT("Go to start"))));
        Row->AddSlot().AutoWidth().Padding(0, 0, 6, 0)[First];
        Row->AddSlot().AutoWidth().Padding(0, 0, 6, 0)[Key(TEXT("previous"), Text(TEXT("‹"), 22), [this] { Scrub(Cursor - 1); }, FSlateColor(Tile), FMargin(12, 6), Available)];
        auto PlayText = Text(FString(), 18);
        PlayText->SetText(TAttribute<FText>::CreateLambda([this] { return FText::FromString(Playing
            ? Local(TEXT("Пауза"), TEXT("Кідіріс"), TEXT("Pause")) : Local(TEXT("Пуск"), TEXT("Бастау"), TEXT("Play"))); }));
        Row->AddSlot().AutoWidth().Padding(0, 0, 6, 0)
            [SNew(SBox).WidthOverride(108)[Key(TEXT("play"), PlayText, [this] { ToggleReplay(); }, FSlateColor(Goal), FMargin(10, 6), Available)]];
        Row->AddSlot().AutoWidth().Padding(0, 0, 6, 0)[Key(TEXT("next"), Text(TEXT("›"), 22), [this] { Scrub(Cursor + 1); }, FSlateColor(Tile), FMargin(12, 6), Available)];
        Row->AddSlot().AutoWidth().Padding(0, 0, 6, 0)[Key(TEXT("end"), Text(TEXT("›|"), 19), [this] { Scrub(Trace.Num()); }, FSlateColor(Tile), FMargin(12, 6), Available)];
        Row->AddSlot().FillWidth(1).VAlign(VAlign_Center).Padding(17, 0)
            [SNew(SSlider).IsEnabled(Available).SliderBarColor(Rim).SliderHandleColor(Copper)
                .Value_Lambda([this] { return Trace.IsEmpty() ? 0.f : float(Cursor) / Trace.Num(); })
                .OnValueChanged_Lambda([this](float Value) { Scrub(FMath::RoundToInt(Value * Trace.Num())); })];
        auto CountText = Text(FString(), 18, Copper);
        CountText->SetText(TAttribute<FText>::CreateLambda([this] { return FText::FromString(FString::Printf(TEXT("%d / %d"), Cursor, Trace.Num())); }));
        Row->AddSlot().AutoWidth().VAlign(VAlign_Center)[SNew(SBox).WidthOverride(112).HAlign(HAlign_Right)[CountText]];
        auto Description = Text(FString(), 20, Paper);
        Description->SetWrapTextAt(1510.f);
        Description->SetText(TAttribute<FText>::CreateLambda([this] { return FText::FromString(StepDescription()); }));
        return SNew(SBorder).BorderImage(&Frame).Padding(FMargin(22, 15))
            [SNew(SVerticalBox) + SVerticalBox::Slot().AutoHeight()[Row]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)[Description]];
    }

    TSharedRef<SWidget> LegendEntry(FLinearColor Tint, const FString& Caption)
    {
        return SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 6, 0)
                [SNew(SBox).WidthOverride(12).HeightOverride(12)[SNew(SBorder).BorderImage(&KeyShape).BorderBackgroundColor(Tint)]]
            + SHorizontalBox::Slot().AutoWidth()[Text(Caption, 15, Faded)];
    }

    TSharedRef<SWidget> NodeBar(const FString& Caption, bool Pruning)
    {
        auto Count = Text(FString(), 16, Paper);
        Count->SetText(TAttribute<FText>::CreateLambda([this, Pruning]
            { return FText::AsNumber(Pruning ? PrunedNodes : FullNodes); }));
        return SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                [SNew(SBox).WidthOverride(194)[Text(Caption, 16, Faded)]]
            + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
                [SNew(SBox).HeightOverride(7)[SNew(SProgressBar)
                    .Style(&BarStyle).FillColorAndOpacity(Pruning ? Copper : Expanded)
                    .Percent_Lambda([this, Pruning] { return TOptional<float>(FullNodes > 0 ? (Pruning ? float(PrunedNodes) / FullNodes : 1.f) : 0.f); })]]
            + SHorizontalBox::Slot().AutoWidth().Padding(12, 0, 0, 0)
                [SNew(SBox).WidthOverride(72).HAlign(HAlign_Right)[Count]];
    }

    TSharedRef<SWidget> Board()
    {
        auto Cells = SNew(SVerticalBox);
        auto Columns = SNew(SHorizontalBox);
        Columns->AddSlot().AutoWidth()[SNew(SBox).WidthOverride(35)];
        for (int32 X = 0; X < Width; ++X)
            Columns->AddSlot().AutoWidth().Padding(3, 0)[SNew(SBox).WidthOverride(74).HAlign(HAlign_Center)[Text(FString::Chr(TCHAR('A' + X)), 17, Faded)]];
        Cells->AddSlot().AutoHeight().Padding(0, 0, 0, 6)[Columns];
        for (int32 Y = 0; Y < Height; ++Y)
        {
            auto Line = SNew(SHorizontalBox);
            Line->AddSlot().AutoWidth().VAlign(VAlign_Center)[SNew(SBox).WidthOverride(35).HAlign(HAlign_Center)[Text(FString::FromInt(Y + 1), 15, Faded)]];
            for (int32 X = 0; X < Width; ++X)
            {
                const FIntPoint Cell(X, Y);
                auto Label = SNew(STextBlock).Font(FSlateFontInfo(Sans, 22)).ColorAndOpacity(Paper)
                    .Text_Lambda([this, Cell] { return FText::FromString(Mark(Cell)); });
                auto CanPick = TAttribute<bool>::CreateLambda([this, Cell] { return Topic != ETopic::Supplies && !IsWall(Cell) && !IsClosed(Cell); });
                auto Tint = TAttribute<FSlateColor>::CreateLambda([this, Cell] { return FSlateColor(Color(Cell)); });
                const FName Id(*FString::Printf(TEXT("%c%d"), TCHAR('A' + X), Y + 1));
                auto Face = SNew(SOverlay) + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[Label];
                if (Topic == ETopic::Path)
                    Face->AddSlot()[SNew(SRouteArrow).Direction_Lambda([this, Cell] { return AtResult() ? Arrows.FindRef(Cell) : FIntPoint::ZeroValue; })];
                auto CellKey = Key(Id, Face, [this, Cell] { Pick(Cell); }, Tint, FMargin(0), CanPick);
                CellKey->SetHAlign(HAlign_Fill);
                CellKey->SetVAlign(VAlign_Fill);
                CellKey->SetToolTipText(FText::FromString(Id.ToString()));
                Line->AddSlot().AutoWidth().Padding(3, 2)[SNew(SBox).WidthOverride(74).HeightOverride(47)[CellKey]];
            }
            Cells->AddSlot().AutoHeight()[Line];
        }
        return Cells;
    }

    TSharedRef<SWidget> Toggle(FName Id, const FString& Caption, bool& Setting, bool MutuallyExclusive = false, bool Value = false)
    {
        auto Tint = TAttribute<FSlateColor>::CreateLambda([&Setting, MutuallyExclusive, Value]
            { return FSlateColor((MutuallyExclusive ? Setting == Value : Setting) ? Goal : Tile); });
        return Key(Id, Text(Caption, 19), [this, &Setting, MutuallyExclusive, Value]
            { Setting = MutuallyExclusive ? Value : !Setting; Calculate(); }, Tint);
    }

    TSharedRef<SWidget> DoorKey(FName Id, const FString& TileId, bool& IsOpen)
    {
        auto Label = Text(FString(), 18);
        Label->SetText(TAttribute<FText>::CreateLambda([this, TileId, &IsOpen]
        {
            return FText::FromString(TileId + TEXT("   ·   ") + (IsOpen
                ? Local(TEXT("открыта"), TEXT("ашық"), TEXT("open")) : Local(TEXT("закрыта"), TEXT("жабық"), TEXT("closed"))));
        }));
        const auto Tint = TAttribute<FSlateColor>::CreateLambda([&IsOpen] { return FSlateColor(IsOpen ? Tile : Goal); });
        return Key(Id, Label, [this, &IsOpen] { IsOpen = !IsOpen; Calculate(); }, Tint);
    }

    TSharedRef<SWidget> NumberPicker(int32& Setting, TArray<int32> Choices, const TCHAR* Suffix)
    {
        auto Row = SNew(SHorizontalBox);
        for (int32 Value : Choices)
        {
            auto Tint = TAttribute<FSlateColor>::CreateLambda([&Setting, Value] { return FSlateColor(Setting == Value ? Goal : Tile); });
            Row->AddSlot().AutoWidth().Padding(0, 0, 10, 0)
            [Key(FName(*FString::FromInt(Value)), Text(FString::FromInt(Value) + Suffix, 19), [this, &Setting, Value] { Setting = Value; Calculate(); }, Tint)];
        }
        return Row;
    }

    void Rebuild()
    {
        Controls.Reset();
        auto Body = SNew(SVerticalBox);
        auto Header = SNew(SHorizontalBox);
        Header->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 24, 0)
            [Key(TEXT("back"), Text(Local(TEXT("‹  Главное меню"), TEXT("‹  Басты мәзір"), TEXT("‹  Main menu")), 18), [this] { Back(); }, FSlateColor(Tile))];
        Header->AddSlot().FillWidth(1).VAlign(VAlign_Center)
            [Text(Local(TEXT("Демонстрация ИИ"), TEXT("ЖИ демонстрациясы"), TEXT("AI demonstration")), 49, Paper, true)];
        auto VerificationLabel = Text(FString(), 18);
        VerificationLabel->SetText(TAttribute<FText>::CreateLambda([this]
        {
            return FText::FromString(ShowingVerification ? Local(TEXT("К опыту"), TEXT("Тәжірибеге оралу"), TEXT("Back to experiment"))
                : Local(TEXT("Проверить результат"), TEXT("Нәтижені тексеру"), TEXT("Verify result")));
        }));
        const auto VerificationTint = TAttribute<FSlateColor>::CreateLambda([this] { return FSlateColor(ShowingVerification ? Start : Frontier); });
        Header->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 10, 0)
            [Key(TEXT("verify"), VerificationLabel, [this] { ShowingVerification = !ShowingVerification; }, VerificationTint)];
        Header->AddSlot().AutoWidth().VAlign(VAlign_Center)
            [Key(TEXT("reset"), Text(Local(TEXT("Сбросить опыт"), TEXT("Тәжірибені қалпына келтіру"), TEXT("Reset experiment")), 18, Paper), [this] { ResetTopic(); }, FSlateColor(Tile))];
        Body->AddSlot().AutoHeight().Padding(0, 0, 0, 20)[Header];
        auto Tabs = SNew(SHorizontalBox);
        const TPair<ETopic, FString> Choices[] = {
            {ETopic::Path, Local(TEXT("A*  ·  Поиск пути"), TEXT("A*  ·  Жол іздеу"), TEXT("A*  ·  Pathfinding"))},
            {ETopic::Supplies, Local(TEXT("CSP  ·  Ограничения"), TEXT("CSP  ·  Шектеулер"), TEXT("CSP  ·  Constraints"))},
            {ETopic::Hunt, Local(TEXT("Minimax  ·  Тактика"), TEXT("Minimax  ·  Тактика"), TEXT("Minimax  ·  Tactics"))}
        };
        for (const auto& Choice : Choices)
        {
            const ETopic Target = Choice.Key;
            const bool IsLast = Target == ETopic::Hunt;
            const FString Week = FString::Format(*Local(TEXT("Неделя {0}"), TEXT("{0}-апта"), TEXT("Week {0}")), {2 + int32(Target)});
            auto Tint = TAttribute<FSlateColor>::CreateLambda([this, Target] { return FSlateColor(Topic == Target ? Goal : Wall); });
            auto Tab = SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[Text(Week, 15, Faded)]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 0).HAlign(HAlign_Center)[Text(Choice.Value, 23, Paper, true)];
            Tabs->AddSlot().FillWidth(1).Padding(0, 0, IsLast ? 0 : 10, 0)
                [Key(FName(*FString::Printf(TEXT("tab%d"), int32(Target))), Tab, [this, Target] { Select(Target); }, Tint, FMargin(14, 9))];
        }
        Body->AddSlot().AutoHeight().Padding(0, 0, 0, 16)[Tabs];
        auto Copy = SNew(SVerticalBox);
        FString Heading, Description, Try, Legend;
        if (Topic == ETopic::Path)
        {
            Heading = Local(TEXT("Маршрут по отделению"), TEXT("Бөлімше ішіндегі жол"), TEXT("Route through the ward"));
            Description = Local(TEXT("A* раскрывает клетку с наименьшим f = g + h. g — число шагов от старта, h — манхэттенское расстояние до цели. Можно двигаться в четырёх направлениях."), TEXT("A* f = g + h мәні ең аз ұяшықты ашады. g — бастау нүктесінен жасалған қадам саны, h — мақсатқа дейінгі Манхэттен қашықтығы. Төрт бағытта жүруге болады."), TEXT("A* expands the tile with the lowest f = g + h. g is the number of steps from the start. h is the Manhattan distance to the goal. Movement has four directions."));
            Try = Local(TEXT("Закройте D2. С исходными точками путь изменится с 5 до 13 шагов. Закройте и D6, чтобы остаться без прохода."), TEXT("D2 есігін жабыңыз. Бастапқы нүктелермен жол 5 қадамнан 13 қадамға ұзарады. D6 есігін де жапсаңыз, жол қалмайды."), TEXT("Close D2. With the default endpoints the route grows from 5 to 13 steps. Close D6 as well to block passage."));
            Legend = Local(TEXT("S — старт    G — цель    × — закрытая дверь"), TEXT("S — бастау    G — мақсат    × — жабық есік"), TEXT("S — start    G — goal    × — closed door"));
        }
        else if (Topic == ETopic::Supplies)
        {
            Heading = Local(TEXT("Расстановка припасов"), TEXT("Жабдықтарды орналастыру"), TEXT("Supply placement"));
            Description = Local(TEXT("CSP выбирает предмет с наименьшим числом доступных мест. Слишком близкие позиции исключаются. Если мест не осталось, поиск возвращается к предыдущему выбору."), TEXT("CSP жарамды орны ең аз затты таңдайды. Тым жақын орындар алынып тасталады. Орын қалмаса, іздеу алдыңғы таңдауға оралады."), TEXT("CSP chooses the item with the fewest legal locations. It rejects any tile too close to another item. If no tile remains, it backtracks to the previous decision."));
            Try = Local(TEXT("Сравните 3 м и 4 м. По шагам видно, почему обе позиции батареи отпадают при 4 м."), TEXT("3 м мен 4 м-ді салыстырыңыз. Қадамдардан 4 м кезінде батареяның екі орны да неге жарамсыз екенін көресіз."), TEXT("Compare 3 m and 4 m. The trace shows why both battery locations fail at 4 m."));
            Legend = Local(TEXT("M — аптечка    A — патроны    B — батарея"), TEXT("M — дәрі қобдишасы    A — оқ-дәрі    B — батарея"), TEXT("M — medkit    A — ammunition    B — battery"));
        }
        else
        {
            Heading = Local(TEXT("Решение охотника"), TEXT("Аңшының шешімі"), TEXT("Hunter's next move"));
            Description = Local(TEXT("Охотник повышает оценку, игрок её снижает. На предельной глубине учитываются расстояние и выходы игрока. Захват даёт высшую оценку."), TEXT("Аңшы бағаны көтереді, ойыншы төмендетеді. Тереңдік шегінде арақашықтық пен ойыншының шығу жолдары есептеледі. Ұстап алу ең жоғары баға береді."), TEXT("The hunter maximizes the score. The player minimizes it. At the depth limit, the score uses distance and the player’s available exits. Capture scores highest."));
            Try = Local(TEXT("Сравните число узлов при глубине 3. Знак <= означает верхнюю границу оценки хода."), TEXT("3 тереңдікте түйіндер санын салыстырыңыз. <= белгісі жүріс бағасының жоғарғы шегін көрсетеді."), TEXT("Compare the node counts at depth 3. <= marks an upper bound for the move score."));
            Legend = Local(TEXT("H — охотник    P — игрок    + — выбранный ход"), TEXT("H — аңшы    P — ойыншы    + — таңдалған жүріс"), TEXT("H — hunter    P — player    + — chosen move"));
        }
        Copy->AddSlot().AutoHeight()[Text(Heading, 35, Paper, true)];
        auto Explanation = Text(Description, 21, Faded);
        Explanation->SetWrapTextAt(824.f);
        Copy->AddSlot().AutoHeight().Padding(0, 8, 0, 14)[Explanation];
        if (Topic != ETopic::Supplies)
        {
            auto Targets = SNew(SHorizontalBox);
            const bool RouteTopic = Topic == ETopic::Path;
            Targets->AddSlot().AutoWidth().Padding(0, 0, 9, 0)[Toggle(TEXT("first"), RouteTopic ? Local(TEXT("Выбрать старт"), TEXT("Бастау нүктесі"), TEXT("Select start")) : Local(TEXT("Поставить охотника"), TEXT("Аңшыны қою"), TEXT("Place hunter")), PickFirst, true, true)];
            Targets->AddSlot().AutoWidth()[Toggle(TEXT("second"), RouteTopic ? Local(TEXT("Выбрать цель"), TEXT("Мақсат нүктесі"), TEXT("Select goal")) : Local(TEXT("Поставить игрока"), TEXT("Ойыншыны қою"), TEXT("Place player")), PickFirst, true, false)];
            Copy->AddSlot().AutoHeight()[Targets];
        }
        if (Topic == ETopic::Path)
        {
            auto Doors = SNew(SHorizontalBox);
            Doors->AddSlot().AutoWidth().Padding(0, 0, 9, 0)[DoorKey(TEXT("upper"), TEXT("D2"), UpperOpen)];
            Doors->AddSlot().AutoWidth()[DoorKey(TEXT("lower"), TEXT("D6"), LowerOpen)];
            Copy->AddSlot().AutoHeight().Padding(0, 12, 0, 0)[Doors];
        }
        else if (Topic == ETopic::Supplies)
        {
            Copy->AddSlot().AutoHeight().Padding(0, 12, 0, 8)[Text(Local(TEXT("Минимальная дистанция между предметами"), TEXT("Заттар арасындағы ең кіші арақашықтық"), TEXT("Minimum distance between items")), 18)];
            Copy->AddSlot().AutoHeight()[NumberPicker(Separation, {1, 3, 4, 6}, *Local(TEXT(" м"), TEXT(" м"), TEXT(" m")))];
        }
        else
        {
            Copy->AddSlot().AutoHeight().Padding(0, 10, 0, 0)[SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 14, 0)
                    [Text(Local(TEXT("Глубина в парах ходов"), TEXT("Тереңдік, жүріс жұбы"), TEXT("Depth in move pairs")), 18)]
                + SHorizontalBox::Slot().AutoWidth()[NumberPicker(Depth, {1, 2, 3}, TEXT(""))]];
        }
        auto ResultText = Text(FString(), 26, Paper, true);
        ResultText->SetText(TAttribute<FText>::CreateLambda([this] { return FText::FromString(Outcome()); }));
        ResultText->SetWrapTextAt(745.f);
        auto Results = SNew(SVerticalBox);
        Results->AddSlot().AutoHeight()[ResultText];
        auto MetricLine = Text(FString(), 17, Faded);
        MetricLine->SetText(TAttribute<FText>::CreateLambda([this] { return FText::FromString(Metrics()); }));
        Results->AddSlot().AutoHeight().Padding(0, 5, 0, 0)[MetricLine];
        if (Topic == ETopic::Hunt)
        {
            Results->AddSlot().AutoHeight().Padding(0, 6, 0, 0)[NodeBar(Local(TEXT("С alpha-beta"), TEXT("Alpha-beta арқылы"), TEXT("With alpha-beta")), true)];
            Results->AddSlot().AutoHeight().Padding(0, 2, 0, 0)[NodeBar(Local(TEXT("Полный перебор"), TEXT("Толық іздеу"), TEXT("Full search")), false)];
        }
        Copy->AddSlot().AutoHeight().Padding(0, 15, 0, 0)[SNew(SBorder).BorderImage(&OutcomeFrame).Padding(FMargin(15, 10))[Results]];
        auto Hint = Text(Try, 17, Faded);
        Hint->SetWrapTextAt(819.f);
        Copy->AddSlot().AutoHeight().Padding(0, 11, 0, 0)[Hint];
        auto MapCopy = SNew(SVerticalBox);
        MapCopy->AddSlot().AutoHeight().Padding(0, 0, 0, 7)[Text(Local(TEXT("Схема отделения"), TEXT("Бөлімше сызбасы"), TEXT("Ward layout")), 24, Paper, true)];
        MapCopy->AddSlot().AutoHeight()[Board()];
        MapCopy->AddSlot().AutoHeight().Padding(35, 8, 0, 0)[Text(Legend, 16, Faded)];
        const bool RouteTopic = Topic == ETopic::Path;
        auto Swatches = SNew(SHorizontalBox);
        Swatches->AddSlot().AutoWidth().Padding(0, 0, 14, 0)[LegendEntry(RouteTopic ? Route : Topic == ETopic::Supplies ? Start : Route,
            RouteTopic ? Local(TEXT("Маршрут"), TEXT("Жол"), TEXT("Route")) : Topic == ETopic::Supplies ? Local(TEXT("Выбрано"), TEXT("Таңдалды"), TEXT("Placed")) : Local(TEXT("Ход"), TEXT("Жүріс"), TEXT("Move")))];
        if (RouteTopic) Swatches->AddSlot().AutoWidth().Padding(0, 0, 14, 0)[LegendEntry(Expanded, Local(TEXT("Проверено"), TEXT("Тексерілді"), TEXT("Expanded")))];
        if (Topic != ETopic::Hunt) Swatches->AddSlot().AutoWidth()[LegendEntry(Frontier, RouteTopic
            ? Local(TEXT("В очереди"), TEXT("Кезекте"), TEXT("Queued")) : Local(TEXT("Вариант"), TEXT("Нұсқа"), TEXT("Candidate")))];
        MapCopy->AddSlot().AutoHeight().Padding(35, 6, 0, 0)[Swatches];
        auto Page = SNew(SHorizontalBox);
        Page->AddSlot().AutoWidth().Padding(0, 0, 14, 0)[SNew(SBorder).BorderImage(&Frame).Padding(20)[MapCopy]];
        auto Details = SNew(SWidgetSwitcher).WidgetIndex_Lambda([this] { return ShowingVerification ? 1 : 0; })
            + SWidgetSwitcher::Slot().VAlign(VAlign_Top)[Copy]
            + SWidgetSwitcher::Slot().VAlign(VAlign_Top)[BuildVerification()];
        Page->AddSlot().FillWidth(1)[SNew(SBorder).BorderImage(&Frame).Padding(22)[Details]];
        Body->AddSlot().AutoHeight()[Page];
        Body->AddSlot().AutoHeight().Padding(0, 14, 0, 0)[Player()];
        auto Framing = SNew(SBorder).BorderImage(&Fill).BorderBackgroundColor(FLinearColor(.004f, .007f, .011f, .64f))
            .Padding(FMargin(119, 54))[Body];
        auto Scaled = SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
            [SNew(SBox).WidthOverride(1920).HeightOverride(1080)[Framing]];
        ChildSlot[SNew(SOverlay) + SOverlay::Slot()[WardTitleMenu::Shade(Game)] + SOverlay::Slot()[Scaled]];
    }
};

TWeakPtr<SDemo> Active;

TSharedRef<SWidget> Build(TWeakObjectPtr<AEWardCampaignMode> Game)
{
    auto Demo = SNew(SDemo).Game(Game);
    Active = Demo;
    return Demo;
}
}

void TickWardAIDemoAssessment115(AEWardCampaignMode& Game)
{
    using namespace WardAIDemo115;
    using namespace WardData;
    static TArray<TSharedPtr<FJsonValue>> Checks;
    static int32 Step = 0;
    static double Previous = 0.0;
    const double Now = FPlatformTime::Seconds();
    if (Now - Previous < (Step == 1 ? 4.0 : Step == 5 ? 3.0 : 1.25)) return;
    Previous = Now;
    auto Check = [](FString Name, bool Passed)
    {
        Json Item = MakeShared<FJsonObject>();
        Item->SetStringField(TEXT("name"), Name);
        Item->SetBoolField(TEXT("passed"), Passed);
        Checks.Add(MakeShared<FJsonValueObject>(Item));
        UE_LOG(LogTemp, Display, TEXT("AI_DEMO115 %s %s"), *Name, Passed ? TEXT("PASS") : TEXT("FAIL"));
    };
    auto Shot = [](const TCHAR* Name)
    {
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Prototype/AIDemo117") / (FString(Name) + TEXT(".png")), true, false);
    };
    if (Step == 0)
    {
        Game.Show(TEXT("title"));
        ++Step;
        return;
    }
    if (Step == 1)
    {
        const auto MenuItem = Game.Interface->TitleActions.FindRef(TEXT("page_ai_demo")).Pin();
        Check(TEXT("main_menu_has_enabled_ai_demo_button"), MenuItem && MenuItem->IsEnabled());
        Shot(TEXT("main_menu"));
        ++Step;
        return;
    }
    if (Step == 2)
    {
        const TSharedPtr<SWidget> MenuItem = Game.Interface->TitleActions.FindRef(TEXT("page_ai_demo")).Pin();
        if (MenuItem && MenuItem->GetType() == TEXT("SButton")) StaticCastSharedPtr<SButton>(MenuItem)->SimulateClick();
        else Game.Command(TEXT("page_ai_demo"));
        Check(TEXT("title_action_opens_demo_without_legacy_campaign"), Game.Screen == TEXT("ai_demo") && Game.BookWorld && Active.IsValid());
        ++Step;
        return;
    }
    const auto Demo = Active.Pin();
    if (Demo && Step == 3)
    {
        Check(TEXT("astar_five_steps_through_upper_door"), Demo->Path.Num() == 5 && Demo->Path.Contains({3, 1}) && Demo->Grid.Expanded > 0);
        Check(TEXT("astar_detours_through_lower_door"), Demo->Click(TEXT("upper")) && Demo->Path.Num() == 13 && Demo->Path.Contains({3, 5}));
        Check(TEXT("astar_trace_matches_final_route"), Demo->Trace.Num() > 13 && Demo->Trace.Last().Kind == EWardAITraceKind::PathFound
            && Demo->Trace.Last().Count == Demo->Path.Num());
        Check(TEXT("astar_bfs_confirms_length_of_legal_detour"), Demo->Verification.Matches && Demo->Verification.ReferenceSteps == 13);
        Check(TEXT("verification_panel_opens"), Demo->Click(TEXT("verify")) && Demo->ShowingVerification);
        Shot(TEXT("astar_verify"));
    }
    else if (Demo && Step == 4)
    {
        Check(TEXT("experiment_controls_can_be_restored"), Demo->Click(TEXT("verify")) && !Demo->ShowingVerification);
        Check(TEXT("controls_rewind_and_advance_real_search"), Demo->Click(TEXT("start")) && Demo->Cursor == 0 && Demo->Visited.IsEmpty()
            && Demo->Click(TEXT("next")) && Demo->Cursor == 1 && Demo->Visited.IsEmpty()
            && Demo->Click(TEXT("next")) && Demo->Cursor == 2 && Demo->Visited.Contains(Demo->From));
        Check(TEXT("autoplay_starts_from_trace_cursor"), Demo->Click(TEXT("play")) && Demo->Playing);
    }
    else if (Demo && Step == 5)
    {
        Check(TEXT("autoplay_advances_and_can_pause"), Demo->Cursor > 2 && Demo->Click(TEXT("play")) && !Demo->Playing);
        Check(TEXT("end_jumps_to_result"), Demo->Click(TEXT("end")) && Demo->Cursor == Demo->Trace.Num());
        Check(TEXT("astar_reports_unreachable_with_both_doors_closed"), Demo->Click(TEXT("lower")) && Demo->Path.IsEmpty()
            && Demo->Grid.Blocked.Contains({3, 1}) && Demo->Grid.Blocked.Contains({3, 5}));
        Check(TEXT("astar_bfs_independently_confirms_no_path"), Demo->Verification.HasValidInput && Demo->Verification.Matches
            && Demo->Verification.ReferenceSteps == INDEX_NONE);
    }
    else if (Demo && Step == 6)
    {
        const FIntPoint GoalBefore = Demo->To;
        Demo->Click(TEXT("D6"));
        Check(TEXT("astar_cannot_select_a_closed_door"), Demo->To == GoalBefore);
        Check(TEXT("astar_user_can_move_both_endpoints"), Demo->Click(TEXT("first")) && Demo->Click(TEXT("B3"))
            && Demo->Click(TEXT("second")) && Demo->Click(TEXT("C3")) && Demo->Path.Num() == 1);
        Check(TEXT("csp_three_assignments_at_three_metres"), Demo->Click(TEXT("tab1")) && Demo->Solved && Demo->Placement.Num() == 3);
        Check(TEXT("csp_exhaustive_search_accepts_returned_assignment"), Demo->Verification.Matches && Demo->Verification.Combinations == 8
            && Demo->Verification.Allowed == 8 && Demo->Verification.PlacedMin >= 300.0);
        Shot(TEXT("csp"));
    }
    else if (Demo && Step == 7)
    {
        Check(TEXT("csp_reports_impossible_four_metre_separation"), Demo->Click(TEXT("4")) && !Demo->Solved && Demo->Placement.IsEmpty());
        Check(TEXT("csp_exhaustive_search_proves_no_placement"), Demo->Verification.Matches && Demo->Verification.Combinations == 8
            && Demo->Verification.Allowed == 0 && FMath::IsNearlyEqual(Demo->Verification.BestMin, FMath::Sqrt(13.0) * 100.0));
        Check(TEXT("csp_trace_records_conflicts_and_backtracking"), Demo->Trace.ContainsByPredicate([](const auto& E) { return E.Kind == EWardAITraceKind::RejectLocation && E.Value < E.Count; })
            && Demo->Trace.ContainsByPredicate([](const auto& E) { return E.Kind == EWardAITraceKind::Backtrack; }));
        for (int32 Index = 0; Index < Demo->Trace.Num(); ++Index)
        {
            if (Demo->Trace[Index].Kind == EWardAITraceKind::RejectLocation) break;
            if (Index == 0) Demo->Click(TEXT("start"));
            Demo->Click(TEXT("next"));
        }
        Demo->Click(TEXT("next"));
        Demo->Click(TEXT("verify"));
        Shot(TEXT("csp_verify"));
    }
    else if (Demo && Step == 8)
    {
        Demo->Click(TEXT("verify"));
        Check(TEXT("csp_can_recover_after_invalid_constraints"), Demo->Click(TEXT("reset")) && Demo->Separation == 3 && Demo->Solved && Demo->Placement.Num() == 3 && Demo->Cursor == Demo->Trace.Num());
        Check(TEXT("minimax_alpha_beta_keeps_the_same_move"), Demo->Click(TEXT("tab2")) && Demo->SameMove && Demo->FullNodes >= Demo->PrunedNodes && Demo->Cutoffs > 0);
        Check(TEXT("minimax_trace_records_candidate_moves"), Demo->Trace.Num() > 1 && Demo->Trace.Last().Kind == EWardAITraceKind::ChooseMove
            && Demo->Trace.Last().Cell == Demo->NextMove && Demo->Trace.Last().Count == Demo->PrunedNodes);
        Check(TEXT("minimax_reference_confirms_same_move_and_score"), Demo->Verification.Matches && Demo->Verification.FullMove == Demo->NextMove
            && FMath::IsNearlyEqual(Demo->Verification.PrunedScore, Demo->Verification.FullScore));
        Demo->Click(TEXT("verify"));
        Shot(TEXT("minimax_verify"));
    }
    else if (Demo && Step == 9)
    {
        Demo->Click(TEXT("verify"));
        const int32 Before = Demo->FullNodes;
        Check(TEXT("minimax_depth_changes_search_cost"), Demo->Click(TEXT("3")) && Demo->Depth == 3 && Demo->FullNodes > Before && Demo->SameMove);
        Check(TEXT("minimax_separate_positions_are_editable"), Demo->Click(TEXT("first")) && Demo->Click(TEXT("A2"))
            && Demo->Hunter == FIntPoint(0, 1) && Demo->Click(TEXT("second")) && Demo->Click(TEXT("H6")) && Demo->Runner == FIntPoint(7, 5));
        Check(TEXT("back_returns_to_main_menu"), Demo->Click(TEXT("back")) && Game.Screen == TEXT("title"));
    }
    else if (Step == 10)
    {
        Check(TEXT("demo_releases_preview_on_close"), !Active.IsValid());
        const bool Passed = Checks.Num() == 29 && !Checks.ContainsByPredicate([](const auto& Result)
            { return !Boolean(Result->AsObject(), TEXT("passed")); });
        Json Report = MakeShared<FJsonObject>();
        Report->SetArrayField(TEXT("checks"), Checks);
        Report->SetBoolField(TEXT("success"), Passed);
        Write(FPaths::ProjectSavedDir() / TEXT("Prototype/ai_demo_check.json"), Report);
        FPlatformMisc::RequestExit(false);
    }
    ++Step;
}
