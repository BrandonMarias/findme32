# Configuración Backend NestJS para GPS Tracker

## 1. Configuración CORS en main.ts

```typescript
import { NestFactory } from '@nestjs/core';
import { AppModule } from './app.module';

async function bootstrap() {
  const app = await NestFactory.create(AppModule);
  
  // Configuración CORS
  app.enableCors({
    origin: '*', // Permitir cualquier origen (dispositivos IoT)
    methods: ['GET', 'POST', 'PUT', 'DELETE', 'OPTIONS'],
    allowedHeaders: [
      'Content-Type',
      'Authorization',
      'Accept',
    ],
    maxAge: 3600, // Cache de preflight por 1 hora
  });
  
  await app.listen(3000);
}
bootstrap();
```

## 2. Controlador GPS Tracker (gpstracker.controller.ts)

```typescript
import { 
  Controller, 
  Get, 
  Post, 
  Body, 
  Query, 
  Headers, 
  HttpException, 
  HttpStatus,
  UseGuards 
} from '@nestjs/common';
import { GpsTrackerService } from './gpstracker.service';

@Controller('gpstracker')
export class GpsTrackerController {
  constructor(private readonly gpsTrackerService: GpsTrackerService) {}

  // Endpoint GET para recibir coordenadas por query params
  // Ahora el token viene en query string debido a limitaciones del SIM7600
  @Get()
  async receiveGpsDataGet(
    @Query('lat') latitude: string,
    @Query('lon') longitude: string,
    @Query('token') deviceToken: string,
  ) {
    // El token puede venir en query o en header
    const token = deviceToken || headerToken;
    
    // Validar que el token existe
    if (!token) {
      throw new HttpException(
        'Device token is required (query param or header)',
        HttpStatus.UNAUTHORIZED,
      );
    }

    // Validar coordenadas
    if (!latitude || !longitude) {
      throw new HttpException(
        'latitude and longitude are required',
        HttpStatus.BAD_REQUEST,
      );
    }

    // Validar formato de coordenadas
    const lat = parseFloat(latitude);
    const lon = parseFloat(longitude);

    if (isNaN(lat) || isNaN(lon)) {
      throw new HttpException(
        'Invalid coordinate format',
        HttpStatus.BAD_REQUEST,
      );
    }

    if (lat < -90 || lat > 90 || lon < -180 || lon > 180) {
      throw new HttpException(
        'Coordinates out of valid range',
        HttpStatus.BAD_REQUEST,
      );
    }

    // Guardar datos
    const result = await this.gpsTrackerService.saveLocation({
      deviceToken: token,
      latitude: lat,
      longitude: lon,
      timestamp: new Date(),
    });

    return {
      success: true,
      message: 'Location received successfully',
      data: result,
    };
  }

  // Endpoint POST alternativo (para JSON)
  @Post()
  async receiveGpsDataPost(
    @Body() body: { lat: number; lon: number; token?: string },
    @Headers('x-device-token') deviceToken?: string,
  ) {
    const token = body.token || deviceToken;
    
    if (!token) {
      throw new HttpException(
        'Device token is required',
        HttpStatus.UNAUTHORIZED,
      );
    }

    if (!body.lat || !body.lon) {
      throw new HttpException(
        'latitude and longitude are required',
        HttpStatus.BAD_REQUEST,
      );
    }

    const result = await this.gpsTrackerService.saveLocation({
      deviceToken: token,
      latitude: body.lat,
      longitude: body.lon,
      timestamp: new Date(),
    });

    return {
      success: true,
      message: 'Location received successfully',
      data: result,
    };
  }

  // Endpoint para consultar ubicaciones de un dispositivo
  @Get('history')
  async getDeviceHistory(
    @Query('token') token: string,
    @Query('limit') limit?: string,
  ) {
    if (!token) {
      throw new HttpException(
        'Device token is required',
        HttpStatus.UNAUTHORIZED,
      );
    }

    const maxLimit = limit ? parseInt(limit) : 100;
    const history = await this.gpsTrackerService.getDeviceHistory(
      token,
      maxLimit,
    );

    return {
      success: true,
      count: history.length,
      data: history,
    };
  }
  
  // Endpoint para obtener última ubicación de un dispositivo
  @Get('latest')
  async getLatestLocation(@Query('token') token: string) {
    if (!token) {
      throw new HttpException(
        'Device token is required',
        HttpStatus.UNAUTHORIZED,
      );
    }

    const location = await this.gpsTrackerService.getLatestLocation(token);
    
    if (!location) {
      throw new HttpException(
        'No location found for this device',
        HttpStatus.NOT_FOUND,
      );
    }

    return {
      success: true,
      data: location,
    };
  }
}
```

## 3. Servicio GPS Tracker (gpstracker.service.ts)

```typescript
import { Injectable } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { GpsLocation } from './entities/gps-location.entity';

@Injectable()
export class GpsTrackerService {
  constructor(
    @InjectRepository(GpsLocation)
    private gpsLocationRepository: Repository<GpsLocation>,
  ) {}

  async saveLocation(data: {
    deviceToken: string;
    latitude: number;
    longitude: number;
    timestamp: Date;
  }) {
    const location = this.gpsLocationRepository.create(data);
    return await this.gpsLocationRepository.save(location);
  }

  async getDeviceHistory(deviceToken: string, limit: number = 100) {
    return await this.gpsLocationRepository.find({
      where: { deviceToken },
      order: { timestamp: 'DESC' },
      take: limit,
    });
  }

  async getLatestLocation(deviceToken: string) {
    return await this.gpsLocationRepository.findOne({
      where: { deviceToken },
      order: { timestamp: 'DESC' },
    });
  }
}
```

## 4. Entidad (entities/gps-location.entity.ts)

```typescript
import { Entity, Column, PrimaryGeneratedColumn, CreateDateColumn, Index } from 'typeorm';

@Entity('gps_locations')
@Index(['deviceToken', 'timestamp'])
export class GpsLocation {
  @PrimaryGeneratedColumn('uuid')
  id: string;

  @Column({ type: 'varchar', length: 255 })
  @Index()
  deviceToken: string;

  @Column({ type: 'decimal', precision: 10, scale: 7 })
  latitude: number;

  @Column({ type: 'decimal', precision: 10, scale: 7 })
  longitude: number;

  @Column({ type: 'timestamp' })
  @Index()
  timestamp: Date;

  @CreateDateColumn()
  createdAt: Date;
}
```

## 5. Módulo (gpstracker.module.ts)

```typescript
import { Module } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { GpsTrackerController } from './gpstracker.controller';
import { GpsTrackerService } from './gpstracker.service';
import { GpsLocation } from './entities/gps-location.entity';

@Module({
  imports: [TypeOrmModule.forFeature([GpsLocation])],
  controllers: [GpsTrackerController],
  providers: [GpsTrackerService],
  exports: [GpsTrackerService],
})
export class GpsTrackerModule {}
```

## 6. Guard Opcional para Validar Tokens (opcional)

```typescript
import { Injectable, CanActivate, ExecutionContext, UnauthorizedException } from '@nestjs/common';

@Injectable()
export class DeviceTokenGuard implements CanActivate {
  // Lista de tokens válidos (en producción, guardar en base de datos)
  private readonly validTokens = new Set([
    'TOKEN_DISPOSITIVO_1',
    'TOKEN_DISPOSITIVO_2',
    'TOKEN_DISPOSITIVO_3',
  ]);

  canActivate(context: ExecutionContext): boolean {
    const request = context.switchToHttp().getRequest();
    const deviceToken = request.headers['x-device-token'];

    if (!deviceToken) {
      throw new UnauthorizedException('Device token is required');
    }

    if (!this.validTokens.has(deviceToken)) {
      throw new UnauthorizedException('Invalid device token');
    }

    return true;
  }
}

// Uso en el controlador:
// @UseGuards(DeviceTokenGuard)
// @Get()
// async receiveGpsDataGet(...) { ... }
```

## 7. Configuración de Base de Datos (app.module.ts)

```typescript
import { Module } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { GpsTrackerModule } from './gpstracker/gpstracker.module';

@Module({
  imports: [
    TypeOrmModule.forRoot({
      type: 'postgres', // o 'mysql', 'mariadb', etc.
      host: 'localhost',
      port: 5432,
      username: 'tu_usuario',
      password: 'tu_password',
      database: 'gps_tracker_db',
      entities: [__dirname + '/**/*.entity{.ts,.js}'],
      synchronize: true, // Solo en desarrollo!
    }),
    GpsTrackerModule,
  ],
})
export class AppModule {}
```

## 8. Instalación de Dependencias

```bash
npm install @nestjs/typeorm typeorm pg
# O para MySQL:
npm install @nestjs/typeorm typeorm mysql2
```

## 9. Variables de Entorno (.env)

```env
PORT=3000
DB_HOST=localhost
DB_PORT=5432
DB_USERNAME=tu_usuario
DB_PASSWORD=tu_password
DB_DATABASE=gps_tracker_db

# Lista de tokens válidos (separados por coma)
VALID_DEVICE_TOKENS=TOKEN_1,TOKEN_2,TOKEN_3
```

## 10. Configuración Adicional de Seguridad

### Helmet (Protección de Headers)
```bash
npm install helmet
```

```typescript
// En main.ts
import helmet from 'helmet';

async function bootstrap() {
  const app = await NestFactory.create(AppModule);
  
  app.use(helmet({
    crossOriginResourcePolicy: { policy: "cross-origin" },
  }));
  
  // ... resto de configuración
}
```

### Rate Limiting (Limitar peticiones)
```bash
npm install @nestjs/throttler
```

```typescript
// En app.module.ts
import { ThrottlerModule } from '@nestjs/throttler';

@Module({
  imports: [
    ThrottlerModule.forRoot({
      ttl: 60, // 60 segundos
      limit: 10, // máximo 10 peticiones por minuto por dispositivo
    }),
    // ... otros módulos
  ],
})
export class AppModule {}

// En el controlador:
import { Throttle } from '@nestjs/throttler';

@Controller('gpstracker')
export class GpsTrackerController {
  @Throttle(10, 60) // 10 peticiones por 60 segundos
  @Get()
  async receiveGpsDataGet(...) { ... }
}
```

## 11. Nginx como Reverse Proxy (Producción)

```nginx
server {
    listen 80;
    server_name mibackend.com;

    location /gpstracker {
        proxy_pass http://localhost:3000/gpstracker;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection 'upgrade';
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
        proxy_cache_bypass $http_upgrade;
        
        # Permitir headers personalizados
        proxy_set_header x-device-token $http_x_device_token;
    }
}
```

## Notas Importantes:

1. **Cambiar el token en cada dispositivo**: Editar la línea `#define DEVICE_TOKEN` en el código ESP32
2. **Cambiar el APN**: En la línea `AT+CSTT=\"internet.com\"` según tu operador
3. **HTTPS en producción**: Usar certificados SSL/TLS
4. **Validar tokens**: Implementar el DeviceTokenGuard para mayor seguridad
5. **Logs**: Agregar logging para monitorear peticiones
6. **Webhooks**: Considerar notificaciones cuando se reciben coordenadas
