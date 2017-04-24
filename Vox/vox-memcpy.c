/*!
 * ==== STM32 VoiceChanger (vox) ====
 * Asynchronous memcpy and memset, with DMA
 *
 * written by laobubu, for SensorTile
 * 
 * Copyright 2017 laobubu
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "vox-memcpy.h"
#include "stm32l4xx_hal.h"
#include <stdlib.h>

// DMA 解决部分扯淡的传输
// DMA2_Channel3 是 SAI2 的
// DMA1_Channel{4-7} 是麦克风滤波器的

static DMA_Channel_TypeDef* const DMAChannels[] = {
	DMA1_Channel1,
	DMA1_Channel2,
	DMA1_Channel3,
	DMA2_Channel1,
	DMA2_Channel2,
	DMA2_Channel4,
	DMA2_Channel5,
	DMA2_Channel6,
	DMA2_Channel7,
};

void voxmc_init(voxmc_handle_t *handle){	// id = 0,1,2...
	static int count = 0;
	
	DMA_HandleTypeDef *hdma = (DMA_HandleTypeDef*)malloc(sizeof(DMA_HandleTypeDef));
	hdma->Instance = DMAChannels[count];
	
	DMA_InitTypeDef *init = &hdma->Init;
	init->Direction = DMA_MEMORY_TO_MEMORY;
	init->PeriphInc = DMA_PINC_ENABLE;
	init->MemInc = DMA_MINC_ENABLE;
	init->PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	init->MemDataAlignment = DMA_MDATAALIGN_BYTE;
	init->Mode = DMA_NORMAL;
	init->Priority = DMA_PRIORITY_LOW;
	
	HAL_DMA_Init(hdma);
	
	handle->dma = hdma;
	handle->id = count++;
}

void voxmc_memcpy(voxmc_handle_t *handle, void *dst, const void*src, unsigned int bytes)
{
	if (bytes <= 0) return;
	
	DMA_HandleTypeDef *hdma = (DMA_HandleTypeDef*)handle->dma;
	
	HAL_DMA_PollForTransfer(hdma, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
	hdma->Instance->CCR |= 1UL<<6; // src memory inc enabled
	HAL_DMA_Start(hdma, (uint32_t)src, (uint32_t)dst, bytes);
}

void voxmc_memset(voxmc_handle_t *handle, void *dst, const int value, unsigned int bytes)
{
	if (bytes <= 0) return;
	
	DMA_HandleTypeDef *hdma = (DMA_HandleTypeDef*)handle->dma;
	
	HAL_DMA_PollForTransfer(hdma, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
	hdma->Instance->CCR &= ~(1UL<<6); // src memory inc disabled
	handle->_tmp1 = value & 0xFF;
	HAL_DMA_Start(hdma, (uint32_t)&handle->_tmp1, (uint32_t)dst, bytes);
}

void voxmc_wait(voxmc_handle_t *handle)
{
	DMA_HandleTypeDef *hdma = (DMA_HandleTypeDef*)handle->dma;
	HAL_DMA_PollForTransfer(hdma, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
}
