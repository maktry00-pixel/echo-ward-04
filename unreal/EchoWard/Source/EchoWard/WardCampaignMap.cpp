#include "WardCampaignMap.h"
#include "WardCampaign.h"
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Input/Reply.h"
#include "Misc/Paths.h"
using namespace WardData;
class SWardCampaignMap : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SWardCampaignMap){}SLATE_ARGUMENT(AEWardCampaignMode*,Mode)SLATE_END_ARGS()
    void Construct(const FArguments& A){Mode=A._Mode;SetClipping(EWidgetClipping::ClipToBounds);}
    virtual FVector2D ComputeDesiredSize(float)const override{return FVector2D(1000,600);}
    virtual int32 OnPaint(const FPaintArgs& Args,const FGeometry& Geo,const FSlateRect& Clip,FSlateWindowElementList& Draw,int32 Layer,const FWidgetStyle& Style,bool Enabled)const override
    {
        auto* G=Mode.Get();if(!G)return Layer;const int32 Floor=G->Values.FindRef(TEXT("map_floor"));const FVector2D Size=Geo.GetLocalSize();
        const bool Archive=G->Values.FindRef(TEXT("map_area"))==2;
        FString Sector;for(const FString& Prefix:TArray<FString>{TEXT("annex_"),TEXT("iso_"),TEXT("diag_"),TEXT("res_")})if(G->CurrentRoom.StartsWith(Prefix))Sector=Prefix;
        auto Visible=[&](Json R){const FString Id=String(R,TEXT("id"));const int Level=Number(R,TEXT("level"));if(Level!=-2&&Level!=Floor)return false;if(Floor==-1)return true;if(Archive)return Id==TEXT("archive_w")||Id==TEXT("archive_e")||Id==TEXT("corridor")||Id==TEXT("ward04")||Id==TEXT("ward05")||Id.StartsWith(TEXT("isolation_"));if(!Sector.IsEmpty())return Id.StartsWith(Sector);for(const FString& P:TArray<FString>{TEXT("annex_"),TEXT("iso_"),TEXT("diag_"),TEXT("res_")})if(Id.StartsWith(P))return false;return true;};
        FVector2D Min(10000,10000),Max(-10000,-10000);
        for(const auto& V:Array(G->Data,TEXT("rooms"))){Json R=V->AsObject();auto& A=Array(R,TEXT("rect"));if(A.Num()!=4||!Visible(R))continue;Min.X=FMath::Min(Min.X,A[0]->AsNumber());Min.Y=FMath::Min(Min.Y,A[1]->AsNumber());Max.X=FMath::Max(Max.X,A[0]->AsNumber()+A[2]->AsNumber());Max.Y=FMath::Max(Max.Y,A[1]->AsNumber()+A[3]->AsNumber());}
        const double Scale=FMath::Min((Size.X-50)/FMath::Max(1.,Max.X-Min.X),(Size.Y-50)/FMath::Max(1.,Max.Y-Min.Y))*Zoom;
        FVector2D Center=(Min+Max)*.5;
        auto Point=[&](FVector2D P){return (P-Center)*Scale+Size*.5+Pan;};
        const FSlateBrush* Brush=FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));const FSlateFontInfo Font(FPaths::ProjectContentDir()/TEXT("CampaignData/Fonts/NotoSans.ttf"),12);
        for(const auto& V:Array(G->Data,TEXT("rooms")))
        {
            Json R=V->AsObject();auto& A=Array(R,TEXT("rect"));if(A.Num()!=4||!Visible(R))continue;
            const FString Id=String(R,TEXT("id"));const bool Seen=G->Visited.Contains(Id);const FVector2D Position=Point(FVector2D(A[0]->AsNumber(),A[1]->AsNumber())),Dimensions(FMath::Max(2.,A[2]->AsNumber()*Scale-1),FMath::Max(2.,A[3]->AsNumber()*Scale-1));
            FSlateDrawElement::MakeBox(Draw,Layer,Geo.ToPaintGeometry(Dimensions,FSlateLayoutTransform(Position)),Brush,ESlateDrawEffect::None,FLinearColor(.22,.25,.23));
            FSlateDrawElement::MakeBox(Draw,Layer+1,Geo.ToPaintGeometry(Dimensions-FVector2D(3,3),FSlateLayoutTransform(Position+FVector2D(1.5,1.5))),Brush,ESlateDrawEffect::None,Id==G->CurrentRoom?FLinearColor(.24f,.025f,.03f):Seen?FLinearColor(.065f,.075f,.07f):FLinearColor(.012f,.016f,.015f));
            if(Seen&&Dimensions.X>50&&Dimensions.Y>24)FSlateDrawElement::MakeText(Draw,Layer+1,Geo.ToPaintGeometry(Dimensions,FSlateLayoutTransform(Position+FVector2D(4,4))),FText::FromString(G->Tr(Id==TEXT("ward04")?TEXT("04"):Id==TEXT("ward05")?TEXT("05"):String(R,TEXT("name")))),Font,ESlateDrawEffect::None,FLinearColor(.65f,.77f,.70f));
        }
        if(G->Hero()&&(G->Hero()->GetActorLocation().Z<0?-1:0)==Floor)
        {
            const FVector P=G->Hero()->GetActorLocation();const FVector2D At=Point(FVector2D(P.Y/100,-P.X/100));
            TArray<FVector2f> Triangle={FVector2f(At.X,At.Y-10),FVector2f(At.X-7,At.Y+7),FVector2f(At.X+7,At.Y+7),FVector2f(At.X,At.Y-10)};FSlateDrawElement::MakeLines(Draw,Layer+5,Geo.ToPaintGeometry(),Triangle,ESlateDrawEffect::None,FLinearColor(.85,.88,.8),true,3);
        }
        for(const auto& E:G->Objects)
        {
            auto* O=E.Value.Get();if(!O)continue;const bool Goal=E.Key==G->Goal(),Save=O->ObjectKind==TEXT("save"),Door=O->ObjectKind==TEXT("door");if(!Goal&&!Door&&(!Save||!G->AutoVisited.Contains(E.Key)))continue;const FVector P=O->GetActorLocation();if((P.Z<0?-1:0)!=Floor)continue;
            const FVector2D MapPoint(P.Y/100,-P.X/100);if(MapPoint.X<Min.X||MapPoint.X>Max.X||MapPoint.Y<Min.Y||MapPoint.Y>Max.Y)continue;bool InRoom=false;for(auto V:Array(G->Data,TEXT("rooms"))){Json R=V->AsObject();if(!Visible(R))continue;const auto& A=Array(R,TEXT("rect"));if(A.Num()==4&&MapPoint.X>=A[0]->AsNumber()-.2&&MapPoint.Y>=A[1]->AsNumber()-.2&&MapPoint.X<=A[0]->AsNumber()+A[2]->AsNumber()+.2&&MapPoint.Y<=A[1]->AsNumber()+A[3]->AsNumber()+.2)InRoom=true;}if(!InRoom)continue;const FVector2D At=Point(MapPoint);
            if(Door){FSlateDrawElement::MakeBox(Draw,Layer+2,Geo.ToPaintGeometry(FVector2D(12,3),FSlateLayoutTransform(At-FVector2D(6,1.5))),Brush,ESlateDrawEffect::None,O->Locked?FLinearColor(.7,.035,.035):FLinearColor(.52,.59,.53));}
            if(Goal||Save){TArray<FVector2f> Diamond={FVector2f(At.X,At.Y-7),FVector2f(At.X+7,At.Y),FVector2f(At.X,At.Y+7),FVector2f(At.X-7,At.Y),FVector2f(At.X,At.Y-7)};FSlateDrawElement::MakeLines(Draw,Layer+4,Geo.ToPaintGeometry(),Diamond,ESlateDrawEffect::None,Goal?FLinearColor(.9f,.06f,.07f):FLinearColor(.5f,.57f,.52f),true,2);}
        }
        return Layer+6;
    }
    virtual FReply OnMouseWheel(const FGeometry&,const FPointerEvent& E)override{Zoom=FMath::Clamp(Zoom*(E.GetWheelDelta()>0?1.2:1/1.2),1.,12.);Invalidate(EInvalidateWidgetReason::Paint);return FReply::Handled();}
    virtual FReply OnMouseButtonDown(const FGeometry&,const FPointerEvent& E)override{if(E.GetEffectingButton()==EKeys::LeftMouseButton)return FReply::Handled().CaptureMouse(SharedThis(this));return FReply::Unhandled();}
    virtual FReply OnMouseMove(const FGeometry& Geo,const FPointerEvent& E)override{if(HasMouseCapture()){Pan+=Geo.AbsoluteToLocal(E.GetScreenSpacePosition())-Geo.AbsoluteToLocal(E.GetLastScreenSpacePosition());Invalidate(EInvalidateWidgetReason::Paint);return FReply::Handled();}return FReply::Unhandled();}
    virtual FReply OnMouseButtonUp(const FGeometry&,const FPointerEvent&)override{return FReply::Handled().ReleaseMouseCapture();}
private:TWeakObjectPtr<AEWardCampaignMode> Mode;double Zoom=1;FVector2D Pan=FVector2D::ZeroVector;
};
TSharedRef<SWidget> MakeWardCampaignMap(AEWardCampaignMode* Mode){return SNew(SWardCampaignMap).Mode(Mode);}
