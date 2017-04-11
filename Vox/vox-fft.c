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


void vox_fft_interpolate(const vox_fft_t *fft, float f, vox_complex_t *out) {
	const vox_complex_t *src = fft->fft;
	
	uint32_t x0i = VOX_FLOOR(f / VOX_FFTRES);
	uint32_t x1i = VOX_CEIL(f / VOX_FFTRES);
	
	float x0 = x0i * VOX_FFTRES, x1 = x1i * VOX_FFTRES;
	float y0, y1;
	
	// Êµ²¿
	y0 = src[x0i].real, y1 = src[x1i].real;
	out->real = y0 + (f - x0) * ((y1 - y0)/(x1-x0));
	
	// Ðé²¿
	y0 = src[x0i].imag, y1 = src[x1i].imag;
	out->imag = y0 + (f - x0) * ((y1 - y0)/(x1-x0));
}
