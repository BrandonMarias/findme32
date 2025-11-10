# FindMe32 - GPS Tracker

Sistema de rastreo GPS que envía ubicaciones cada 5 minutos a un servidor backend.

## 📋 Características

- ✅ Envío automático de coordenadas GPS cada 5 minutos
- ✅ Autenticación mediante token único por dispositivo
- ✅ Conexión HTTP/HTTPS al backend
- ✅ Manejo de errores y reintentos
- ✅ Logs detallados por Serial Monitor

## 🔧 Configuración del Dispositivo ESP32

### 1. Editar el Token del Dispositivo

En `src/findme32/findme32.cpp`, cambiar:

```cpp
#define DEVICE_TOKEN "TU_TOKEN_UNICO_AQUI"  // ⚠️ Cambiar en cada dispositivo
```

**Ejemplo de tokens únicos:**
- Dispositivo 1: `"ESP32_TRACKER_001"`
- Dispositivo 2: `"ESP32_TRACKER_002"`
- Dispositivo 3: `"ESP32_TRACKER_003"`

### 2. Configurar el Endpoint del Backend

```cpp
#define API_ENDPOINT "mibackend.com"        // Tu dominio
#define API_PATH "/gpstracker"              // Ruta del endpoint
#define API_PORT "80"                       // 80 para HTTP, 443 para HTTPS
```

### 3. Configurar el APN de tu Operador

En la función `verificarConexionGPRS()`, línea 227:

```cpp
gsmSerial.println("AT+CSTT=\"internet.com\",\"\",\"\"");  // Cambiar según operador
```

**APNs comunes en México:**
- Telcel: `"internet.itelcel.com"`
- Movistar: `"internet.movistar.com.mx"`
- AT&T: `"internet.att.com.mx"`
- Unefon: `"internet.unefon.com.mx"`

## 📡 Comandos AT Utilizados

### GPS/GNSS
- `AT+CGNSPWR=1` - Encender GPS
- `AT+CGNSSEQ="RMC"` - Configurar secuencia NMEA
- `AT+CGNSINF` - Obtener información de ubicación

### HTTP
- `AT+HTTPINIT` - Inicializar servicio HTTP
- `AT+HTTPTERM` - Terminar sesión HTTP
- `AT+HTTPPARA="URL","..."` - Configurar URL
- `AT+HTTPPARA="USERDATA","x-device-token: ..."` - Agregar header personalizado
- `AT+HTTPACTION=0` - Ejecutar petición GET (0=GET, 1=POST, 2=HEAD)
- `AT+HTTPREAD` - Leer respuesta del servidor

### GPRS
- `AT+CGATT?` - Verificar conexión GPRS
- `AT+CSTT="APN","user","pass"` - Configurar APN
- `AT+CIICR` - Activar conexión de datos
- `AT+CIFSR` - Obtener dirección IP

## 🚀 Compilación y Carga

### PlatformIO (Recomendado)

```bash
# Compilar
pio run

# Cargar al dispositivo
pio run --target upload

# Monitor serial
pio device monitor
```

### Cambiar el directorio fuente

En `platformio.ini`:
```ini
[platformio]
src_dir = src/findme32  # Para GPS tracker
# src_dir = src/main    # Para control por SMS
```

## 📊 Monitoreo

El sistema imprime logs detallados por el puerto serial (115200 baud):

```
>> =============================
>> GPS Tracker - FindMe32
>> =============================
>> Device Token: ESP32_TRACKER_001
>> API Endpoint: mibackend.com/gpstracker
>> Intervalo: 5 minutos
>> =============================

>> Esperando que el módulo GSM esté listo...
>> Módulo GSM respondiendo
>> Inicializando GPS...
>> Verificando conexión GPRS...
>> GPRS conectado
>> IP obtenida: 10.xxx.xxx.xxx

>> Sistema listo. Comenzando ciclo de envío...

>> ===== CICLO DE ENVÍO =====
>> Tiempo transcurrido: 300 segundos
>> Obteniendo coordenadas GPS...
>> Intento 1 de 5
>> Coordenadas obtenidas: 19.432608, -99.133209
>> Enviando ubicación al servidor...
>> Inicializando HTTP...
>> Ejecutando petición HTTP GET...
>> ✓ Ubicación enviada exitosamente (200 OK)
>> ✓ Ciclo completado exitosamente
>> ============================
```

## 🌐 Backend NestJS

Ver archivo `BACKEND_NESTJS_CONFIG.md` para:
- Configuración CORS completa
- Controladores y servicios
- Esquema de base de datos
- Validación de tokens
- Rate limiting
- Ejemplos de código completo

### Endpoint esperado:

**GET** `http://mibackend.com/gpstracker?lat=19.432608&lon=-99.133209`

**Headers:**
```
x-device-token: ESP32_TRACKER_001
```

**Respuesta esperada:**
```json
{
  "success": true,
  "message": "Location received successfully",
  "data": {
    "id": "uuid",
    "deviceToken": "ESP32_TRACKER_001",
    "latitude": 19.432608,
    "longitude": -99.133209,
    "timestamp": "2025-11-09T12:00:00.000Z"
  }
}
```

## 🔒 Seguridad

1. **Token único por dispositivo**: Cada ESP32 debe tener un token diferente
2. **Validación en backend**: Implementar whitelist de tokens válidos
3. **HTTPS en producción**: Cambiar `API_PORT` a `"443"` y URL a `https://`
4. **Rate limiting**: Limitar peticiones por dispositivo
5. **Logs de auditoría**: Registrar todas las peticiones

## 🛠️ Solución de Problemas

### GPS no obtiene coordenadas
- Asegurarse de estar en un lugar con visibilidad al cielo
- El primer fix puede tomar 1-2 minutos
- Verificar que la antena GPS esté conectada

### Error de conexión HTTP
- Verificar que el APN sea correcto
- Comprobar que hay señal GSM/3G/4G
- Revisar que el endpoint esté accesible desde internet

### Módulo GSM no responde
- Verificar conexiones de TX/RX
- Revisar alimentación (el módulo necesita 2A mínimo)
- Presionar el botón PWR manualmente si es necesario

## 📝 Notas Adicionales

- **Consumo de batería**: El GPS consume bastante energía. Considerar apagarlo entre lecturas.
- **Intervalo configurable**: Cambiar `INTERVALO_ENVIO` para ajustar la frecuencia de envío
- **Almacenamiento local**: Se puede agregar SD card para guardar ubicaciones cuando no hay conexión
- **Modo sleep**: Implementar deep sleep entre ciclos para ahorrar batería

## 📄 Licencia

Este proyecto es de código abierto. Úsalo libremente.

## 👨‍💻 Autor

Brandon - FindMe32 Project
