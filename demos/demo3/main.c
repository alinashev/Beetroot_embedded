#include <stdint.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_attr.h"            /* IRAM_ATTR */
#include "esp_intr_alloc.h"      /* esp_intr_alloc, ETS_GPIO_INTR_SOURCE */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc.h"             /* REG_WRITE / REG_READ / DR_REG_GPIO_BASE / BIT */
#include "soc/gpio_reg.h"        /* GPIO matrix + STATUS + GPIO_PIN0_REG */
#include "soc/io_mux_reg.h"      /* IO_MUX: FUN_IE / FUN_PU / FUN_DRV / MCU_SEL */
#include "soc/periph_defs.h"     /* ETS_GPIO_INTR_SOURCE */

static const char *TAG = "gpiohood";

#define LED  2u
#define BTN  8u

/* регістр конфігурації піна n: у IDF немає GPIO_PIN_REG(n), є лише GPIO_PIN0_REG;
 * регістри йдуть з кроком 4 байти, тож адресу рахуємо самі. */
#define GPIO_PIN_REG(n)  (GPIO_PIN0_REG + (n) * 4)


/* ===== ДЕМО 1: налаштування піна через IO_MUX (без gpio_config) ===== */
void demo1_iomux(void)
{
    ESP_LOGI(TAG, "== Демо 1: пін через IO_MUX руками ==");

    /* КНОПКА (GPIO8): не вихід (matrix) + у IO_MUX дозвіл входу й підтяжка */
    REG_WRITE(GPIO_ENABLE_W1TC_REG, 1u << BTN);
    uint32_t b = REG_READ(IO_MUX_GPIO8_REG);
    b &= ~(MCU_SEL << MCU_SEL_S);  b |= (PIN_FUNC_GPIO << MCU_SEL_S);   /* функція = GPIO */
    b |=  FUN_IE;                                                       /* дозволити вхід */
    b |=  FUN_PU;                                                       /* підтяжка вгору */
    b &= ~FUN_PD;                                                       /* без підтяжки вниз */
    REG_WRITE(IO_MUX_GPIO8_REG, b);
    ESP_LOGI(TAG, "кнопка GPIO8: FUN_IE+FUN_PU | IO_MUX=0x%08X", (unsigned)REG_READ(IO_MUX_GPIO8_REG));

    /* LED (GPIO2): вихід (matrix) + сила драйвера у IO_MUX */
    REG_WRITE(GPIO_ENABLE_W1TS_REG, 1u << LED);
    uint32_t l = REG_READ(IO_MUX_GPIO2_REG);
    l &= ~(MCU_SEL << MCU_SEL_S);  l |= (PIN_FUNC_GPIO << MCU_SEL_S);
    l &= ~(FUN_DRV << FUN_DRV_S);  l |= (3u << FUN_DRV_S);              /* сила 3 (~40 мА) */
    REG_WRITE(IO_MUX_GPIO2_REG, l);
    ESP_LOGI(TAG, "LED GPIO2: FUN_DRV=3 | IO_MUX=0x%08X", (unsigned)REG_READ(IO_MUX_GPIO2_REG));

    for (int i = 0; i < 6; i++) {
        REG_WRITE(GPIO_OUT_W1TS_REG, 1u << LED);
        int lvl = (REG_READ(GPIO_IN_REG) >> BTN) & 1u;
        ESP_LOGI(TAG, "LED on  | кнопка=%d %s", lvl, lvl ? "(відпущена)" : "(натиснута)");
        vTaskDelay(pdMS_TO_TICKS(300));
        REG_WRITE(GPIO_OUT_W1TC_REG, 1u << LED);
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}
/* ▸ СПРОБУЙ САМ: постав FUN_DRV=0 і порівняй яскравість світлодіода. */


/* ===== ДЕМО 2: переривання кнопки через регістри ===== */
static volatile uint32_t g_presses;

static void IRAM_ATTR gpio_isr(void *arg)
{
    (void)arg;
    uint32_t st = REG_READ(GPIO_STATUS_REG);        /* які піни спрацювали */
    if (st & (1u << BTN)) g_presses++;
    REG_WRITE(GPIO_STATUS_W1TC_REG, st);            /* СКИНУТИ флаги — обовʼязково! */
}

void demo2_irq(void)
{
    ESP_LOGI(TAG, "== Демо 2: переривання кнопки через регістри ==");

    /* пін уже вхід+підтяжка з демо 1; налаштуємо переривання РЕГІСТРАМИ */
    uint32_t cfg = REG_READ(GPIO_PIN_REG(BTN));
    cfg &= ~(0x7u  << 7);   cfg |= (2u << 7);       /* INT_TYPE=2: спадний фронт */
    cfg &= ~(0x1Fu << 13);  cfg |= (1u << 13);      /* INT_ENA=1: дозвіл (S3 — один біт) */
    REG_WRITE(GPIO_PIN_REG(BTN), cfg);
    ESP_LOGI(TAG, "GPIO_PIN%u_REG=0x%08X (INT_TYPE=2, INT_ENA=1)", BTN, (unsigned)REG_READ(GPIO_PIN_REG(BTN)));

    /* свій обробник на СПІЛЬНИЙ вектор GPIO (низькорівнево, без gpio-ISR-сервісу) */
    intr_handle_t h;
    esp_intr_alloc(ETS_GPIO_INTR_SOURCE, ESP_INTR_FLAG_IRAM, gpio_isr, NULL, &h);

    ESP_LOGI(TAG, "натискайте кнопку ~8 с (ловимо фронт 1->0)...");
    uint32_t last = 0;
    int64_t end = esp_timer_get_time() + 8000000;
    while (esp_timer_get_time() < end) {
        if (g_presses != last) { last = g_presses; ESP_LOGI(TAG, "натискань: %u", (unsigned)last); }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    ESP_LOGI(TAG, "усього натискань: %u", (unsigned)g_presses);
}
/* ▸ СПРОБУЙ САМ: зміни INT_TYPE на 3 (будь-який фронт) — лови і натиск, і відпускання. */


void app_main(void)
{
    demo1_iomux();     /* пін через IO_MUX руками */
    demo2_irq();       /* переривання через регістри */
    ESP_LOGI(TAG, "усі демо завершено.");
}