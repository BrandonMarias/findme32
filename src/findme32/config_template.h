#ifndef CONFIG_H
#define CONFIG_H

// ============================
// CONFIGURACIÓN DEL DISPOSITIVO
// ============================
#define DEVICE_TOKEN "YOUR_DEVICE_TOKEN_HERE"
#define API_ENDPOINT "YOUR_API_ENDPOINT_HERE"
#define API_PATH "/api/gps/gpstracker"
#define API_PORT "443"

// ============================
// PINES Y CONFIGURACIÓN HARDWARE
// ============================
#define PWR_PIN 10
#define RXD1_PIN 20
#define TXD1_PIN 21
#define BAUD_RATE 115200 

// Pines de control de estado
#define PIN_ACTIVE 9    // LED/Relé cuando isActive=true
#define PIN_INACTIVE 8  // LED/Relé cuando isActive=false

// ============================
// LÓGICA DE MOVIMIENTO
// ============================
#define UMBRAL_MOVIMIENTO_METROS 25.0
#define INTERVALO_LECTURA_GPS (20 * 1000)   // 20 segundos
#define INTERVALO_HEARTBEAT (5 * 60 * 1000) // 5 minutos

// ============================
// CONFIGURACIÓN APN
// ============================
#define APN_TELCEL "internet.itelcel.com"

// ============================
// TIMEOUTS Y REINTENTOS
// ============================
#define GPS_MAX_INTENTOS 20
#define GPS_DELAY_INTENTO 1000
#define HTTP_TIMEOUT 60000
#define NETWORK_REGISTER_TIMEOUT 30

#endif // CONFIG_H
