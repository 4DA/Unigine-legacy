/* Font
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
#include <stdarg.h>
#include "texture.h"
#include "font.h"

#include "font_data.h"

Font::Font() {
	
	int width = 256;
	int height = 256;
	unsigned char *data = new unsigned char[width * height];
	
	for(int i = 0, j = 0; i < width * height; i++) {
		if(font_data_rle[j] == font_data_rle[j + 1]) {
			int n = font_data_rle[j + 2];
			for(int k = 0; k < n + 2; k++) data[i++] = (unsigned char)font_data_rle[j];
			i -= 1;
			j += 3;
		} else {
			data[i] = (unsigned char)font_data_rle[j++];
		}
	}
	
	glGenTextures(1,&tex_id);
	glBindTexture(GL_TEXTURE_2D,tex_id);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D,0,GL_LUMINANCE,width,height,0,GL_LUMINANCE,GL_UNSIGNED_BYTE,data);
	
	int size = width;
	step = size / 16;
	for(int y = 0, i = 0; y < 16; y++) {
		for(int x = 0; x < 16; x++, i++) {
			unsigned char *ptr;
			space[i][0] = 0;
			for(int j = 0; j < step; j++) {
				ptr = data + size * y * step + x * step + j;
				int k;
				for(k = 0; k < step; k++) {
					if(*ptr != 0) break;
					ptr += size;
				}
				if(k != step) break;
				space[i][0]++;
			}
			space[i][1] = 0;
			if(space[i][0] == step) continue;
			for(int j = step - 1; j >= 0; j--) {
				ptr = data + size * y * step + x * step + j;
				int k;
				for(k = 0; k < step; k++) {
					if(*ptr != 0) break;
					ptr += size;
				}
				if(k != step) break;
				space[i][1]++;
			}
			space[i][1] = step - space[i][0] - space[i][1];
		}
	}
	delete data;
	
	list_id = glGenLists(256);
	for(int y = 0, i = 0; y < 16; y++) {
		for(int x = 0; x < 16; x++, i++) {
			float s = (float)x / 16.0f + (float)space[i][0] / (float)size;
			float t = (float)y / 16.0f;
			float ds = (float)space[i][1] / (float)size;
			float dt = 1.0 / 16.0;
			glNewList(list_id + i,GL_COMPILE);
			glBegin(GL_QUADS);
			glTexCoord2f(s,t);
			glVertex2i(0,0);
			glTexCoord2f(s,t + dt);
			glVertex2i(0,step);
			glTexCoord2f(s + ds,t + dt);
			glVertex2i(space[i][1],step);
			glTexCoord2f(s + ds,t);
			glVertex2i(space[i][1],0);
			glEnd();
			glTranslatef(space[i][1],0,0);
			glEndList();
		}
	}
}

Font::~Font() {
	glDeleteTextures(1,&tex_id);
	glDeleteLists(256,list_id);
}

/*
 */
void Font::enable(int w,int h) {
	width = w;
	height = h;
	glGetFloatv(GL_PROJECTION_MATRIX,projection);
	glGetFloatv(GL_MODELVIEW_MATRIX,modelview);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0,width,height,0,-1,1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,tex_id);
}

void Font::disable() {
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(projection);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(modelview);
}

/*
 */
void Font::puts(float x,float y,const char *str) {
	for(int i = 0; i < 2; i++) {
		if(i == 0) glBlendFunc(GL_ZERO,GL_ONE_MINUS_SRC_COLOR);
		else glBlendFunc(GL_ONE,GL_ONE);
		glPushMatrix();
		glTranslatef(x,y,0);
		char *s = (char*)str;
		for(int lines = 0; *s; s++) {
			if(*s == '\n') {
				lines++;
				glLoadIdentity();
				glTranslatef(x,y + step * lines,0);
			} else if(*s == '\r') {
				glLoadIdentity();
				glTranslatef(x,y + step * lines,0);
			} else if(*s == '\t') {
				glTranslatef(step * 2,0,0);
			} else if(*s == ' ') {
				glTranslatef(step / 2,0,0);
			} else {
				glCallList(list_id + *(unsigned char*)s);
			}
		}
		glPopMatrix();
	}
}

/*
 */
void Font::printf(float x,float y,const char *format,...) {
	char buf[1024];
	va_list argptr;
	va_start(argptr,format);
	vsprintf(buf,format,argptr);
	va_end(argptr);
	puts(x,y,buf);
}

/*
 */
void Font::printfc(float x,float y,const char *format,...) {
	char buf[1024];
	va_list argptr;
	va_start(argptr,format);
	vsprintf(buf,format,argptr);
	va_end(argptr);
	char *s = buf;
	char *line = buf;
	for(int length = 0, lines = 0;; s++) {
		if(*s != '\n' && *s != '\0') {
			if(*s == ' ') length += step / 2;
			else if(*s == '\t') length += step * 2;
			else length += space[*(unsigned char*)s][1];
		}
		else {
			if(*s == '\0') {
				printf(x - length / 2,y + step * lines,"%s",line);
				break;
			}
			*s = '\0';
			puts(x - length / 2,y + step * lines,line);
			line = s + 1;
			length = 0;
			lines++;
		}
	}
}
