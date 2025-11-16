# Arquitectura Modular - GPS Tracker FindMe32

## Resumen
El código ha sido refactorizado de un archivo monolítico de ~750 líneas a una arquitectura modular y orientada a objetos con separación clara de responsabilidades.

## Estructura de Archivos

```
src/findme32/
├── config.h           # Configuración central (tokens, pines, umbrales)
├── GeoUtils.h/cpp     # Utilidades geográficas (cálculo de distancias)
├── GSMModule.h/cpp    # Gestión completa del módulo GSM/GPRS
├── GPSModule.h/cpp    # Control y parseo de GPS
├── HTTPClient.h/cpp   # Cliente HTTPS con SSL/TLS y SNI
└── findme32.cpp       # Archivo principal (simplificado a ~150 líneas)
```

## Módulos

### 1. config.h
**Propósito**: Centralizar todas las constantes de configuración.

**Contiene**:
- `DEVICE_TOKEN`: Token del dispositivo
- `API_ENDPOINT`, `API_PATH`: Configuración del servidor
- `PWR_PIN`, `RXD1_PIN`, `TXD1_PIN`: Pines del hardware
- `UMBRAL_MOVIMIENTO_METROS`: Umbral de detección de movimiento (20m)
- `INTERVALO_LECTURA_GPS`: Intervalo de lectura GPS (30s)
- `INTERVALO_HEARTBEAT`: Intervalo de heartbeat (5 min)
- `APN_TELCEL`: Configuración APN de Telcel
- Timeouts para red, GPRS y HTTP

### 2. GeoUtils (GeoUtils.h/cpp)
**Propósito**: Funciones de utilidad geográfica.

**Funciones**:
- `double calcularDistancia(lat1, lon1, lat2, lon2)`: Calcula distancia en metros usando fórmula de Haversine

**Uso**:
```cpp
double dist = calcularDistancia(lat_anterior, lon_anterior, lat_nueva, lon_nueva);
if (dist > UMBRAL_MOVIMIENTO_METROS) {
  // Movimiento detectado
}
```

### 3. GSMModule (GSMModule.h/cpp)
**Propósito**: Gestión completa del módulo GSM/GPRS A7670SA.

**Características**:
- Inicialización con control de alimentación (PWR_PIN)
- Registro en red celular con reintentos (30 intentos max)
- Sincronización de reloj con AT+CFUN=1,1 (reinicio profundo)
- Configuración y activación de GPRS/PDP
- Verificación de calidad de señal

**Métodos Principales**:
- `void begin()`: Inicializa módulo (power, serial, comunicación)
- `bool esperarRegistroRed(int maxIntentos)`: Espera registro en red
- `bool verificarConexionGPRS()`: Configura APN y activa contexto PDP
- `bool estaContextoPDPActivo()`: Verifica estado del contexto PDP
- `bool verificarYSincronizarReloj()`: Sincroniza reloj con red (soluciona error 715)
- `void verificarCalidadSenal()`: Muestra calidad de señal (AT+CSQ)
- `HardwareSerial& getSerial()`: Acceso al serial para otros módulos

**Uso**:
```cpp
GSMModule gsm(Serial1, PWR_PIN, RXD1_PIN, TXD1_PIN, BAUD_RATE);
gsm.begin();
gsm.esperarRegistroRed();
gsm.verificarYSincronizarReloj();
gsm.verificarConexionGPRS();
```

### 4. GPSModule (GPSModule.h/cpp)
**Propósito**: Control del GPS y parseo de coordenadas.

**Estructura de Datos**:
```cpp
struct GpsData {
  double lat;
  double lon;
  bool valida;  // Indica si las coordenadas son válidas
};
```

**Métodos Principales**:
- `bool inicializar()`: Enciende GPS con AT+CGNSSPWR=1
- `GpsData obtenerCoordenadas()`: Lee y parsea AT+CGNSSINFO (10 reintentos)
- `GpsData parsearCGNSSINFO(String)`: Parsea respuesta GPS con direcciones N/S/E/W

**Características**:
- Parsing robusto de respuesta AT+CGNSSINFO
- Conversión automática de direcciones (S→negativo, W→negativo)
- Validación de coordenadas (no envía 0.0, 0.0)
- Reintentos automáticos (10 intentos, 2s entre intentos)

**Uso**:
```cpp
GPSModule gps(gsmSerial);
gps.inicializar();
GpsData pos = gps.obtenerCoordenadas();
if (pos.valida) {
  Serial.println("Lat: " + String(pos.lat, 6) + ", Lon: " + String(pos.lon, 6));
}
```

### 5. HTTPClient (HTTPClient.h/cpp)
**Propósito**: Cliente HTTPS para envío de ubicaciones.

**Métodos Principales**:
- `bool enviarUbicacion(double lat, double lon)`: Envía ubicación al servidor
- `bool inicializarHTTP()`: Configura SSL/TLS, SNI
- `bool parsearRespuestaHTTP(String)`: Parsea y valida respuesta HTTP

**Características**:
- **SSL/TLS**: TLS 1.2 forzado (`sslversion=3`)
- **SNI**: Server Name Indication habilitado (para Cloudflare tunnel)
- **Sin validación de certificado**: `authmode=0`
- **Verificación de PDP**: Verifica contexto antes de enviar
- **Manejo de errores**: Detecta errores 715, 703, 714
- **Respuesta inteligente**: Solo lee respuesta si `DataLen > 0`

**Códigos de Error**:
- `715`: Timeout SSL/TLS o certificado inválido (revisar reloj y SNI)
- `703`: Error DNS
- `714`: Timeout HTTP
- `200/201/204`: Éxito

**Uso**:
```cpp
HTTPClient httpClient(gsm);
if (httpClient.enviarUbicacion(lat, lon)) {
  Serial.println("Ubicación enviada exitosamente");
}
```

### 6. findme32.cpp (Principal)
**Propósito**: Lógica principal de la aplicación (simplificado a ~150 líneas).

**Funcionalidad**:
- **Setup**:
  - Inicializa todos los módulos (GSM, GPS, GPRS)
  - Sincroniza reloj
  - Prepara sistema para lectura
  
- **Loop**:
  - **Cada 30 segundos**: Lee GPS
  - **Detección de movimiento**: Si movimiento > 20m → envía
  - **Heartbeat (5 min)**: Envía ubicación actual si hay cambios

**Variables Globales**:
```cpp
GSMModule gsm(Serial1, PWR_PIN, RXD1_PIN, TXD1_PIN, BAUD_RATE);
GPSModule gps(Serial1);
HTTPClient httpClient(gsm);
```

## Flujo de Operación

### Inicialización (setup)
```
1. Serial.begin(115200)
2. gsm.begin()
   ├─ Control de alimentación (PWR_PIN)
   ├─ Serial1.begin()
   └─ Verificar comunicación (AT)
3. gsm.esperarRegistroRed()
   └─ Reintentos hasta registro (max 30)
4. gsm.verificarCalidadSenal()
5. gsm.verificarYSincronizarReloj()
   ├─ Verificar fecha (detectar 1970/1980/2000)
   ├─ AT+CTZU=1, AT+CLTS=1, AT&W
   └─ AT+CFUN=1,1 (reinicio profundo si necesario)
6. gps.inicializar()
   └─ AT+CGNSSPWR=1
7. gsm.verificarConexionGPRS()
   ├─ AT+CGDCONT (configurar APN)
   ├─ AT+CGACT=1,1 (activar PDP)
   └─ AT+CGPADDR=1 (verificar IP)
```

### Loop Principal
```
Cada 30 segundos:
├─ Verificar GPRS activo
├─ gps.obtenerCoordenadas()
└─ Si válido:
   ├─ Primer fix → httpClient.enviarUbicacion()
   └─ Fix subsecuente:
      ├─ Calcular distancia
      └─ Si > 20m → httpClient.enviarUbicacion()

Cada 5 minutos (Heartbeat):
└─ Si posición válida y cambió:
   └─ httpClient.enviarUbicacion()
```

## Ventajas de la Arquitectura Modular

### 1. Separación de Responsabilidades
- Cada módulo tiene un propósito único y bien definido
- Fácil de entender qué hace cada parte del código

### 2. Reutilización
- Los módulos pueden usarse en otros proyectos
- Ejemplo: `GSMModule` puede usarse en cualquier proyecto con A7670SA

### 3. Mantenibilidad
- Bugs se localizan más fácilmente
- Cambios en un módulo no afectan otros
- Código más limpio y legible

### 4. Testeo
- Cada módulo puede probarse independientemente
- Más fácil debuggear problemas específicos

### 5. Escalabilidad
- Agregar nuevas funcionalidades es más simple
- Ejemplo: Agregar soporte para otros sensores

## Cambios Respecto al Código Original

### Eliminado
- ✂️ Funciones globales dispersas (`esperarRespuesta`, `inicializarGPS`, etc.)
- ✂️ Código duplicado de manejo de AT commands
- ✂️ Hardcoded values mezclados con lógica

### Agregado
- ✅ Clases con encapsulación (GSMModule, GPSModule, HTTPClient)
- ✅ Archivo de configuración central (config.h)
- ✅ Módulo de utilidades geográficas (GeoUtils)
- ✅ Documentación inline en headers

### Mejorado
- 📈 Código principal reducido de ~750 a ~150 líneas
- 📈 Mejor organización y legibilidad
- 📈 Reutilización de código entre módulos
- 📈 Manejo consistente de errores

## Compatibilidad

- ✅ Funcionalidad idéntica al código original
- ✅ Mismo comportamiento de movimiento (20m) y heartbeat (5 min)
- ✅ Mismas correcciones de SSL/TLS (reloj, SNI)
- ✅ Compatible con PlatformIO y Arduino framework

## Próximos Pasos Sugeridos

1. **Agregar logs a SD**: Crear módulo `SDLogger` para guardar eventos
2. **Modo bajo consumo**: Implementar sleep entre lecturas GPS
3. **Configuración dinámica**: Leer config desde archivo JSON
4. **OTA Updates**: Agregar soporte para actualizaciones remotas
5. **Multi-sensor**: Extender con acelerómetro, temperatura, etc.

## Compilación

El código debe compilar sin errores con PlatformIO:
```bash
pio run -t upload
```

Todos los archivos están ubicados en `src/findme32/` y deben incluirse en la compilación.
