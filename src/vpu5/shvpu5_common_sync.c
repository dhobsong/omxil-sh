/**
   src/vpu5/shvpu5_common_sync.c

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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>
#include "mcvdec.h"
#include "shvpu5_common_log.h"
#include "shvpu5_avcdec.h"
#include "shvpu5_avcdec_omx.h"
#include "shvpu5_avcdec_meram.h"

static pthread_cond_t cond_vp5[2] = {
	PTHREAD_COND_INITIALIZER,
	PTHREAD_COND_INITIALIZER
};
static pthread_mutex_t mutex_vp5[2] = {
	PTHREAD_MUTEX_INITIALIZER,
	PTHREAD_MUTEX_INITIALIZER
};
static int counter_vp5[2][2], status_vp5[2][2];

#define VP5_MODULE_VLC		0
#define VP5_MODULE_CE		1

static inline void
_uf_vp5_restart(void *context, long mode, int module)
{
	pthread_mutex_lock(&mutex_vp5[module]);
	status_vp5[module][mode] = 0;
	counter_vp5[module][mode]++;
	pthread_cond_broadcast(&cond_vp5[module]);
	pthread_mutex_unlock(&mutex_vp5[module]);
	return;
}

static inline void
_uf_vp5_sleep(void *context, long mode, int module)
{
	int prev, ret;

	pthread_mutex_lock(&mutex_vp5[module]);
	if (status_vp5[module][mode] == 0) {
		logd("MODULE(%d) not running!\n", module);
		pthread_mutex_unlock(&mutex_vp5[module]);
		return;
	}

	//pthread_mutex_lock(&mutex_vp5[module]);
	prev = counter_vp5[module][mode];
	do {
		ret = pthread_cond_wait(&cond_vp5[module], &mutex_vp5[module]);
	} while (prev == counter_vp5[module][mode]);
	pthread_mutex_unlock(&mutex_vp5[module]);

	return;
}

static inline void
_uf_vp5_start(void *context, long mode, int module)
{
	pthread_mutex_lock(&mutex_vp5[module]);
	status_vp5[module][mode] = 1;
	pthread_mutex_unlock(&mutex_vp5[module]);

	return;
}

void
mciph_uf_ce_restart(void *context, long mode)
{
	logd("%s invoked.\n", __FUNCTION__);
#ifdef MERAM_ENABLE
	if (mode == MCIPH_DEC) {
		MCVDEC_CONTEXT_T *dec_context = (MCVDEC_CONTEXT_T *)context;
		shvpu_avcdec_PrivateType *shvpu_avcdec_Private =
	                (shvpu_avcdec_PrivateType *)dec_context->user_info;
		if(shvpu_avcdec_Private->features.tl_conv_mode == OMX_FALSE) {
			_uf_vp5_restart(context, mode, VP5_MODULE_CE);
			return;
		}
		finish_meram_write(&shvpu_avcdec_Private->meram_data,
			shvpu_avcdec_Private->meram_data.decY_icb);
		finish_meram_write(&shvpu_avcdec_Private->meram_data,
			shvpu_avcdec_Private->meram_data.decC_icb);
	}
#endif
	_uf_vp5_restart(context, mode, VP5_MODULE_CE);
	return;
}

void
mciph_uf_ce_sleep(void *context, long mode)
{
	logd("%s invoked.\n", __FUNCTION__);
	_uf_vp5_sleep(context, mode, VP5_MODULE_CE);
	logd("%s terminating...\n", __FUNCTION__);
	return;
}

void
mciph_uf_ce_start(void *context, long mode, void *start_info)
{
	logd("%s invoked.\n", __FUNCTION__);
#ifdef MERAM_ENABLE
	if (mode == MCIPH_DEC) {
		MCVDEC_CONTEXT_T *dec_context = (MCVDEC_CONTEXT_T *)context;
		int i;
		shvpu_avcdec_PrivateType *shvpu_avcdec_Private =
	                (shvpu_avcdec_PrivateType *)dec_context->user_info;
		MCVDEC_FMEM_INDEX_T *fmem_index;
		if(shvpu_avcdec_Private->features.tl_conv_mode == OMX_FALSE) {
			_uf_vp5_start(context, mode, VP5_MODULE_CE);
			return;
		}
		fmem_index = (MCVDEC_FMEM_INDEX_T *)start_info;
		set_meram_address(&shvpu_avcdec_Private->meram_data,
			shvpu_avcdec_Private->meram_data.decY_icb,
			*fmem_index->ce_img_addr.decY_addr);
		set_meram_address(&shvpu_avcdec_Private->meram_data,
			shvpu_avcdec_Private->meram_data.decC_icb,
			*fmem_index->ce_img_addr.decC_addr);
		*fmem_index->ce_img_addr.decY_addr =
			meram_get_icb_address(
				shvpu_avcdec_Private->meram_data.meram,
				shvpu_avcdec_Private->meram_data.decY_icb, 0);
		*fmem_index->ce_img_addr.decC_addr =
			meram_get_icb_address(
				shvpu_avcdec_Private->meram_data.meram,
				shvpu_avcdec_Private->meram_data.decC_icb, 0);

		if (*fmem_index->ce_img_addr.fmem_x_size[MCVDEC_FMX_DEC] < 1024)
			*fmem_index->ce_img_addr.fmem_x_size[MCVDEC_FMX_DEC]
				= 1024;
	}
#endif
	_uf_vp5_start(context, mode, VP5_MODULE_CE);
	return;
}

void
mciph_uf_vlc_restart(void *context, long mode)
{
	logd("%s invoked.\n", __FUNCTION__);
	_uf_vp5_restart(context, mode, VP5_MODULE_VLC);
	return;
}

void
mciph_uf_vlc_sleep(void *context, long mode)
{
	logd("%s invoked.\n", __FUNCTION__);
	_uf_vp5_sleep(context, mode, VP5_MODULE_VLC);
	logd("%s terminating...\n", __FUNCTION__);
	return;
}

void
mciph_uf_vlc_start(void *context, long mode)
{
	logd("%s invoked.\n", __FUNCTION__);
	_uf_vp5_start(context, mode, VP5_MODULE_VLC);
	return;
}

