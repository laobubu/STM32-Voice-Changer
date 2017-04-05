/**
 * SensorTile Voice Process
 *
 *  (C) 2017 laobubu
 *
 * 采用 Q15 格式计算，输入/输出长度为 1024 个元素。
 */

#include <string.h>

#include "vox.h"

int vox_init_pitch(void);
int vox_proc_pitch(vox_buf_t *buf);

vox_buf_t vox_bufs[VOX_BUFCNT];
static vox_buf_t *buf_filling = NULL;

static int vox_process(vox_buf_t *buf) {
	vox_proc_pitch(buf);
	return 0;
}

/*
 * 初始化
 */
void vox_init() {
	memset(vox_bufs, 0, sizeof(vox_bufs));
	buf_filling = vox_bufs;
	
	vox_init_pitch();
}

/* 
 * 有新的语音数据进来时用的
 * 
 * data   	数据数组
 * len    	数据个数，单位是“个”，不是字节
 */
void vox_feed(uint16_t *data, uint32_t len) {
	if (buf_filling->status != VOX_FILLING) return;
	
	uint32_t 	len1 = VOX_BUFLEN - buf_filling->len, // 实际要拷贝的short个数
						len2 = 0; // 拷贝完后还未处理的数量
	if (len > len1) { len2 = len - len1; } // 进来的数据量有点大
	else { len1 = len; }
	
	memcpy(buf_filling->data + buf_filling->len, data, len1 * 2);
	buf_filling->len += len1;
	
	/* 这个 buffer 是否已经填满？ */
	if (buf_filling->len >= VOX_BUFLEN) {
		buf_filling->status = VOX_PROCESSING; // 标记为可以开始处理了
		if (++buf_filling >= &vox_bufs[VOX_BUFCNT]) buf_filling = vox_bufs; // 移动指针到下一个 buf
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
			if (0 == vox_play(buf)) {
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
