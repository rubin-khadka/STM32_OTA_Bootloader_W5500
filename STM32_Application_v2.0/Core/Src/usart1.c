/*
 * usart1.c
 *
 *  Created on: Mar 10, 2026
 *      Author: Rubin Khadka
 */

#include "usart1.h"
#include "stm32f103xb.h"

#define USART1_RX_BUF_SIZE 512
#define USART1_TX_BUF_SIZE 256

/* Global buffer instances */
static uint8_t USART1_rxbuf_storage[USART1_RX_BUF_SIZE];
static uint8_t USART1_txbuf_storage[USART1_TX_BUF_SIZE];
volatile USART1_Buffer_t usart1_rx_buf;
volatile USART1_Buffer_t usart1_tx_buf;

void USART1_Init(void)
{
  // Enable clocks
  RCC->APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;

  // PA9 as TX (Alternate function push-pull)
  GPIOA->CRH &= ~(GPIO_CRH_CNF9 | GPIO_CRH_MODE9);
  GPIOA->CRH |= GPIO_CRH_CNF9_1 | GPIO_CRH_MODE9;

  // PA10 as RX (Floating input)
  GPIOA->CRH &= ~(GPIO_CRH_CNF10 | GPIO_CRH_MODE10);
  GPIOA->CRH |= GPIO_CRH_CNF10_0;

  // Disable USART
  USART1->CR1 &= ~USART_CR1_UE;

  // 115200 baud @ 72MHz
  USART1->BRR = 0x271;

  // Clear status
  USART1->SR = 0;

  // Initialize buffers
  UART1_BufferInit(&usart1_rx_buf, USART1_rxbuf_storage, USART1_RX_BUF_SIZE);
  UART1_BufferInit(&usart1_tx_buf, USART1_txbuf_storage, USART1_TX_BUF_SIZE);

  // Configure USART
  USART1->CR1 = USART_CR1_RE | USART_CR1_TE | USART_CR1_RXNEIE | USART_CR1_UE;

  // Enable interrupt in NVIC
  NVIC_EnableIRQ(USART1_IRQn);
}

void UART1_BufferInit(volatile USART1_Buffer_t *buff, uint8_t *storage, uint16_t size)
{
  buff->buffer = storage;
  buff->size = size;
  buff->head = 0;
  buff->tail = 0;
  buff->count = 0;
}

bool USART1_BufferEmpty(volatile USART1_Buffer_t *buff)
{
  return (buff->count == 0);
}

// Check if buffer is full
bool USART1_BufferFull(volatile USART1_Buffer_t *buff)
{
  return (buff->count >= buff->size);
}

// Write to ANY buffer (TX or RX)
bool USART1_BufferWrite(volatile USART1_Buffer_t *buff, uint8_t data)
{
  __disable_irq();

  if(USART1_BufferFull(buff))
  {
    __enable_irq();
    return false;  // Buffer full
  }

  buff->buffer[buff->head] = data;
  buff->head = (buff->head + 1) % buff->size;
  buff->count++;

  __enable_irq();
  return true;
}

// Check if RX data is available
bool USART1_DataAvailable(void)
{
  return !USART1_BufferEmpty(&usart1_rx_buf);
}

// Read from ANY buffer
uint8_t USART1_BufferRead(volatile USART1_Buffer_t *buff)
{
  uint8_t data = 0;

  __disable_irq();

  if(!USART1_BufferEmpty(buff))
  {
    data = buff->buffer[buff->tail];
    buff->tail = (buff->tail + 1) % buff->size;
    buff->count--;
  }

  __enable_irq();
  return data;
}

void USART1_SendChar(char c)
{
  // Wait if TX buffer is full
  while(USART1_BufferFull(&usart1_tx_buf));

  __disable_irq();

  // Write to TX buffer
  USART1_BufferWrite(&usart1_tx_buf, (uint8_t) c);

  // ALWAYS enable TX interrupt
  USART1->CR1 |= USART_CR1_TXEIE;

  __enable_irq();
}

// Send a string
void USART1_SendString(const char *str)
{
  while(*str)
  {
    USART1_SendChar(*str++);
  }
}

// Get a character from RX buffer
uint8_t USART1_GetChar(void)
{
  return USART1_BufferRead(&usart1_rx_buf);
}

// Send a 32-bit number as ASCII string via UART
void USART1_SendNumber(uint32_t num)
{
  char buffer[16];
  int i = 0;

  // Handle 0 separately
  if(num == 0)
  {
    USART1_SendChar('0');
    return;
  }

  // Convert number to string (reverse order)
  while(num > 0)
  {
    buffer[i++] = '0' + (num % 10);
    num /= 10;
  }

  // Send in correct order
  while(i > 0)
  {
    USART1_SendChar(buffer[--i]);
  }
}

void USART1_SendHex(uint8_t value)
{
  char hex[3];
  hex[0] = "0123456789ABCDEF"[value >> 4];
  hex[1] = "0123456789ABCDEF"[value & 0x0F];
  hex[2] = 0;
  USART1_SendString(hex);
}

void USART1_IRQHandler(void)
{
  // Handle received data - WRITE to RX buffer
  if(USART1->SR & USART_SR_RXNE)
  {
    uint8_t data = USART1->DR;
    // Write to RX buffer (ignore if full - data lost)
    USART1_BufferWrite(&usart1_rx_buf, data);
  }

  // Handle transmit - READ from TX buffer
  if((USART1->CR1 & USART_CR1_TXEIE) && (USART1->SR & USART_SR_TXE))
  {
    if(!USART1_BufferEmpty(&usart1_tx_buf))
    {
      // Read from TX buffer and send
      USART1->DR = USART1_BufferRead(&usart1_tx_buf);
    }

    // Disable TX interrupt if buffer is empty
    if(USART1_BufferEmpty(&usart1_tx_buf))
    {
      USART1->CR1 &= ~USART_CR1_TXEIE;
    }
  }
}

