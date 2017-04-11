/**
 * SensorTile Voice Process
 *
 *  (C) 2017 laobubu
 *
 */

#include <string.h>

#include "vox.h"
#include "vox-memcpy.h"

int vox_init_pitch(void);
int vox_proc_pitch(vox_buf_t *buf);

vox_buf_t vox_bufs[VOX_BUFCNT];
static vox_buf_t *buf_filling = NULL;

static voxmc_handle_t voxmc[2];

static int vox_process(vox_buf_t *buf) {
	vox_proc_pitch(buf);
	return 0;
}

static int __vox_play(vox_buf_t *buf) {
	// 添加 crossover
	static int16_t lastProcessed[VOX_CROSSOVER];
	int16_t *src1 = lastProcessed,
					*src2 = buf->data + buf->playOffset; //src2 == dst
	for (int i=1;i<=VOX_CROSSOVER;i++) {
		register int32_t s1 = *src1, s2 = *src2, out;
		out = (( s2 * i ) + ( s1 * (VOX_CROSSOVER - i) )) / VOX_CROSSOVER;
		*src2 = out;
		
		src1++, src2++;
	}
	
	memcpy(lastProcessed, buf->data + buf->playOffset + buf->len, sizeof(lastProcessed));
	
	// 然后播放
	return vox_play(buf);
}

/*
 * 初始化
 */
void vox_init() {
	memset(vox_bufs, 0, sizeof(vox_bufs));
	buf_filling = vox_bufs;
	
	voxmc_init(&voxmc[0]);
	voxmc_init(&voxmc[1]);
	
	vox_init_pitch();
}

/* 
 * 有新的语音数据进来时用的
 * 
 * data   	数据数组
 * len    	数据个数，单位是“个”，不是字节
 */
void vox_feed(uint16_t *data, uint32_t len) {
	static int16_t old_spl_data[VOX_SPLLEN] = {0};
	
	if (buf_filling->status != VOX_FILLING) return;
	
	uint32_t 	len1 = VOX_SPLLEN - buf_filling->len, // 实际要拷贝的short个数
						len2 = 0; // 拷贝完后还未处理的数量
	if (len > len1) { len2 = len - len1; } // 进来的数据量有点大
	else { len1 = len; }
	
	memcpy(buf_filling->data + VOX_SPLLEN + buf_filling->len, data, len1*2);
	buf_filling->len += len1;
	
	/* 这个 buffer 是否已经有了足够的新数据？ */
	if (buf_filling->len >= VOX_SPLLEN) {
		// 把上次采样的数据 old_spl_data 塞到此 buf 开头
		memcpy(buf_filling->data, old_spl_data, VOX_SPLLEN*2);
		
		// 然后把现在新采的数据保存到 old_spl_data
		// 使用 vox-memcpy 的 DMA 异步处理
		voxmc_memcpy(&voxmc[0], old_spl_data, buf_filling->data + VOX_SPLLEN, VOX_SPLLEN*2); 
		
		// 设置初始参数，然后标记为可以开始处理了
		buf_filling->playOffset = VOX_SPLLEN/2;
		buf_filling->status = VOX_PROCESSING;
		
		// 移动指针到下一个 buf
		if (++buf_filling >= &vox_bufs[VOX_BUFCNT]) buf_filling = vox_bufs;
	}
	
	if (len2) vox_feed(data + len1, len2);
}

/*
 * 放在 main 中不断循环调用。优先开始播放最近一个已经可以播放的buf，然后再处理可以处理一个buf
 */
void vox_cycle() {
	vox_buf_t *buf;
	
  buf = vox_bufs;
	while (buf != &vox_bufs[VOX_BUFCNT]) {
		if (buf->status == VOX_PROCESSED) { 
			if (0 == __vox_play(buf)) {
				buf->len = 0;
				buf->status = VOX_FILLING;
			} 
			break;
		}
		buf++;
	}
	
  buf = vox_bufs;
	while (buf != &vox_bufs[VOX_BUFCNT]) {
		if (buf->status == VOX_PROCESSING) { 
			if (0 == vox_process(buf)) {
				buf->status = VOX_PROCESSED;
			} 
			break;
		}
		buf++;
	}
}
