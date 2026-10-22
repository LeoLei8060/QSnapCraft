#pragma once

#include <QtGlobal>

#include <array>

inline constexpr std::array<qreal, 10> penSizeSteps()
{
    return {1.0, 2.0, 3.0, 5.0, 8.0, 12.0, 20.0, 32.0, 48.0, 64.0};
}
