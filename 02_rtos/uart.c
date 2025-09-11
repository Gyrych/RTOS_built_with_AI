/**
  ******************************************************************************
  * @file    uart.c
  * @author  RTOS Team
  * @brief   RTOS UART (USART1) DMA driver: RX circular + IDLE, TX DMA
  ******************************************************************************
  */

#include "uart.h"
#include "core.h"

#if RTOS_UART_ENABLE

/* ================== Private definitions ================== */

#define UARTx                           USART1
#define UARTx_GPIO                      GPIOA
#define UARTx_GPIO_CLK                  RCC_AHB1Periph_GPIOA
#define UARTx_CLK_CMD()                 RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE)
#define UARTx_GPIO_CLK_CMD()            RCC_AHB1PeriphClockCmd(UARTx_GPIO_CLK, ENABLE)
#define UARTx_TX_PIN                    GPIO_Pin_9
#define UARTx_RX_PIN                    GPIO_Pin_10
#define UARTx_TX_PIN_SRC                GPIO_PinSource9
#define UARTx_RX_PIN_SRC                GPIO_PinSource10
#define UARTx_AF                        GPIO_AF_USART1

#define UARTx_IRQn                      USART1_IRQn
#define UARTx_IRQHandler                rtos_uart_usart1_irq_handler

#define UARTx_DMA_TX_STREAM_IRQn        DMA2_Stream7_IRQn
#define UARTx_DMA_RX_STREAM_IRQn        DMA2_Stream5_IRQn

/* Internal TX buffer for blocking/non-blocking copies */
#ifndef RTOS_UART_TX_BUF_SIZE
#define RTOS_UART_TX_BUF_SIZE           512u
#endif

/* ================== Private variables ================== */

static volatile uint8_t  s_rx_ring[RTOS_UART_RX_BUF_SIZE];
static volatile uint16_t s_rx_head = 0; /* last processed index */
static rtos_uart_rx_cb_t s_rx_cb = 0;

static volatile uint8_t  s_tx_busy = 0;
static uint8_t           s_tx_buf[RTOS_UART_TX_BUF_SIZE];

/* TX queue and active length for non-blocking DMA continuation */
static volatile uint16_t s_tx_len_active = 0;

#ifndef RTOS_UART_TXQ_SIZE
#define RTOS_UART_TXQ_SIZE                2048u
#endif
static volatile uint32_t s_txq_head = 0; /* write index */
static volatile uint32_t s_txq_tail = 0; /* read index */
static uint8_t           s_txq[RTOS_UART_TXQ_SIZE];

static inline uint32_t txq_free_space(void)
{
    uint32_t h = s_txq_head, t = s_txq_tail;
    if (h >= t) return (RTOS_UART_TXQ_SIZE - (h - t) - 1);
    return (t - h - 1);
}

static inline uint32_t txq_data_size(void)
{
    uint32_t h = s_txq_head, t = s_txq_tail;
    if (h >= t) return (h - t);
    return (RTOS_UART_TXQ_SIZE - (t - h));
}

static inline void txq_push_byte(uint8_t b)
{
    s_txq[s_txq_head] = b;
    s_txq_head = (s_txq_head + 1) % RTOS_UART_TXQ_SIZE;
}

static inline uint32_t txq_peek_linear(uint8_t** ptr)
{
    *ptr = (uint8_t*)&s_txq[s_txq_tail];
    if (s_txq_head >= s_txq_tail) return (s_txq_head - s_txq_tail);
    return (RTOS_UART_TXQ_SIZE - s_txq_tail);
}

static inline void txq_consume(uint32_t n)
{
    s_txq_tail = (s_txq_tail + n) % RTOS_UART_TXQ_SIZE;
}

/* ================== Helpers ================== */

static inline uint16_t dma_get_rx_write_index(void)
{
    /* In circular mode with memory increment, NDTR indicates remaining items.
       Write index = (BufferSize - NDTR) % BufferSize */
    uint16_t remaining = (uint16_t)RTOS_UART_RX_DMA_STREAM->NDTR;
    return (uint16_t)((RTOS_UART_RX_BUF_SIZE - remaining) % RTOS_UART_RX_BUF_SIZE);
}

static void uart_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    UARTx_GPIO_CLK_CMD();

    GPIO_InitStructure.GPIO_Pin   = UARTx_TX_PIN | UARTx_RX_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(UARTx_GPIO, &GPIO_InitStructure);

    GPIO_PinAFConfig(UARTx_GPIO, UARTx_TX_PIN_SRC, UARTx_AF);
    GPIO_PinAFConfig(UARTx_GPIO, UARTx_RX_PIN_SRC, UARTx_AF);
}

static void uart_dma_rx_init(void)
{
    DMA_InitTypeDef DMA_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);

    DMA_Cmd(RTOS_UART_RX_DMA_STREAM, DISABLE);
    DMA_DeInit(RTOS_UART_RX_DMA_STREAM);

    DMA_StructInit(&DMA_InitStructure);
    DMA_InitStructure.DMA_Channel            = RTOS_UART_RX_DMA_CHANNEL;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(UARTx->DR);
    DMA_InitStructure.DMA_Memory0BaseAddr    = (uint32_t)s_rx_ring;
    DMA_InitStructure.DMA_DIR                = DMA_DIR_PeripheralToMemory;
    DMA_InitStructure.DMA_BufferSize         = RTOS_UART_RX_BUF_SIZE;
    DMA_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode               = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority           = DMA_Priority_High;
    DMA_InitStructure.DMA_FIFOMode           = DMA_FIFOMode_Disable;
    DMA_InitStructure.DMA_FIFOThreshold      = DMA_FIFOThreshold_Full;
    DMA_InitStructure.DMA_MemoryBurst        = DMA_MemoryBurst_Single;
    DMA_InitStructure.DMA_PeripheralBurst    = DMA_PeripheralBurst_Single;
    DMA_Init(RTOS_UART_RX_DMA_STREAM, &DMA_InitStructure);

    /* Optionally enable DMA half/transfer complete interrupts (unused because we use IDLE) */
    DMA_ITConfig(RTOS_UART_RX_DMA_STREAM, DMA_IT_TC | DMA_IT_HT | DMA_IT_TE, ENABLE);

    NVIC_SetPriority(UARTx_DMA_RX_STREAM_IRQn, RTOS_UART_IRQn_PRIORITY);
    NVIC_EnableIRQ(UARTx_DMA_RX_STREAM_IRQn);

    DMA_Cmd(RTOS_UART_RX_DMA_STREAM, ENABLE);
}

static void uart_dma_tx_init(void)
{
    DMA_InitTypeDef DMA_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);

    DMA_Cmd(RTOS_UART_TX_DMA_STREAM, DISABLE);
    DMA_DeInit(RTOS_UART_TX_DMA_STREAM);

    DMA_StructInit(&DMA_InitStructure);
    DMA_InitStructure.DMA_Channel            = RTOS_UART_TX_DMA_CHANNEL;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(UARTx->DR);
    DMA_InitStructure.DMA_Memory0BaseAddr    = (uint32_t)s_tx_buf; /* placeholder; will be reloaded */
    DMA_InitStructure.DMA_DIR                = DMA_DIR_MemoryToPeripheral;
    DMA_InitStructure.DMA_BufferSize         = 0; /* set on each transfer */
    DMA_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode               = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority           = DMA_Priority_Medium;
    DMA_InitStructure.DMA_FIFOMode           = DMA_FIFOMode_Disable;
    DMA_InitStructure.DMA_FIFOThreshold      = DMA_FIFOThreshold_Full;
    DMA_InitStructure.DMA_MemoryBurst        = DMA_MemoryBurst_Single;
    DMA_InitStructure.DMA_PeripheralBurst    = DMA_PeripheralBurst_Single;
    DMA_Init(RTOS_UART_TX_DMA_STREAM, &DMA_InitStructure);

    DMA_ITConfig(RTOS_UART_TX_DMA_STREAM, DMA_IT_TC | DMA_IT_TE, ENABLE);
    NVIC_SetPriority(UARTx_DMA_TX_STREAM_IRQn, RTOS_UART_IRQn_PRIORITY);
    NVIC_EnableIRQ(UARTx_DMA_TX_STREAM_IRQn);
}

static void uart_core_init(void)
{
    USART_InitTypeDef USART_InitStructure;

    UARTx_CLK_CMD();

    USART_StructInit(&USART_InitStructure);
    USART_InitStructure.USART_BaudRate            = RTOS_UART_BAUDRATE;
    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
    USART_InitStructure.USART_Parity              = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(UARTx, &USART_InitStructure);

    /* Enable IDLE interrupt; RXNE not used due to DMA */
    USART_ITConfig(UARTx, USART_IT_IDLE, ENABLE);

    /* Enable DMA requests */
    USART_DMACmd(UARTx, USART_DMAReq_Rx, ENABLE);
    USART_DMACmd(UARTx, USART_DMAReq_Tx, ENABLE);

    NVIC_SetPriority(UARTx_IRQn, RTOS_UART_IRQn_PRIORITY);
    NVIC_EnableIRQ(UARTx_IRQn);

    /* Enable UART */
    USART_Cmd(UARTx, ENABLE);
}

/* ================== Public API ================== */

void rtos_uart_init(void)
{
    uart_gpio_init();
    uart_dma_rx_init();
    uart_dma_tx_init();
    uart_core_init();
}

void rtos_uart_set_rx_callback(rtos_uart_rx_cb_t callback)
{
    s_rx_cb = callback;
}

static int uart_try_start_tx_dma(const uint8_t* data, uint16_t length)
{
    if (length == 0) return 0;
    if (s_tx_busy) return -1; /* busy */

    /* Copy to internal buffer (simple single-flight implementation) */
    if (length > RTOS_UART_TX_BUF_SIZE) length = RTOS_UART_TX_BUF_SIZE;
    for (uint16_t i = 0; i < length; ++i) {
#if RTOS_UART_AUTO_CRLF
        if (data[i] == '\n') {
            /* Expand to CRLF if fits */
            if (i + 1 < RTOS_UART_TX_BUF_SIZE) {
                s_tx_buf[i] = '\r';
                s_tx_buf[i + 1] = '\n';
                length = (length + 1 <= RTOS_UART_TX_BUF_SIZE) ? (length + 1) : length;
                ++i;
            } else {
                s_tx_buf[i] = '\n';
            }
        } else {
            s_tx_buf[i] = data[i];
        }
#else
        s_tx_buf[i] = data[i];
#endif
    }

    s_tx_busy = 1;

    /* Program DMA with this buffer */
    DMA_Cmd(RTOS_UART_TX_DMA_STREAM, DISABLE);
    RTOS_UART_TX_DMA_STREAM->M0AR = (uint32_t)s_tx_buf;
    RTOS_UART_TX_DMA_STREAM->NDTR = (uint16_t)length;
    DMA_ClearFlag(RTOS_UART_TX_DMA_STREAM, DMA_FLAG_TCIF7 | DMA_FLAG_TEIF7 | DMA_FLAG_HTIF7 | DMA_FLAG_DMEIF7 | DMA_FLAG_FEIF7);
    DMA_Cmd(RTOS_UART_TX_DMA_STREAM, ENABLE);

    return (int)length;
}

static void uart_kick_tx_if_idle(void)
{
    if (s_tx_busy) return;
    uint8_t* ptr;
    uint32_t avail = txq_peek_linear(&ptr);
    if (avail == 0) return;
    uint16_t len = (uint16_t)((avail > 0xFFFFu) ? 0xFFFFu : avail);
    s_tx_busy = 1;
    s_tx_len_active = len;
    DMA_Cmd(RTOS_UART_TX_DMA_STREAM, DISABLE);
    RTOS_UART_TX_DMA_STREAM->M0AR = (uint32_t)ptr;
    RTOS_UART_TX_DMA_STREAM->NDTR = len;
    DMA_ClearFlag(RTOS_UART_TX_DMA_STREAM, DMA_FLAG_TCIF7 | DMA_FLAG_TEIF7 | DMA_FLAG_HTIF7 | DMA_FLAG_DMEIF7 | DMA_FLAG_FEIF7);
    DMA_Cmd(RTOS_UART_TX_DMA_STREAM, ENABLE);
}

int rtos_uart_write(const uint8_t* data, uint16_t length)
{
    if (!data || length == 0) return 0;
    uint32_t written = 0;
    rtos_enter_critical();
    while (written < length && txq_free_space() > 0) {
#if RTOS_UART_AUTO_CRLF
        uint8_t ch = data[written++];
        if (ch == '\n') {
            if (txq_free_space() > 0) txq_push_byte('\r');
            txq_push_byte('\n');
        } else {
            txq_push_byte(ch);
        }
#else
        txq_push_byte(data[written++]);
#endif
    }
    uart_kick_tx_if_idle();
    rtos_exit_critical();
    return (int)written;
}

int rtos_uart_write_blocking(const uint8_t* data, uint16_t length)
{
    if (!data || length == 0) return 0;
    int queued = rtos_uart_write(data, length);
    /* Wait for queue drain */
    while (txq_data_size() || s_tx_busy) { __asm("nop"); }
    return queued;
}

/* ================== ISRs ================== */

void rtos_uart_usart1_irq_handler(void)
{
    /* IDLE detection */
    if (USART_GetITStatus(UARTx, USART_IT_IDLE) != RESET) {
        (void)UARTx->SR; (void)UARTx->DR; /* Clear IDLE by sequence */

        uint16_t write_index = dma_get_rx_write_index();
        uint16_t data_len = 0;

        if (write_index >= s_rx_head) {
            data_len = (uint16_t)(write_index - s_rx_head);
            if (data_len && s_rx_cb) {
                s_rx_cb((const uint8_t*)&s_rx_ring[s_rx_head], data_len);
            }
        } else {
            /* Wrapped region: head..end, then 0..write_index */
            data_len = (uint16_t)(RTOS_UART_RX_BUF_SIZE - s_rx_head);
            if (data_len && s_rx_cb) {
                s_rx_cb((const uint8_t*)&s_rx_ring[s_rx_head], data_len);
            }
            if (write_index && s_rx_cb) {
                s_rx_cb((const uint8_t*)&s_rx_ring[0], write_index);
            }
        }

        s_rx_head = write_index;
    }
}

void rtos_uart_dma_tx_irq_handler(void)
{
    if (DMA_GetITStatus(RTOS_UART_TX_DMA_STREAM, DMA_IT_TCIF7)) {
        DMA_ClearITPendingBit(RTOS_UART_TX_DMA_STREAM, DMA_IT_TCIF7);
        /* consume the sent bytes */
        if (s_tx_len_active) {
            txq_consume(s_tx_len_active);
            s_tx_len_active = 0;
        }
        s_tx_busy = 0;
    }
    if (DMA_GetITStatus(RTOS_UART_TX_DMA_STREAM, DMA_IT_TEIF7)) {
        DMA_ClearITPendingBit(RTOS_UART_TX_DMA_STREAM, DMA_IT_TEIF7);
        s_tx_busy = 0;
    }
    /* kick next segment if any */
    rtos_enter_critical();
    uart_kick_tx_if_idle();
    rtos_exit_critical();
}

void rtos_uart_dma_rx_irq_handler(void)
{
    /* Not strictly needed if using only IDLE segmentation; clear error flags */
    if (DMA_GetITStatus(RTOS_UART_RX_DMA_STREAM, DMA_IT_TEIF5)) {
        DMA_ClearITPendingBit(RTOS_UART_RX_DMA_STREAM, DMA_IT_TEIF5);
    }
    if (DMA_GetITStatus(RTOS_UART_RX_DMA_STREAM, DMA_IT_TCIF5)) {
        DMA_ClearITPendingBit(RTOS_UART_RX_DMA_STREAM, DMA_IT_TCIF5);
    }
    if (DMA_GetITStatus(RTOS_UART_RX_DMA_STREAM, DMA_IT_HTIF5)) {
        DMA_ClearITPendingBit(RTOS_UART_RX_DMA_STREAM, DMA_IT_HTIF5);
    }
}

#endif /* RTOS_UART_ENABLE */


