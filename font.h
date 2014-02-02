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

#ifndef __FONT_H__
#define __FONT_H__

#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glext.h>

class Font {
public:
	
	Font(const char *name);
	virtual ~Font();
	
	void enable(int w,int h);
	void disable();
	
	void puts(float x,float y,const char *str);
	void printf(float x,float y,const char *format,...);
	void printfc(float x,float y,const char *format,...);
	
protected:
	
	GLuint tex_id;
	GLuint list_id;
	
	int step;
	int space[256][2];
	
	int width;
	int height;
	
	float modelview[16];
	float projection[16];
};

#endif /* __FONT_H__ */
