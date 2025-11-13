#ifndef FINDME32HTTPTEST_H
#define FINDME32HTTPTEST_H

#include <Arduino.h>

class FindMe32HttpTest {
public:
  // Constructor: usa Serial1 por defecto y pines compatibles con el proyecto
  FindMe32HttpTest(HardwareSerial &serial = Serial1, int pwrPin = 10, int rxPin = 20, int txPin = 21, unsigned long baud = 115200);
  void begin();
  // Ejecuta una única prueba HTTP GET con la URL fija y retorna true si obtuvo 2xx
  bool runTest();

private:
  HardwareSerial &gsm;
  int pwrPin;
  int rxPin;
  int txPin;
  unsigned long baudRate;

  String waitResponse(unsigned long timeout_ms = 5000);
  bool ensureGPRS();
};

#endif // FINDME32HTTPTEST_H
