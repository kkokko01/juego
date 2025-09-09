#include "mbed.h"
#include "TM1638.h"
#include <cstdio>
#include <cstring>
#include <ctime>
#include <chrono>

#define RONDAS 10

TM1638 tm(D10, D13, D11); // STB, CLK, DIO

// Conversion simple a segmentos (mayúsculas)
uint8_t charToSegment(char c) {
    if (c >= 'a' && c <= 'z') c -= 32;
    switch (c) {
        case '0': return 0x3F;
        case '1': return 0x06;
        case '2': return 0x5B;
        case '3': return 0x4F;
        case '4': return 0x66;
        case '5': return 0x6D;
        case '6': return 0x7D;
        case '7': return 0x07;
        case '8': return 0x7F;
        case '9': return 0x6F;
        case 'A': return 0x77;
        case 'C': return 0x39;
        case 'E': return 0x79;
        case 'F': return 0x71;
        case 'H': return 0x76;
        case 'L': return 0x38;
        case 'O': return 0x3F;
        case 'P': return 0x73;
        case 'R': return 0x50;
        case 'S': return 0x6D;
        case 'T': return 0x78;
        case 'U': return 0x3E;
        case '-': return 0x40;
        case ' ': return 0x00;
        default:  return 0x00;
    }
}

void displayText8(const char* text) {
    for (int i = 0; i < 8; i++) {
        char c = (i < (int)strlen(text)) ? text[i] : ' ';
        tm.displayDigit(i, charToSegment(c));
    }
}

void scrollText8(const char* text, int delay_ms = 3e3) {
    int len = strlen(text);
    const int ventana = 8;
    char buf[ventana + 1];
    char textoPad[64];
    if (len + ventana + 1 > (int)sizeof(textoPad)) {
        // truncar si es muy largo
        len = sizeof(textoPad) - ventana - 1;
    }
    strcpy(textoPad, text);
    for (int i = 0; i < ventana; i++) textoPad[len + i] = ' ';
    textoPad[len + ventana] = '\0';

    int total = strlen(textoPad);
    for (int pos = 0; pos <= total - ventana; pos++) {
        strncpy(buf, textoPad + pos, ventana);
        buf[ventana] = '\0';
        displayText8(buf);
        thread_sleep_for(delay_ms);
    }
}

int main() {
    tm.init();
    tm.setBrightness(4);
    tm.clearDisplay();
    tm.clearLeds();
    srand(time(NULL));

    int aciertos = 0, fallas = 0;
    int tiempos[RONDAS] = {0};

    // Intro
    scrollText8("JUEGO REACCION", 500);
    thread_sleep_for(500);

    for (int ronda = 0; ronda < RONDAS; ronda++) {
        tm.clearDisplay();
        tm.clearLeds();

        // Mostrar número de ronda
        char temp[9];
        snprintf(temp, sizeof(temp), "RONDA %d", ronda + 1);
        displayText8(temp);
        thread_sleep_for(1e3);

        // Espera breve aleatoria para que no se anticipen
        int espera = 300 + (rand() % 1200);
        tm.clearDisplay();
        thread_sleep_for(espera);

        int led = rand() % 8;
        tm.displayLed(led, true);
        displayText8("   GO   ");

        Timer t;
        t.reset();
        t.start();
        bool responded = false;
        const int TIMEOUT_MS = 5000; // timeout en ms

        while (!responded) {
            uint8_t tecla = tm.readKeys(); // con la librería corregida devuelve 0..8
            if (tecla != 0) {
                t.stop();
                int t_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                               t.elapsed_time()
                           ).count();
                tiempos[ronda] = t_ms;
                if ((int)tecla == led + 1) {
                    aciertos++;
                    displayText8("ACIERTO ");
                } else {
                    fallas++;
                    displayText8("FALLA   ");
                }
                responded = true;
            } else {
                // timeout
                if (std::chrono::duration_cast<std::chrono::milliseconds>(t.elapsed_time()).count() > TIMEOUT_MS) {
                    t.stop();
                    tiempos[ronda] = TIMEOUT_MS;
                    fallas++;
                    displayText8("TIMEOUT ");
                    responded = true;
                }
            }
            thread_sleep_for(1e3);
        }

        tm.displayLed(led, false);
        thread_sleep_for(2e3);
    }

    // calcular promedio
    int suma = 0;
    for (int i = 0; i < RONDAS; i++) suma += tiempos[i];
    int promedio = (RONDAS > 0) ? suma / RONDAS : 0;

    // mostrar resultados en loop
    while (true) {
        char buf[16];
        snprintf(buf, sizeof(buf), "ACIERTOS %d", aciertos);
        scrollText8(buf, 350);

        snprintf(buf, sizeof(buf), "FALLAS %d", fallas);
        scrollText8(buf, 350);

        int score = aciertos * 10 - fallas * 5;
        snprintf(buf, sizeof(buf), "SCORE %d", score);
        scrollText8(buf, 350);

        snprintf(buf, sizeof(buf), "T %dms", promedio);
        scrollText8(buf, 350);
    }
}