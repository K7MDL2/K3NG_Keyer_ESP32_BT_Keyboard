#include "keyer_gps.h"
#include <cmath>

bool gps_data_fresh(bool valid, uint32_t age_ms) {
  return valid && age_ms < 5000;
}

// Brief indoor fix dropouts retain the last grid instead of saving the fallback.
bool gps_location_held(bool valid, uint32_t age_ms) {
  return valid && age_ms < 60000;
}

int positionToMaidenhead(double latitude, double longitude, char grid[]) {
  const int pairs = 4;
  const double scaling[] = {360., 360./18., (360./18.)/10.,
      ((360./18.)/10.)/24., (((360./18.)/10.)/24.)/10.,
      ((((360./18.)/10.)/24.)/10.)/24.,
      (((((360./18.)/10.)/24.)/10.)/24.)/10.};

  for (int i = 0; i < pairs; ++i) {
    int index = static_cast<int>(std::floor(std::fmod(180.0 + longitude, scaling[i]) / scaling[i+1]));
    grid[i*2] = (i&1) ? 0x30+index : (i&2) ? 0x61+index : 0x41+index;
    index = static_cast<int>(std::floor(std::fmod(90.0 + latitude, scaling[i]/2) / (scaling[i+1]/2)));
    grid[i*2+1] = (i&1) ? 0x30+index : (i&2) ? 0x61+index : 0x41+index;
  }
  grid[pairs*2] = 0;
  return 1;
}
