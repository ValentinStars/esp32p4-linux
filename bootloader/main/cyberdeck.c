#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <dirent.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_ldo_regulator.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_ek79007.h"
#include "esp_lcd_panel_ops.h"

#include "usb/usb_host.h"
#include "usb/hid_host.h"
#include "usb/hid_usage_keyboard.h"
#include "usb/msc_host.h"
#include "usb/msc_host_vfs.h"
#include "esp_vfs_fat.h"

#include "cyberdeck.h"
#include "cyberdeck_font.h"

static const char *TAG = "CYBERDECK";

// Extern from uc-rv32ima.c
extern void uart_rx_push(uint8_t c);

// --- MIPI-DSI LCD State ---
static esp_ldo_channel_handle_t s_ldo_mipi_phy = NULL;
static esp_lcd_panel_handle_t s_lcd_panel = NULL;
static uint16_t *s_fb = NULL;

#define FONT_W 8
#define FONT_H 16
#define CONSOLE_COLS (LCD_H_RES / FONT_W)  // 128
#define CONSOLE_ROWS (LCD_V_RES / FONT_H)  // 37

static int s_cursor_x = 0;
static int s_cursor_y = 0;
static uint16_t s_fg_color = 0x07E0; // Cyberpunk Green (RGB565)
static uint16_t s_bg_color = 0x0000; // Black

// ANSI Parser State
static enum {
    ANSI_STATE_NORMAL,
    ANSI_STATE_ESC,
    ANSI_STATE_CSI
} s_ansi_state = ANSI_STATE_NORMAL;
static char s_csi_buf[32];
static int s_csi_idx = 0;

static void lcd_draw_char(int col, int row, uint8_t ch, uint16_t fg, uint16_t bg)
{
    if (!s_fb || col < 0 || col >= CONSOLE_COLS || row < 0 || row >= CONSOLE_ROWS) return;
    if (ch > 127) ch = '?';

    const uint8_t *bitmap = font8x16[ch];
    int start_x = col * FONT_W;
    int start_y = row * FONT_H;

    for (int y = 0; y < FONT_H; y++) {
        uint8_t bits = bitmap[y];
        uint16_t *line = &s_fb[(start_y + y) * LCD_H_RES + start_x];
        for (int x = 0; x < FONT_W; x++) {
            line[x] = (bits & (0x80 >> x)) ? fg : bg;
        }
    }
}

static void lcd_scroll_up(void)
{
    if (!s_fb) return;
    // Move rows 1..CONSOLE_ROWS-1 up to 0..CONSOLE_ROWS-2
    memmove(s_fb, s_fb + LCD_H_RES * FONT_H, LCD_H_RES * (LCD_V_RES - FONT_H) * sizeof(uint16_t));
    // Clear last line
    memset(s_fb + LCD_H_RES * (LCD_V_RES - FONT_H), 0, LCD_H_RES * FONT_H * sizeof(uint16_t));
}

static void lcd_clear_screen(void)
{
    if (!s_fb) return;
    memset(s_fb, 0, LCD_H_RES * LCD_V_RES * sizeof(uint16_t));
    s_cursor_x = 0;
    s_cursor_y = 0;
}

static void handle_csi(void)
{
    s_csi_buf[s_csi_idx] = '\0';
    char cmd = s_csi_buf[s_csi_idx - 1];

    if (cmd == 'J') {
        // Clear screen
        lcd_clear_screen();
    } else if (cmd == 'K') {
        // Clear line from cursor
        if (s_fb) {
            for (int c = s_cursor_x; c < CONSOLE_COLS; c++) {
                lcd_draw_char(c, s_cursor_y, ' ', s_fg_color, s_bg_color);
            }
        }
    } else if (cmd == 'H' || cmd == 'f') {
        // Move cursor: [row;colH
        int r = 1, c = 1;
        sscanf(s_csi_buf, "%d;%d", &r, &c);
        s_cursor_y = (r > 0 ? r - 1 : 0);
        s_cursor_x = (c > 0 ? c - 1 : 0);
        if (s_cursor_y >= CONSOLE_ROWS) s_cursor_y = CONSOLE_ROWS - 1;
        if (s_cursor_x >= CONSOLE_COLS) s_cursor_x = CONSOLE_COLS - 1;
    } else if (cmd == 'm') {
        // SGR Color codes
        int code = 0;
        char *p = s_csi_buf;
        while (*p && *p != 'm') {
            if (isdigit((unsigned char)*p)) {
                code = atoi(p);
                if (code == 0) {
                    s_fg_color = 0x07E0; // Reset to green
                    s_bg_color = 0x0000;
                } else if (code == 1) {
                    s_fg_color = 0x27E0; // Bright green
                } else if (code == 30) s_fg_color = 0x0000; // Black
                else if (code == 31) s_fg_color = 0xF800; // Red
                else if (code == 32) s_fg_color = 0x07E0; // Green
                else if (code == 33) s_fg_color = 0xFFE0; // Yellow
                else if (code == 34) s_fg_color = 0x001F; // Blue
                else if (code == 35) s_fg_color = 0xF81F; // Magenta
                else if (code == 36) s_fg_color = 0x07FF; // Cyan
                else if (code == 37) s_fg_color = 0xFFFF; // White
                while (isdigit((unsigned char)*p)) p++;
            } else {
                p++;
            }
        }
    }
}

void cyberdeck_console_write(const char *buf, size_t len)
{
    if (!s_fb || !buf) return;

    for (size_t i = 0; i < len; i++) {
        char c = buf[i];

        if (s_ansi_state == ANSI_STATE_NORMAL) {
            if (c == 0x1B) {
                s_ansi_state = ANSI_STATE_ESC;
            } else if (c == '\n') {
                s_cursor_x = 0;
                s_cursor_y++;
                if (s_cursor_y >= CONSOLE_ROWS) {
                    lcd_scroll_up();
                    s_cursor_y = CONSOLE_ROWS - 1;
                }
            } else if (c == '\r') {
                s_cursor_x = 0;
            } else if (c == '\b') {
                if (s_cursor_x > 0) {
                    s_cursor_x--;
                    lcd_draw_char(s_cursor_x, s_cursor_y, ' ', s_fg_color, s_bg_color);
                }
            } else if (c == '\t') {
                s_cursor_x = (s_cursor_x + 8) & ~7;
                if (s_cursor_x >= CONSOLE_COLS) {
                    s_cursor_x = 0;
                    s_cursor_y++;
                    if (s_cursor_y >= CONSOLE_ROWS) {
                        lcd_scroll_up();
                        s_cursor_y = CONSOLE_ROWS - 1;
                    }
                }
            } else if ((uint8_t)c >= 32) {
                lcd_draw_char(s_cursor_x, s_cursor_y, (uint8_t)c, s_fg_color, s_bg_color);
                s_cursor_x++;
                if (s_cursor_x >= CONSOLE_COLS) {
                    s_cursor_x = 0;
                    s_cursor_y++;
                    if (s_cursor_y >= CONSOLE_ROWS) {
                        lcd_scroll_up();
                        s_cursor_y = CONSOLE_ROWS - 1;
                    }
                }
            }
        } else if (s_ansi_state == ANSI_STATE_ESC) {
            if (c == '[') {
                s_ansi_state = ANSI_STATE_CSI;
                s_csi_idx = 0;
            } else {
                s_ansi_state = ANSI_STATE_NORMAL;
            }
        } else if (s_ansi_state == ANSI_STATE_CSI) {
            if (s_csi_idx < sizeof(s_csi_buf) - 2) {
                s_csi_buf[s_csi_idx++] = c;
            }
            if ((c >= '@' && c <= '~') && c != '[') {
                handle_csi();
                s_ansi_state = ANSI_STATE_NORMAL;
            }
        }
    }
}

uint16_t *cyberdeck_get_framebuffer(int *width, int *height)
{
    if (width) *width = LCD_H_RES;
    if (height) *height = LCD_V_RES;
    return s_fb;
}

// --- MIPI-DSI Display Initialization ---
static esp_err_t lcd_init(void)
{
    ESP_LOGI(TAG, "Configuring LDO 3 (2.5V) for MIPI DPHY...");
    esp_ldo_channel_config_t ldo_cfg = {
        .chan_id = 3,
        .voltage_mv = 2500,
    };
    esp_err_t ret = esp_ldo_acquire_channel(&ldo_cfg, &s_ldo_mipi_phy);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "LDO acquire failed: %s", esp_err_to_name(ret));
    }

    ESP_LOGI(TAG, "Initializing MIPI-DSI Host Bus (2 data lanes)...");
    esp_lcd_dsi_bus_handle_t mipi_dsi_bus;
    esp_lcd_dsi_bus_config_t bus_config = EK79007_PANEL_BUS_DSI_2CH_CONFIG();
    ret = esp_lcd_new_dsi_bus(&bus_config, &mipi_dsi_bus);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_lcd_new_dsi_bus failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Initializing DBI IO for EK79007...");
    esp_lcd_panel_io_handle_t mipi_dbi_io;
    esp_lcd_dbi_io_config_t dbi_config = EK79007_PANEL_IO_DBI_CONFIG();
    ret = esp_lcd_new_panel_io_dbi(mipi_dsi_bus, &dbi_config, &mipi_dbi_io);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_lcd_new_panel_io_dbi failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Configuring DPI panel 1024x600 60Hz RGB565...");
    esp_lcd_dpi_panel_config_t dpi_config = EK79007_1024_600_PANEL_60HZ_CONFIG(LCD_COLOR_PIXEL_FORMAT_RGB565);
    ek79007_vendor_config_t vendor_config = {
        .mipi_config = {
            .dsi_bus = mipi_dsi_bus,
            .dpi_config = &dpi_config,
            .lane_num = 2,
        },
    };
    esp_lcd_panel_dev_config_t lcd_dev_config = {
        .reset_gpio_num = -1,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
        .vendor_config = &vendor_config,
    };

    ret = esp_lcd_new_panel_ek79007(mipi_dbi_io, &lcd_dev_config, &s_lcd_panel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_lcd_new_panel_ek79007 failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Resetting and initializing panel...");
    esp_lcd_panel_reset(s_lcd_panel);
    esp_lcd_panel_init(s_lcd_panel);

    ret = esp_lcd_dpi_panel_get_frame_buffer(s_lcd_panel, 1, (void **)&s_fb);
    if (ret != ESP_OK || !s_fb) {
        ESP_LOGE(TAG, "Failed to get framebuffer pointer: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "✓ MIPI-DSI LCD 1024x600 Ready! Framebuffer: %p", s_fb);
    lcd_clear_screen();

    // Welcome banner on LCD
    const char *banner =
        "\033[32m"
        "========================================================================================\n"
        "             ESP32-P4 CYBERDECK LINUX (RV32IMA @ 400MHz, 32MB PSRAM)                   \n"
        "                      Console: 1024x600 MIPI-DSI | USB Host Ready                      \n"
        "========================================================================================\n\033[0m\n";
    cyberdeck_console_write(banner, strlen(banner));

    return ESP_OK;
}

// --- USB Host HID Keyboard Handling ---
static const char hid_scancode_ascii[128] = {
    0, 0, 0, 0,
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', // 0x04-0x10
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', // 0x11-0x1d
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',               // 0x1e-0x27
    '\r', 0x1b, 0x7f, '\t', ' ', '-', '=', '[', ']', '\\', '#', ';', '\'', '`', ',', '.', '/' // 0x28-0x38
};

static const char hid_scancode_shift[128] = {
    0, 0, 0, 0,
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    '!', '@', '#', '$', '%', '^', '&', '*', '(', ')',
    '\r', 0x1b, 0x7f, '\t', ' ', '_', '+', '{', '}', '|', '~', ':', '"', '~', '<', '>', '?'
};

static uint8_t s_prev_keys[6] = {0};

void cyberdeck_keyboard_input_char(uint8_t c)
{
    uart_rx_push(c);
}

static void decode_keyboard_report(const hid_keyboard_input_report_boot_t *report)
{
    bool shift = report->modifier.left_shift || report->modifier.right_shift;
    bool ctrl = report->modifier.left_ctr || report->modifier.rigth_ctr;

    for (int i = 0; i < 6; i++) {
        uint8_t key = report->key[i];
        if (key == 0) continue;

        // Check if key was already pressed in previous report (debounce/repeat handled by key up)
        bool already_pressed = false;
        for (int j = 0; j < 6; j++) {
            if (s_prev_keys[j] == key) {
                already_pressed = true;
                break;
            }
        }
        if (already_pressed) continue;

        // Handle Arrow keys
        if (key == 0x52) { // Up
            cyberdeck_keyboard_input_char('\033');
            cyberdeck_keyboard_input_char('[');
            cyberdeck_keyboard_input_char('A');
            continue;
        } else if (key == 0x51) { // Down
            cyberdeck_keyboard_input_char('\033');
            cyberdeck_keyboard_input_char('[');
            cyberdeck_keyboard_input_char('B');
            continue;
        } else if (key == 0x4F) { // Right
            cyberdeck_keyboard_input_char('\033');
            cyberdeck_keyboard_input_char('[');
            cyberdeck_keyboard_input_char('C');
            continue;
        } else if (key == 0x50) { // Left
            cyberdeck_keyboard_input_char('\033');
            cyberdeck_keyboard_input_char('[');
            cyberdeck_keyboard_input_char('D');
            continue;
        }

        // Standard characters
        if (key < 128) {
            char ch = shift ? hid_scancode_shift[key] : hid_scancode_ascii[key];
            if (ch != 0) {
                if (ctrl && (ch >= 'a' && ch <= 'z')) {
                    ch = ch - 'a' + 1; // Ctrl+A..Z -> 1..26
                }
                cyberdeck_keyboard_input_char((uint8_t)ch);
            }
        }
    }

    memcpy(s_prev_keys, report->key, sizeof(s_prev_keys));
}

static void hid_interface_callback(hid_host_device_handle_t hid_device_handle,
                                   const hid_host_interface_event_t event,
                                   void *arg)
{
    if (event == HID_HOST_INTERFACE_EVENT_INPUT_REPORT) {
        uint8_t data[64] = {0};
        size_t data_len = 0;
        hid_host_device_get_raw_input_report_data(hid_device_handle, data, sizeof(data), &data_len);
        if (data_len >= sizeof(hid_keyboard_input_report_boot_t)) {
            decode_keyboard_report((const hid_keyboard_input_report_boot_t *)data);
        }
    } else if (event == HID_HOST_INTERFACE_EVENT_DISCONNECTED) {
        ESP_LOGI(TAG, "HID Device disconnected");
        hid_host_device_close(hid_device_handle);
    }
}

static void hid_host_driver_callback(hid_host_device_handle_t hid_device_handle,
                                     const hid_host_driver_event_t event,
                                     void *arg)
{
    if (event == HID_HOST_DRIVER_EVENT_CONNECTED) {
        ESP_LOGI(TAG, "HID Device connected! Opening...");
        const hid_host_device_config_t dev_config = {
            .callback = hid_interface_callback,
            .callback_arg = NULL,
        };
        if (hid_host_device_open(hid_device_handle, &dev_config) == ESP_OK) {
            hid_class_request_set_protocol(hid_device_handle, HID_REPORT_PROTOCOL_BOOT);
            hid_class_request_set_idle(hid_device_handle, 0, 0);
            hid_host_device_start(hid_device_handle);
            ESP_LOGI(TAG, "✓ USB Keyboard Ready for Linux input!");
        }
    }
}

// --- USB Mass Storage (MSC) Handling ---
static msc_host_device_handle_t s_msc_device = NULL;
static msc_host_vfs_handle_t s_msc_vfs = NULL;

static void msc_event_callback(const msc_host_event_t *event, void *arg)
{
    if (event->event == MSC_DEVICE_CONNECTED) {
        ESP_LOGI(TAG, "USB MSC Flash Drive connected! Address: %d", event->device.address);
        esp_err_t ret = msc_host_install_device(event->device.address, &s_msc_device);
        if (ret == ESP_OK) {
            const esp_vfs_fat_mount_config_t mount_config = {
                .format_if_mount_failed = false,
                .max_files = 5,
                .allocation_unit_size = 16 * 1024,
            };
            ret = msc_host_vfs_register(s_msc_device, "/usbdrive", &mount_config, &s_msc_vfs);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "✓ USB Flash Drive mounted at /usbdrive!");
                const char *msg = "\n\033[36m[USB STORAGE] Flash drive connected & mounted at /usbdrive\033[0m\n";
                cyberdeck_console_write(msg, strlen(msg));

                DIR *d = opendir("/usbdrive");
                if (d) {
                    printf("\n--- Файлы на USB-флешке (/usbdrive) ---\n");
                    struct dirent *de;
                    int count = 0;
                    while ((de = readdir(d)) != NULL) {
                        printf("  [%s] %s\n", (de->d_type == DT_DIR) ? "DIR " : "FILE", de->d_name);
                        count++;
                    }
                    closedir(d);
                    if (count == 0) printf("  (Флешка пуста)\n");
                    printf("--------------------------------------\n\n");
                }
            } else {
                ESP_LOGE(TAG, "msc_host_vfs_register failed: %s", esp_err_to_name(ret));
            }
        }
    } else if (event->event == MSC_DEVICE_DISCONNECTED) {
        ESP_LOGI(TAG, "USB Flash Drive disconnected");
        if (s_msc_vfs) {
            msc_host_vfs_unregister(s_msc_vfs);
            s_msc_vfs = NULL;
        }
        if (s_msc_device) {
            msc_host_uninstall_device(s_msc_device);
            s_msc_device = NULL;
        }
    }
}

static void usb_lib_task(void *arg)
{
    ESP_LOGI(TAG, "Starting USB Host Library event loop...");
    while (1) {
        uint32_t event_flags;
        usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
    }
}

static esp_err_t usb_host_init_all(void)
{
    ESP_LOGI(TAG, "Installing USB Host driver...");
    const usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };
    esp_err_t ret = usb_host_install(&host_config);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "usb_host_install returned %s", esp_err_to_name(ret));
        return ret;
    }

    xTaskCreatePinnedToCore(usb_lib_task, "usb_events", 4096, NULL, 5, NULL, 1);

    ESP_LOGI(TAG, "Installing USB Host HID Driver...");
    const hid_host_driver_config_t hid_cfg = {
        .create_background_task = true,
        .task_priority = 5,
        .stack_size = 4096,
        .core_id = 1,
        .callback = hid_host_driver_callback,
        .callback_arg = NULL,
    };
    ret = hid_host_install(&hid_cfg);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "hid_host_install failed: %s", esp_err_to_name(ret));
    }

    ESP_LOGI(TAG, "Installing USB Host MSC Driver...");
    const msc_host_driver_config_t msc_cfg = {
        .create_backround_task = true,
        .task_priority = 5,
        .stack_size = 4096,
        .core_id = 1,
        .callback = msc_event_callback,
        .callback_arg = NULL,
    };
    ret = msc_host_install(&msc_cfg);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "msc_host_install failed: %s", esp_err_to_name(ret));
    }

    return ESP_OK;
}

esp_err_t cyberdeck_init(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "    INITIALIZING ESP32-P4 CYBERDECK     ");
    ESP_LOGI(TAG, "========================================");

    // 1. Initialize MIPI-DSI LCD
    lcd_init();

    // 2. Initialize USB Host (HID Keyboard + MSC Drive)
    usb_host_init_all();

    return ESP_OK;
}
