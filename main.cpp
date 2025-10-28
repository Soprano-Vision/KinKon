/**
 * KINKON
 * 
 * Copyright (C)2025 SopranoVision. All rights reserved.
 ** /

/* ---------------------------------------------------------------
   Recipe:

   M/B:      RP2040-Zero (compatible)
   Player:   DFPlayer mini (compatible) + ~32GB microSD card
   GPS:      NEO 8M (compatible)
   OLED:     SSD1306 (compatible)
   Speaker:  8ohm 1W (compatible)
   PSU:      USB A or C 5V-2A
   Wires:    Many...

                   +------USB------+
DFPlayer Vcc <-  5V     |     |     00 -> DFPPlayer RX (UART)
DFP,GPS,OLED <-> GND    |_____|     01 <- GPS TX (UART)  
GPS,OLED Vcc <-  3V3                02
                 29                 03
                 28   RP2040 Zero   04 -> OLED SDA RX/TX (I2C)
        (GND <-) 27                 05 -> OLED SCL RX/TX (I2C)
        (GND <-) 26                 06
        (GND <-) 15                 07
        (GND <-) 14                 08 -> Debugger RX (UART)
                    13 12 11 10 09

  ----------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <map>
#include <string>
#include <vector>
#include <set>
#include <sstream>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"
#include "./libs/pico-ssd1306/ssd1306.h"
#include "./libs/pico-ssd1306/textRenderer/TextRenderer.h"
#include "./libs/pico-ssd1306/shapeRenderer/ShapeRenderer.h"


// Profiles
#define NAME            "KINKON"
#define VERSION         "V.1.8"
#define COPYRIGHT       "(C)2025"
#define COMPANY_NAME    "SopranoVision"

// UART Port
#define DFPLAYER_GPS_UART   uart0
#define DEBUG_UART          uart1

// GPIO Port
#define PIN_DFPLAYER_TX 0   // GPIO0 -> DFPlayer RX
#define PIN_GPS_RX      1   // GPIO1 <- GPS TX
#define PIN_DSPLAY_SDA  4   // GPIO4 -> OLED SDA TX
#define PIN_DSPLAY_SCL  5   // GPIO5 -> OLED SCL TX
#define PIN_DEBUG_TX    8   // GPIO4 -> Debugger TX
#define PIN_SETUP_35    14  // GPIO14 <- Speed setting (35km/h)
#define PIN_SETUP_85    15  // GPIO15 <- Speed setting (85km/h)
#define PIN_SETUP_105   26  // GPIO26 <- Speed setting (105km/h)
#define PIN_SETUP_MILE  27  // GPIO26 <- Speed setting (Display mile)

// Comm Params
#define BAUD_DFPLAYER_GPS   9600    // Baud rate for DFPLAYER/GPS
#define BAUD_DEBUG          9600    // Baud rate for DEBUG

// OLED Params
#define DISPLAY_I2C_ADDR    0x3c

#define SAT_PARGE_COUNT     10

// DFPlayer mini's command. （8 bytes, CRC is ignore.）
const uint8_t DFPLAYER_LOOP_CMD[] = {0x7E, 0xFF, 0x06, 0x11, 0x00, 0x00, 0x01, 0xEF};
const uint8_t DFPLAYER_STOP_CMD[] = {0x7E, 0xFF, 0x06, 0x16, 0x00, 0x00, 0x00, 0xEF};
const uint8_t DFPLAYER_VOL05_CMD[] = {0x7E, 0xFF, 0x06, 0x06, 0x00, 0x00, 5, 0xEF}; // Volume:5=0x05
const uint8_t DFPLAYER_VOL30_CMD[] = {0x7E, 0xFF, 0x06, 0x06, 0x00, 0x00, 30, 0xEF};  // Volume:5=0x05

struct SAT_INFO
 {
    bool    locked  = false;    // GSA: Is using?
    float   snr     = 0.0f;     // GSV: SNR Signal level.
    int     loss    = 0;        // GSV: Lossted count.
};

// Globals
float SPEED_THRESHOLD_KMH = 3.0f;
pico_ssd1306::SSD1306 *display;

/// @brief 
/// @param  
void InitDisplay(void)
{
    // Port setup...
    i2c_init(i2c0, 400 * 1000);
    gpio_set_function(PIN_DSPLAY_SDA, GPIO_FUNC_I2C); // SDA
    gpio_set_function(PIN_DSPLAY_SCL, GPIO_FUNC_I2C); // SCL
    gpio_pull_up(PIN_DSPLAY_SDA);
    gpio_pull_up(PIN_DSPLAY_SCL);

    // Make instance
    display = new pico_ssd1306::SSD1306(i2c0, DISPLAY_I2C_ADDR, pico_ssd1306::Size::W128xH64);
}

/// @brief 
/// @param  
void InitUART(void)
{
    // UART0: DFPlayer TX + GPS RX
    uart_init(DFPLAYER_GPS_UART, BAUD_DFPLAYER_GPS);
    uart_set_format(DFPLAYER_GPS_UART, 8, 1, UART_PARITY_NONE);
    gpio_set_function(PIN_DFPLAYER_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_GPS_RX, GPIO_FUNC_UART);

    // UART1: Debug output
    uart_init(DEBUG_UART, BAUD_DEBUG);
    uart_set_format(DEBUG_UART, 8, 1, UART_PARITY_NONE);
    gpio_set_function(PIN_DEBUG_TX, GPIO_FUNC_UART);

    sleep_ms(500);
}

/// @brief 
/// @param  
void InitGPIO(void)
{
    gpio_init(PIN_SETUP_35);
    gpio_set_dir(PIN_SETUP_35, GPIO_IN);
    gpio_pull_up(PIN_SETUP_35);

    gpio_init(PIN_SETUP_85);
    gpio_set_dir(PIN_SETUP_85, GPIO_IN);
    gpio_pull_up(PIN_SETUP_85);

    gpio_init(PIN_SETUP_105);
    gpio_set_dir(PIN_SETUP_105, GPIO_IN);
    gpio_pull_up(PIN_SETUP_105);

    gpio_init(PIN_SETUP_MILE);
    gpio_set_dir(PIN_SETUP_MILE, GPIO_IN);
    gpio_pull_up(PIN_SETUP_MILE);
}

/// @brief 
/// @param display 
void DrawSplash(pico_ssd1306::SSD1306* display)
{
    display->clear();

    pico_ssd1306::drawText(display, font_12x16, NAME, 0, 0);
    pico_ssd1306::drawText(display, font_12x16, VERSION, 0, 16);
    pico_ssd1306::drawText(display, font_8x8, COPYRIGHT, 0, 32);
    pico_ssd1306::drawText(display, font_8x8, COMPANY_NAME, 0, 40);

    display->sendBuffer();
}

/// @brief 
/// @param display 
/// @param sat_infos 
/// @param message 
/// @param speed 
void UpdateDisplay(pico_ssd1306::SSD1306* display,
                   const std::map<int, SAT_INFO> &sat_infos,
                   const char* message,
                   const float speed,
                   const bool is_mile)
{
    using namespace pico_ssd1306;

    display->clear();

    // 上半分にメッセージと速度表示
    char temp[32];
    sprintf(temp, "%.1f %s", is_mile ? speed, "mile" : "km/h");
    drawText(display, font_12x16, temp, 0, 0);
    drawText(display, font_8x8, message, 0, 16);

    // Max SNR line.（60dB）→ y = 31
    drawLine(display, 0, 31, 127, 31);

    // Fixable SNR line.（30dB）→ y = 47
    drawLine(display, 0, 47, 127, 47);

    // Graph of SNR.（y=32〜63）
    const int bar_area_top = 32;
    const int bar_area_bottom = 63;
    const int bar_area_height = bar_area_bottom - bar_area_top + 1;

    int x = 0;
    for (const auto &[id, info] : sat_infos)
    {
        if (x + 4 > 128)
        {
            break;
        }

        int bar_height = static_cast<int>(info.snr / 60.0f * bar_area_height);
        int y = bar_area_bottom - bar_height + 1;

        if (y < bar_area_top)
         {
            y = bar_area_top;
            bar_height = bar_area_bottom - bar_area_top + 1;
        }

        if (info.locked)
        {
            fillRect(display, x, y, x + 4, y + bar_height);
        }
        else
        {
            drawRect(display, x, y, x + 4, y + bar_height);
        }

        x += 5;
    }

    display->sendBuffer();
}

/// @brief 
/// @param msg 
void debug_out(const char *msg)
{
    uart_puts(DEBUG_UART, msg);
}

/// @brief 
/// @param knots 
/// @return 
inline float knots_to_kmh(const float knots)
{
    return knots * 1.852f;
}

/// @brief 
/// @param sentence 
/// @param is_active 
/// @return 
float parseRMC(const char *sentence, bool *is_active)
{
    char copy[256];
    strncpy(copy, sentence, sizeof(copy));
    copy[sizeof(copy) - 1] = '\0';

    char *tokens[12]; 
    int i = 0;
    char *token = strtok(copy, ",");

    while (token && i < 12)
    {
        tokens[i++] = token;
        token = strtok(NULL, ",");
    }

    if (i >= 8 && tokens[7] && strlen(tokens[7]) > 0)
    {
        if (tokens[2][0] == 'A')
        {
            *is_active = true;
        }
        else
        {
            *is_active = false;
        }

        return knots_to_kmh(strtof(tokens[7], NULL));
    }

    return 0.0f;
}

/// @brief 
/// @param sentence 
/// @param sat_infos 
void parseGSV(const char *sentence, std::map<int, SAT_INFO> &sat_infos)
{
    std::stringstream ss(sentence);
    std::string token;
    std::vector<std::string> fields;

    while (std::getline(ss, token, ','))
    {
        fields.push_back(token);
    }

    std::set<int> seen_ids;

    for (size_t i = 4; i + 3 < fields.size(); i += 4)
    {
        if (!fields[i].empty())
        {
            int sat_id = std::atoi(fields[i].c_str());
            float snr = fields[i + 3].empty() ? 0.0f : std::atof(fields[i + 3].c_str());

            SAT_INFO &info = sat_infos[sat_id];
            info.snr = snr;
            info.loss = 0; // Reset!
            seen_ids.insert(sat_id);
        }
    }

    // Incriment counter, if not received sat info, 
    for (auto &[id, info] : sat_infos)
    {
        if (seen_ids.find(id) == seen_ids.end())
        {
            info.loss++;
        }
    }

    // If lassed 10 counts, remove info.
    for (auto it = sat_infos.begin(); it != sat_infos.end();)
    {
        if (it->second.loss >= SAT_PARGE_COUNT)
        {
            it = sat_infos.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

/// @brief 
/// @param sentence 
/// @param sat_infos 
void parseGSA(const char *sentence, std::map<int, SAT_INFO> &sat_infos)
{
    std::stringstream ss(sentence);
    std::string token;
    std::vector<std::string> fields;

    while (std::getline(ss, token, ','))
    {
        fields.push_back(token);
    }

    for (auto &[id, info] : sat_infos)
    {
        info.locked = false;
    }

    for (size_t i = 3; i <= 14 && i < fields.size(); ++i)
    {
        if (!fields[i].empty())
        {
            int sat_id = std::atoi(fields[i].c_str());
            sat_infos[sat_id].locked = true;
        }
    }
}

/// @brief 
/// @param cmd 
/// @param len 
void SendDFPlayerCmt(const uint8_t* cmd, size_t len)
{
    uart_write_blocking(DFPLAYER_GPS_UART, cmd, len);
    sleep_ms(100);
}

/// @brief 
/// @return 
int main()
{
    stdio_init_all();

    InitGPIO();

    InitDisplay();
    DrawSplash(display);

    InitUART();
    SendDFPlayerCmt(DFPLAYER_VOL05_CMD, sizeof(DFPLAYER_VOL05_CMD));
    SendDFPlayerCmt(DFPLAYER_LOOP_CMD, sizeof(DFPLAYER_LOOP_CMD));
    sleep_ms(2500);
    SendDFPlayerCmt(DFPLAYER_STOP_CMD, sizeof(DFPLAYER_STOP_CMD));
    
    std::map<int, SAT_INFO> sat_infos;

    bool is_playing = true;
    char line[256];
    int i = 0;
    bool is_mile = false;

    while (true)
    {
        if (gpio_get(PIN_SETUP_35) == 0)
        {
            if (SPEED_THRESHOLD_KMH != 35.0f)
            {
                SendDFPlayerCmt(DFPLAYER_VOL30_CMD, sizeof(DFPLAYER_VOL30_CMD));
                SPEED_THRESHOLD_KMH = 35.0f;
                debug_out("*** Change to 35.0\n");
            }
        }
        else if (gpio_get(PIN_SETUP_85) == 0)
        {
            if (SPEED_THRESHOLD_KMH != 85.0f)
            {
                SendDFPlayerCmt(DFPLAYER_VOL30_CMD, sizeof(DFPLAYER_VOL30_CMD));
                SPEED_THRESHOLD_KMH = 85.0f;
                debug_out("*** Change to 85.0\n");
            }
        }
        else if (gpio_get(PIN_SETUP_105) == 0)
        {
            if (SPEED_THRESHOLD_KMH != 105.0f)
            {
                SendDFPlayerCmt(DFPLAYER_VOL30_CMD, sizeof(DFPLAYER_VOL30_CMD));
                SPEED_THRESHOLD_KMH = 105.0f;
                debug_out("*** Change to 105.0\n");
            }
        }
        else
        {
            if (SPEED_THRESHOLD_KMH != 3.0f)
            {
                SendDFPlayerCmt(DFPLAYER_VOL05_CMD, sizeof(DFPLAYER_VOL30_CMD));
                SPEED_THRESHOLD_KMH = 3.0f;
                debug_out("*** Change to 3.0\n");
            }
        }

        if (gpio_get(PIN_SETUP_MILE) == 0)
        {
            if (!is_mile)
            {
                is_mile = true;
                debug_out("*** Change to mile\n");
            }
        }
        else
        {
            if (is_mile)
            {
                is_mile = false;
                debug_out("*** Change to km/h\n");
            }
        }

        if (uart_is_readable(DFPLAYER_GPS_UART))
        {
            // debug_out("+");
            const char c = uart_getc(DFPLAYER_GPS_UART);
            // debug_out(".\n");

            if (i >= sizeof(line) - 1)
            {
                i = 0;
                debug_out("Response length over flow!\r\n");
                continue;
            }

            if (c == '$')
            {
                float speed = 0.0f;
                char message[128];
                line[i] = '\0';
                i = 0;
                float display_speed = 0.0f;

                bool update = false;

                if (strncmp(line, "$GNRMC", 6) == 0)
                {
                    update = true;
                    bool is_active = false;
                    speed = parseRMC(line, &is_active);
                    sprintf(message, "%s MODE:%d", is_active ? "FIXED" : "WAIT...", static_cast<int>(SPEED_THRESHOLD_KMH));

                    debug_out(message);
                    debug_out("[DEBUG] Recv:");
                    debug_out(line);

                    char buf[64];
                    display_speed = is_mile ? (speed * 0.621371f) : speed;
                    snprintf(buf, sizeof(buf), "[DEBUG] Speed: %.1f %s\n", display_speed, is_mile ? "mile" : "km/h");
                    debug_out(buf);

                    if (speed >= SPEED_THRESHOLD_KMH)
                    {
                        if (!is_playing)
                        {
                            SendDFPlayerCmt(DFPLAYER_LOOP_CMD, sizeof(DFPLAYER_LOOP_CMD));
                            is_playing = true;
                            debug_out("[DEBUG] PLAY!\r\n");
                        }
                    }
                    else
                    {
                        if (is_playing)
                        {
                            SendDFPlayerCmt(DFPLAYER_STOP_CMD, sizeof(DFPLAYER_STOP_CMD));
                            is_playing = false;
                            debug_out("[DEBUG] STOP!\r\n");
                        }
                    }
                }
                else if (strncmp(line + 3, "GSV", 3) == 0)
                {
                    parseGSV(line, sat_infos);
                }
                else if (strncmp(line, "$GNGSA", 6) == 0)
                {
                    parseGSA(line, sat_infos);
                }

                if (update)
                {
                    UpdateDisplay(display, sat_infos, message, speed, is_mile);
                }
            }

            line[i] = c;
            ++i;
        }
    }

    return 0;
}