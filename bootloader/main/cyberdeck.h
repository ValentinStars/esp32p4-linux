#pragma once

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LCD_H_RES   1024
#define LCD_V_RES   600

// Initialize MIPI-DSI LCD Framebuffer, USB Host (HID Keyboard + MSC Storage)
esp_err_t cyberdeck_init(void);

// Terminal ANSI console output to MIPI-DSI display
void cyberdeck_console_write(const char *buf, size_t len);

// Direct framebuffer access (for Bad Apple video player / GUI apps)
uint16_t *cyberdeck_get_framebuffer(int *width, int *height);

// Push key character from physical USB keyboard directly to Linux UART RX ring
void cyberdeck_keyboard_input_char(uint8_t c);

#ifdef __cplusplus
}
#endif
