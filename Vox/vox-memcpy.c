/**
 * STM32 async memcpy with DMA
 *
 *  (C) 2017 laobubu
 *
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
	init->Priority = DMA_PRIORITY_MEDIUM;
	
	HAL_DMA_Init(hdma);
	
	handle->dma = hdma;
	handle->id = count++;
}

void voxmc_memcpy(voxmc_handle_t *handle, void *dst, const void*src, unsigned int bytes)
{
	DMA_HandleTypeDef *hdma = (DMA_HandleTypeDef*)handle->dma;
	
	HAL_DMA_PollForTransfer(hdma, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
	HAL_DMA_Start(hdma, (uint32_t)src, (uint32_t)dst, bytes);
}

void voxmc_wait(voxmc_handle_t *handle)
{
	DMA_HandleTypeDef *hdma = (DMA_HandleTypeDef*)handle->dma;
	HAL_DMA_PollForTransfer(hdma, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
}
