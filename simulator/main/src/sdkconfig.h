
#include <stdbool.h>
#include <stdint.h>

#ifndef SDKCONFIG_H
#define SDKCONFIG_H

#define CONFIG_SD2IEC_USE_SDCARD 1
#define CONFIG_SD2IEC_USE_SPI_PARTITION 1
#define CONFIG_REMOTE_DISPLAY 1
#define CONFIG_LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_24 1


extern uint8_t device_address;


typedef enum {
        ESP_OK
} esp_err_t;


typedef enum {
    ESP_LOG_NONE,       /*!< No log output */
    ESP_LOG_ERROR,      /*!< Critical errors, software module can not recover on its own */
    ESP_LOG_WARN,       /*!< Error conditions from which recovery measures have been taken */
    ESP_LOG_INFO,       /*!< Information messages which describe normal flow of events */
    ESP_LOG_DEBUG,      /*!< Extra information which is not necessary for normal use (values, pointers, sizes, etc). */
    ESP_LOG_VERBOSE     /*!< Bigger chunks of debugging information, or frequent messages which can potentially flood the output. */
} esp_log_level_t;

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

void esp_log_write(esp_log_level_t level, const char* tag, const char* format, ...) __attribute__ ((format (printf, 3, 4)));


#define LOG_FORMAT(letter, format)  LOG_COLOR_ ## letter #letter " (%" PRIu32 ") %s: " format LOG_RESET_COLOR "\n"

#define ESP_LOG_LEVEL(level, tag, format, ...) do {                     \
        if (level==ESP_LOG_ERROR )          { esp_log_write(ESP_LOG_ERROR,      tag, LOG_FORMAT(E, format), esp_log_timestamp(), tag, ##__VA_ARGS__); } \
        else if (level==ESP_LOG_WARN )      { esp_log_write(ESP_LOG_WARN,       tag, LOG_FORMAT(W, format), esp_log_timestamp(), tag, ##__VA_ARGS__); } \
        else if (level==ESP_LOG_DEBUG )     { esp_log_write(ESP_LOG_DEBUG,      tag, LOG_FORMAT(D, format), esp_log_timestamp(), tag, ##__VA_ARGS__); } \
        else if (level==ESP_LOG_VERBOSE )   { esp_log_write(ESP_LOG_VERBOSE,    tag, LOG_FORMAT(V, format), esp_log_timestamp(), tag, ##__VA_ARGS__); } \
        else                                { esp_log_write(ESP_LOG_INFO,       tag, LOG_FORMAT(I, format), esp_log_timestamp(), tag, ##__VA_ARGS__); } \
    } while(0)


#define ESP_LOG_LEVEL_LOCAL2(level, tag, format, ...) do {               \
        if ( LOG_LOCAL_LEVEL >= level ) ESP_LOG_LEVEL(level, tag, format, ##__VA_ARGS__); \
    } while(0)

#define ESP_LOG_LEVEL_LOCAL(level, tag, format, ...) do {               \
        printf(format "\n", ##__VA_ARGS__); \
    } while(0)

#define ESP_LOGE( tag, format, ... ) ESP_LOG_LEVEL_LOCAL(ESP_LOG_ERROR,   tag, format __VA_OPT__(,) __VA_ARGS__)
#define ESP_LOGW( tag, format, ... ) ESP_LOG_LEVEL_LOCAL(ESP_LOG_WARN,    tag, format __VA_OPT__(,) __VA_ARGS__)
#define ESP_LOGI( tag, format, ... ) ESP_LOG_LEVEL_LOCAL(ESP_LOG_INFO,    tag, format __VA_OPT__(,) __VA_ARGS__)
#define ESP_LOGD( tag, format, ... ) ESP_LOG_LEVEL_LOCAL(ESP_LOG_DEBUG,   tag, format __VA_OPT__(,) __VA_ARGS__)
#define ESP_LOGV( tag, format, ... ) ESP_LOG_LEVEL_LOCAL(ESP_LOG_VERBOSE, tag, format __VA_OPT__(,) __VA_ARGS__)


#endif
