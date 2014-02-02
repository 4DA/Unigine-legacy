/* OpenGL pixel buffer
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

#ifndef _WIN32

/* linux
 */
#include <vector>
#include <GL/gl.h>
#include <GL/glx.h>
#include "pbuffer.h"

/*
 */
struct PBuffer_data {
	Display *display;
	
	GLXPbuffer pbuffer;
	GLXContext context;
	
	GLXPbuffer old_pbuffer;
	GLXContext old_context;
};

/*
 */
PBuffer::PBuffer(int width,int height,int flag) : width(width), height(height) {
	
	Display *display = glXGetCurrentDisplay();
	int screen = DefaultScreen(display);
	GLXContext old_context = glXGetCurrentContext();
	
	std::vector<int> attrib;
	
	if(flag & RGB || flag & RGBA) {
		attrib.push_back(GLX_RED_SIZE);
		attrib.push_back(flag & FLOAT ? 32 : 8);
		attrib.push_back(GLX_GREEN_SIZE);
		attrib.push_back(flag & FLOAT ? 32 : 8);
		attrib.push_back(GLX_BLUE_SIZE);
		attrib.push_back(flag & FLOAT ? 32 : 8);
		if(flag & RGBA) {
			attrib.push_back(GLX_ALPHA_SIZE);
			attrib.push_back(flag & FLOAT ? 32 : 8);
		}
	}
	
	if(flag & DEPTH) {
		attrib.push_back(GLX_DEPTH_SIZE);
		attrib.push_back(24);
	}
	
	if(flag & STENCIL) {
		attrib.push_back(GLX_STENCIL_SIZE);
		attrib.push_back(8);
	}
	
	if(flag & FLOAT) {
		attrib.push_back(GLX_FLOAT_COMPONENTS_NV);
		attrib.push_back(true);
	}
	
	if(flag & MULTISAMPLE_2 || flag & MULTISAMPLE_4) {
		attrib.push_back(GLX_SAMPLE_BUFFERS_ARB);
		attrib.push_back(true);
		attrib.push_back(GLX_SAMPLES_ARB);
		attrib.push_back(flag & MULTISAMPLE_2 ? 2 : 4);
	}
	
	attrib.push_back(GLX_DRAWABLE_TYPE);
	attrib.push_back(GLX_PBUFFER_BIT);
	attrib.push_back(GLX_RENDER_TYPE);
	attrib.push_back(GLX_RGBA_BIT);
	attrib.push_back(0);
	
	std::vector<int> pattrib;
	
	pattrib.push_back(GLX_LARGEST_PBUFFER);
	pattrib.push_back(true);
	pattrib.push_back(GLX_PRESERVED_CONTENTS);
	pattrib.push_back(true);
	
	GLXPbuffer pbuffer;
	GLXContext context;
	
	try {
		int count;
		GLXFBConfig *config;
		
		const char *extensions = glXQueryExtensionsString(display,screen);
		
		if(strstr(extensions,"GLX_SGIX_pbuffer") && strstr(extensions,"GLX_SGIX_fbconfig")) {
			pattrib.push_back(0);
			
			config = glXChooseFBConfigSGIX(display,screen,&attrib[0],&count);
			if(!config) throw("glXChooseFBConfigSGIX() failed");
			
			pbuffer = glXCreateGLXPbufferSGIX(display,config[0],width,height,&pattrib[0]);
			if(!pbuffer) throw("glXCreateGLXPbufferSGIX() failed");
			
			context = glXCreateContextWithConfigSGIX(display,config[0],GLX_RGBA_TYPE,old_context,true);
			if(!context) throw("glXCreateContextWithConfigSGIX() failed");
			
		} else {
			pattrib.push_back(GLX_PBUFFER_WIDTH);
			pattrib.push_back(width);
			pattrib.push_back(GLX_PBUFFER_HEIGHT);
			pattrib.push_back(height);
			pattrib.push_back(0);
			
			config = glXChooseFBConfig(display,screen,&attrib[0],&count);	
			if(!config) throw("glXChooseFBConfig() failed");
			
			pbuffer = glXCreatePbuffer(display,config[0],&pattrib[0]);
			if(!pbuffer) throw("glXCreatePbuffer() failed");
			
			XVisualInfo *visual = glXGetVisualFromFBConfig(display,config[0]);
			if(!visual) throw("glXGetVisualFromFBConfig() failed");
			
			context = glXCreateContext(display,visual,old_context,true);
			if(!context) throw("glXCreateContext() failed");
		}
	}
	catch(const char *error) {
		fprintf(stderr,"PBuffer::PBuffer: %s\n",error);
		pbuffer = glXGetCurrentDrawable();
		context = old_context;
	}
	
	data = new PBuffer_data;
	data->display = display;
	
	data->pbuffer = pbuffer;
	data->context = context;
	
	data->old_pbuffer = glXGetCurrentDrawable();
	data->old_context = old_context;
}

/*
 */
PBuffer::~PBuffer() {
	if(data->context) glXDestroyContext(data->display,data->context);
	if(data->pbuffer) glXDestroyPbuffer(data->display,data->pbuffer);
	delete data;
}

/*
 */
void PBuffer::enable() {
	glXWaitGL();	// very BIG bug fix :)
	
	data->old_pbuffer = glXGetCurrentDrawable();
	data->old_context = glXGetCurrentContext();
	
	if(!glXMakeCurrent(data->display,data->pbuffer,data->context)) {
		fprintf(stderr,"PBuffer::enable(): glXMakeCurrent() failed\n");
	}
}

/*
 */
void PBuffer::disable() {
	glXWaitGL();
	if(!glXMakeCurrent(data->display,data->old_pbuffer,data->old_context)) {
		fprintf(stderr,"PBuffer::disable(): glXMakeCurrent() failed\n");
	}
}

#else

/* windows
 */
#include <vector>
#include <windows.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/wglext.h>
#include "pbuffer.h"

/*
 */
struct PBuffer_data {
	HDC hdc;
	HPBUFFERARB pbuffer;
	HGLRC context;
	
	HDC old_hdc;
	HGLRC old_context;
};

static PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = 0;
static PFNWGLCREATEPBUFFERARBPROC wglCreatePbufferARB = 0;
static PFNWGLGETPBUFFERDCARBPROC wglGetPbufferDCARB = 0;
static PFNWGLRELEASEPBUFFERDCARBPROC wglReleasePbufferDCARB = 0;
static PFNWGLDESTROYPBUFFERARBPROC wglDestroyPbufferARB = 0;

/*
 */
PBuffer::PBuffer(int width,int height,int flag) : width(width), height(height) {
	
	HDC old_hdc = wglGetCurrentDC();
	HGLRC old_context = wglGetCurrentContext();
	
	std::vector<int> attrib;
	
	if(flag & RGB || flag & RGBA) {
		attrib.push_back(WGL_RED_BITS_ARB);
		attrib.push_back(flag & FLOAT ? 32 : 8);
		attrib.push_back(WGL_GREEN_BITS_ARB);
		attrib.push_back(flag & FLOAT ? 32 : 8);
		attrib.push_back(WGL_BLUE_BITS_ARB);
		attrib.push_back(flag & FLOAT ? 32 : 8);
		if(flag & RGBA) {
			attrib.push_back(WGL_ALPHA_BITS_ARB);
			attrib.push_back(flag & FLOAT ? 32 : 8);
		}
	}
	
	if(flag & DEPTH) {
		attrib.push_back(WGL_DEPTH_BITS_ARB);
		attrib.push_back(24);
	}
	
	if(flag & STENCIL) {
		attrib.push_back(WGL_STENCIL_BITS_ARB);
		attrib.push_back(8);
	}
	
	if(flag & FLOAT) {
		attrib.push_back(WGL_FLOAT_COMPONENTS_NV);
		attrib.push_back(true);
	}
	
	if(flag & MULTISAMPLE_2 || flag & MULTISAMPLE_4) {
		attrib.push_back(WGL_SAMPLE_BUFFERS_ARB);
		attrib.push_back(true);
		attrib.push_back(WGL_SAMPLES_ARB);
		attrib.push_back(flag & MULTISAMPLE_2 ? 2 : 4);
	}
	
	attrib.push_back(WGL_DRAW_TO_PBUFFER_ARB);
	attrib.push_back(true);
	attrib.push_back(WGL_SUPPORT_OPENGL_ARB);
	attrib.push_back(true);
	attrib.push_back(WGL_PIXEL_TYPE_ARB);
	attrib.push_back(WGL_TYPE_RGBA_ARB);
	attrib.push_back(0);
	
	HDC hdc;
	HPBUFFERARB pbuffer;
	HGLRC context;
	
	try {
		if(!wglChoosePixelFormatARB) wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
		if(!wglChoosePixelFormatARB) throw("wglGetProcAddress(\"wglChoosePixelFormatARB\") failed");
		
		if(!wglCreatePbufferARB) wglCreatePbufferARB = (PFNWGLCREATEPBUFFERARBPROC)wglGetProcAddress("wglCreatePbufferARB");
		if(!wglCreatePbufferARB) throw("wglGetProcAddress(\"wglCreatePbufferARB\") failed");
		
		if(!wglGetPbufferDCARB) wglGetPbufferDCARB = (PFNWGLGETPBUFFERDCARBPROC)wglGetProcAddress("wglGetPbufferDCARB");
		if(!wglGetPbufferDCARB) throw("wglGetProcAddress(\"wglGetPbufferDCARB\") failed");
		
		if(!wglReleasePbufferDCARB) wglReleasePbufferDCARB = (PFNWGLRELEASEPBUFFERDCARBPROC)wglGetProcAddress("wglReleasePbufferDCARB");
		if(!wglReleasePbufferDCARB) throw("wglGetProcAddress(\"wglReleasePbufferDCARB\") failed\n");
		
		if(!wglDestroyPbufferARB) wglDestroyPbufferARB = (PFNWGLDESTROYPBUFFERARBPROC)wglGetProcAddress("wglDestroyPbufferARB");
		if(!wglDestroyPbufferARB) throw("wglGetProcAddress(\"wglDestroyPbufferARB\") failed\n");
		
		int format;
		unsigned int count;
		wglChoosePixelFormatARB(old_hdc,&attrib[0],NULL,1,&format,&count);
		if(count == 0) throw("wglChoosePixelFormatARB() failed");
		
		int pattrib[] = { 0 };
		
		pbuffer = wglCreatePbufferARB(old_hdc,format,width,height,pattrib);
		if(!pbuffer) throw("wglCreatePbufferARB() failed");
		
		hdc = wglGetPbufferDCARB(pbuffer);
		if(!hdc) throw("wglGetPbufferDCARB() failed");
		
		context = wglCreateContext(hdc);
		if(!context) throw("wglCreateContext() failed");
		
		if(!wglShareLists(old_context,context)) throw("wglShareLists() failed");
	}
	catch(const char *error) {
		fprintf(stderr,"PBuffer::PBuffer: %s\n",error);
		hdc = old_hdc;
		context = old_context;
	}
	
	data = new PBuffer_data;
	data->hdc = hdc;
	data->pbuffer = pbuffer;
	data->context = context;
	
	data->old_hdc = old_hdc;
	data->old_context = old_context;
}

/*
 */
PBuffer::~PBuffer() {
	wglDeleteContext(data->context);
	wglReleasePbufferDCARB(data->pbuffer,data->hdc);
	wglDestroyPbufferARB(data->pbuffer);
	wglMakeCurrent(data->hdc,data->context);
}

/*
 */
void PBuffer::enable() {
	data->old_hdc = wglGetCurrentDC();
	data->old_context = wglGetCurrentContext();
	
	if(!wglMakeCurrent(data->hdc,data->context)) {
		fprintf(stderr,"PBuffer::disable(): wglMakeCurrent() failed\n");
	}
}

/*
 */
void PBuffer::disable() {
	if(!wglMakeCurrent(data->old_hdc,data->old_context)) {
		fprintf(stderr,"PBuffer::disable(): wglMakeCurrent() failed\n");
	}
}

#endif
