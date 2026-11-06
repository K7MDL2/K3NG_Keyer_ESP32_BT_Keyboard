#ifndef KEYER_GPS_H
#define KEYER_GPS_H

#include <cstdint>

bool gps_data_fresh(bool valid, uint32_t age_ms);
bool gps_location_held(bool valid, uint32_t age_ms);
// Caller supplies finite GPS coordinates and room for eight characters plus NUL.
int positionToMaidenhead(double latitude, double longitude, char grid[]);

#endif
