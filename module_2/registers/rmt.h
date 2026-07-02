#pragma once
#include <stdint.h>
#include <stddef.h>

// один RMT-символ: дві ділянки (рівень + тривалість у тіках)
typedef struct {
    uint16_t dur0; uint8_t lvl0;
    uint16_t dur1; uint8_t lvl1;
} rmt_item_t;

// налаштувати канал і пін НА РЕГІСТРАХ; самі підберіть дільники під розрішення
void rmt_raw_init(int gpio, uint32_t resolution_hz);

// зібрати символи в 32-бітні слова, записати в CHnDATA (+ нуль-маркер) і запустити
void rmt_raw_send_items(const rmt_item_t *items, size_t count);

// зручна обгортка: тривалості в МІКРОСЕКУНДАХ, рівень чергується від start_level
void rmt_raw_send_pulses(const uint32_t *durations_us, size_t count, int start_level);

// меандр: частота (Гц), скважність (%), кількість періодів
void rmt_raw_square(uint32_t freq_hz, uint8_t duty_pct, uint32_t periods);

// дочекатися завершення (опитуванням TX_END)
void rmt_raw_wait(void);