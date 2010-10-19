/**
   src/vpu5/shvpu5_avcenc.h

   This component implements H.264 / MPEG-4 AVC video codec.
   The H.264 / MPEG-4 AVC video encoder/decoder is implemented
   on the Renesas's VPU5HG middleware library.

   Copyright (C) 2010 IGEL Co., Ltd
   Copyright (C) 2010 Renesas Solutions Corp.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation; either version 2.1 of
   the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
   02110-1301 USA

*/
#ifndef __SIMPLE_AVCENC_H_
#define __SIMPLE_AVCENC_H_
#include <pthread.h>
#include <bellagio/tsemaphore.h>
#include "mciph.h"
#include "mcvenc.h"
#include "uiomux/uiomux.h"

#define SHVPU_AVCENC_OUTBUF_SIZE (1280 * 720)

typedef enum {
	SHVPU_BUFFER_STATUS_NONE = 0,
	SHVPU_BUFFER_STATUS_READY,
	SHVPU_BUFFER_STATUS_SET,
	SHVPU_BUFFER_STATUS_FILL
} shvpu_buffer_status_t;

typedef struct {
	MCVENC_STRM_BUFF_INFO_T bufferInfo;
	shvpu_buffer_status_t	status;
	int			frameId;
} shvpu_avcenc_outbuf_t;

typedef struct {
	MCIPH_DRV_INFO_T*	pDrvInfo;
	/** @param mode for VPU5HG video decoder */
	MCIPH_WORK_INFO_T	wbufVpu5;
	MCIPH_VPU5_INIT_T	vpu5Init;
	UIOMux*			uiomux;
	void*			pContext;
	pthread_t		intrHandler;
	int			frameId;
	unsigned char		isEndInput;
	tsem_t			uioSem;
	int			isExit;

	/* only for encode */
	shvpu_avcenc_outbuf_t streamBuffer[2];
} shvpu_codec_t;

int logd(const char *format, ...);
int loge(const char *format, ...);

void *
pmem_alloc(size_t size, int align, unsigned long *paddr);
void
pmem_free(void *vaddr, size_t size);

unsigned long
uio_virt_to_phys(void *context, long mode, unsigned long addr);
void *
uio_phys_to_virt(unsigned long paddr);

long
encode_init(int width, int height,
	    int bitrate, int framerate, shvpu_codec_t **ppCodec);
int
encode_header(void *context, unsigned char *pBuffer, size_t nBufferLen);
int
encode_main(MCVENC_CONTEXT_T *pContext, int frameId,
	    unsigned char *pBuffer, int nWidth, int nHeight);
int
encode_endcode(void *context, unsigned char *pBuffer, size_t nBufferLen);

#endif /* __SIMPLE_AVCENC_H_ */