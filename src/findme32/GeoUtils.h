#ifndef GEOUTILS_H
#define GEOUTILS_H

// ============================
// UTILIDADES GEOGRÁFICAS
// ============================

/**
 * Calcula la distancia entre dos coordenadas GPS usando la fórmula de Haversine
 * @param lat1 Latitud punto 1 (decimal)
 * @param lon1 Longitud punto 1 (decimal)
 * @param lat2 Latitud punto 2 (decimal)
 * @param lon2 Longitud punto 2 (decimal)
 * @return Distancia en metros
 */
double calcularDistancia(double lat1, double lon1, double lat2, double lon2);

#endif // GEOUTILS_H
