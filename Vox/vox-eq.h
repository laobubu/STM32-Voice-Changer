#ifndef __VOX_EQ_H
#define __VOX_EQ_H

/*
 一个靠一大堆直线堆出来的蜜汁均衡器
 */

#include "vox.h"
#include "vox-fft.h"

typedef struct {
	float f1, gain1; // 左点的频率和增益
	float f2, gain2; // 右点的频率和增益
	
	// 下面的为私有属性，由 vox_eq_compile_line 生成。不要直接修改！
	float _k;
	int _i1, _i2;
} vox_eq_line_t;

typedef struct {
	vox_eq_line_t *lines;
	int count;
} vox_eq_t;

#define vox_eq_from_lines(lines)  ((vox_eq_t){(lines), sizeof(lines)/sizeof(vox_eq_line_t)})

void vox_eq_compile_line(vox_eq_line_t *line);
void vox_eq_compile(vox_eq_t *eq);

void vox_eq_apply(vox_eq_t *eq, vox_fft_t *fft);

#endif
