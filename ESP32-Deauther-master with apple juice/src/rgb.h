#ifndef RGB_H
#define RGB_H
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Adafruit_NeoPixel.h>

void handle_rgb();
void handle_setColor();
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);

#endif