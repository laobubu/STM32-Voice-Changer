#include "vox.h"
#include "vox-fft.h"

static vox_fft_t fft = {{0}};

int vox_init_delay() {
	vox_init_fft();
	return 0;
}

int vox_proc_delay(vox_buf_t *buf){
	vox_fft_begin(&fft, buf);
	
	// ...
	
	vox_fft_end(&fft, buf);
	return 0;
}
