// DTWeather.cpp
#include "DTWeather.h"
#include "noaaweatherfetcher.h"
#include "System.h"
#include "Source.h"
#include <iostream>
#include "Quan.h"

namespace DTWeather
{

CPrecipitation fetchPrecipitation(const std::string &weatherSource,
                                  double             latitude,
                                  double             longitude,
                                  const QDateTime   &intervalStart,
                                  const QDateTime   &intervalEnd)
{
    NOAAWeatherFetcher fetcher;

    if (weatherSource == "openmeteo")
    {
        CPrecipitation precip = fetcher.getOpenMeteoPrecipitation(
            latitude, longitude, intervalStart, intervalEnd);
        if (precip.n == 0)
            std::cerr << "[Weather] Warning: "
                      << fetcher.lastError().toStdString() << "\n";
        return precip;
    }

    if (weatherSource == "openmeteo_historical")
    {
        CPrecipitation precip = fetcher.getOpenMeteoHistoricalPrecipitation(
            latitude, longitude, intervalStart, intervalEnd);
        if (precip.n == 0)
            std::cerr << "[Weather] Warning: "
                      << fetcher.lastError().toStdString() << "\n";
        return precip;
    }

    std::cerr << "[Weather] Unknown weather_source: '" << weatherSource
              << "' (expected 'openmeteo' or 'openmeteo_historical')\n";
    return CPrecipitation();
}

void injectPrecipitation(System *system, const CPrecipitation &precip)
{
    if (precip.n == 0)
    {
        std::cerr << "[Weather] injectPrecipitation: empty precipitation, skipping.\n";
        return;
    }

    Source *src = system->source("Rain");
    if (!src)
    {
        std::cerr << "[Weather] injectPrecipitation: source 'Rain' not found.\n";
        return;
    }

    auto *var = src->Variable("timeseries");
    if (!var)
    {
        std::cerr << "[Weather] injectPrecipitation: variable 'timeseries' not found on 'Rain'.\n";
        return;
    }

    var->SetTimeSeries(precip);
    std::cout << "[Weather] Injected " << precip.n
              << " precipitation bins into 'Rain' source.\n";
}

}   // namespace DTWeather
