#pragma once
#include <algorithm>
inline constexpr float CardBaseWidth=126.f;
inline constexpr float CardBaseHeight=86.f;
inline float CardWidth(int size=100){return CardBaseWidth*std::clamp(size,60,140)/100.f;}
inline float CardHeight(int size=100){return CardBaseHeight*std::clamp(size,60,140)/100.f;}
