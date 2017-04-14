#include <string.h>

#include "vox.h"
#include "vox-fft.h"
#include "vox-eq.h"

static vox_fft_t fft = {{0}};
vox_complex_t fft_tmp[VOX_FFTLEN];

static const float cutoff   = VOX_SPLFREQ;  	//Hz
static const float since    = 100;  	  //Hz
static float shift    = -400;       //Hz

// 一些滤波器

static vox_eq_line_t filter1lines[] = {
	// 滤掉低频
	{ 0, 0.7f,	400, 1.0f },
	{ 0, 0.8f,	300, 1.0f },
	// 滤掉高频
	{ 10000, 1.0f,  18000, 0.3f }
};
static vox_eq_t filter1 = vox_eq_from_lines(filter1lines);

int vox_init_pitch() {
	vox_init_fft();
	vox_eq_compile(&filter1);
	return 0;
}

void vox_pitch_set(float shiftHz) {
	shift = shiftHz;
}

int vox_proc_pitch(vox_buf_t *buf){
	vox_fft_begin(&fft, buf);
	
	int count = (cutoff - since) / VOX_FFTRES;
	int readOffset = (since)/VOX_FFTRES;   // unit: bin
	int writeOffset = (since+shift)/VOX_FFTRES;
	
	if (writeOffset < 0) { // 数组越界
		readOffset -= writeOffset;
		count += writeOffset;
		writeOffset = 0;
	}
	
	if (count + readOffset  > VOX_FFTLEN) count = VOX_FFTLEN - readOffset ;
	if (count + writeOffset > VOX_FFTLEN) count = VOX_FFTLEN - writeOffset;
	
	// filter noise
	{
		vox_complex_t *c = fft.fft;
		int i = VOX_FFTLEN;
		while (c++,i--) {
			const float thresh = 5e16;
			register float r = c->real, i = c->imag;
			if (r*r+i*i > thresh*thresh) {
				c->real = 0;
				c->imag = 0;
			}
		}
	}
	
	memcpy(fft_tmp, fft.fft + readOffset,  count * sizeof(vox_complex_t));
	memcpy(fft.fft + writeOffset, fft_tmp, count * sizeof(vox_complex_t));
	
	vox_eq_apply(&filter1, &fft);
	
	vox_fft_end(&fft, buf);
	return 0;
}
