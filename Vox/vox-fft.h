/*
 * SensorTile Voice Process
 * Do FFT audio process in STM32.
 *  providing:
 *    - FFT/iFFT
 *    - Simple LPH/HPF
 *    - Interpolate on FFT data
 *
 *  (C) 2017 laobubu
 *
 * 很多音效处理需要用到 FFT，可以利用这个头文件提供的函数，跳过处理的过程。
 */

#ifndef _VOX_FFT_H
#define _VOX_FFT_H

#include "vox.h"

#define VOX_FFTLEN (VOX_BUFLEN)

#define VOX_FFTRES ((float)VOX_SPLFREQ / VOX_FFTLEN)  // FFT 分辨率，每一个元素表示的频率跨度，单位Hz

typedef struct _vox_complex {
	float real, imag;
} vox_complex_t;

// 初始化，只需要一次
void vox_init_fft(void);

typedef struct {
	vox_complex_t fft[VOX_FFTLEN];   // 你要处理的 FFT 复数数据
	float timeShift; // 正数，时域向前相移
	float timeRatio; // 输出长度是输入长度的多少倍。 不要使用！！！
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

void vox_fft_interpolate(const vox_fft_t *fft, float f, vox_complex_t *out); // 线性差值求某个频率，输出是复数

/*
 简单的 频域滤波器，在频域长这样子（以HPF为例）

   
1 ↑    ._________
  |   /
  |  /
o +-+--+-------------→ f/Hz
    |  freq
    |
    freq_end

下降是一个斜线，斜率是 v，单位是 1/Hz
*/
void vox_fft_lpf(vox_fft_t *fft, const float freq, const float freq_end);
void vox_fft_hpf(vox_fft_t *fft, const float freq, const float freq_end);

#endif
