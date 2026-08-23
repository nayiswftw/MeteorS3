#pragma once

/*
 * Weather intelligence engine.
 *
 * Pure functions that derive insights and alerts from raw weather
 * and air-quality data.  No global state access — everything is
 * passed in and written out through parameters.
 */

#include "core/Types.h"

namespace weather {

/// Analyse the forecast to find notable upcoming conditions.
void rebuildInsights(const WeatherData& w,
                     const AirData&     a,
                     InsightData&       out);

/// Generate a prioritised alert list from current conditions.
/// Returns the number of alerts written (up to MAX_ALERTS).
int  rebuildAlerts(const WeatherData& w,
                   const AirData&     a,
                   AlertItem*         out,
                   int                maxAlerts);

}  // namespace weather
