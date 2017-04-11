#include <string.h>

#include "vox.h"
#include "vox-fft.h"

static vox_fft_t fft = {{0}};
vox_complex_t fft_tmp[VOX_FFTLEN];

static const float cutoff   = 20000;  	//Hz
static const float since    = 100;  	  //Hz
static float shift    = -400;       //Hz

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
	
	if (writeOffset < 0) { // 数组越界
		readOffset -= writeOffset;
		count += writeOffset;
		writeOffset = 0;
	}
	
	if (count + readOffset  > VOX_FFTLEN) count = VOX_FFTLEN - readOffset ;
	if (count + writeOffset > VOX_FFTLEN) count = VOX_FFTLEN - writeOffset;
	
	vox_fft_hpf(&fft, 400,  -100);
	vox_fft_hpf(&fft, 300,  -50);
	vox_fft_hpf(&fft, 200,  0);
	
	memcpy(fft_tmp, fft.fft + readOffset,  count * sizeof(vox_complex_t));
	memcpy(fft.fft + writeOffset, fft_tmp, count * sizeof(vox_complex_t));
	
	vox_fft_lpf(&fft, 8000, 15000);
	vox_fft_hpf(&fft, 200,  50);
	
	vox_fft_end(&fft, buf);
	return 0;
}
