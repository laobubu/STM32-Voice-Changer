/*
 * 很多音效处理需要用到 FFT，可以利用这个头文件提供的函数，跳过处理的过程。
 *
 * 需要注意的是这个玩意儿会拷贝一份上次处理的数据（为了避免卷积等操作上不接下的情况）
 */

#ifndef _VOX_FFT_H
#define _VOX_FFT_H

#include "vox.h"

#define VOX_FFTLEN (VOX_BUFLEN*2)  // 考虑到卷积会利用前面的数据，因此扩宽一点

#define VOX_FFTRES ((float)VOX_SPLFREQ / VOX_FFTLEN)  // FFT 分辨率，每一个元素表示的频率跨度，单位Hz

// 初始化，只需要一次
void vox_init_fft(void);

typedef struct {
	float lastData[VOX_BUFLEN]; // 上次处理的数据
	float fft[VOX_FFTLEN*2];   // 你要处理的 FFT 复数数据
	float timeShift; // 正数，时域向前相移
	float timeRatio; // 输出长度是输入长度的多少倍
} vox_fft_t;

// 每一个轮回，只需要依次……
/* 1. 输入新数据             */ void vox_fft_begin(vox_fft_t *fft, const vox_buf_t *buf);
/* 2. 处理                   */
/*   - 发生了相移？调用这个  */   void vox_fft_phase_shift(vox_fft_t *fft, float delta); // 建议不要用
/* 3. ifft得结果             */ void vox_fft_end(vox_fft_t *fft, vox_buf_t *buf);

// 一些可能用得到的计算工具
#define VOX_ABS(x) ((x)>0?(x):-(x))
#define VOX_FLOOR(x)  ((int)(x))
#define VOX_CEIL(x)  ((int)(0.999999f+(x)))

void vox_fft_interpolate(const vox_fft_t *fft, float f, float *out); // 线性差值求某个频率，输出是复数

#endif
