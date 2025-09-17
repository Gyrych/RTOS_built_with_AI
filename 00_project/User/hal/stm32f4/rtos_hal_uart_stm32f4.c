#include "rtos_config.h"
#include "../../../../02_rtos/hal/rtos_hal_uart.h"
#include "../../config/stm32f4/core/stm32f4xx.h"
#include "../../config/stm32f4/config/stm32f4xx_conf.h"

/* USART1 + DMA2 Stream5 (RX) / Stream7 (TX) */

static volatile uint8_t* s_rx_buf = 0;
static uint16_t s_rx_buf_size = 0;

/* GPIO/AF mapping for PA9/PA10 */
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

#define UARTx_DMA_TX_STREAM             DMA2_Stream7
#define UARTx_DMA_RX_STREAM             DMA2_Stream5
#define UARTx_DMA_RX_STREAM_IRQn        DMA2_Stream5_IRQn
#define UARTx_DMA_TX_STREAM_IRQn        DMA2_Stream7_IRQn
#define UARTx_DMA_CHANNEL               DMA_Channel_4

void rtos_hal_uart_set_rx_buffer(volatile uint8_t* rx_buf, uint16_t buf_size)
{
    s_rx_buf = rx_buf;
    s_rx_buf_size = buf_size;
}

void rtos_hal_uart_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;

    /* GPIO */
    UARTx_GPIO_CLK_CMD();
    GPIO_InitStructure.GPIO_Pin   = UARTx_TX_PIN | UARTx_RX_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(UARTx_GPIO, &GPIO_InitStructure);
    GPIO_PinAFConfig(UARTx_GPIO, UARTx_TX_PIN_SRC, UARTx_AF);
    GPIO_PinAFConfig(UARTx_GPIO, UARTx_RX_PIN_SRC, UARTx_AF);

    /* UART core */
    UARTx_CLK_CMD();
    USART_StructInit(&USART_InitStructure);
    USART_InitStructure.USART_BaudRate            = RTOS_UART_BAUDRATE;
    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
    USART_InitStructure.USART_Parity              = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(UARTx, &USART_InitStructure);

    /* DMA RX circular */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);
    DMA_Cmd(UARTx_DMA_RX_STREAM, DISABLE);
    DMA_DeInit(UARTx_DMA_RX_STREAM);
    DMA_StructInit(&DMA_InitStructure);
    DMA_InitStructure.DMA_Channel            = UARTx_DMA_CHANNEL;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(UARTx->DR);
    DMA_InitStructure.DMA_Memory0BaseAddr    = (uint32_t)s_rx_buf;
    DMA_InitStructure.DMA_DIR                = DMA_DIR_PeripheralToMemory;
    DMA_InitStructure.DMA_BufferSize         = s_rx_buf_size;
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
    DMA_Init(UARTx_DMA_RX_STREAM, &DMA_InitStructure);
    DMA_Cmd(UARTx_DMA_RX_STREAM, ENABLE);

    /* DMA TX normal (configured once; length/address will be updated per transfer) */
    DMA_Cmd(UARTx_DMA_TX_STREAM, DISABLE);
    DMA_DeInit(UARTx_DMA_TX_STREAM);
    DMA_StructInit(&DMA_InitStructure);
    DMA_InitStructure.DMA_Channel            = UARTx_DMA_CHANNEL;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(UARTx->DR);
    DMA_InitStructure.DMA_Memory0BaseAddr    = 0; /* set on each transfer */
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
    DMA_Init(UARTx_DMA_TX_STREAM, &DMA_InitStructure);

    /* Enable IDLE interrupt; RXNE not used due to DMA */
    USART_ITConfig(UARTx, USART_IT_IDLE, ENABLE);

    /* Enable DMA requests */
    USART_DMACmd(UARTx, USART_DMAReq_Rx, ENABLE);
    USART_DMACmd(UARTx, USART_DMAReq_Tx, ENABLE);

    /* NVIC */
    NVIC_SetPriority(UARTx_IRQn, RTOS_UART_IRQn_PRIORITY);
    NVIC_EnableIRQ(UARTx_IRQn);
    NVIC_SetPriority(UARTx_DMA_TX_STREAM_IRQn, RTOS_UART_IRQn_PRIORITY);
    NVIC_EnableIRQ(UARTx_DMA_TX_STREAM_IRQn);
    NVIC_SetPriority(UARTx_DMA_RX_STREAM_IRQn, RTOS_UART_IRQn_PRIORITY);
    NVIC_EnableIRQ(UARTx_DMA_RX_STREAM_IRQn);

    /* Enable UART */
    USART_Cmd(UARTx, ENABLE);
}

uint16_t rtos_hal_uart_get_rx_write_index(void)
{
    uint16_t remaining = (uint16_t)UARTx_DMA_RX_STREAM->NDTR;
    return (uint16_t)((s_rx_buf_size - remaining) % s_rx_buf_size);
}

int rtos_hal_uart_irq_is_idle_and_clear(void)
{
    if (USART_GetITStatus(UARTx, USART_IT_IDLE) != RESET) {
        (void)UARTx->SR; (void)UARTx->DR; /* Clear IDLE by sequence */
        return 1;
    }
    return 0;
}

void rtos_hal_uart_tx_start(const uint8_t* ptr, uint16_t len)
{
    DMA_Cmd(UARTx_DMA_TX_STREAM, DISABLE);
    UARTx_DMA_TX_STREAM->M0AR = (uint32_t)ptr;
    UARTx_DMA_TX_STREAM->NDTR = len;
    DMA_ClearFlag(UARTx_DMA_TX_STREAM, DMA_FLAG_TCIF7 | DMA_FLAG_TEIF7 | DMA_FLAG_HTIF7 | DMA_FLAG_DMEIF7 | DMA_FLAG_FEIF7);
    DMA_ITConfig(UARTx_DMA_TX_STREAM, DMA_IT_TC | DMA_IT_TE, ENABLE);
    DMA_Cmd(UARTx_DMA_TX_STREAM, ENABLE);
}

int rtos_hal_uart_dma_tx_irq_is_tc_and_clear(void)
{
    if (DMA_GetITStatus(UARTx_DMA_TX_STREAM, DMA_IT_TCIF7)) {
        DMA_ClearITPendingBit(UARTx_DMA_TX_STREAM, DMA_IT_TCIF7);
        return 1;
    }
    if (DMA_GetITStatus(UARTx_DMA_TX_STREAM, DMA_IT_TEIF7)) {
        DMA_ClearITPendingBit(UARTx_DMA_TX_STREAM, DMA_IT_TEIF7);
    }
    return 0;
}

void rtos_hal_uart_dma_rx_irq_clear_all(void)
{
    if (DMA_GetITStatus(UARTx_DMA_RX_STREAM, DMA_IT_TEIF5)) {
        DMA_ClearITPendingBit(UARTx_DMA_RX_STREAM, DMA_IT_TEIF5);
    }
    if (DMA_GetITStatus(UARTx_DMA_RX_STREAM, DMA_IT_TCIF5)) {
        DMA_ClearITPendingBit(UARTx_DMA_RX_STREAM, DMA_IT_TCIF5);
    }
    if (DMA_GetITStatus(UARTx_DMA_RX_STREAM, DMA_IT_HTIF5)) {
        DMA_ClearITPendingBit(UARTx_DMA_RX_STREAM, DMA_IT_HTIF5);
    }
}


