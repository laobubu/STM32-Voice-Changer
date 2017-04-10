#include <string.h>

#include "vox.h"
#include "vox-fft.h"

static vox_fft_t fft = {{0}};
vox_complex_t fft_tmp[VOX_FFTLEN];

static float cutoff   = 20000;  	//Hz
static float since    = 600;  	  //Hz
static float shift    = 400;       //Hz

static float lp_since = 10000;  // Hz, where decay starts
static float lp_decay = 0.4;  // percent per bin

int vox_init_pitch() {
	vox_init_fft();
	return 0;
}

void vox_pitch_set(float shiftHz) {
	shift = shiftHz;
}

int vox_proc_pitch(vox_buf_t *buf){
//	const int32_t shift_since = lowPass / VOX_FFTRES;
//	const int32_t shift_len = (highPass - lowPass) / VOX_FFTRES;
//	const int32_t shift_step = shift / VOX_FFTRES;
//	
//	float *src = fft.fft + (shift_since) * 2;
//	float *dst = fft.fft + (shift_since + shift_step) * 2;
//	int32_t len = shift_len * sizeof(float) * 2;
//	
//	if (shift_step < 0) { /// 数据向前移动
//		memcpy(dst, src, len);
//	} else { /// 数据向后面动
//		for(int i = len / sizeof(float); i >=0; i--) {
//			*dst++ = *src++; // 慢速复制，保证不重叠
//		}
//	}
//	
	vox_fft_begin(&fft, buf);
	
	int count = (cutoff - since) / VOX_FFTRES;
	int readOffset = (since)/VOX_FFTRES;   // unit: bin
	int writeOffset = (since+shift)/VOX_FFTRES;
	
	memcpy(fft_tmp, fft.fft + readOffset,  count);
	memcpy(fft.fft + writeOffset, fft_tmp, count);
	
	//decay. LP
	
	writeOffset = lp_since/VOX_FFTRES;
	vox_complex_t *c_i = fft.fft + writeOffset;
	float p_i = 1.0f;
	for (int i = writeOffset; i < VOX_FFTLEN; i++, c_i++) {
		if (p_i < 0.01f) {
			memset(c_i, 0, (VOX_FFTLEN - i)*sizeof(vox_complex_t));
			break;
		}
		c_i->real *= p_i;
		p_i *= lp_decay;
	}
	
	vox_fft_end(&fft, buf);
	return 0;
}
