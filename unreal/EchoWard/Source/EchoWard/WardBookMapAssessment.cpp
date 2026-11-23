#include "WardCampaign.h"
#include "WardCampaignUI.h"
#include "WardBookMap.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
using namespace WardData;
void AEWardCampaignMode::TickBookMapAssessment(float Delta)
{
    static int Step=0,LocationIndex=0;static double Since=FPlatformTime::Seconds();static TArray<TSharedPtr<FJsonValue>> Checks;static Json Before;
    auto* PC=UGameplayStatics::GetPlayerController(this,0);const double T=FPlatformTime::Seconds()-Since;
    auto Next=[&](){++Step;Since=FPlatformTime::Seconds();};auto Check=[&](FString Name,bool Pass){Json C=MakeShared<FJsonObject>();C->SetStringField(TEXT("name"),Name);C->SetBoolField(TEXT("passed"),Pass);Checks.Add(MakeShared<FJsonValueObject>(C));};
    auto Shot=[&](FString Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Prototype/BookMapCaptures/")+Name+TEXT(".png"),true,false);};
    const auto& Locations=Array(WardBookMapData(),TEXT("locations"));
    if(Step==0){NewGame(TEXT("normal"));Show(TEXT(""));Before=Capture();Command(TEXT("page_map"));Language=TEXT("ru");Interface->Refresh();Next();}
    else if(Step==1&&T>3)
    {
        int Rooms=0;TSet<FString> Ids;bool Clean=true,Bounds=true;for(auto V:Locations)for(auto R:Array(V->AsObject(),TEXT("rooms"))){Json Room=R->AsObject();++Rooms;Ids.Add(String(Room,TEXT("id")));Clean&=Room->Values.Num()==3&&!Room->HasField(TEXT("items"));const auto& A=Array(Room,TEXT("rect"));Bounds&=A.Num()==4&&A[2]->AsNumber()>0&&A[3]->AsNumber()>0;}
        Check(TEXT("book_contains_13_locations_and_328_unique_rooms"),Locations.Num()==13&&Rooms==328&&Ids.Num()==328);Check(TEXT("presentation_data_contains_only_room_geometry"),Clean&&Boolean(WardBookMapData(),TEXT("presentation_only"))&&Bounds);Check(TEXT("M_opens_complete_map_without_quest_item"),Interface->BookMapVisible&&!Flag(TEXT("map"))&&Screen==TEXT("map")&&PC->IsPaused());Check(TEXT("all_328_rooms_render_in_overview"),Interface->BookMapState&&Interface->BookMapState->PaintedRooms==328&&Interface->BookMapState->PaintedLocations==13);Shot(TEXT("book_all_1080"));Next();
    }
    else if(Step==2&&T>1){Command(TEXT("field_atlas_")+String(Locations[LocationIndex]->AsObject(),TEXT("id")));Next();}
    else if(Step==3&&T>1.5)
    {
        Json L=Locations[LocationIndex]->AsObject();const FString Id=String(L,TEXT("id"));Check(TEXT("location_draws_all_rooms_")+Id,Interface->BookMapState->Location==Id&&Interface->BookMapState->PaintedRooms==Array(L,TEXT("rooms")).Num()&&Interface->BookMapState->PaintedLocations==1);Shot(TEXT("book_")+Id+TEXT("_1080"));Next();
    }
    else if(Step==4&&T>.7){++LocationIndex;if(LocationIndex<Locations.Num()){Step=2;Since=FPlatformTime::Seconds();}else Next();}
    else if(Step==5&&T>1){Command(TEXT("field_atlas_all"));Next();}
    else if(Step==6&&T>1)
    {
        auto W=Interface->TitleActions[TEXT("atlas_view")].Pin();const FGeometry G=W->GetCachedGeometry();const FVector2D Size=G.GetLocalSize();const double S=FMath::Min((Size.X-32)/574.,(Size.Y-32)/136.);const FVector2D At=G.LocalToAbsolute((FVector2D(35,35)-FVector2D(574,136)*.5)*S+Size*.5);const FPointerEvent Click(0,At,At,TSet<FKey>(),EKeys::LeftMouseButton,0,FModifierKeysState());W->OnMouseButtonDown(G,Click);W->OnMouseButtonUp(G,Click);Check(TEXT("overview_click_opens_first_floor"),Interface->BookMapState->Location==TEXT("1F"));Step=60;Since=FPlatformTime::Seconds();
    }
    else if(Step==60&&T>1)
    {
        auto W=Interface->TitleActions[TEXT("atlas_view")].Pin();const FGeometry G=W->GetCachedGeometry();const FVector2D Size=G.GetLocalSize();const double S=FMath::Min((Size.X-32)/70.,(Size.Y-32)/52.);const FVector2D Room=G.LocalToAbsolute((FVector2D(6,6)-FVector2D(70,52)*.5)*S+Size*.5);const FPointerEvent Hover(0,Room,Room,TSet<FKey>(),EKeys::Invalid,0,FModifierKeysState());W->OnMouseMove(G,Hover);Check(TEXT("native_hover_identifies_room_R01"),Interface->BookMapState->HoveredRoom==TEXT("R01"));const FPointerEvent Wheel(0,Room,Room,TSet<FKey>(),EKeys::Invalid,1,FModifierKeysState());W->OnMouseWheel(G,Wheel);Check(TEXT("native_mouse_wheel_zooms_at_cursor"),Interface->BookMapState->Zoom>1&&!Interface->BookMapState->Pan.IsNearlyZero());const FPointerEvent Down(0,Room,Room,TSet<FKey>{EKeys::LeftMouseButton},EKeys::LeftMouseButton,0,FModifierKeysState());FReply Reply=W->OnMouseButtonDown(G,Down);Check(TEXT("left_drag_requests_mouse_capture"),Reply.GetMouseCaptor().IsValid());Command(TEXT("field_atlas_reset"));Check(TEXT("reset_restores_scale_and_position"),Interface->BookMapState->Zoom==1&&Interface->BookMapState->Pan.IsNearlyZero());Command(TEXT("field_atlas_zoom_in"));Check(TEXT("zoom_button_changes_scale"),Interface->BookMapState->Zoom>1);Command(TEXT("field_atlas_reset"));Step=7;Since=FPlatformTime::Seconds();
    }
    else if(Step==7&&T>1){Command(TEXT("field_map_current"));Check(TEXT("legacy_live_map_still_available"),!Interface->BookMapVisible&&!Flag(TEXT("map")));Command(TEXT("field_map_book"));Command(TEXT("field_atlas_1F"));Language=TEXT("en");Interface->Refresh();Next();}
    else if(Step==8&&T>1.5){Shot(TEXT("book_en_1080"));Next();}
    else if(Step==9&&T>.7){Language=TEXT("kk");Interface->Refresh();Next();}
    else if(Step==10&&T>1.5){Shot(TEXT("book_kk_1080"));Next();}
    else if(Step==11&&T>.7){Language=TEXT("ru");Interface->Refresh();FSystemResolution::RequestResolutionChange(1280,720,EWindowMode::Windowed);Next();}
    else if(Step==12&&T>2){Check(TEXT("720p_map_contains_full_first_floor"),Interface->BookMapState->PaintedRooms==32);Shot(TEXT("book_720"));Next();}
    else if(Step==13&&T>.7){Command(TEXT("field_close"));Json After=Capture();Check(TEXT("viewing_book_does_not_change_legacy_world"),Array(Data,TEXT("rooms")).Num()==54&&Before->GetObjectField(TEXT("flags"))->Values.Num()==After->GetObjectField(TEXT("flags"))->Values.Num()&&Items.FindRef(TEXT("bottle"))==3&&Journal.IsEmpty());Check(TEXT("closing_restores_game"),Screen.IsEmpty()&&!PC->IsPaused());bool Pass=true;for(auto V:Checks)Pass&=Boolean(V->AsObject(),TEXT("passed"));Json R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("success"),Pass);R->SetNumberField(TEXT("count"),Checks.Num());R->SetArrayField(TEXT("checks"),Checks);Write(FPaths::ProjectSavedDir()/TEXT("Prototype/book_map_check.json"),R);FPlatformMisc::RequestExit(false);Next();}
}
