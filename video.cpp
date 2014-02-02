/* Video
 *
 * Copyright (C) 2003-2004, Alexander Zaprjagaev <frustum@frustum.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include "video.h"

Video::Video(const char *name,int width,int height,int bitrate) {
	
	avcodec_init();
	avcodec_register_all();
	
	codec = avcodec_find_encoder(CODEC_ID_MPEG2VIDEO);
	c = avcodec_alloc_context();
	
	c->bit_rate = bitrate;
	c->width = width;
	c->height = height;
	c->frame_rate = 25;
	c->frame_rate_base= 1;
	c->gop_size = 10;
	c->max_b_frames = 1;
	
	avcodec_open(c,codec);
	
	picture = avcodec_alloc_frame();
	
	yuv = avcodec_alloc_frame();
	int size = avpicture_get_size(PIX_FMT_YUV420P,width,height);
	yuv_data = (uint8_t*)malloc(size);
	avpicture_fill((AVPicture*)yuv,yuv_data,PIX_FMT_YUV420P,width,height);
	
	outbuf_size = 1024 * 1024;
	outbuf = (uint8_t*)malloc(outbuf_size);
	
	file = fopen(name,"wb");
}

Video::~Video() {
	char buf[4] = { 0x00, 0x00, 0x01, 0xb7 };
	fwrite(buf,1,4,file);
	fclose(file);
	avcodec_close(c);
	free(c);
	free(yuv);
	free(yuv_data);
	free(picture);
}

/*
 */
void Video::save(unsigned char *data,int flip) {
	
	if(flip) {
		for(int y = 0; y < c->height / 2; y++) {
			unsigned char *l0 = &data[c->width * y * 3];
			unsigned char *l1 = &data[c->width * (c->height - y - 1) * 3];
			for(int x = 0; x < c->width * 3; x++) {
				unsigned char l = *l0;
				*l0++ = *l1;
				*l1++ = l;
			}
		}
	}
	
	avpicture_fill((AVPicture*)picture,(uint8_t*)data,PIX_FMT_RGB24,c->width,c->height);
	img_convert((AVPicture*)yuv,PIX_FMT_YUV420P,(AVPicture*)picture,PIX_FMT_RGB24,c->width,c->height);
	
	int out_size = avcodec_encode_video(c,outbuf,outbuf_size,yuv);
	fwrite(outbuf,1,out_size,file);
}
