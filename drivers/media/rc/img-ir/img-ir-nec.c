/*
 * ImgTec IR Decoder setup for NEC protocol.
 *
 * Copyright 2010-2014 Imagination Technologies Ltd.
 */

#include "img-ir-hw.h"

/* Convert NEC data to a scancode */
static int img_ir_nec_scancode(int len, u64 raw, int *scancode, u64 protocols)
{
	unsigned int addr, addr_inv, data, data_inv;
	/* a repeat code has no data */
	if (!len)
		return IMG_IR_REPEATCODE;
	if (len != 32)
		return -EINVAL;
	/* raw encoding: ddDDaaAA */
	addr     = (raw >>  0) & 0xff;
	addr_inv = (raw >>  8) & 0xff;
	data     = (raw >> 16) & 0xff;
	data_inv = (raw >> 24) & 0xff;
	if ((data_inv ^ data) != 0xff) {
		/* 32-bit NEC (used by Apple and TiVo remotes) */
		/* scan encoding: aaAAddDD */
		*scancode = addr_inv << 24 |
			    addr     << 16 |
			    data_inv <<  8 |
			    data;
	} else if ((addr_inv ^ addr) != 0xff) {
		/* Extended NEC */
		/* scan encoding: AAaaDD */
		*scancode = addr     << 16 |
			    addr_inv <<  8 |
			    data;
	} else {
		/* Normal NEC */
		/* scan encoding: AADD */
		*scancode = addr << 8 |
			    data;
	}
	return IMG_IR_SCANCODE;
}

/* Convert NEC scancode to NEC data filter */
static int img_ir_nec_filter(const struct rc_scancode_filter *in,
			     struct img_ir_filter *out, u64 protocols)
{
	unsigned int addr, addr_inv, data, data_inv;
	unsigned int addr_m, addr_inv_m, data_m, data_inv_m;

	data       = in->data & 0xff;
	data_m     = in->mask & 0xff;

	if ((in->data | in->mask) & 0xff000000) {
		/* 32-bit NEC (used by Apple and TiVo remotes) */
		/* scan encoding: aaAAddDD */
		addr_inv   = (in->data >> 24) & 0xff;
		addr_inv_m = (in->mask >> 24) & 0xff;
		addr       = (in->data >> 16) & 0xff;
		addr_m     = (in->mask >> 16) & 0xff;
		data_inv   = (in->data >>  8) & 0xff;
		data_inv_m = (in->mask >>  8) & 0xff;
	} else if ((in->data | in->mask) & 0x00ff0000) {
		/* Extended NEC */
		/* scan encoding AAaaDD */
		addr       = (in->data >> 16) & 0xff;
		addr_m     = (in->mask >> 16) & 0xff;
		addr_inv   = (in->data >>  8) & 0xff;
		addr_inv_m = (in->mask >>  8) & 0xff;
		data_inv   = data ^ 0xff;
		data_inv_m = data_m;
	} else {
		/* Normal NEC */
		/* scan encoding: AADD */
		addr       = (in->data >>  8) & 0xff;
		addr_m     = (in->mask >>  8) & 0xff;
		addr_inv   = addr ^ 0xff;
		addr_inv_m = addr_m;
		data_inv   = data ^ 0xff;
		data_inv_m = data_m;
	}

	/* raw encoding: ddDDaaAA */
	out->data = data_inv << 24 |
		    data     << 16 |
		    addr_inv <<  8 |
		    addr;
	out->mask = data_inv_m << 24 |
		    data_m     << 16 |
		    addr_inv_m <<  8 |
		    addr_m;
	return 0;
}

/*
 * NEC decoder
 * See also http://www.sbprojects.com/knowledge/ir/nec.php
 *        http://wiki.altium.com/display/ADOH/NEC+Infrared+Transmission+Protocol
 */
struct img_ir_decoder img_ir_nec = {
	.type = RC_BIT_NEC,
	.control = {
		.decoden = 1,
		.code_type = IMG_IR_CODETYPE_PULSEDIST,
	},
	/* main timings */
	.unit = 562500, /* 562.5 us */
	.timings = {
		/* leader symbol */
		.ldr = {
			.pulse = { 16	/* 9ms */ },
			.space = { 8	/* 4.5ms */ },
		},
		/* 0 symbol */
		.s00 = {
			.pulse = { 1	/* 562.5 us */ },
			.space = { 1	/* 562.5 us */ },
		},
		/* 1 symbol */
		.s01 = {
			.pulse = { 1	/* 562.5 us */ },
			.space = { 3	/* 1687.5 us */ },
		},
		/* free time */
		.ft = {
			.minlen = 32,
			.maxlen = 32,
			.ft_min = 10,	/* 5.625 ms */
		},
	},
	/* repeat codes */
	.repeat = 108,			/* 108 ms */
	.rtimings = {
		/* leader symbol */
		.ldr = {
			.space = { 4	/* 2.25 ms */ },
		},
		/* free time */
		.ft = {
			.minlen = 0,	/* repeat code has no data */
			.maxlen = 0,
		},
	},
	/* scancode logic */
	.scancode = img_ir_nec_scancode,
	.filter = img_ir_nec_filter,
};
