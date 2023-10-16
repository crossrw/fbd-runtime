#include <math.h>
#include <time.h>
#include "fbdsun.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif // M_PI

#define RADIANS(x) ((x) * M_PI / 180.0)
#define DEGREES(x) ((x) * 180.0 / M_PI)

/**
 * @brief Расчёт угла возвышения солнца над горизонтом
 * 
 * @param lat Широта
 * @param lon Долгота
 * @param unix Текущее время
 * @param gmt Смещение временной зоны в минутах
 * @return tSignal Угол над горизонтом в сотых долях градуса. Положительное значение - солнце над горизонтом, отрицательное значение - солнце под горизонтом.
 */
tSignal sunPosition(float lat, float lon, time_t unix, int gmt) {
    lat = RADIANS(lat);
    float hours = (unix % 86400ul) / 86400.0;       // decimal hours
    float JulCent = (unix / 86400.0 - 10957.5) / 36525.0;                                                                   // Julian Century
    //
    float GeomMeanLong = RADIANS(fmod(280.46646 + JulCent * (36000.76983 + JulCent * 0.0003032), 360));                     // Geom Mean Long Sun
    float GeomMeanAnom = RADIANS(357.52911 + JulCent * (36000 - 0.0001537 * JulCent));                                      // Geom Mean Anom Sun
    //
    float EccEart = 0.016708634 - JulCent * (0.000042037 + 0.0000001267 * JulCent);                                         // Eccent Earth Orbit
    float SunEqCtr = RADIANS(sin(GeomMeanAnom) * (1.914602 - JulCent * (0.004817 + 0.000014 * JulCent)) + sin(2 * GeomMeanAnom) * (0.019993 - 0.000101 * JulCent) + sin(3 * GeomMeanAnom) * 0.000289); // Sun Eq of Ctr
    float SunApp = GeomMeanLong + SunEqCtr - 0.0001 - 0.00008 * sin(RADIANS(125.04 - 1934.136 * JulCent));                  // Sun App Long
    float MeanOblq = 23 + (26 + ((21.448 - JulCent * (46.815 + JulCent * (0.00059 - JulCent * 0.001813)))) / 60.0) / 60.0;  // Mean Obliq Ecliptic
    float ObliqCor = RADIANS(MeanOblq + 0.00256 * cos(RADIANS(125.04 - 1934.136 * JulCent)));                               // Obliq Corr
    float decl = asin(sin(ObliqCor) * sin(SunApp));                                                                         // Sun Declin
    float y = tan(ObliqCor / 2) * tan(ObliqCor / 2);
    float eqTime = 4 * DEGREES(y * sin(2 * GeomMeanLong) - 2 * EccEart * sin(GeomMeanAnom) + 4 * EccEart * y * sin(GeomMeanAnom) * cos(2 * GeomMeanLong) - 0.5 * y * y * sin(4 * GeomMeanLong) - 1.25 * EccEart * EccEart * sin(2 * GeomMeanAnom));   // Eq of Time (minutes)
    float hrAngl = fmod(hours * 1440 + eqTime + 4 * lon, 1440) / 4;                                                         // True Solar Time (min)
    hrAngl = hrAngl + (hrAngl < 0 ? 180 : -180);                                                                            // Hour Angle
    float zen = acos(sin(lat) * sin(decl) + cos(lat) * cos(decl) * cos(RADIANS(hrAngl)));                                   // Zenith
    //
    return lroundf((90 - DEGREES(zen))*100);
}
