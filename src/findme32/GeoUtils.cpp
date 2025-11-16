#include "GeoUtils.h"
#include "GeoUtils.h"
#include <math.h>

// Convertir grados a radianes
#define DEG_TO_RAD(deg) ((deg) * M_PI / 180.0)

double calcularDistancia(double lat1, double lon1, double lat2, double lon2) {
  const double EARTH_RADIUS_METERS = 6371000.0;

  double dLat = DEG_TO_RAD(lat2 - lat1);
  double dLon = DEG_TO_RAD(lon2 - lon1);

  double a = sin(dLat / 2) * sin(dLat / 2) +
             cos(DEG_TO_RAD(lat1)) * cos(DEG_TO_RAD(lat2)) *
             sin(dLon / 2) * sin(dLon / 2);
  
  double c = 2 * atan2(sqrt(a), sqrt(1 - a));
  
  return EARTH_RADIUS_METERS * c;
}
