# FindMe32 GPS Tracker

Sistema de rastreo GPS con ESP32-C3 y módulo A7670SA GSM/GPRS.

## Configuración

### 1. Clonar el repositorio
```bash
git clone https://github.com/BrandonMarias/findme32.git
cd findme32
```

### 2. Configurar credenciales

#### Para el módulo GPS Tracker (findme32):

Copia el archivo de plantilla de configuración:
```bash
cp src/findme32/config_template.h src/findme32/config.h
```

Edita `src/findme32/config.h` y reemplaza los valores de ejemplo con tus credenciales:

```cpp
#define DEVICE_TOKEN "tu_token_aqui"
#define API_ENDPOINT "tu_dominio_aqui"
#define API_PATH "/api/gps/gpstracker"
```

#### Para el módulo SMS (findme):

Copia el archivo de plantilla de configuración:
```bash
cp src/findme/findme_config_template.h src/findme/findme_config.h
```

Edita `src/findme/findme_config.h` y agrega los números de teléfono autorizados:

```cpp
const String numerosAutorizados[] = {
    "+527771234567",  // Tu número 1
    "+527779876543",  // Tu número 2
    // ... más números
};
```

### 3. Compilar y subir

```bash
platformio run --target upload
```

## Características

- ✅ Detección de movimiento con umbral configurable (25m por defecto)
- ✅ Cálculo de velocidad en km/h
- ✅ Heartbeat cada 5 minutos
- ✅ SSL/TLS con SNI para túneles Cloudflare
- ✅ Sincronización automática de reloj con red celular
- ✅ Control de pines según estado del dispositivo (isActive)
- ✅ Reinicio automático del GPS tras 3 fallos consecutivos
- ✅ Arquitectura modular y escalable

## Estructura del Proyecto

```
src/
├── findme32/                    # Módulo GPS Tracker
│   ├── config.h                 # Configuración (NO incluido en Git)
│   ├── config_template.h        # Plantilla de configuración
│   ├── GSMModule.h/cpp          # Manejo del módulo GSM/GPRS
│   ├── GPSModule.h/cpp          # Control del GPS
│   ├── HTTPClient.h/cpp         # Cliente HTTPS
│   ├── GeoUtils.h/cpp           # Utilidades geográficas
│   └── findme32.cpp             # Programa principal
│
└── findme/                      # Módulo control SMS
    ├── findme_config.h          # Números autorizados (NO incluido en Git)
    ├── findme_config_template.h # Plantilla de números
    └── findme.cpp               # Programa de control por SMS
```

## Módulos

### FindMe32 - GPS Tracker
Sistema automático de rastreo GPS con detección de movimiento y envío a API.

**Variables de Configuración:**
- `DEVICE_TOKEN`: Token único del dispositivo
- `API_ENDPOINT`: Dominio de tu API
- `API_PATH`: Ruta del endpoint
- `UMBRAL_MOVIMIENTO_METROS`: Distancia mínima para considerar movimiento (25m)
- `INTERVALO_LECTURA_GPS`: Frecuencia de lectura GPS (20s)
- `INTERVALO_HEARTBEAT`: Intervalo de heartbeat (5 min)

### FindMe - Control por SMS
Sistema de control remoto del dispositivo mediante comandos SMS.

**Variables de Configuración:**
- `numerosAutorizados[]`: Array de números de teléfono autorizados en formato internacional

**Comandos SMS disponibles:**
- `Localizar`: Obtiene y envía la ubicación GPS actual
- `Apagar`: Activa el relevador (GPIO 9)
- `Prender`: Desactiva el relevador (GPIO 9)

## Hardware

- ESP32-C3-DevKitM-1
- Módulo A7670SA (GSM/GPRS/GPS)
- Pines:
  - PWR: GPIO 10
  - RX: GPIO 20
  - TX: GPIO 21
  - PIN_ACTIVE: GPIO 9 (activo cuando isActive=true)
  - PIN_INACTIVE: GPIO 8 (activo cuando isActive=false)

## Seguridad

⚠️ **IMPORTANTE**: Los siguientes archivos contienen información sensible y están excluidos del repositorio:

- `src/findme32/config.h` - Credenciales de API y tokens
- `src/findme/findme_config.h` - Números de teléfono autorizados

Estos archivos están incluidos en `.gitignore` para tu protección. Usa las plantillas (`*_template.h`) como base para crear tus archivos de configuración.

## Licencia

MIT
