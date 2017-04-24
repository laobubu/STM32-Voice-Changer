/*!
 * ==== STM32 VoiceChanger (vox) ====
 * Pitch manipulating module.
 *
 * written by laobubu, for SensorTile
 * 
 * Copyright 2017 laobubu
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma diag_suppress 2803

#include <string.h>
#include <arm_math.h>

#include "vox.h"
#include "vox-fft.h"
#include "vox-eq.h"
#include "vox-memcpy.h"

static vox_fft_t fft = {{0}};
vox_complex_t fft_tmp[VOX_FFTLEN];

static const float cutoff   = VOX_SPLFREQ;  	//Hz
static const float since    = 100;  	  //Hz
static float shift    = -400;       //Hz

static voxmc_handle_t mc1, mc2, mc3;

// 一些滤波器

static vox_eq_line_t filter1lines[] = {
	// Sex filter
	{ 50,  1.0f, 500,  1.0f }, // left
	{ 500, 1.0f, 1000, 1.0f }, // right
	// 滤掉低频
	{ 0, 0.7f,	400, 1.0f },
	{ 0, 0.8f,	300, 1.0f },
	// 滤掉高频
	{ 16000, 1.0f,  20000, 0.6f },
	{ 9000, 1.0f,  18000, 0.3f },
};
static vox_eq_t filter1 = vox_eq_from_lines(filter1lines);

int vox_init_pitch() {
	vox_init_fft();
	vox_eq_compile(&filter1);
	
	voxmc_init(&mc1);
	voxmc_init(&mc2);
	voxmc_init(&mc3);
	return 0;
}

void vox_pitch_set_shift(float shiftHz) {
	shift = shiftHz;
}

// 设置性感度， -1.0 ~ 0 偏男性， 0 ~ +1.0 偏女性
void vox_pitch_set_sex(float ratio) {
	if (ratio < -1.0f) ratio = -1.0f;
	else if (ratio > 1.0f) ratio = 1.0f;
	
	float width = 50, width2 = 200;
	float center = ratio * 300 + 400;
	float gain = 1.0f - VOX_ABS(ratio) / 2;
	filter1lines[0].f2 = center - width;
	filter1lines[0].f1 = filter1lines[0].f2 - width2;
	filter1lines[0].gain1 = gain;
	vox_eq_compile_line(&filter1lines[0]);
	
	filter1lines[1].f1 = center + 50;
	filter1lines[1].f2 = filter1lines[1].f1 + width2;
	filter1lines[1].gain2 = gain;
	vox_eq_compile_line(&filter1lines[1]);
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
	
	// shift
	memcpy(fft_tmp, fft.fft + readOffset,  count * sizeof(vox_complex_t));
	
	voxmc_memcpy(&mc1, fft.fft + writeOffset, fft_tmp, count * sizeof(vox_complex_t));
	voxmc_memset(&mc2, fft.fft + writeOffset + count, 0, (VOX_FFTLEN - writeOffset - count) * sizeof(vox_complex_t));
	//voxmc_memset(&mc3, fft.fft, 0, writeOffset * sizeof(vox_complex_t));
	
	voxmc_wait(&mc1);
	voxmc_wait(&mc2);
	//voxmc_wait(&mc3);
	
	// filter noise
	{
		vox_complex_t *c = fft.fft + writeOffset;
		int i = count;
		while (c++,i--) {
			const float thresh = 5e17;
			register float r = c->real, i = c->imag;
			if (r*r+i*i > thresh*thresh) {
				c->real = 0;
				c->imag = 0;
			}
		}
	}
	
	vox_eq_apply(&filter1, &fft);
	
	vox_fft_end(&fft, buf);
	return 0;
}
