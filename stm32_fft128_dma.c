// stm32_fft128_dma.c
//

#include "stm32_wm5102_init.h"

extern uint16_t pingIN[BUFSIZE], pingOUT[BUFSIZE], pongIN[BUFSIZE], pongOUT[BUFSIZE];
int rx_proc_buffer, tx_proc_buffer;
volatile int RX_buffer_full = 0;
volatile int TX_buffer_empty = 0;

#define N (BUFSIZE/2)

#define TRIGGER 12000
#define MAGNITUDE_SCALING_FACTOR 32

typedef struct
{
  float real;
  float imag;
} COMPLEX;

#include "fft.h"

COMPLEX twiddle[N];
COMPLEX cbuf[N];
int16_t sinebuf[N];
int16_t outbuffer[N];

void DMA1_Stream3_IRQHandler()
{	
	if(DMA_GetITStatus(DMA1_Stream3,DMA_IT_TCIF3))
    {
        // clear pending interrupt
        DMA_ClearITPendingBit(DMA1_Stream3,DMA_IT_TCIF3);
 
        if(DMA_GetCurrentMemoryTarget(DMA1_Stream3))
        {
            // Memory 1 is current target (that's pongIN being used)
					rx_proc_buffer = PING;
				}
        else
        {rx_proc_buffer = PONG;
           }
 RX_buffer_full = 1;
					 
    }
}


void DMA1_Stream4_IRQHandler()
{	
    if(DMA_GetITStatus(DMA1_Stream4,DMA_IT_TCIF4))
    {
        // clear pending interrupt
        DMA_ClearITPendingBit(DMA1_Stream4,DMA_IT_TCIF4);
 
        if(DMA_GetCurrentMemoryTarget(DMA1_Stream4))
        {
            // Memory 1 is current target (that's pongIN being used)
					tx_proc_buffer = PING;
				}
        else
        {tx_proc_buffer = PONG;
           }
 TX_buffer_empty = 1;
					 
    }
}

void process_buffer()
{
  int i;
  uint16_t *rxbuf, *txbuf;
	  int16_t left_sample, right_sample;

  // determine which buffers to use
	if (rx_proc_buffer == PING) rxbuf = pingIN; else rxbuf = pongIN;
  if (tx_proc_buffer == PING) txbuf = pingOUT; else txbuf = pongOUT;
  for (i = 0; i < (BUFSIZE/2) ; i++) 
  {
    left_sample = *rxbuf++;
    right_sample = *rxbuf++;
    cbuf[i].real = (float)left_sample;
    cbuf[i].imag = 0.0;
    sinebuf[i] = left_sample;
  } 

  fft(cbuf,(BUFSIZE/2),twiddle);

  for (i = 0; i < (BUFSIZE/2) ; i++) 
  {
    left_sample = (int16_t)(sqrt(cbuf[i].real*cbuf[i].real
                  + cbuf[i].imag*cbuf[i].imag)/MAGNITUDE_SCALING_FACTOR);
		outbuffer[i]=left_sample;
    if (i==0) left_sample = TRIGGER;
   
    *txbuf++ = left_sample;
    *txbuf++ = sinebuf[i];
  }
  TX_buffer_empty = 0;
  RX_buffer_full	= 0;
}

int main(void)
{
  int n;
  for (n=0 ; n< N ; n++)
  {
    twiddle[n].real = cos(PI*n/N);
    twiddle[n].imag = -sin(PI*n/N);
  }	

  stm32_wm5102_init(FS_8000_HZ, WM5102_LINE_IN, IO_METHOD_DMA);
  while(1)
	{
		while (!(RX_buffer_full && TX_buffer_empty)){}
    GPIO_SetBits(GPIOD, GPIO_Pin_15);
		process_buffer();
    GPIO_ResetBits(GPIOD, GPIO_Pin_15);
	}
}
