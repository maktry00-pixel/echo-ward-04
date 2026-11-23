#include "WardCampaignXray.h"
#include "WardCampaignUI.h"
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
class SWardCampaignXray : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SWardCampaignXray){}SLATE_ARGUMENT(FWardCampaignUI*,Interface)SLATE_END_ARGS()
    void Construct(const FArguments& A){UI=A._Interface;}
    virtual FVector2D ComputeDesiredSize(float)const override{return FVector2D(640,300);}
    virtual int32 OnPaint(const FPaintArgs&,const FGeometry& Geo,const FSlateRect&,FSlateWindowElementList& Draw,int32 Layer,const FWidgetStyle&,bool)const override
    {
        FSlateDrawElement::MakeBox(Draw,Layer,Geo.ToPaintGeometry(),FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")),ESlateDrawEffect::None,FLinearColor(.002f,.012f,.018f));
        const FVector2D Center=Geo.GetLocalSize()*.5;
        auto DrawImage=[&](bool Film)
        {
            const double Angle=Film?UI->FilmTurn*PI/2:0;const FLinearColor Color=Film?FLinearColor(.42f,.66f,.6f,.82f):FLinearColor(.025f,.18f,.19f,.8f);
            auto Position=[&](FVector2D P){return Center+FVector2D(P.X*FMath::Cos(Angle)-P.Y*FMath::Sin(Angle),P.X*FMath::Sin(Angle)+P.Y*FMath::Cos(Angle))+(Film?UI->FilmOffset:FVector2D::ZeroVector);};
            auto Line=[&](TArray<FVector2D> Points){for(auto& P:Points)P=Position(P);FSlateDrawElement::MakeLines(Draw,Layer+(Film?2:1),Geo.ToPaintGeometry(),Points,ESlateDrawEffect::None,Color,true,Film?2.f:1.f);};
            Line({FVector2D(0,-115),FVector2D(0,110)});
            for(int32 Rib=0;Rib<8;++Rib)for(double Side:{-1.,1.}){TArray<FVector2D> Points;for(int32 I=0;I<=22;++I){const double T=I/22.*PI*.93;Points.Add(FVector2D(Side*(77-Rib*3)*FMath::Sin(T),-88+Rib*22+18*(FMath::Cos(T)-1)));}Line(Points);}
            for(double Side:{-1.,1.}){TArray<FVector2D> Points;for(int32 I=0;I<=40;++I){const double T=I/40.*PI*2;Points.Add(FVector2D(Side*38+29*FMath::Sin(T),-5+80*FMath::Cos(T)));}Line(Points);}
            for(FVector2D At:{FVector2D(-108,-115),FVector2D(108,115)}){Line({At-FVector2D(9,0),At+FVector2D(9,0)});Line({At-FVector2D(0,9),At+FVector2D(0,9)});}
        };
        DrawImage(false);DrawImage(true);return Layer+3;
    }
    virtual FReply OnMouseButtonDown(const FGeometry& Geo,const FPointerEvent& Event)override{if(Event.GetEffectingButton()==EKeys::LeftMouseButton){Last=Geo.AbsoluteToLocal(Event.GetScreenSpacePosition());return FReply::Handled().CaptureMouse(SharedThis(this));}return FReply::Unhandled();}
    virtual FReply OnMouseMove(const FGeometry& Geo,const FPointerEvent& Event)override{if(HasMouseCapture()){const FVector2D Now=Geo.AbsoluteToLocal(Event.GetScreenSpacePosition());UI->FilmOffset+=Now-Last;Last=Now;Invalidate(EInvalidateWidgetReason::Paint);return FReply::Handled();}return FReply::Unhandled();}
    virtual FReply OnMouseButtonUp(const FGeometry&,const FPointerEvent&)override{return FReply::Handled().ReleaseMouseCapture();}
private:FWardCampaignUI* UI=nullptr;FVector2D Last;
};
TSharedRef<SWidget> MakeWardCampaignXray(FWardCampaignUI* Interface){return SNew(SWardCampaignXray).Interface(Interface);}
