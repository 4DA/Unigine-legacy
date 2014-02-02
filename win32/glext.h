/* OpenGL extensions
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

#ifndef __GLEXT_H__
#define __GLEXT_H__

#ifdef _WIN32
#include <windows.h>
#include <GL/gl.h>
#include <GL/glext.h>

extern PFNGLACTIVESTENCILFACEEXTPROC glActiveStencilFaceEXT;

extern PFNGLACTIVETEXTUREARBPROC glActiveTexture;
extern PFNGLTEXIMAGE3DPROC glTexImage3D;

extern PFNGLGENPROGRAMSARBPROC glGenProgramsARB;
extern PFNGLBINDPROGRAMARBPROC glBindProgramARB;
extern PFNGLDELETEPROGRAMSARBPROC glDeleteProgramsARB;
extern PFNGLPROGRAMSTRINGARBPROC glProgramStringARB;
extern PFNGLPROGRAMENVPARAMETER4FVARBPROC glProgramEnvParameter4fvARB;
extern PFNGLPROGRAMLOCALPARAMETER4FVARBPROC glProgramLocalParameter4fvARB;
extern PFNGLENABLEVERTEXATTRIBARRAYARBPROC glEnableVertexAttribArrayARB;
extern PFNGLVERTEXATTRIBPOINTERARBPROC glVertexAttribPointerARB;
extern PFNGLDISABLEVERTEXATTRIBARRAYARBPROC glDisableVertexAttribArrayARB;
extern PFNGLVERTEXATTRIB4FVARBPROC glVertexAttrib4fvARB;

extern PFNGLGENPROGRAMSNVPROC glGenProgramsNV;
extern PFNGLBINDPROGRAMNVPROC glBindProgramNV;
extern PFNGLDELETEPROGRAMSNVPROC glDeleteProgramsNV;
extern PFNGLLOADPROGRAMNVPROC glLoadProgramNV;
extern PFNGLPROGRAMNAMEDPARAMETER4FVNVPROC glProgramNamedParameter4fvNV;

extern PFNGLGENBUFFERSARBPROC glGenBuffersARB;
extern PFNGLBINDBUFFERARBPROC glBindBufferARB;
extern PFNGLBUFFERDATAARBPROC glBufferDataARB;
extern PFNGLDELETEBUFFERSARBPROC glDeleteBuffersARB;

extern PFNGLGENQUERIESARBPROC glGenQueriesARB;
extern PFNGLDELETEQUERIESARBPROC glDeleteQueriesARB;
extern PFNGLBEGINQUERYARBPROC glBeginQueryARB;
extern PFNGLENDQUERYARBPROC glEndQueryARB;
extern PFNGLGETQUERYOBJECTUIVARBPROC glGetQueryObjectuivARB;

void glext_init();

#endif

#endif /* __GLEXT_H__ */
