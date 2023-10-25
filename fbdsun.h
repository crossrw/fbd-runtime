#ifndef FBDRTSUN_H
#define	FBDRTSUN_H

#include <time.h>
#include "fbdrt.h"

/**
 * @brief Расчёт угла возвышения солнца над горизонтом
 * 
 * @param lat Широта
 * @param lon Долгота
 * @param unix Текущее время
 * @param gmt Смещение временной зоны в минутах
 * @return tSignal Угол над горизонтом в сотых долях градуса. Положительное значение - солнце над горизонтом, отрицательное значение - солнце под горизонтом.
 */
tSignal sunPosition(float lat, float lon, time_t unix);

#endif  // FBDRTSUN_H
