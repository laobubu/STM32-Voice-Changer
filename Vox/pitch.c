#include <string.h>

#include "vox.h"
#include "vox-fft.h"

static vox_fft_t fft = {{0}};
vox_complex_t fft_tmp[VOX_FFTLEN];

static float cutoff   = 10000;  	//Hz
static float since    = 600;  	  //Hz
static float shift    = -200;       //Hz

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
	memcpy(fft_tmp, fft.fft + (int)(since/VOX_FFTRES), count);
	memcpy(fft.fft + (int)((since+shift)/VOX_FFTRES), fft_tmp, count);
	
	vox_fft_end(&fft, buf);
	return 0;
}
