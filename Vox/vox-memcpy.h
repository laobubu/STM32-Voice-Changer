#ifndef _vox_memcpy_h_
#define _vox_memcpy_h_

typedef struct {
	void *dma;
	int id;
	char _tmp1;
} voxmc_handle_t;

void voxmc_init(voxmc_handle_t *handle);
void voxmc_memcpy(voxmc_handle_t *handle, void *dst, const void*src, unsigned int bytes);
void voxmc_memset(voxmc_handle_t *handle, void *dst, const int value, unsigned int bytes);
void voxmc_wait(voxmc_handle_t *handle); // wait until completed

#endif
