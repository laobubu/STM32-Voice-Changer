/*!
 * ==== STM32 VoiceChanger (vox) ====
 * Simple equalizer for Complex-in-float based on line segments.
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
 
#include "vox-eq.h"

void vox_eq_compile_line(vox_eq_line_t *line) {
	line->_i1 = line->f1 / VOX_FFTRES;
	line->_i2 = line->f2 / VOX_FFTRES;
	line->_k  = (line->gain2 - line->gain1) / (line->_i2 - line->_i1);
}

void vox_eq_compile(vox_eq_t *eq) {
	int i = eq->count;
	vox_eq_line_t *line = eq->lines;
	while (i--) vox_eq_compile_line(line++);
}

void vox_eq_apply(vox_eq_t *eq, vox_fft_t *fft) {
	int ci_i = VOX_FFTLEN;
	vox_complex_t *ci = fft->fft;
	
	int eqi;
	vox_eq_line_t *line;
	
	float gain;
	
	while (ci_i--) {
		gain = 1.0f;
		eqi = eq->count;
		line = eq->lines;
		
		while (eqi--) {
			     if (ci_i <= line->_i1) { if (line->gain1 != 1.0f) gain *= line->gain1; }
			else if (ci_i >= line->_i2) { if (line->gain2 != 1.0f) gain *= line->gain2; }
			else                        { gain *= line->gain1 + line->_k * (ci_i - line->_i1); }
			line++;
		}
		
		if (gain != 1.0f) {
			ci->real *= gain;
			ci->imag *= gain;
		}
		
		ci++;
	}
}
