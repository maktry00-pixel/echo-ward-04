#pragma once

#include <algorithm>
#include <cmath>

namespace WardMotion107
{
struct FCycle
{
    float Phase;
    bool Contact;
};

// Both the step sound and pose consume this cycle. The period stays continuous
// when the player changes speed, stance or animation blend.
inline FCycle AdvanceCycle(float Phase, float Travel, float Marker, float ContactsPerCycle)
{
    const float End = Phase + Travel;
    const auto Count = [=](float Value) { return std::floor((Value - Marker) * ContactsPerCycle); };
    return {End - std::floor(End), Count(End) > Count(Phase)};
}

inline float Follow(float Value, float Goal, float Seconds, float Rate)
{
    return Goal + (Value - Goal) * std::exp(-Seconds * Rate);
}

inline float Ease(float Start, float End, float Value)
{
    const float T = std::clamp((Value - Start) / (End - Start), 0.f, 1.f);
    return T * T * (3.f - 2.f * T);
}

inline float DoorSwing(float Age)
{
    return Ease(.15f, .56f, Age);
}

inline float LatchDegrees(float Age, bool Opening)
{
    const float Close = Ease(.36f, .48f, Age) * (1.f - Ease(.48f, .56f, Age));
    const float Open = Ease(.075f, .155f, Age) * (1.f - Ease(.25f, .43f, Age));
    return Opening ? 29.f * Open : 12.f * Close;
}
}
