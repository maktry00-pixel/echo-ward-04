#include "WardBookMap.h"
#include "WardCampaign.h"
#include "WardInterface66.h"
#include "WardHospitalGameplay.h"
#include "WardBookWorld.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/PlatformMemory.h"
using namespace WardData;
namespace
{
    Json LoadWardBookMap()
    {
        const bool Legacy=FParse::Param(FCommandLine::Get(),TEXT("WardLegacyCampaign"));
        Json Data=Read(FPaths::ProjectContentDir()/(Legacy?TEXT("CampaignData/book_map.json"):TEXT("CampaignData/hospital_world.json")));
        // These layers remain in the atlas for authoring. Runtime uses the merged dressing and navigation.
        for(const TCHAR* Key:{TEXT("clutter_dressing"),TEXT("microdetails_dressing"),TEXT("item_types_dressing"),TEXT("collision_features")})
            Data->RemoveField(Key);
        return Data;
    }
    Json& WardBookMapCache()
    {
        static Json Data=LoadWardBookMap();
        return Data;
    }
}
Json WardBookMapData(){return WardBookMapCache();}
void PrepareWardBookMapConstruction()
{
    if(Object(WardBookMapCache(),TEXT("environment_dressing"))->HasField(TEXT("runtime_box_count")))
        WardBookMapCache()=LoadWardBookMap();
}
void FinishWardBookMapConstruction()
{
    Json Environment=Object(WardBookMapCache(),TEXT("environment_dressing"));
    if(!Environment->HasField(TEXT("boxes")))return;
    const auto Before=FPlatformMemory::GetStats();
    Environment->SetNumberField(TEXT("runtime_box_count"),Array(Environment,TEXT("boxes")).Num());
    // All collision and render instances now own their transforms. Keep the
    // runtime layout, lighting and road data, and release the construction rows.
    Environment->RemoveField(TEXT("boxes"));
    FMemory::Trim();
    const auto After=FPlatformMemory::GetStats();
    UE_LOG(LogTemp,Display,TEXT("MAP71_MEMORY working_mib=%.0f->%.0f commit_mib=%.0f->%.0f"),
        Before.UsedPhysical/1048576.,After.UsedPhysical/1048576.,Before.UsedVirtual/1048576.,After.UsedVirtual/1048576.);
}
namespace
{
    FString LocalName(Json L,AEWardCampaignMode* G){return String(L,G&&G->Language==TEXT("en")?TEXT("name_en"):G&&G->Language==TEXT("kk")?TEXT("name_kk"):TEXT("name"));}
    class SWardBookMap : public SLeafWidget
    {
    public:
        SLATE_BEGIN_ARGS(SWardBookMap){}SLATE_ARGUMENT(AEWardCampaignMode*,Mode)SLATE_ARGUMENT(TSharedPtr<FWardBookMapState>,State)SLATE_ARGUMENT(TFunction<void(const FString&)>,Select)SLATE_END_ARGS()
        void Construct(const FArguments& A){Mode=A._Mode;State=A._State;Select=A._Select;SetClipping(EWidgetClipping::ClipToBounds);SetToolTipText(FText::GetEmpty());}
        virtual FVector2D ComputeDesiredSize(float)const override{return FVector2D(1600,620);}
        FVector2D Extent()const
        {
            if(State->Location==TEXT("all"))return FVector2D(574,68*FMath::DivideAndRoundUp(Array(WardBookMapData(),TEXT("locations")).Num(),7));
            for(auto V:Array(WardBookMapData(),TEXT("locations")))if(String(V->AsObject(),TEXT("id"))==State->Location){const auto& B=Array(V->AsObject(),TEXT("bounds"));if(B.Num()==4)return FVector2D(B[2]->AsNumber()+8,B[3]->AsNumber()+8);}
            return FVector2D(70,52);
        }
        double Scale(const FGeometry& G)const{const FVector2D S=G.GetLocalSize(),E=Extent();return FMath::Max(.01,FMath::Min((S.X-32)/E.X,(S.Y-32)/E.Y)*State->Zoom);}
        FVector2D Point(const FGeometry& G,FVector2D P)const{return (P-Extent()*.5)*Scale(G)+G.GetLocalSize()*.5+State->Pan;}
        FVector2D PlanPoint(const FGeometry& G,FVector2D P)const{return (P-G.GetLocalSize()*.5-State->Pan)/Scale(G)+Extent()*.5;}
        virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& D,int32 Layer,const FWidgetStyle&,bool)const override
        {
            State->PaintedRooms=0;State->PaintedLocations=0;State->PlayerMarkers66=0;State->GoalMarkers66=0;const auto* Brush=FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));const double S=Scale(G);const bool All=State->Location==TEXT("all");
            auto Box=[&](FVector2D P,FVector2D Size,FLinearColor Color,int L){FSlateDrawElement::MakeBox(D,L,G.ToPaintGeometry(Size*S,FSlateLayoutTransform(Point(G,P))),Brush,ESlateDrawEffect::None,Color);};
            auto Label=[&](FVector2D P,const FString& T,int Size,FLinearColor Color,int L){FSlateDrawElement::MakeText(D,L,G.ToPaintGeometry(FVector2D(2000,200),FSlateLayoutTransform(Point(G,P))),FText::FromString(T),FSlateFontInfo(FPaths::ProjectContentDir()/TEXT("CampaignData/Fonts/NotoSans.ttf"),Size),ESlateDrawEffect::None,Color);};
            int Index=0;for(auto V:Array(WardBookMapData(),TEXT("locations")))
            {
                Json L=V->AsObject();const FString Id=String(L,TEXT("id"));const int I=Index++;if(!All&&Id!=State->Location)continue;
                ++State->PaintedLocations;const FVector2D Base=All?FVector2D((I%7)*82+8,(I/7)*68+18):FVector2D(4,4);
                const auto& Bounds=Array(L,TEXT("bounds"));if(Bounds.Num()!=4)continue;const double Unit=All?62./Bounds[2]->AsNumber():1.;
                auto Position=[&](double X,double Y){return Base+FVector2D(X-Bounds[0]->AsNumber(),Y-Bounds[1]->AsNumber())*Unit;};
                if(All){Box(FVector2D((I%7)*82+2,(I/7)*68+2),FVector2D(78,64),FLinearColor(.008,.012,.013),Layer);const int Font=FMath::Clamp(int(S*1.9),9,16);FString Caption=Id+TEXT(" / ")+LocalName(L,Mode.Get());const int Limit=FMath::Max(5,int(72*S/(Font*.65)));if(Caption.Len()>Limit)Caption=Caption.Left(Limit-2)+TEXT("..");Label(FVector2D((I%7)*82+5,(I/7)*68+5),Caption,Font,FLinearColor(.6,.68,.66),Layer+4);}
                for(auto C:Array(L,TEXT("corridors"))){const auto& A=C->AsArray();if(A.Num()==4)Box(Position(A[0]->AsNumber(),A[1]->AsNumber()),FVector2D(A[2]->AsNumber(),A[3]->AsNumber())*Unit,FLinearColor(.027,.037,.038),Layer+1);}
                for(auto R:Array(L,TEXT("rooms")))
                {
                    Json Room=R->AsObject();if(Boolean(Room,TEXT("hidden"))&&Mode.IsValid()&&!Mode->Flag(TEXT("discovered_RH00")))continue;const auto& A=Array(Room,TEXT("rect"));if(A.Num()!=4)continue;++State->PaintedRooms;
                    const FVector2D P=Position(A[0]->AsNumber(),A[1]->AsNumber()),Size(A[2]->AsNumber()*Unit,A[3]->AsNumber()*Unit);
                    Box(P,Size,FLinearColor(.24,.29,.28),Layer+2);
                    const double Inset=.8/S;Box(P+FVector2D(Inset),Size-FVector2D(2*Inset),FLinearColor(.012,.019,.02),Layer+3);
                    const double PixelWidth=Size.X*S,PixelHeight=Size.Y*S;const int Font=FMath::Clamp(int(PixelWidth/10),9,16);
                    if(PixelWidth>36&&PixelHeight>24)Label(P+FVector2D(3/S),String(Room,TEXT("id")),Font,FLinearColor(.75,.8,.76),Layer+4);
                    if(!All&&PixelWidth>95&&PixelHeight>75){FString Name=Mode.IsValid()?Mode->Tr(String(Room,TEXT("name"))):String(Room,TEXT("name"));const int MaxChars=FMath::Max(6,int(PixelWidth/(Font*.62))-1);TArray<FString> Words;Name.ParseIntoArray(Words,TEXT(" "),true);FString Line;int Row=0;for(const FString& Word:Words){if((Line.Len()+Word.Len()+1)>MaxChars&&!Line.IsEmpty()){Label(P+FVector2D(3/S,(24+Row*18)/S),Line,Font-1,FLinearColor(.45,.53,.51),Layer+4);++Row;Line.Empty();if(Row>=2)break;}Line+=(Line.IsEmpty()?TEXT(""):TEXT(" "))+Word;}if(Row<2&&!Line.IsEmpty())Label(P+FVector2D(3/S,(24+Row*18)/S),Line,Font-1,FLinearColor(.45,.53,.51),Layer+4);}
                }
                if(!All)for(auto V2:Array(WardBookMapData(),TEXT("partitions"))){Json P=V2->AsObject();if(String(P,TEXT("site"))!=Id)continue;const FVector A=Vector(Array(P,TEXT("a"))),B=Vector(Array(P,TEXT("b"))),O=Vector(Array(L,TEXT("origin")));const double T=Number(P,TEXT("thickness"));Box(Position(FMath::Min(A.X,B.X)-O.X-T*.5,FMath::Min(A.Y,B.Y)-O.Y-T*.5),FVector2D(FMath::Abs(A.X-B.X)+T,FMath::Abs(A.Y-B.Y)+T),FLinearColor(.24,.32,.31),Layer+3);}
                for(auto C:Array(L,TEXT("cores"))){Json Core=C->AsObject();const auto& A=Array(Core,TEXT("rect"));if(A.Num()==4){const FVector2D P=Position(A[0]->AsNumber(),A[1]->AsNumber());Box(P,FVector2D(A[2]->AsNumber(),A[3]->AsNumber())*Unit,FLinearColor(.08,.11,.11),Layer+2);if(!All)Label(P+FVector2D(.4,2),String(Core,TEXT("name")),10,FLinearColor(.4,.49,.46),Layer+4);}}
                if(Mode.IsValid()&&Mode->BookWorld&&Mode->Hero())
                {
                    auto Marker=[&](FVector At,const FString& Site,bool Goal)
                    {
                        if(Site!=Id)return;const FVector O=Vector(Array(L,TEXT("origin")));const FVector2D P=Point(G,Position(At.X/100-O.X,At.Y/100-O.Y));const FLinearColor C=Goal?FLinearColor(.9,.12,.10):FLinearColor(.55,.9,.9);
                        TArray<FVector2f> Lines;if(Goal){Lines={{float(P.X),float(P.Y-7)},{float(P.X+7),float(P.Y)},{float(P.X),float(P.Y+7)},{float(P.X-7),float(P.Y)},{float(P.X),float(P.Y-7)}};++State->GoalMarkers66;}
                        else{Lines={{float(P.X),float(P.Y-8)},{float(P.X-6),float(P.Y+6)},{float(P.X+6),float(P.Y+6)},{float(P.X),float(P.Y-8)}};++State->PlayerMarkers66;}
                        FSlateDrawElement::MakeLines(D,Layer+7,G.ToPaintGeometry(),Lines,ESlateDrawEffect::None,C,true,2);
                    };
                    Marker(Mode->Hero()->GetActorLocation(),WardInterface66::Site(*Mode),false);
                    const FString GoalId=WardInterface66::ObjectiveId(*Mode);if(Mode->HospitalStory)if(auto* A=Mode->HospitalStory->Props.FindRef(GoalId))Marker(A->GetActorLocation(),Mode->BookWorld->LocationAt(A->GetActorLocation()+FVector(0,0,96)),true);
                }

            }
            return Layer+8;
        }
        virtual FReply OnMouseWheel(const FGeometry& G,const FPointerEvent& E)override
        {
            const FVector2D Mouse=G.AbsoluteToLocal(E.GetScreenSpacePosition()),Before=PlanPoint(G,Mouse);State->Zoom=FMath::Clamp(State->Zoom*(E.GetWheelDelta()>0?1.25:.8),1.,24.);State->Pan+=Mouse-Point(G,Before);if(State->Zoom==1)State->Pan=FVector2D::ZeroVector;Invalidate(EInvalidateWidgetReason::Paint);return FReply::Handled();
        }
        virtual FReply OnMouseButtonDown(const FGeometry&,const FPointerEvent& E)override{if(E.GetEffectingButton()!=EKeys::LeftMouseButton)return FReply::Unhandled();Down=E.GetScreenSpacePosition();Dragged=false;return FReply::Handled().CaptureMouse(SharedThis(this));}
        virtual FReply OnMouseMove(const FGeometry& G,const FPointerEvent& E)override
        {
            if(HasMouseCapture()){if(FVector2D::Distance(Down,E.GetScreenSpacePosition())>5)Dragged=true;if(Dragged)State->Pan+=G.AbsoluteToLocal(E.GetScreenSpacePosition())-G.AbsoluteToLocal(E.GetLastScreenSpacePosition());Invalidate(EInvalidateWidgetReason::Paint);return FReply::Handled();}
            const FVector2D P=PlanPoint(G,G.AbsoluteToLocal(E.GetScreenSpacePosition()));FString Tip;State->HoveredRoom.Empty();int I=0;for(auto V:Array(WardBookMapData(),TEXT("locations"))){Json L=V->AsObject();const int N=I++;if(State->Location!=TEXT("all")&&String(L,TEXT("id"))!=State->Location)continue;const auto& B=Array(L,TEXT("bounds"));const double Unit=State->Location==TEXT("all")?62./B[2]->AsNumber():1.;const FVector2D Base=State->Location==TEXT("all")?FVector2D((N%7)*82+8,(N/7)*68+18):FVector2D(4,4);const FVector2D Q=(P-Base)/Unit+FVector2D(B[0]->AsNumber(),B[1]->AsNumber());for(auto R:Array(L,TEXT("rooms"))){Json Room=R->AsObject();if(Boolean(Room,TEXT("hidden"))&&Mode.IsValid()&&!Mode->Flag(TEXT("discovered_RH00")))continue;const auto& A=Array(Room,TEXT("rect"));if(Q.X>=A[0]->AsNumber()&&Q.Y>=A[1]->AsNumber()&&Q.X<=A[0]->AsNumber()+A[2]->AsNumber()&&Q.Y<=A[1]->AsNumber()+A[3]->AsNumber()){State->HoveredRoom=String(Room,TEXT("id"));Tip=String(Room,TEXT("id"))+TEXT(" / ")+(Mode.IsValid()?Mode->Tr(String(Room,TEXT("name"))):String(Room,TEXT("name")))+FString::Printf(TEXT(" / %.0f x %.0f m"),A[2]->AsNumber(),A[3]->AsNumber());break;}}}SetToolTipText(FText::FromString(Tip));return FReply::Unhandled();
        }
        virtual FReply OnMouseButtonUp(const FGeometry& G,const FPointerEvent& E)override
        {
            if(E.GetEffectingButton()!=EKeys::LeftMouseButton)return FReply::Unhandled();if(!Dragged&&State->Location==TEXT("all")){const FVector2D P=PlanPoint(G,G.AbsoluteToLocal(E.GetScreenSpacePosition()));const int X=FMath::FloorToInt(P.X/82),Y=FMath::FloorToInt(P.Y/68),I=Y*7+X;const auto& L=Array(WardBookMapData(),TEXT("locations"));if(X>=0&&X<7&&Y>=0&&L.IsValidIndex(I))Select(String(L[I]->AsObject(),TEXT("id")));}return FReply::Handled().ReleaseMouseCapture();
        }
        virtual FReply OnMouseButtonDoubleClick(const FGeometry&,const FPointerEvent&)override{State->Reset();Invalidate(EInvalidateWidgetReason::Paint);return FReply::Handled();}
    private:TWeakObjectPtr<AEWardCampaignMode> Mode;TSharedPtr<FWardBookMapState> State;TFunction<void(const FString&)> Select;FVector2D Down;bool Dragged=false;
    };
}
TSharedRef<SWidget> MakeWardBookMap(AEWardCampaignMode* Mode,TSharedRef<FWardBookMapState> State,TFunction<void(const FString&)> Select){return SNew(SWardBookMap).Mode(Mode).State(State).Select(Select);}
