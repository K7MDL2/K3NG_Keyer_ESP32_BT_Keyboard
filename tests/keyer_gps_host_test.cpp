#include "keyer_gps.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <initializer_list>

static void original_grid(double latitude, double longitude, char grid[]) {
  const double scaling[] = {360., 360./18., (360./18.)/10.,
      ((360./18.)/10.)/24., (((360./18.)/10.)/24.)/10.};
  for (int i = 0; i < 4; ++i) {
    int index = (int)floor(fmod(180.0 + longitude, scaling[i]) / scaling[i+1]);
    grid[i*2] = (i&1) ? 0x30+index : (i&2) ? 0x61+index : 0x41+index;
    index = (int)floor(fmod(90.0 + latitude, scaling[i]/2) / (scaling[i+1]/2));
    grid[i*2+1] = (i&1) ? 0x30+index : (i&2) ? 0x61+index : 0x41+index;
  }
  grid[8] = 0;
}

static void check_grid(double latitude, double longitude, const char *expected = nullptr) {
  char actual[11];
  std::memset(actual, '#', sizeof(actual));
  char original[9];
  original_grid(latitude, longitude, original);
  assert(positionToMaidenhead(latitude, longitude, actual + 1) == 1);
  assert(actual[0] == '#' && actual[10] == '#');
  assert(actual[9] == 0 && std::strlen(actual + 1) == 8);
  assert(std::strcmp(actual + 1, original) == 0);
  if (expected) assert(std::strcmp(actual + 1, expected) == 0);
}

int main() {
  check_grid(43.999, -79.495, "FN03gx09");
  check_grid(0., 0., "JJ00aa00");
  check_grid(-90., -180., "AA00aa00");
  for (int lat = -90; lat < 90; ++lat) {
    for (int lon = -180; lon < 180; ++lon) {
      check_grid(lat, lon);
      check_grid(lat + 0.49999, lon + 0.99999);
    }
  }
  for (double latitude : {-90., -45., 0., 45., 90.}) {
    for (double longitude : {-180., -90., 0., 90., 180.})
      check_grid(latitude, longitude);
  }
  for (uint32_t age : {0u, 4999u, 5000u, 59999u, 60000u, UINT32_MAX}) {
    assert(!gps_data_fresh(false, age) && !gps_location_held(false, age));
    assert(gps_data_fresh(true, age) == (age < 5000));
    assert(gps_location_held(true, age) == (age < 60000));
  }
  puts("PASS: GPS grid vectors, 129625 baseline-equivalent coordinates, nine-byte output bounds, freshness/holdover boundaries");
}
