/**
 * SensorTile Voice Process
 * Do FFT audio process in STM32.
 *  providing:
 *    - FFT/iFFT
 *    - Simple LPH/HPF
 *    - Interpolate on FFT data
 *
 *  (C) 2017 laobubu
 *
 */

#include "vox-fft.h"
#include <arm_math.h>
#include <arm_const_structs.h>

static char inited = 0;
static arm_rfft_fast_instance_f32 S;
static float tmp[VOX_FFTLEN];

void vox_init_fft(void) {
	if (inited) return;
	arm_rfft_fast_init_f32(&S, VOX_FFTLEN);
	inited = 1;
}

void vox_fft_begin(vox_fft_t *fft, const vox_buf_t *buf) {
	arm_q15_to_float((short*)buf->data, tmp, VOX_BUFLEN);
	arm_rfft_fast_f32(&S, tmp, (float*)fft->fft, 0);
	fft->timeShift = 0;
	fft->timeRatio = 1.0;
}

void vox_fft_phase_shift(vox_fft_t *fft, float delta) {
	fft->timeShift += delta;
}

void vox_fft_end(vox_fft_t *fft, vox_buf_t *buf) {
	buf->len = buf->len * fft->timeRatio;
	buf->playOffset = buf->playOffset + fft->timeShift;
	
	arm_rfft_fast_f32(&S, (float*)fft->fft, tmp, 1);
	arm_float_to_q15(tmp, (short*)buf->data, VOX_BUFLEN);
}

void vox_fft_lpf(vox_fft_t *fft, const float freq, const float freq2)
{
	int writeOffset = freq / VOX_FFTRES;
	vox_complex_t *c_i = fft->fft + writeOffset;
	float p_i = 1.0f;
	const float v = VOX_FFTRES / (freq2 - freq);
	for (int i = writeOffset; i < VOX_FFTLEN; i++, c_i++) {
		if (p_i < 0.01f) {
			memset(c_i, 0, (VOX_FFTLEN - i) * sizeof(vox_complex_t));
			break;
		}
		c_i->real *= p_i;
		c_i->imag *= p_i;
		p_i -= v;
	}
}

void vox_fft_hpf(vox_fft_t *fft, const float freq, const float freq2)
{
	int writeOffset = freq / VOX_FFTRES;
	vox_complex_t *c_i = fft->fft + writeOffset;
	float p_i = 1.0f;
	const float v = VOX_FFTRES / (freq - freq2);
	for (int i = writeOffset; i > 0; i--, c_i--) {
		if (p_i < 0.01f) {
			memset(fft->fft, 0, i * sizeof(vox_complex_t));
			break;
		}
		c_i->real *= p_i;
		c_i->imag *= p_i;
		p_i -= v;
	}
}

void vox_fft_interpolate(const vox_fft_t *fft, float f, vox_complex_t *out) {
	//FIXME: 应该使用 mag/phase 来计算才对
	const vox_complex_t *src = fft->fft;
	
	uint32_t x0i = VOX_FLOOR(f / VOX_FFTRES);
	uint32_t x1i = VOX_CEIL(f / VOX_FFTRES);
	
	float x0 = x0i * VOX_FFTRES, x1 = x1i * VOX_FFTRES;
	float y0, y1;
	
	// 实部
	y0 = src[x0i].real, y1 = src[x1i].real;
	out->real = y0 + (f - x0) * ((y1 - y0)/(x1-x0));
	
	// 虚部
	y0 = src[x0i].imag, y1 = src[x1i].imag;
	out->imag = y0 + (f - x0) * ((y1 - y0)/(x1-x0));
}
