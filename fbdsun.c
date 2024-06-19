#include <math.h>
#include <time.h>
#include "fbdsun.h"

#define RADIANS(x) ((x) * 0.0174532925f)
#define DEGREES(x) ((x) * 57.2957795f)

/**
 * @brief Расчёт угла возвышения солнца над горизонтом
 * This is a function that calculates the sun position (altitude) at a given location (latitude and longitude) and time (Unix timestamp).
 * It uses the solar coordinate algorithm from the U.S. Naval Observatory Astronomical Applications Department.
 * @param lat Широта
 * @param lon Долгота
 * @param unix Текущее время
 * @return tSignal Угол над горизонтом в сотых долях градуса. Положительное значение - солнце над горизонтом, отрицательное значение - солнце под горизонтом.
 */
tSignal sunPosition(float lat, float lon, time_t unix) {
    lat = RADIANS(lat);
    float hours = (unix % 86400ul) / 86400.0f;                                                                                      // decimal hours
    float JulCent = (unix / 86400.0f - 10957.5f) / 36525.0f;                                                                        // Julian Century
    //
    float GeomMeanLong = RADIANS(fmodf(280.46646f + JulCent * (36000.76983f + JulCent * 0.0003032f), 360));                         // Geom Mean Long Sun
    float GeomMeanAnom = RADIANS(357.52911f + JulCent * (36000.0f - 0.0001537f * JulCent));                                         // Geom Mean Anom Sun
    //
    float EccEart = 0.016708634f - JulCent * (0.000042037f + 0.0000001267f * JulCent);                                              // Eccent Earth Orbit
    float SunEqCtr = RADIANS(sinf(GeomMeanAnom) * (1.914602f - JulCent * (0.004817f + 0.000014f * JulCent)) + sinf(2 * GeomMeanAnom) * (0.019993f - 0.000101f * JulCent) + sinf(3 * GeomMeanAnom) * 0.000289f); // Sun Eq of Ctr
    float SunApp = GeomMeanLong + SunEqCtr - 0.0001f - 0.00008f * sinf(RADIANS(125.04f - 1934.136f * JulCent));                     // Sun App Long
    float MeanOblq = 23 + (26 + ((21.448f - JulCent * (46.815f + JulCent * (0.00059f - JulCent * 0.001813f)))) / 60.0f) / 60.0f;    // Mean Obliq Ecliptic
    float ObliqCor = RADIANS(MeanOblq + 0.00256f * cosf(RADIANS(125.04f - 1934.136f * JulCent)));                                   // Obliq Corr
    float decl = asinf(sinf(ObliqCor) * sinf(SunApp));                                                                              // Sun Declin
    float y = tanf(ObliqCor / 2) * tanf(ObliqCor / 2);
    float eqTime = 4 * DEGREES(y * sinf(2 * GeomMeanLong) - 2 * EccEart * sinf(GeomMeanAnom) + 4 * EccEart * y * sinf(GeomMeanAnom) * cosf(2 * GeomMeanLong) - 0.5f * y * y * sinf(4 * GeomMeanLong) - 1.25f * EccEart * EccEart * sinf(2 * GeomMeanAnom));   // Eq of Time (minutes)
    float hrAngl = fmodf(hours * 1440 + eqTime + 4 * lon, 1440) / 4;                                                                // True Solar Time (min)
    hrAngl = hrAngl + (hrAngl < 0 ? 180 : -180);                                                                                    // Hour Angle
    float zen = acosf(sinf(lat) * sinf(decl) + cosf(lat) * cosf(decl) * cosf(RADIANS(hrAngl)));                                     // Zenith
    //
    return lroundf((90 - DEGREES(zen))*100);
}
