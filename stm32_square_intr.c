// stm32_square_intr.c

#include "stm32_wm5102_init.h"

#define LOOP_SIZE 64

int16_t square_table[LOOP_SIZE] = {
10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
-10000, -10000, -10000, -10000, -10000, -10000, -10000, -10000,
-10000, -10000, -10000, -10000, -10000, -10000, -10000, -10000,
-10000, -10000, -10000, -10000, -10000, -10000, -10000, -10000,
-10000, -10000, -10000, -10000, -10000, -10000, -10000, -10000};

static int square_ptr = 0;

void SPI2_IRQHandler()
{	
  int16_t left_out_sample = 0;
  int16_t right_out_sample = 0;
  int16_t left_in_sample = 0;
  int16_t right_in_sample = 0;

  if (SPI_I2S_GetFlagStatus(I2Sx, I2S_FLAG_CHSIDE) == SET) 
  {
    left_in_sample = SPI_I2S_ReceiveData(I2Sx);
    left_out_sample = square_table[square_ptr];
    square_ptr = (square_ptr+1) % LOOP_SIZE;
    while (SPI_I2S_GetFlagStatus(I2Sxext, SPI_I2S_FLAG_TXE ) != SET){}
    SPI_I2S_SendData(I2Sxext, left_out_sample);			
  }
  else
  {
    right_in_sample = SPI_I2S_ReceiveData(I2Sx);
    right_out_sample = 0;
    while (SPI_I2S_GetFlagStatus(I2Sxext, SPI_I2S_FLAG_TXE ) != SET){}
    SPI_I2S_SendData(I2Sxext, right_out_sample);
  }
}


int main(void)
{
  stm32_wm5102_init(FS_8000_HZ, WM5102_LINE_IN, IO_METHOD_INTR);
  while(1){}
}
