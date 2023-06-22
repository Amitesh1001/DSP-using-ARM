// stm32_noise_cancellation_dma.c 
//

#include "stm32_wm5102_init.h"
#include "bilinear.h"

extern uint16_t pingIN[BUFSIZE], pingOUT[BUFSIZE], pongIN[BUFSIZE], pongOUT[BUFSIZE];
int rx_proc_buffer, tx_proc_buffer;
volatile int RX_buffer_full = 0;
volatile int TX_buffer_empty = 0;

#define BLOCK_SIZE 1
#define NUM_TAPS 128

float32_t beta = 1e-11;

float32_t firStateF32[BLOCK_SIZE + NUM_TAPS -1];

float32_t firCoeffs32[NUM_TAPS] = {0.0};

arm_lms_instance_f32 S;

float32_t input, signoise, wn, yn, yout, error;

float w[NUM_SECTIONS][2] = {0.0f, 0.0f}; // IIR coefficients

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
  float32_t refnoise, signal;
  int16_t left_in_sample, right_in_sample;
	int section;
	
  // determine which buffers to use
	if (rx_proc_buffer == PING) rxbuf = pingIN; else rxbuf = pongIN;
  if (tx_proc_buffer == PING) txbuf = pingOUT; else txbuf = pongOUT;

  for (i=0 ; i<(BUFSIZE/2) ; i++)
  {		
	  right_in_sample = *rxbuf++;
	  left_in_sample = *rxbuf++;
		refnoise = (float32_t)(right_in_sample);
    signal = (float32_t)(left_in_sample);
		
    input = refnoise;
    for (section=0 ; section<NUM_SECTIONS ; section++)
    {
      wn = input - a[section][1]*w[section][0]
           - a[section][2]*w[section][1];
      yn = b[section][0]*wn + b[section][1]*w[section][0]
           + b[section][2]*w[section][1];
      w[section][1] = w[section][0];
      w[section][0] = wn;
      input = yn;
    }
    signoise = yn + signal;
    arm_lms_f32(&S, &refnoise, &signoise, &yout, &error, 1);
    *txbuf++ = (int16_t)(signoise);
    *txbuf++ = (int16_t)(error);
 	} 
  TX_buffer_empty = 0;
  RX_buffer_full = 0;
}



int main()
{
  arm_lms_init_f32(&S, NUM_TAPS, (float32_t *)&firCoeffs32[0], &firStateF32[0], beta, 1);
  stm32_wm5102_init(FS_8000_HZ, WM5102_LINE_IN, IO_METHOD_DMA);
  while(1)	{
		while (!(RX_buffer_full && TX_buffer_empty)){}
    GPIO_SetBits(GPIOD, GPIO_Pin_15);
		process_buffer();
    GPIO_ResetBits(GPIOD, GPIO_Pin_15);
	}
 	
}
