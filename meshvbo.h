/* MeshVBO (vertex buffer object)
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

#ifndef __MESH_VBO_H__
#define __MESH_VBO_H__

#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glext.h>

#include "mesh.h"

class MeshVBO : public Mesh {
public:

	MeshVBO(const char *name);
	MeshVBO(const Mesh *mesh);
	virtual ~MeshVBO();
	
	virtual int render(int ppl = 0,int s = -1);
	virtual int renderShadowVolume(int s = -1);
	
protected:
	std::vector<GLuint> vbo_id;
};

#endif /* __MESH_VBO_H__ */
