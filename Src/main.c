/**
 ******************************************************************************
 * @file    AudioLoop/Src/main.c
 * @author  Central Labs
 * @version V1.0.1
 * @date    27-Sept-2016
 * @brief   Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2015 STMicroelectronics</center></h2>
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/

#include <string.h> /* strlen */
#include <stdio.h>  /* sprintf */
#include <math.h>   /* trunc */
#include "main.h"
#include "../vox/vox.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define AUDIO_CHANNELS 				        1
#define AUDIO_SAMPLING_FREQUENCY 		        VOX_SPLFREQ
#define AUDIO_IN_BUF_LEN   (AUDIO_SAMPLING_FREQUENCY/1000)
#define AUDIO_OUT_BUF_LEN  (VOX_SPLLEN *2)  //stereo

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

uint16_t PCM_Buffer[AUDIO_IN_BUF_LEN];
volatile int16_t audio_out_buffer[AUDIO_OUT_BUF_LEN];

extern SAI_HandleTypeDef haudio_out_sai;
static void *PCM1774_X_0_handle = NULL;

int MEMSInterrupt = 0;
SensorAxes_t acceleration;
float mroll, mpitch;
static void *LSM6DSM_X_0_handle = NULL;
/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
void vox_pitch_set_shift(float shiftHz);
void vox_pitch_set_sex(float ratio);

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main( void ) 
{
  /* STM32F4xx HAL library initialization:
  - Configure the Flash prefetch, instruction and Data caches
  - Configure the Systick to generate an interrupt each 1 msec
  - Set NVIC Group Priority to 4
  - Global MSP (MCU Support Package) initialization
  */
  HAL_Init();
  
  /* Configure the system clock */
  SystemClock_Config();
  
  /* Configure Audio Output peripheral (SAI) and external DAC */
  BSP_AUDIO_OUT_Init(PCM1774_0, &PCM1774_X_0_handle, NULL, 20, AUDIO_SAMPLING_FREQUENCY); 
  BSP_AUDIO_OUT_SetVolume(PCM1774_X_0_handle, 32);
  
  /* Configure Audio Input peripheral - DFSDM */  
  BSP_AUDIO_IN_Init(AUDIO_SAMPLING_FREQUENCY, 16, AUDIO_CHANNELS);  
  
	/* 启动加速度计 */
	if (BSP_ACCELERO_Init( LSM6DSM_X_0, &LSM6DSM_X_0_handle ) != COMPONENT_OK)
  {
    while(1);
  }
	BSP_ACCELERO_Sensor_Enable( LSM6DSM_X_0_handle );

  /* Start Microphone acquisition */
  BSP_AUDIO_IN_Record(PCM_Buffer, 0);
	
	/* VoX init */
	vox_init();
	memset((char*)audio_out_buffer, 0, sizeof(audio_out_buffer));
  
  while (1)
  {
		vox_cycle();
    /* Go to Sleep, everything is done in the interrupt service routines */
    //__WFI();
		
		if (MEMSInterrupt) {
			MEMSInterrupt = 0;
			//TODO: handle TAP events
		}
	
		if ( BSP_ACCELERO_Get_Axes( LSM6DSM_X_0_handle, &acceleration ) != COMPONENT_ERROR )
		{
			
			const float alpha = 0.5;
			 
			static float fXg = 0;
			static float fYg = 0;
			static float fZg = 0;
			
			fXg = acceleration.AXIS_X * alpha + (fXg * (1.0f - alpha));
			fYg = acceleration.AXIS_Y * alpha + (fYg * (1.0f - alpha));
			fZg = acceleration.AXIS_Z * alpha + (fZg * (1.0f - alpha));
			
			mroll  = atan2(-fYg, fZg) * 57.295828f;   //向下倒放 +180 正举着 +90 向上平放 0 
			mpitch = atan2(fXg, sqrt(fYg*fYg + fZg*fZg)) * 57.295828f;   // 左-90 右 +90
			
			signed int vox_pitch_shift_hz = (80 - mroll) * 5;
			if (vox_pitch_shift_hz > 400) vox_pitch_shift_hz = 400;
			if (vox_pitch_shift_hz <-300) vox_pitch_shift_hz =-300;
			vox_pitch_set_shift(vox_pitch_shift_hz);
			
			float vox_pitch_new_sex = mpitch / 60.0f;
			if (-0.05f < vox_pitch_new_sex && vox_pitch_new_sex < 0.05f) vox_pitch_new_sex = 0.0f;
			vox_pitch_set_sex(vox_pitch_new_sex);
		}
  }
}

char audio_out_passed_half = 1; // 只有在 DMA Audio OUT 到一半以后时，才开始输入新数据
void BSP_AUDIO_OUT_HalfTransfer_CallBack(void) {
	audio_out_passed_half = 1;
}

int vox_play(vox_buf_t *buf) {
	static int actived = 0;
	int i,j;
	
	if (!audio_out_passed_half) return 1;
	audio_out_passed_half = 0;
	
	int16_t *data = (int16_t*)buf->data + buf->playOffset;
	
	for (i=j=0; i < buf->len; i++,data++) {
    audio_out_buffer[j++] = *data;   // left channel
    audio_out_buffer[j++] = *data;  // right channel
	}
	
	if (!actived) {
		BSP_AUDIO_OUT_Play(PCM1774_X_0_handle,(uint16_t*)audio_out_buffer, AUDIO_OUT_BUF_LEN);
		actived = 1;
	}
	return 0;
}

/**
* @brief  User function that is called when 1 ms of PDM data is available.
* 		  In this application only PDM to PCM conversion and USB streaming
*                  is performed.
* 		  User can add his own code here to perform some DSP or audio analysis.
* @param  none
* @retval None
*/
void AudioProcess(void)
{
  vox_feed(PCM_Buffer, AUDIO_IN_BUF_LEN);
}

/**
* @brief  Transfer Complete user callback, called by BSP functions.
* @param  None
* @retval None
*/
void BSP_AUDIO_IN_TransferComplete_CallBack(void)
{
  AudioProcess();
}

/**
* @brief  Half Transfer Complete user callback, called by BSP functions.
* @param  None
* @retval None
*/
void BSP_AUDIO_IN_HalfTransfer_CallBack(void)
{
  AudioProcess();
}

/**
* @brief  EXTI line detection callbacks
* @param  GPIO_Pin: Specifies the pins connected EXTI line
* @retval None
*/
void HAL_GPIO_EXTI_Callback( uint16_t GPIO_Pin )
{
  MEMSInterrupt=1;
}

/**
 * @brief  This function is executed in case of error occurrence
 * @param  None
 * @retval None
 */
void Error_Handler( void )
{

  while (1)
  {}
}



#ifdef  USE_FULL_ASSERT

/**
 * @brief  Reports the name of the source file and the source line number
 *   where the assert_param error has occurred
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed( uint8_t *file, uint32_t line )
{

  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  
  while (1)
  {}
}

#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
