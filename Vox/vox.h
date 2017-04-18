/*
 迷之 Voice Process
 
 数据格式为 Q15，长度为 （VOX_SPLLEN*2）
	 前 一半 是老数据，后 一半 才是新数据。这是为了避免衔接时候的毛刺。
	 播放的时候从 1/4 开始，而且播放前会对 1/4:(1/4 + VOX_CROSSOVER) 做 fade-in
 */

#ifndef _VoX_H_
#define _VoX_H_

#include <stdint.h>

#define VOX_SPLFREQ 32000   // 采样频率 48000,32000,16000
#define VOX_SPLLEN  1024    // 每一次实际{获取的|要播放的}采样的长度
#define VOX_BUFLEN  (VOX_SPLLEN*2)   // 数据处理区长度
#define VOX_CROSSOVER 48    // 过渡效果的采样数量，不要超过 (VOX_SPLLEN/2)

typedef struct {
	enum vox_buf_status_t {
		VOX_FILLING, VOX_PROCESSING, VOX_PROCESSED,
	} status;
	uint32_t len; // length filled; length to be played
	uint32_t playOffset; // where playing begins
	int16_t data[VOX_BUFLEN];
}	vox_buf_t;

#define VOX_BUFCNT 5
extern vox_buf_t vox_bufs[VOX_BUFCNT];

void vox_init(void);
void vox_cycle(void);
void vox_feed(uint16_t *data, uint32_t len);

// not-implemented in vox.c   return 0 if successfully play.
int vox_play(vox_buf_t *buf);

#endif
