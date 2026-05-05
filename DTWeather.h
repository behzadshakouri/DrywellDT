// DTWeather.h
#pragma once

#include <QDateTime>
#include <string>
#include "Precipitation.h"
class System;

namespace DTWeather
{
CPrecipitation fetchPrecipitation(
    const std::string &weatherSource,
    double             latitude,
    double             longitude,
    const QDateTime   &intervalStart,
    const QDateTime   &intervalEnd);

void injectPrecipitation(System *system, const CPrecipitation &precip);
}
