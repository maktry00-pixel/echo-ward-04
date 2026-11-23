#include "WardCampaign.h"
#include "WardGameplay76.h"
#include "WardCampaignUI.h"
#include "WardHospitalGameplay.h"
#include "WardEchoAssets.h"
#include "WardHeldGrip108.h"
#include "WardMotion107.h"
#include "WardContact98.h"
#include "WardRescue104.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateImageBrush.h"
#include "Engine/Texture2D.h"
#include "Fonts/CompositeFont.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"
using namespace WardData;

namespace
{
constexpr int32 NotePixelsWide=768;
constexpr int32 NotePixelsHigh=1152;
constexpr double NoteScale=.022;
const FVector NoteHalf(.3,NotePixelsWide*NoteScale*.5,NotePixelsHigh*NoteScale*.5);

bool CollectedDocument(const AEWardCampaignMode& Mode,const FString& Id)
{
    if(!Id.StartsWith(TEXT("doc:")))return false;
    const FString Key=Id.RightChop(4);
    return Mode.Completed.Contains(Key)&&(Mode.Journal.Contains(TEXT("@doc:")+Key)||Mode.Journal.Contains(TEXT("@blank:")+Key));
}

const FSlateBrush* NotePaper(bool& Imported)
{
    static UTexture2D* Paper=FImageUtils::ImportFileAsTexture2D(FPaths::ProjectDir()/TEXT("WardNote109/LinedNote.png"));
    static const FSlateImageBrush Printed(Paper,FVector2D(NotePixelsWide,NotePixelsHigh));
    static const FSlateColorBrush Fallback(FLinearColor(.77,.71,.58));
    if(Paper&&!Paper->IsRooted())Paper->AddToRoot();
    Imported=Paper!=nullptr;
    if(Imported)return &Printed;
    return &Fallback;
}

TSharedPtr<const FCompositeFont> NoteTypeface()
{
    static TSharedPtr<const FCompositeFont> Typeface=[]
    {
        auto Font=MakeShared<FCompositeFont>();
        FTypefaceEntry Handwriting;
        Handwriting.Name=TEXT("Regular");
        Handwriting.Font=FFontData(FPaths::ProjectDir()/TEXT("WardNote109/Caveat-Regular.ttf"),EFontHinting::Default,EFontLoadingPolicy::LazyLoad);
        Font->DefaultTypeface.Fonts.Add(MoveTemp(Handwriting));
        FTypefaceEntry Fallback;
        Fallback.Name=TEXT("Regular");
        Fallback.Font=FFontData(FPaths::ProjectContentDir()/TEXT("CampaignData/Fonts/NotoSans.ttf"),EFontHinting::Default,EFontLoadingPolicy::LazyLoad);
        Font->FallbackTypeface.Typeface.Fonts.Add(MoveTemp(Fallback));
        return Font;
    }();
    return Typeface;
}

void AddNoteFace(AActor& Actor,USceneComponent* Root,const FString& Title,const FString& Body)
{
    auto* Face=NewObject<UWidgetComponent>(&Actor);
    Face->SetupAttachment(Root);
    Face->SetWidgetSpace(EWidgetSpace::World);
    Face->SetDrawSize(FVector2D(NotePixelsWide,NotePixelsHigh));
    Face->SetRelativeLocation(FVector(-.135,0,0));
    Face->SetRelativeRotation(FRotator(0,180,0));
    Face->SetRelativeScale3D(FVector(NoteScale));
    Face->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    bool HasTexture=false;
    const FSlateBrush* Paper=NotePaper(HasTexture);
    if(HasTexture)Face->ComponentTags.Add(TEXT("WardNote109Paper"));
    if(FPaths::FileExists(FPaths::ProjectDir()/TEXT("WardNote109/Caveat-Regular.ttf")))Face->ComponentTags.Add(TEXT("WardNote109Text"));
    const auto Font=NoteTypeface();
    const float BodySize=Body.Len()>930?24.f:Body.Len()>600?29.f:34.f;
    const FLinearColor Ink(.042f,.032f,.027f);
    Face->SetSlateWidget(SNew(SOverlay)
        +SOverlay::Slot()[SNew(SImage).Image(Paper)]
        +SOverlay::Slot().Padding(FMargin(92,82,106,66))[
            SNew(SVerticalBox)
            +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,15)[
                SNew(STextBlock).Text(FText::FromString(Title)).Font(FSlateFontInfo(Font,46)).ColorAndOpacity(Ink).WrapTextAt(570.f)]
            +SVerticalBox::Slot().AutoHeight()[
                SNew(STextBlock).Text(FText::FromString(Body)).Font(FSlateFontInfo(Font,BodySize))
                .ColorAndOpacity(Ink).WrapTextAt(570.f).LineHeightPercentage(.94f)]]);
    Face->RegisterComponent();
    Actor.AddInstanceComponent(Face);
}
}

namespace WardHeldGrip108
{
namespace
{
struct FHolding
{
    FProfile Profile;
    FVector Offset = FVector::ZeroVector;
    FVector Velocity = FVector::ZeroVector;
    FVector PreviousSpeed = FVector::ZeroVector;
    FVector PreviousEye = FVector::ZeroVector;
    double LastTime = -1;
};
TMap<TWeakObjectPtr<const AActor>, FHolding> Held;

FHolding& State(const AActor& Actor)
{
    if(auto* Previous=Held.Find(&Actor)) return *Previous;
    for(auto It=Held.CreateIterator();It;++It) if(!It.Key().IsValid()) It.RemoveCurrent();
    return Held.Add(&Actor);
}
}

FProfile Classify(const FString& Id, const FString& Kind, FVector Half)
{
    Half = Half.ComponentMax(FVector(.2));
    if (Id.StartsWith(TEXT("doc:")))
        return {EGrip::Notebook,Half,FVector(40,-5,.5),FVector(-7,9,-11),.10f,false};
    if (Id == TEXT("map") || Kind == TEXT("map"))
        return {EGrip::Page, Half, FVector(36,0,-1), FVector(-6,0,0), .12f, true};
    if (Kind == TEXT("key") || Id.Contains(TEXT("key"), ESearchCase::IgnoreCase) ||
        Id == TEXT("fuse") || Id == TEXT("spare_fuse") || Id == TEXT("badge") ||
        Id == TEXT("battery") || Id.Contains(TEXT("vial"), ESearchCase::IgnoreCase))
        return {EGrip::Pinch, Half, FVector(38,11,-10), FVector(-14,-12,0), .08f, false};
    if (Id == TEXT("fuel") || Id == TEXT("coolant") || Id == TEXT("evidence_case"))
        return {EGrip::Handle, Half, FVector(39,11,-22), FVector(0,-10,0), .84f, false};
    if (Id == TEXT("boards") || Id == TEXT("medkit") || Half.Y > 12 || Half.Z > 24 || Half.X > 14)
        return {EGrip::Cradle, Half, FVector(44,1,-8), FVector(-5,-8,0), .72f, true};
    return {EGrip::Wrap, Half, FVector(39,13,-13), FVector(-12,-12,4), Id == TEXT("bottle")?.38f:.26f, false};
}

FProfile Phone()
{
    return {EGrip::Phone, FVector(.9,4.0,8.0), FVector(35,12,-14), FVector(0,-8,-7), .24f, false};
}

FProfile Preview(const AActor* Actor)
{
    const auto* Hold=Held.Find(Actor);
    return Hold?Hold->Profile:FProfile{};
}

FHand Anchor(const FProfile& P, bool Left)
{
    const double Side = Left?-1.:1.;
    const FVector Across(0,0,Side);
    const double Width = P.Half.Y;
    switch(P.Grip)
    {
        case EGrip::Page:
            return {FVector(3.7,Side*(Width+7.),-P.Half.Z*.78), FVector(.10,-Side,.22), Across};
        case EGrip::Notebook:
            return {FVector(3.3,Width+4.0,-P.Half.Z*.91),FVector(.24,-.40,.89),FVector(-.66,-.67,.25)};
        case EGrip::Pinch:
            return {FVector(-5,Width+6.,-FMath::Min(4.+P.Half.Z,9.)), FVector(.57,-.65,.49), Across};
        case EGrip::Phone:
            return {FVector(-2.2,Width+2.5,-P.Half.Z+1), FVector(.04,-.48,.88), FVector(.16,-.87,-.46)};
        case EGrip::Handle:
            return {FVector(-6,2,P.Half.Z+2), FVector(1,-.12,-.15), Across};
        case EGrip::Cradle:
            return {FVector(-FMath::Min(P.Half.X+8,21.),Side*FMath::Clamp(Width*.76,4.,16.),-P.Half.Z-3.2),
                FVector(.98,-Side*.20,-.11), FVector(.03,Side*.94,.14*Side)};
        default:
            return {FVector(-FMath::Min(P.Half.X+3,10.),FMath::Clamp(Width*.52+1.,2.,9.),-4), FVector(1,-.32,.26), Across};
    }
}

bool Place(const AEWardCampaignHero& Hero, AActor& Actor, const FProfile& P, float Raise)
{
    const auto* Game=WardCampaign(&Hero);
    if(Game&&WardRescue104::Downed(*Game))return false;
    const FQuat View = Hero.GetControlRotation().Quaternion();
    const FVector Eye = Hero.Camera->GetComponentLocation();
    const auto Motion = WardMotion107::Frame(Hero);
    const float Movement = FMath::Clamp(Hero.MotionWeight52,0.f,1.45f);
    const float Sway = P.Grip == EGrip::Page || P.Grip == EGrip::Notebook ? .32f : .60f+.28f*P.Weight;
    FVector Rest = P.Rest + Hero.WeaponMotion52*Sway;
    auto& Hold=State(Actor);
    const double Now=Hero.GetWorld()->GetTimeSeconds();
    const double Delta=Now-Hold.LastTime;
    const FVector Speed=Hero.GetVelocity();
    if(Hold.LastTime<0 || Delta>.25 || FVector::DistSquared(Hold.PreviousEye,Eye)>FMath::Square(120.))
    {
        Hold.Offset=Hold.Velocity=FVector::ZeroVector;
    }
    else if(Delta>UE_SMALL_NUMBER)
    {
        const FVector Acceleration=View.UnrotateVector((Speed-Hold.PreviousSpeed)/Delta);
        const FVector Goal=(-Acceleration*(.0008+.0018*P.Weight)).GetClampedToMaxSize(1.4+2.6*P.Weight);
        // Closed-form critically damped spring in camera space, in centimetres.
        const double Frequency=22.-10.*P.Weight;
        const double Decay=FMath::Exp(-Frequency*Delta);
        const FVector Error=Hold.Offset-Goal;
        const FVector Drift=Hold.Velocity+Frequency*Error;
        Hold.Offset=Goal+(Error+Delta*Drift)*Decay;
        Hold.Velocity=(Hold.Velocity-Frequency*Delta*Drift)*Decay;
    }
    Hold.LastTime=Now;
    Hold.PreviousSpeed=Speed;
    Hold.PreviousEye=Eye;
    Rest+=Hold.Offset;
    Rest.Y -= Motion.Turn*(.35f+1.1f*P.Weight);
    Rest.Z += .30f*Motion.Breath-2.6f*Movement*P.Weight-24*(1-Raise);
    const float Pitch=FMath::Clamp(float(FRotator::NormalizeAxis(Hero.GetControlRotation().Pitch)), -68.f,68.f);
    // Leave the object in view while resisting full wrist rotation at the extremes of aim.
    const FRotator Angles(P.Angles.X+Hero.WeaponSway52.Pitch*Sway-.10f*Pitch,
        P.Angles.Y+Hero.WeaponSway52.Yaw*Sway,
        P.Angles.Z+Hero.WeaponSway52.Roll*Sway);
    const FQuat Rotation = View*Angles.Quaternion();
    const auto Box = FCollisionShape::MakeBox(P.Half + FVector(.65));
    FCollisionQueryParams Query(SCENE_QUERY_STAT(HeldObject108), false, &Hero);
    Query.AddIgnoredActor(&Actor);
    FHitResult Hit;
    const FVector Start = Eye + View.RotateVector(FVector(1,0,Rest.Z*.57));
    const FVector End = Eye + View.RotateVector(Rest);
    if (!Hero.GetWorld()->SweepSingleByChannel(Hit,Start,End,Rotation,ECC_Visibility,Box,Query))
    {
        Actor.SetActorLocationAndRotation(End,Rotation);
        return true;
    }
    // Retract toward the chest and lower the hands when the model's full bounds are obstructed.
    if (Hit.bStartPenetrating || Hit.Time < .44f)
    {
        const FVector Tucked = Eye + View.RotateVector(FVector(FMath::Max(P.Half.X+3,17.),Rest.Y*.6,Rest.Z-14));
        if (Hero.GetWorld()->LineTraceTestByChannel(Eye,Tucked,ECC_Visibility,Query) ||
            Hero.GetWorld()->OverlapBlockingTestByChannel(Tucked,Rotation,ECC_Visibility,Box,Query)) return false;
        Actor.SetActorLocationAndRotation(Tucked,Rotation);
        return true;
    }
    Actor.SetActorLocationAndRotation(Hit.Location + Hit.Normal*.4,Rotation);
    return true;
}
}

TArray<AEWardCampaignMode::FPreview51> AEWardCampaignMode::PreviewCatalog51()const
{
    TArray<FPreview51> Out;TSet<FString> Seen;
    auto Add=[&](FString Id,FString Title,FString Kind,FString Visual,bool Document=false){if(!Seen.Contains(Id)){Seen.Add(Id);Out.Add({Id,Title,Kind,Visual,Document});}};
    for(FString Id:{TEXT("weapon"),TEXT("shotgun"),TEXT("knife"),TEXT("flashlight"),TEXT("ammo"),TEXT("shells"),TEXT("medkit"),TEXT("battery"),TEXT("bottle"),TEXT("boards"),TEXT("fuse"),TEXT("badge"),TEXT("fuel"),TEXT("coolant")})
        Add(Id,Id==TEXT("knife")?TEXT("Нож"):Id==TEXT("flashlight")?TEXT("Фонарик"):ItemName(Id),TEXT("item"),Id==TEXT("weapon")?TEXT("equipped_pistol"):Id==TEXT("shotgun")?TEXT("equipped_shotgun"):TEXT("loose_")+Id);
    for(const auto& Pair:Visuals)if(Pair.Key.StartsWith(TEXT("loose_"))&&!Pair.Value.IsEmpty())Add(Pair.Key.RightChop(6),ItemName(Pair.Key.RightChop(6)),TEXT("item"),Pair.Key);
    if(HospitalStory)
    {
        const auto Docs=Object(HospitalStory->Narrative35,TEXT("documents"));
        for(const auto& Pair:Docs->Values){const FString Key(Pair.Key);Add(TEXT("doc:")+Key,HospitalStory->DocumentTitle(TEXT("@doc:")+Key),TEXT("note"),Key,true);}
        for(const auto& Pair:HospitalStory->Definitions)
        {
            const FString Kind=String(Pair.Value,TEXT("kind"));
            if(Kind==TEXT("note"))Add(TEXT("doc:")+Pair.Key,HospitalStory->DocumentTitle(TEXT("@doc:")+Pair.Key),Kind,Pair.Key,true);
            else if(TArray<FString>{TEXT("map"),TEXT("key"),TEXT("weapon"),TEXT("shotgun"),TEXT("medkit"),TEXT("ammo"),TEXT("shells")}.Contains(Kind))Add(Pair.Key,String(Pair.Value,TEXT("label")),Kind,Pair.Key);
        }
    }
    Out.Sort([](const FPreview51& A,const FPreview51& B){return A.Document==B.Document?A.Id<B.Id:!A.Document;});return Out;
}
void AEWardCampaignMode::ClearPreview51()
{
    if(PreviewActor51)PreviewActor51->Destroy();PreviewActor51=nullptr;PreviewItem51.Empty();PreviewDocument51.Empty();
}
bool AEWardCampaignMode::HoldPreview51(const FString& Id)
{
    if((!Admin&&HeldSlot76!=Id&&!CollectedDocument(*this,Id))||!Hero()||Hero()->Dead||
        (HospitalStory&&HospitalStory->ArrivalCinematic()))return false;
    FPreview51 Selected;bool Found=false;for(auto Entry:PreviewCatalog51())if(Entry.Id==Id){Selected=Entry;Found=true;break;}if(!Found)return false;
    ClearPreview51();PreviewItem51=Id==TEXT("weapon")||Selected.Kind==TEXT("weapon")?TEXT("weapon"):Id==TEXT("shotgun")||Selected.Kind==TEXT("shotgun")?TEXT("shotgun"):Id;AdminHands51=true;Agent=false;Hero()->StopInteract();Hero()->GetCharacterMovement()->StopMovementImmediately();
    if(auto* PC=Cast<APlayerController>(Hero()->Controller))PC->SetViewTarget(Hero());
    if(PreviewItem51!=TEXT("weapon")&&PreviewItem51!=TEXT("shotgun"))
    {
        auto* A=GetWorld()->SpawnActor<AActor>();PreviewActor51=A;A->SetOwner(Hero());A->SetActorEnableCollision(false);
        auto* Root=NewObject<USceneComponent>(A);A->SetRootComponent(Root);Root->RegisterComponent();
        TArray<UStaticMeshComponent*> Parts;FBox Bounds(ForceInit);
        auto Part=[&](UStaticMesh* Mesh,const FTransform& At,const TArray<UMaterialInterface*>& Materials)
        {
            if(!Mesh)return;auto* C=NewObject<UStaticMeshComponent>(A);C->SetupAttachment(Root);C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Mesh);C->SetRelativeTransform(At);C->bDisallowNanite=true;
            for(int32 I=0;I<Materials.Num();++I)C->SetMaterial(I,Materials[I]);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);C->SetLightingChannels(true,true,false);C->RegisterComponent();A->AddInstanceComponent(C);Parts.Add(C);Bounds+=Mesh->GetBoundingBox().TransformBy(At);
        };
        if(!Selected.Document)
        {
            const auto* Rows=Visuals.Find(Selected.Visual);
            if(Rows){for(auto Row:*Rows)if(Boolean(Row,TEXT("visible"),true))
            {
                TArray<UMaterialInterface*> Mats;for(auto V:Array(Row,TEXT("materials")))Mats.Add(LoadObject<UMaterialInterface>(nullptr,*MaterialPath(V->AsString())));
                Part(LoadObject<UStaticMesh>(nullptr,*MeshPath(String(Row,TEXT("mesh")))),Transform(Object(Row,TEXT("transform"))),Mats);
            }
            }else if(HospitalStory)if(auto* Source=HospitalStory->Props.FindRef(Selected.Visual))
            {
                TArray<UStaticMeshComponent*> Components;Source->GetComponents(Components);for(auto* C:Components)if(C->GetStaticMesh()&&C->IsVisible())Part(C->GetStaticMesh(),C->GetComponentTransform().GetRelativeTransform(Source->GetActorTransform()),C->GetMaterials());
            }
        }
        if(Parts.Num()&&Bounds.IsValid)
        {
            // Existing source meshes are normalized to a physically plausible handheld extent.
            const FVector Center=Bounds.GetCenter();
            static const TMap<FString,float> LengthCm={
                {TEXT("service_prybar"),66}, {TEXT("spare_fuse"),12}, {TEXT("evidence_case"),38},
                {TEXT("boards"),60}, {TEXT("fuel"),28}, {TEXT("coolant"),27}, {TEXT("bottle"),26},
                {TEXT("battery"),5.5}, {TEXT("fuse"),7}, {TEXT("badge"),8.6}, {TEXT("shells"),7.6},
                {TEXT("medkit"),26}, {TEXT("knife"),27}, {TEXT("flashlight"),22}};
            const float* Length=LengthCm.Find(Id);
            float Extent=Length?*Length:20.f;
            if(!Length && Selected.Kind==TEXT("map")) Extent=30;
            if(!Length && Selected.Kind==TEXT("key")) Extent=8.6f;
            const float Scale=Extent/FMath::Max3(Bounds.GetSize().X,Bounds.GetSize().Y,Bounds.GetSize().Z);
            for(auto* C:Parts){C->SetRelativeLocation((C->GetRelativeLocation()-Center)*Scale);C->SetRelativeScale3D(C->GetRelativeScale3D()*Scale);}
        }
        else
        {
            int Model=Selected.Document||Selected.Kind==TEXT("map")?262:Id==TEXT("knife")?561:Id==TEXT("flashlight")?551:Id==TEXT("battery")?552:Id==TEXT("bottle")?562:Id==TEXT("fuel")||Id==TEXT("coolant")?342:Selected.Kind==TEXT("key")?550:Id==TEXT("fuse")?152:Id==TEXT("medkit")?554:Id==TEXT("ammo")?556:Id==TEXT("shells")?560:292;
            FVector Half=Selected.Document?FVector(NoteHalf.Z,NoteHalf.Y,.075):Id==TEXT("knife")?FVector(.6,2,14):Id==TEXT("flashlight")?FVector(2.4,2.4,11.3):Id==TEXT("battery")?FVector(.8,.8,2.8):Id==TEXT("bottle")?FVector(3.5,3.5,13):Selected.Kind==TEXT("key")||Id==TEXT("badge")?FVector(4.3,2.7,.2):FVector(3,5,10);
            if(Id==TEXT("fuse")||Id==TEXT("spare_fuse"))Half=FVector(1.4,1.4,3.5);
            if(Id==TEXT("medkit"))Half=FVector(6.5,12.5,4.8);
            if(Id==TEXT("fuel")||Id==TEXT("coolant"))Half=FVector(5,10.5,13.5);
            if(Selected.Kind==TEXT("map")&&!Selected.Document)Half=FVector(.18,14,10);
            USceneComponent* Fit=Root;
            if(Selected.Document){Fit=NewObject<USceneComponent>(A);Fit->SetupAttachment(Root);Fit->SetRelativeRotation(FRotator(90,0,0));Fit->RegisterComponent();}
            if(!WardEchoAssets::Attach(A,Fit,Model,FBox(-Half,Half))){ClearPreview51();return false;}
        }
        if(Selected.Document)
        {
            const FString Entry=TEXT("@doc:")+Selected.Visual;
            AddNoteFace(*A,Root,Tr(HospitalStory->DocumentTitle(Entry)),Tr(HospitalStory->DocumentBody(Entry)));
            PreviewDocument51=Selected.Visual;
            if(!Admin){HeldSlot76.Empty();Flag(TEXT("hands_empty76"),true);}
        }
    }
    // Handheld inspection props use the same light separation as weapons.
    if (PreviewActor51)
    {
        TArray<UPrimitiveComponent*> Parts;PreviewActor51->GetComponents(Parts);
        for (auto* Part:Parts) Part->SetLightingChannels(false,true,false);
        FVector Half = Selected.Document ? NoteHalf
            : PreviewActor51->CalculateComponentsBoundingBoxInLocalSpace(true).GetExtent();
        if (Selected.Kind == TEXT("map") && Half.Z < FMath::Min(Half.X,Half.Y)*.33)
        {
            const FTransform Upright(FRotator(90,0,0));
            for (auto* Part : Parts)
                if (Part->GetAttachParent() == PreviewActor51->GetRootComponent())
                    Part->SetRelativeTransform(Part->GetRelativeTransform()*Upright);
            Swap(Half.X,Half.Z);
        }
        WardHeldGrip108::State(*PreviewActor51).Profile=WardHeldGrip108::Classify(Id, Selected.Kind, Half);
    }
    UpdatePreview51();
    Show(TEXT(""));
    Notify(Admin?TEXT("Осмотр: ")+Selected.Title+TEXT(" · F8 — каталог"):
        Selected.Title+(Selected.Document?TEXT(" · 0 — убрать · J — документы"):TEXT(" · 0 — убрать · I — использовать")),3);
    return true;
}
void AEWardCampaignMode::UpdatePreview51()
{
    if(!Hero()||!PreviewActor51)return;
    const bool Visible = (Admin ? AdminHands51 : !HeldSlot76.IsEmpty()||CollectedDocument(*this,PreviewItem51)) && !Hero()->Dead && !Hero()->IsHidden() &&
        !PhoneOpen76 && WardContact98::Pose(*Hero()).Reach<=.035f &&
        WardHeldGrip108::Place(*Hero(),*PreviewActor51,WardHeldGrip108::Preview(PreviewActor51));
    PreviewActor51->SetActorHiddenInGame(!Visible);
}
bool AEWardCampaignMode::PreviewCommand51(const FString& Id)
{
    if(Id==TEXT("page_title")||Id.StartsWith(TEXT("new_"))){ClearPreview51();AdminHands51=false;return false;}
    if(!Id.StartsWith(TEXT("preview_")))return false;
    if(!Admin){Notify(TEXT("Каталог доступен в режиме администратора"));return true;}
    if(Id==TEXT("preview_catalog")){Show(TEXT("admin_catalog"));return true;}
    if(Id==TEXT("preview_close")){Show(TEXT(""));return true;}
    if(Id==TEXT("preview_clear")){ClearPreview51();Show(TEXT(""));return true;}
    if(Id==TEXT("preview_fly")){ClearPreview51();AdminHands51=false;if(auto* PC=Cast<APlayerController>(Hero()->Controller))PC->SetViewTarget(AdminCamera);Show(TEXT(""));return true;}
    if(Id.StartsWith(TEXT("preview_hold_"))){if(!HoldPreview51(Id.RightChop(13)))Notify(TEXT("Не удалось открыть модель предмета"));return true;}
    if(Id.StartsWith(TEXT("preview_read_")))
    {
        const FString Key=Id.RightChop(13);bool Found=false;for(auto E:PreviewCatalog51())if(E.Document&&E.Visual==Key){Found=true;break;}
        if(Found){PreviewDocument51=Key;Show(TEXT("admin_document"));}return true;
    }
    return true;
}
