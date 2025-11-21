#ifndef FINDME_CONFIG_H
#define FINDME_CONFIG_H

// ============================
// NÚMEROS AUTORIZADOS
// ============================
// Agrega aquí los números de teléfono autorizados en formato internacional
// Ejemplo: "+527771234567"
const String numerosAutorizados[] = {
    "+52XXXXXXXXXX",  // Número 1
    "+52XXXXXXXXXX",  // Número 2
    "+52XXXXXXXXXX",  // Número 3
    "+52XXXXXXXXXX",  // Número 4
    "+52XXXXXXXXXX",  // Número 5
    "+52XXXXXXXXXX",  // Número 6
    "+52XXXXXXXXXX"   // Número 7
};

const int totalNumeros = sizeof(numerosAutorizados) / sizeof(numerosAutorizados[0]);

#endif // FINDME_CONFIG_H
