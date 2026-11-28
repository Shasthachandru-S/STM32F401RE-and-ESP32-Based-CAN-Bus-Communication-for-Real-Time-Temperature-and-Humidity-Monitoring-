#include "stm32f4xx.h"
#include <stdint.h>

#define DHT_MASK    (1U << 1)

#define LCD_ADDR    0x27U
#define LCD_BL      0x08U
#define LCD_EN      0x04U
#define LCD_RS      0x01U

#define CPU_HZ      16000000U
#define CYCLES_PER_US (CPU_HZ / 1000000U)

#define CS_PIN      (1U << 6)

#define MCP_RESET   0xC0U
#define MCP_READ    0x03U
#define MCP_WRITE   0x02U
#define MCP_BITMOD  0x05U
#define MCP_RTS_TX0 0x81U

#define CANSTAT     0x0EU
#define CANCTRL     0x0FU
#define CNF1        0x2AU
#define CNF2        0x29U
#define CNF3        0x28U

#define TXB0CTRL    0x30U
#define TXB0SIDH    0x31U
#define TXB0SIDL    0x32U
#define TXB0DLC     0x35U
#define TXB0D0      0x36U
#define TXB0D1      0x37U

#define TXREQ       0x08U
#define TXERR       0x10U
#define MLOA        0x20U
#define ABTF        0x40U

volatile uint8_t temperature = 0;
volatile uint8_t humidity = 0;
volatile uint8_t dht_ok = 0;
volatile uint8_t can_init_ok = 0;
volatile uint8_t tx_ok = 0;
volatile uint32_t tx_count = 0;
volatile uint32_t tx_fail = 0;
volatile uint32_t dht_fail = 0;

static void delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t cycles = us * CYCLES_PER_US;

    while ((uint32_t)(DWT->CYCCNT - start) < cycles) {}
}

static void delay_ms(uint32_t ms)
{
    while (ms--)
        delay_us(1000U);
}

static void timing_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

/* ---------------- I2C LCD ---------------- */

static int i2c_wait(volatile uint32_t *reg,
                    uint32_t mask, uint32_t expected)
{
    uint32_t timeout = 1000000U;

    while ((*reg & mask) != expected) {
        if (--timeout == 0)
            return 0;
    }
    return 1;
}

static void i2c_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    (void)RCC->APB1ENR;

    GPIOB->MODER &= ~((3U << 16) | (3U << 18));
    GPIOB->MODER |=  ((2U << 16) | (2U << 18));
    GPIOB->OTYPER |= (1U << 8) | (1U << 9);

    GPIOB->OSPEEDR &= ~((3U << 16) | (3U << 18));
    GPIOB->OSPEEDR |=  ((2U << 16) | (2U << 18));

    GPIOB->AFR[1] &= ~((15U << 0) | (15U << 4));
    GPIOB->AFR[1] |=  ((4U << 0) | (4U << 4));

    I2C1->CR1 = I2C_CR1_SWRST;
    I2C1->CR1 = 0;

    /* APB1 clock = 16 MHz */
    I2C1->CR2 = 16U;
    I2C1->CCR = 80U;
    I2C1->TRISE = 17U;
    I2C1->CR1 = I2C_CR1_PE;
}

static void i2c_write(uint8_t value)
{
    if (!i2c_wait(&I2C1->SR2, I2C_SR2_BUSY, 0))
        return;

    I2C1->CR1 |= I2C_CR1_START;
    if (!i2c_wait(&I2C1->SR1, I2C_SR1_SB, I2C_SR1_SB))
        return;

    (void)I2C1->SR1;
    I2C1->DR = (uint8_t)(LCD_ADDR << 1);

    if (!i2c_wait(&I2C1->SR1, I2C_SR1_ADDR, I2C_SR1_ADDR)) {
        I2C1->CR1 |= I2C_CR1_STOP;
        return;
    }

    (void)I2C1->SR1;
    (void)I2C1->SR2;

    if (!i2c_wait(&I2C1->SR1, I2C_SR1_TXE, I2C_SR1_TXE)) {
        I2C1->CR1 |= I2C_CR1_STOP;
        return;
    }

    I2C1->DR = value;

    if (!i2c_wait(&I2C1->SR1, I2C_SR1_BTF, I2C_SR1_BTF)) {
        I2C1->CR1 |= I2C_CR1_STOP;
        return;
    }

    I2C1->CR1 |= I2C_CR1_STOP;
}

static void lcd_nibble(uint8_t value, uint8_t rs)
{
    uint8_t x = (value & 0xF0U) | rs | LCD_BL;

    i2c_write(x | LCD_EN);
    delay_us(2);
    i2c_write(x);
    delay_us(2);
}

static void lcd_cmd(uint8_t cmd)
{
    lcd_nibble(cmd, 0);
    lcd_nibble((uint8_t)(cmd << 4), 0);

    if (cmd == 0x01 || cmd == 0x02)
        delay_ms(3);
}

static void lcd_data(uint8_t data)
{
    lcd_nibble(data, LCD_RS);
    lcd_nibble((uint8_t)(data << 4), LCD_RS);
}

static void lcd_string(const char *str)
{
    while (*str)
        lcd_data((uint8_t)*str++);
}

static void lcd_cursor(uint8_t row, uint8_t col)
{
    lcd_cmd(row == 0 ? (uint8_t)(0x80U + col)
                     : (uint8_t)(0xC0U + col));
}

static void lcd_init(void)
{
    delay_ms(50);
    lcd_nibble(0x30, 0);
    delay_ms(5);
    lcd_nibble(0x30, 0);
    delay_ms(2);
    lcd_nibble(0x30, 0);
    delay_ms(2);
    lcd_nibble(0x20, 0);
    delay_ms(2);

    lcd_cmd(0x28);
    lcd_cmd(0x0C);
    lcd_cmd(0x06);
    lcd_cmd(0x01);
}

/* ---------------- DHT11 PA1 ---------------- */

static void dht_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    (void)RCC->AHB1ENR;

    GPIOA->MODER &= ~(3U << 2);
    GPIOA->PUPDR &= ~(3U << 2);
    GPIOA->PUPDR |= (1U << 2);
}

static void dht_output(void)
{
    GPIOA->OTYPER |= DHT_MASK;
    GPIOA->MODER &= ~(3U << 2);
    GPIOA->MODER |= (1U << 2);
}

static void dht_input(void)
{
    GPIOA->MODER &= ~(3U << 2);
}

static int dht_wait_level(uint8_t level, uint32_t timeout_us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t limit = timeout_us * CYCLES_PER_US;

    while (((GPIOA->IDR & DHT_MASK) ? 1U : 0U) != level) {
        if ((uint32_t)(DWT->CYCCNT - start) >= limit)
            return 0;
    }
    return 1;
}

/* Output: humidity first, temperature second */
static int dht_read(uint8_t *hum, uint8_t *temp)
{
    uint8_t data[5] = {0, 0, 0, 0, 0};
    uint32_t i;
    uint8_t bit;

    dht_output();
    GPIOA->BSRR = (uint32_t)DHT_MASK << 16;
    delay_ms(20);

    GPIOA->BSRR = DHT_MASK;
    dht_input();
    delay_us(30);

    if (!dht_wait_level(0, 100)) return 0;
    if (!dht_wait_level(1, 100)) return 0;
    if (!dht_wait_level(0, 100)) return 0;

    for (i = 0; i < 40; i++) {
        if (!dht_wait_level(1, 100)) return 0;

        delay_us(40);
        bit = (GPIOA->IDR & DHT_MASK) ? 1U : 0U;

        data[i / 8U] <<= 1;
        data[i / 8U] |= bit;

        if (!dht_wait_level(0, 100)) return 0;
    }

    if ((uint8_t)(data[0] + data[1] + data[2] + data[3])
        != data[4])
        return 0;

    *hum = data[0];
    *temp = data[2];
    return 1;
}

/* ---------------- MCP2515 SPI1 ---------------- */

static void spi_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN |
                    RCC_AHB1ENR_GPIOBEN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    (void)RCC->APB2ENR;

    GPIOA->MODER &= ~((3U << 10) | (3U << 12) | (3U << 14));
    GPIOA->MODER |=  ((2U << 10) | (2U << 12) | (2U << 14));

    GPIOA->AFR[0] &= ~((15U << 20) | (15U << 24) | (15U << 28));
    GPIOA->AFR[0] |=  ((5U << 20) | (5U << 24) | (5U << 28));

    GPIOB->MODER &= ~(3U << 12);
    GPIOB->MODER |= (1U << 12);
    GPIOB->BSRR = CS_PIN;

    SPI1->CR1 = 0;
    SPI1->CR2 = 0;
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI |
                SPI_CR1_BR_0 | SPI_CR1_BR_1;
    SPI1->CR1 |= SPI_CR1_SPE;
}

static uint8_t spi_transfer(uint8_t data)
{
    uint32_t timeout = 100000U;

    while (!(SPI1->SR & SPI_SR_TXE))
        if (--timeout == 0) return 0xFF;

    *(__IO uint8_t *)&SPI1->DR = data;

    timeout = 100000U;
    while (!(SPI1->SR & SPI_SR_RXNE))
        if (--timeout == 0) return 0xFF;

    return *(__IO uint8_t *)&SPI1->DR;
}

static void mcp_reset(void)
{
    GPIOB->BSRR = CS_PIN << 16;
    spi_transfer(MCP_RESET);
    GPIOB->BSRR = CS_PIN;
    delay_ms(10);
}

static void mcp_write(uint8_t reg, uint8_t value)
{
    GPIOB->BSRR = CS_PIN << 16;
    spi_transfer(MCP_WRITE);
    spi_transfer(reg);
    spi_transfer(value);
    GPIOB->BSRR = CS_PIN;
}

static uint8_t mcp_read(uint8_t reg)
{
    uint8_t value;

    GPIOB->BSRR = CS_PIN << 16;
    spi_transfer(MCP_READ);
    spi_transfer(reg);
    value = spi_transfer(0);
    GPIOB->BSRR = CS_PIN;

    return value;
}

static void mcp_bit_modify(uint8_t reg, uint8_t mask, uint8_t value)
{
    GPIOB->BSRR = CS_PIN << 16;
    spi_transfer(MCP_BITMOD);
    spi_transfer(reg);
    spi_transfer(mask);
    spi_transfer(value);
    GPIOB->BSRR = CS_PIN;
}

static int mcp_init(void)
{
    uint32_t timeout = 100000U;

    mcp_bit_modify(CANCTRL, 0xE0, 0x80);

    while ((mcp_read(CANSTAT) & 0xE0) != 0x80)
        if (--timeout == 0) return 0;

    /* MCP2515 8 MHz crystal; CAN 125 kbps */
    mcp_write(CNF1, 0x03);
    mcp_write(CNF2, 0x91);
    mcp_write(CNF3, 0x01);

    mcp_bit_modify(CANCTRL, 0xE0, 0x00);

    timeout = 100000U;
    while ((mcp_read(CANSTAT) & 0xE0) != 0x00)
        if (--timeout == 0) return 0;

    return 1;
}

/* CAN ID 0x123; data[0]=temperature, data[1]=humidity */
static int can_send_dht(uint16_t id, uint8_t temp, uint8_t hum)
{
    uint32_t timeout = 100000U;
    uint8_t status;

    while (mcp_read(TXB0CTRL) & TXREQ)
        if (--timeout == 0) return 0;

    mcp_bit_modify(TXB0CTRL, ABTF | MLOA | TXERR, 0);

    mcp_write(TXB0SIDH, (uint8_t)(id >> 3));
    mcp_write(TXB0SIDL, (uint8_t)((id & 7U) << 5));

    mcp_write(TXB0DLC, 2);
    mcp_write(TXB0D0, temp);
    mcp_write(TXB0D1, hum);

    GPIOB->BSRR = CS_PIN << 16;
    spi_transfer(MCP_RTS_TX0);
    GPIOB->BSRR = CS_PIN;

    timeout = 1000000U;

    do {
        status = mcp_read(TXB0CTRL);

        if (status & (ABTF | MLOA | TXERR))
            return 0;

        if (!(status & TXREQ))
            return 1;

    } while (--timeout != 0);

    return 0;
}

int main(void)
{
    uint8_t t = 0;
    uint8_t h = 0;

    timing_init();
    dht_init();
    i2c_init();
    lcd_init();
    spi_init();
    mcp_reset();

    can_init_ok = (uint8_t)mcp_init();

    lcd_cmd(0x01);
    lcd_cursor(0, 0);
    lcd_string("DHT11 + CAN");
    lcd_cursor(1, 0);
    lcd_string(can_init_ok ? "CAN Ready" : "CAN Init Error");
    delay_ms(1000);

    while (1) {
        dht_ok = (uint8_t)dht_read(&h, &t);

        if (dht_ok) {
            humidity = h;
            temperature = t;

            lcd_cmd(0x01);
            lcd_cursor(0, 0);
            lcd_string("Temp: ");
            lcd_data((uint8_t)('0' + (t / 10U)));
            lcd_data((uint8_t)('0' + (t % 10U)));
            lcd_string(" C");

            lcd_cursor(1, 0);
            lcd_string("Humidity: ");
            lcd_data((uint8_t)('0' + (h / 10U)));
            lcd_data((uint8_t)('0' + (h % 10U)));
            lcd_string("%");

            if (can_init_ok) {
                tx_ok = (uint8_t)can_send_dht(0x123, t, h);
                if (tx_ok) tx_count++;
                else tx_fail++;
            }
        } else {
            dht_fail++;
            lcd_cmd(0x01);
            lcd_cursor(0, 0);
            lcd_string("DHT11 Read Error");
        }

        /* DHT11 should not be sampled faster than about once per second */
        delay_ms(1000);
    }
}
