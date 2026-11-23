#pragma once

class AEWardCampaignHero;

namespace WardMotion107
{
struct FFrame
{
    float Gait = 0;
    float Breath = 0;
    float Effort = 0;
    float Turn = 0;
    int Exhale = 0;
    bool Active = false;
};

FFrame Frame(const AEWardCampaignHero& Hero);
}
