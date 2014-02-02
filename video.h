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

#ifndef __VIDEO_H__
#define __VIDEO_H__

#include <stdio.h>
#include <malloc.h>
#include <avcodec.h>

class Video {
public:
	Video(const char *name,int width,int height,int bitrate);
	~Video();
	
	void save(unsigned char *data,int flip = 0);
	
protected:
	
	AVCodec *codec;
	AVCodecContext *c;
	AVFrame *picture;
	AVFrame *yuv;
	uint8_t *yuv_data;
	int outbuf_size;
	uint8_t *outbuf;
	FILE *file;
};

#endif /* __VIDEO_H__ */
