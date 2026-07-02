#include <stdint.h>
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc.h"          /* REG_WRITE / REG_READ / DR_REG_GPIO_BASE */
#include "soc/gpio_reg.h"     /* імена регістрів GPIO */

static const char *TAG = "regadv";

#define LED  2u
#define BTN  8u

/* зсуви від DR_REG_GPIO_BASE (S3 TRM, Register Summary): */
#define OFF_OUT        0x04u
#define OFF_OUT_W1TS   0x08u
#define OFF_OUT_W1TC   0x0Cu
#define OFF_ENABLE     0x20u
#define OFF_ENABLE_SET 0x24u
#define OFF_ENABLE_CLR 0x28u
#define OFF_IN         0x3Cu

/* ПОВНІ абсолютні адреси на ESP32-S3 (база 0x60004000). UL — щоб збігалось
* з розрядністю вказівника і на платі, і при перевірці на ПК. */
  #define S3_OUT_REG        0x60004004UL
  #define S3_OUT_W1TS_REG   0x60004008UL
  #define S3_OUT_W1TC_REG   0x6000400CUL
  #define S3_ENABLE_REG     0x60004020UL
  #define S3_ENABLE_W1TS    0x60004024UL
  #define S3_ENABLE_W1TC    0x60004028UL
  #define S3_IN_REG         0x6000403CUL

static inline uint32_t out_bit(uint32_t pin) { return (REG_READ(GPIO_OUT_REG) >> pin) & 1u; }
static void show_reg(const char *name, uint32_t addr, uint32_t off, const char *acc)
{
ESP_LOGI(TAG, "%-22s = 0x%08X = base+0x%02X | %s", name, (unsigned)addr, (unsigned)off, acc);
}


/* ===== ДЕМО 1: карта регістрів — повні адреси S3 ===== */
void demo1_map(void)
{
ESP_LOGI(TAG, "== Демо 1: карта регістрів GPIO (ESP32-S3) ==");
ESP_LOGI(TAG, "DR_REG_GPIO_BASE = 0x%08X   [S3 TRM: IO MUX and GPIO Matrix]", (unsigned)DR_REG_GPIO_BASE);
show_reg("GPIO_OUT_REG",         GPIO_OUT_REG,         OFF_OUT,        "R/W");
show_reg("GPIO_OUT_W1TS_REG",    GPIO_OUT_W1TS_REG,    OFF_OUT_W1TS,   "WO");
show_reg("GPIO_OUT_W1TC_REG",    GPIO_OUT_W1TC_REG,    OFF_OUT_W1TC,   "WO");
show_reg("GPIO_ENABLE_REG",      GPIO_ENABLE_REG,      OFF_ENABLE,     "R/W");
show_reg("GPIO_ENABLE_W1TS_REG", GPIO_ENABLE_W1TS_REG, OFF_ENABLE_SET, "WO");
show_reg("GPIO_ENABLE_W1TC_REG", GPIO_ENABLE_W1TC_REG, OFF_ENABLE_CLR, "WO");
show_reg("GPIO_IN_REG",          GPIO_IN_REG,          OFF_IN,         "RO");
}
/* ▸ СПРОБУЙ САМ: переконайся, що imʼя-макрос дорівнює повному літералу (напр. 0x60004008). */


/* ===== ДЕМО 2: один регістр — ЧОТИРИ способи доступу (W1TS) ===== */
void demo2_fourways(void)
{
ESP_LOGI(TAG, "== Демо 2: W1TS чотирма способами (всі однакові) ==");
REG_WRITE(GPIO_ENABLE_W1TS_REG, 1u << LED);
uint32_t m = 1u << LED;

    REG_WRITE(GPIO_OUT_W1TS_REG, m);                                    /* 1) макрос */
    ESP_LOGI(TAG, "1) REG_WRITE(GPIO_OUT_W1TS_REG)            -> on  | OUT біт=%u", (unsigned)out_bit(LED));
    vTaskDelay(pdMS_TO_TICKS(300));  REG_WRITE(GPIO_OUT_W1TC_REG, m);  vTaskDelay(pdMS_TO_TICKS(150));

    *(volatile uint32_t *)GPIO_OUT_W1TS_REG = m;                        /* 2) вказівник на імʼя */
    ESP_LOGI(TAG, "2) *(volatile u32*)GPIO_OUT_W1TS_REG       -> on  | OUT біт=%u", (unsigned)out_bit(LED));
    vTaskDelay(pdMS_TO_TICKS(300));  REG_WRITE(GPIO_OUT_W1TC_REG, m);  vTaskDelay(pdMS_TO_TICKS(150));

    *(volatile uint32_t *)(DR_REG_GPIO_BASE + OFF_OUT_W1TS) = m;        /* 3) база + зсув */
    ESP_LOGI(TAG, "3) *(volatile u32*)(base + 0x08)           -> on  | OUT біт=%u", (unsigned)out_bit(LED));
    vTaskDelay(pdMS_TO_TICKS(300));  REG_WRITE(GPIO_OUT_W1TC_REG, m);  vTaskDelay(pdMS_TO_TICKS(150));

    *(volatile uint32_t *)0x60004008UL = m;                            /* 4) ПОВНА адреса S3 */
    ESP_LOGI(TAG, "4) *(volatile u32*)0x60004008  (повна S3)   -> on  | OUT біт=%u", (unsigned)out_bit(LED));
    vTaskDelay(pdMS_TO_TICKS(300));  REG_WRITE(GPIO_OUT_W1TC_REG, m);

    ESP_LOGI(TAG, "усі чотири рядки писали в ОДИН регістр 0x60004008");
}
/* ▸ СПРОБУЙ САМ: повтори чотири способи для W1TC (адреса 0x6000400C). */


/* ===== ДЕМО 3: напрям піна — макрос / повна адреса / RMW ===== */
void demo3_enable(void)
{
ESP_LOGI(TAG, "== Демо 3: зробити пін виходом — три записи ==");
uint32_t m = 1u << LED;
REG_WRITE(GPIO_ENABLE_W1TS_REG, m);                                          /* макрос W1TS */
ESP_LOGI(TAG, "макрос ENABLE_W1TS        | ENABLE біт=%u", (unsigned)((REG_READ(GPIO_ENABLE_REG) >> LED) & 1u));

    *(volatile uint32_t *)0x60004028UL = m;                                      /* повна адреса W1TC -> вхід */
    ESP_LOGI(TAG, "*(0x60004028) ENABLE_W1TC | ENABLE біт=%u", (unsigned)((REG_READ(GPIO_ENABLE_REG) >> LED) & 1u));

    REG_WRITE(GPIO_ENABLE_REG, REG_READ(GPIO_ENABLE_REG) | m);                   /* RMW на ENABLE_REG */
    ESP_LOGI(TAG, "ENABLE_REG |= 1<<%u        | ENABLE біт=%u", LED, (unsigned)((REG_READ(GPIO_ENABLE_REG) >> LED) & 1u));
}
/* ▸ СПРОБУЙ САМ: зроби пін входом через повну адресу 0x60004028 (W1TC). */


/* ===== ДЕМО 4: вихід W1TS/W1TC — макрос і повна адреса ===== */
void demo4_out(void)
{
ESP_LOGI(TAG, "== Демо 4: вихід — макрос vs повна адреса ==");
REG_WRITE(GPIO_ENABLE_W1TS_REG, 1u << LED);
uint32_t m = 1u << LED;

    REG_WRITE(GPIO_OUT_W1TS_REG, m);
    ESP_LOGI(TAG, "макрос W1TS: on  | OUT біт=%u", (unsigned)out_bit(LED));
    vTaskDelay(pdMS_TO_TICKS(300));
    *(volatile uint32_t *)0x6000400CUL = m;                /* повна адреса W1TC */
    ESP_LOGI(TAG, "*(0x6000400C): off | OUT біт=%u", (unsigned)out_bit(LED));
    vTaskDelay(pdMS_TO_TICKS(300));
}
/* ▸ СПРОБУЙ САМ: увімкни вихід через повну адресу 0x60004008. */


/* ===== ДЕМО 5: вихід НАПРЯМУ через OUT_REG (RMW) — макрос і адреса ===== */
void demo5_out_rmw(void)
{
ESP_LOGI(TAG, "== Демо 5: OUT_REG напряму (RMW) ==");
REG_WRITE(GPIO_ENABLE_W1TS_REG, 1u << LED);
uint32_t m = 1u << LED;

    REG_WRITE(GPIO_OUT_REG, REG_READ(GPIO_OUT_REG) | m);                          /* макрос RMW */
    ESP_LOGI(TAG, "OUT_REG |= 1<<%u (макрос)        -> OUT біт=%u", LED, (unsigned)out_bit(LED));
    vTaskDelay(pdMS_TO_TICKS(300));

    volatile uint32_t *out = (volatile uint32_t *)0x60004004UL;                   /* повна адреса OUT_REG */
    *out = *out & ~m;
    ESP_LOGI(TAG, "*(0x60004004) &= ~mask (адреса)  -> OUT біт=%u", (unsigned)out_bit(LED));
    ESP_LOGI(TAG, "RMW = три дії; W1TS/W1TC роблять те саме атомарно");
}
/* ▸ СПРОБУЙ САМ: поясни, чому в перериванні RMW небезпечний. */


/* ===== ДЕМО 6: toggle — XOR vs W1TS/W1TC ===== */
void demo6_toggle(void)
{
ESP_LOGI(TAG, "== Демо 6: toggle — XOR vs set/clear ==");
REG_WRITE(GPIO_ENABLE_W1TS_REG, 1u << LED);
uint32_t m = 1u << LED;
for (int i = 0; i < 4; i++) {
REG_WRITE(GPIO_OUT_REG, REG_READ(GPIO_OUT_REG) ^ m);
ESP_LOGI(TAG, "XOR toggle -> OUT біт=%u", (unsigned)out_bit(LED));
vTaskDelay(pdMS_TO_TICKS(200));
}
for (int i = 0; i < 4; i++) {
if (out_bit(LED)) REG_WRITE(GPIO_OUT_W1TC_REG, m);
else              REG_WRITE(GPIO_OUT_W1TS_REG, m);
ESP_LOGI(TAG, "set/clear toggle -> OUT біт=%u", (unsigned)out_bit(LED));
vTaskDelay(pdMS_TO_TICKS(200));
}
}
/* ▸ СПРОБУЙ САМ: який спосіб коротший у машинних інструкціях? (демо 9) */


/* ===== ДЕМО 7: кілька пінів ОДНІЄЮ маскою (через повну адресу) ===== */
void demo7_multi(void)
{
ESP_LOGI(TAG, "== Демо 7: три піни (2,4,5) однією записою ==");
uint32_t m = (1u << 2) | (1u << 4) | (1u << 5);
REG_WRITE(GPIO_ENABLE_W1TS_REG, m);
*(volatile uint32_t *)0x60004008UL = m;                /* повна адреса W1TS */
ESP_LOGI(TAG, "OUT_REG = 0x%08X (біти 2,4,5 разом)", (unsigned)REG_READ(GPIO_OUT_REG));
vTaskDelay(pdMS_TO_TICKS(500));
*(volatile uint32_t *)0x6000400CUL = m;                /* повна адреса W1TC */
ESP_LOGI(TAG, "OUT_REG = 0x%08X (усі згашені)", (unsigned)REG_READ(GPIO_OUT_REG));
ESP_LOGI(TAG, "один регістр керує багатьма пінами — це ховає HAL");
}
/* ▸ СПРОБУЙ САМ: «біжучий вогник» по бітах 2,4,5. */


/* ===== ДЕМО 8: читання входу — ЧОТИРИ способи (Reg GPIO_IN_REG, RO) ===== */
void demo8_in(void)
{
ESP_LOGI(TAG, "== Демо 8: читання входу чотирма способами ==");
gpio_config_t io = { .pin_bit_mask = 1ULL << BTN, .mode = GPIO_MODE_INPUT, .pull_up_en = 1 };
gpio_config(&io);

    int a = (REG_READ(GPIO_IN_REG) >> BTN) & 1u;                                  /* макрос */
    int b = (*(volatile uint32_t *)GPIO_IN_REG >> BTN) & 1u;                      /* імʼя-адреса */
    int c = (*(volatile uint32_t *)(DR_REG_GPIO_BASE + OFF_IN) >> BTN) & 1u;      /* база+зсув */
    int d = (*(volatile uint32_t *)0x6000403CUL >> BTN) & 1u;                     /* повна адреса */
    int h = gpio_get_level(BTN);                                                  /* HAL */
    ESP_LOGI(TAG, "макрос=%d  імʼя=%d  base+0x3C=%d  0x6000403C=%d  HAL=%d", a, b, c, d, h);
    ESP_LOGI(TAG, "усі пʼять читають один регістр входів — мають збігатися");
}
/* ▸ СПРОБУЙ САМ: у циклі ~5 с друкуй значення, натискаючи кнопку. */


/* ===== ДЕМО 9: швидкість — регістр проти HAL ===== */
void demo9_speed(void)
{
ESP_LOGI(TAG, "== Демо 9: швидкість — регістр проти HAL ==");
REG_WRITE(GPIO_ENABLE_W1TS_REG, 1u << LED);
const int N = 100000;
uint32_t m = 1u << LED;

    int64_t t0 = esp_timer_get_time();
    for (int i = 0; i < N; i++) { REG_WRITE(GPIO_OUT_W1TS_REG, m); REG_WRITE(GPIO_OUT_W1TC_REG, m); }
    int64_t t_reg = esp_timer_get_time() - t0;

    t0 = esp_timer_get_time();
    for (int i = 0; i < N; i++) { gpio_set_level(LED, 1); gpio_set_level(LED, 0); }
    int64_t t_hal = esp_timer_get_time() - t0;

    ESP_LOGI(TAG, "%d циклів toggle:", N);
    ESP_LOGI(TAG, "  регістр: %lld мкс  (~%.1f нс/операція)", (long long)t_reg, t_reg * 1000.0 / (2 * N));
    ESP_LOGI(TAG, "  HAL:     %lld мкс  (~%.1f нс/операція)", (long long)t_hal, t_hal * 1000.0 / (2 * N));
    ESP_LOGI(TAG, "  регістр швидший у ~%.1f раза — ось навіщо регістри", (double)t_hal / (double)t_reg);
}
/* ▸ СПРОБУЙ САМ: збільш N у 10 разів; заміряй ще й спосіб через повну адресу. */


void app_main(void)
{
demo1_map();        /* карта адрес S3 */
demo2_fourways();   /* W1TS чотирма способами */
demo3_enable();     /* напрям: макрос / адреса / RMW */
demo4_out();        /* вихід: макрос і повна адреса */
demo5_out_rmw();    /* OUT_REG напряму (RMW) */
demo6_toggle();     /* toggle: XOR vs set/clear */
demo7_multi();      /* кілька пінів однією маскою */
demo8_in();         /* вхід чотирма способами */
demo9_speed();      /* регістр проти HAL */
ESP_LOGI(TAG, "усі демо завершено.");
}