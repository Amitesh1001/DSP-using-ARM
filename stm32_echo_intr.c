// stm32_delay_intr.c

#include "stm32_wm5102_init.h"
#define DELAY_BUF_SIZE 16000
#define GAIN 0.6f

int16_t buffer[DELAY_BUF_SIZE];
int16_t buf_ptr = 0;

void SPI2_IRQHandler()
{	
  int16_t left_out_sample = 0;
  int16_t right_out_sample = 0;
  int16_t left_in_sample = 0;
  int16_t right_in_sample = 0;
  int16_t delayed_sample;
	
  if (SPI_I2S_GetFlagStatus(I2Sx, I2S_FLAG_CHSIDE) == SET) 
  {
    left_in_sample = SPI_I2S_ReceiveData(I2Sx);
    delayed_sample = buffer[buf_ptr];
    left_out_sample = delayed_sample + left_in_sample;
    buffer[buf_ptr] = left_in_sample + delayed_sample*GAIN;
    buf_ptr = (buf_ptr+1)%DELAY_BUF_SIZE;
    while (SPI_I2S_GetFlagStatus(I2Sxext, SPI_I2S_FLAG_TXE ) != SET){}
    SPI_I2S_SendData(I2Sxext, left_out_sample);			
  }
  else
  {
    right_in_sample = SPI_I2S_ReceiveData(I2Sx);
    right_out_sample = right_in_sample;
    while (SPI_I2S_GetFlagStatus(I2Sxext, SPI_I2S_FLAG_TXE ) != SET){}
    SPI_I2S_SendData(I2Sxext, right_out_sample);
  }
}

int main(void)
{
  stm32_wm5102_init(FS_48000_HZ, WM5102_DMIC_IN, IO_METHOD_INTR);
  while(1){}
}
