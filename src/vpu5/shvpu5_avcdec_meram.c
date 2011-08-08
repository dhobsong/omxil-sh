#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <meram/meram.h>
#include "shvpu5_avcdec.h"

static void *meram_base = NULL;
static int mem_fd;
static void
release_icb(shvpu_meram_t *mdata, ICB *icb) {
	if (icb && mdata && mdata->meram) {
		meram_free_icb_memory(mdata->meram, icb);
		meram_unlock_icb(mdata->meram, icb);
	}
}
int
open_meram(shvpu_meram_t *mdata)
{
	MERAM_REG *reg;
	unsigned long tmp;
	mdata->meram = meram_open();
	if (mdata->meram == NULL)
		return -1;
	reg = meram_lock_reg(mdata->meram);
	meram_read_reg(mdata->meram, reg, MEVCR1, &tmp);
	meram_write_reg(mdata->meram, reg, MEVCR1, tmp | 0x20000000);
	meram_unlock_reg(mdata->meram, reg);
	return 0;
}
void
close_meram(shvpu_meram_t *mdata)
{
	if (!mdata || !mdata->meram)
		return;
	if (mdata->decY_icb)
		release_icb(mdata, mdata->decY_icb);
	if (mdata->decC_icb)
		release_icb(mdata, mdata->decC_icb);
	meram_close(mdata->meram);
}
unsigned long
setup_icb(shvpu_meram_t *mdata,
	  ICB **icb,
	  unsigned long pitch,
	  unsigned long lines,
	  int res_lines,
	  int block_lines,
	  int rdnwr,
	  int index)
{
	unsigned int total_len = pitch * lines;
	unsigned int xk_lines;
	unsigned int line_len;
	unsigned long tmp;
	int pitch_2n;
	int md, res;
	int memblk;

	MERAM *meram = mdata->meram;

	md = rdnwr == 0 ? 1 : 2;
	res = rdnwr == 0 ? 2 : 0x20;

	if (pitch <= 1024)
		pitch_2n = 1;
	else if (pitch <= 2048)
		pitch_2n = 2;
	else /* Only support up to 4k pixel width frames */
		pitch_2n = 4;


	if ((*icb = meram_lock_icb(meram, index)) == NULL)
		return -1;

	memblk = meram_alloc_icb_memory(meram, *icb, pitch_2n * res_lines);

	meram_write_icb(meram, *icb, MExxCTL, (block_lines << 28) |
		(memblk  << 16) | 0x708 | md);

	meram_write_icb(meram, *icb, MExxSIZE, (lines-1) << 16 |
		(pitch -1));

	meram_write_icb(meram, *icb, MExxMNCF, ((res_lines - 1) << 16) |
		(res << 24) | (0 << 15));

	meram_write_icb(meram, *icb, MExxBSIZE, pitch | 0x90000000);

	return 0;
}
void
set_meram_address(shvpu_meram_t *mdata, ICB *icb, unsigned long address)
{
	if (icb && mdata)
		meram_write_icb(mdata->meram, icb, MExxSARA, address);
}
void
finish_meram_write(shvpu_meram_t *mdata, ICB *icb) {
	unsigned long tmp;
	if (icb && mdata && mdata->meram) {
		meram_read_icb(mdata->meram, icb, MExxCTL, &tmp);
		meram_write_icb(mdata->meram, icb, MExxCTL, tmp | 0x20);
	}
}

void
finish_meram_read(shvpu_meram_t *mdata, ICB *icb) {
	unsigned long tmp;
	if (icb && mdata && mdata->meram) {
		meram_read_icb(mdata->meram, icb, MExxCTL, &tmp);
		meram_write_icb(mdata->meram, icb, MExxCTL, tmp | 0x10);
	}
}
