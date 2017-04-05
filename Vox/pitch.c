#include <string.h>

#include "vox.h"
#include "vox-fft.h"

static vox_fft_t fft = {{0}};
float fft_tmp[VOX_FFTLEN*2];

static const float cutoff   = 4000;  	//Hz
static const float since    = 600;  	//Hz
static const float ratio    = 1.7;

static const uint32_t fftBarMax = cutoff / VOX_FFTRES * (ratio>1?1:ratio);

int vox_init_pitch() {
	vox_init_fft();
	return 0;
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
	
	float *dst1 = fft_tmp;
	int imax = VOX_FFTLEN / 2 / ratio;
	int isince = since/VOX_FFTRES;
	if (imax > fftBarMax) imax = fftBarMax;
	
	for (int i = isince; i < imax; i++) {
		float f = i * VOX_FFTRES * ratio;
		vox_fft_interpolate(&fft, f, dst1);
		dst1 += 2;
	}
	memcpy(fft.fft+isince, fft_tmp, dst1 - fft_tmp);
	
	vox_fft_end(&fft, buf);
	return 0;
}
