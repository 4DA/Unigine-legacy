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

#include "engine.h"
#include "meshvbo.h"

MeshVBO::MeshVBO(const char *name) : Mesh(name) {
	for(int i = 0; i < getNumSurfaces(); i++) {
		GLuint id;
		glGenBuffersARB(1,&id);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,id);
		glBufferDataARB(GL_ARRAY_BUFFER_ARB,sizeof(Vertex) * getNumVertex(i),getVertex(i),GL_STATIC_DRAW_ARB);
		vbo_id.push_back(id);
	}
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
}

MeshVBO::MeshVBO(const Mesh *mesh) : Mesh(mesh) {
	for(int i = 0; i < getNumSurfaces(); i++) {
		GLuint id;
		glGenBuffersARB(1,&id);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,id);
		glBufferDataARB(GL_ARRAY_BUFFER_ARB,sizeof(Vertex) * getNumVertex(i),getVertex(i),GL_STATIC_DRAW_ARB);
		vbo_id.push_back(id);
	}
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
}

MeshVBO::~MeshVBO() {
	for(int i = 0; i < getNumSurfaces(); i++) glDeleteBuffersARB(1,&vbo_id[i]);
	vbo_id.clear();
}

/*
 */
int MeshVBO::render(int ppl,int s) {
	int num_triangles = 0;
	if(ppl) {
		glEnableVertexAttribArrayARB(0);
		glEnableVertexAttribArrayARB(1);
		glEnableVertexAttribArrayARB(2);
		glEnableVertexAttribArrayARB(3);
		glEnableVertexAttribArrayARB(4);
		if(s < 0) {
			for(int i = 0; i < num_surfaces; i++) {
				glBindBufferARB(GL_ARRAY_BUFFER_ARB,vbo_id[i]);
				glVertexAttribPointerARB(0,3,GL_FLOAT,0,sizeof(Vertex),0);
				glVertexAttribPointerARB(1,3,GL_FLOAT,0,sizeof(Vertex),(void*)(sizeof(vec3) * 1));
				glVertexAttribPointerARB(2,3,GL_FLOAT,0,sizeof(Vertex),(void*)(sizeof(vec3) * 2));
				glVertexAttribPointerARB(3,3,GL_FLOAT,0,sizeof(Vertex),(void*)(sizeof(vec3) * 3));
				glVertexAttribPointerARB(4,2,GL_FLOAT,0,sizeof(Vertex),(void*)(sizeof(vec3) * 4));
				int *indices = surfaces[i]->indices;
				for(int j = 0; j < surfaces[i]->num_strips; j++) {
					glDrawElements(GL_TRIANGLE_STRIP,indices[0],GL_UNSIGNED_INT,indices + 1);
					indices += indices[0] + 1;
				}
				num_triangles += surfaces[i]->num_triangles;
			}
		} else {
			glBindBufferARB(GL_ARRAY_BUFFER_ARB,vbo_id[s]);
			glVertexAttribPointerARB(0,3,GL_FLOAT,0,sizeof(Vertex),0);
			glVertexAttribPointerARB(1,3,GL_FLOAT,0,sizeof(Vertex),(void*)(sizeof(vec3) * 1));
			glVertexAttribPointerARB(2,3,GL_FLOAT,0,sizeof(Vertex),(void*)(sizeof(vec3) * 2));
			glVertexAttribPointerARB(3,3,GL_FLOAT,0,sizeof(Vertex),(void*)(sizeof(vec3) * 3));
			glVertexAttribPointerARB(4,2,GL_FLOAT,0,sizeof(Vertex),(void*)(sizeof(vec3) * 4));
			int *indices = surfaces[s]->indices;
			for(int i = 0; i < surfaces[s]->num_strips; i++) {
				glDrawElements(GL_TRIANGLE_STRIP,indices[0],GL_UNSIGNED_INT,indices + 1);
				indices += indices[0] + 1;
			}
			num_triangles += surfaces[s]->num_triangles;
		}
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
		glDisableVertexAttribArrayARB(4);
		glDisableVertexAttribArrayARB(3);
		glDisableVertexAttribArrayARB(2);
		glDisableVertexAttribArrayARB(1);
		glDisableVertexAttribArrayARB(0);
	} else {
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_NORMAL_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		if(s < 0) {
			for(int i = 0; i < num_surfaces; i++) {
				glBindBufferARB(GL_ARRAY_BUFFER_ARB,vbo_id[i]);
				glVertexPointer(3,GL_FLOAT,sizeof(Vertex),0);
				glNormalPointer(GL_FLOAT,sizeof(Vertex),(void*)(sizeof(vec3) * 1));
				glTexCoordPointer(2,GL_FLOAT,sizeof(Vertex),(void*)(sizeof(vec3) * 4));
				int *indices = surfaces[i]->indices;
				for(int j = 0; j < surfaces[i]->num_strips; j++) {
					glDrawElements(GL_TRIANGLE_STRIP,indices[0],GL_UNSIGNED_INT,indices + 1);
					indices += indices[0] + 1;
				}
				num_triangles += getNumTriangles(i);
			}
		} else {
			glBindBufferARB(GL_ARRAY_BUFFER_ARB,vbo_id[s]);
			glVertexPointer(3,GL_FLOAT,sizeof(Vertex),0);
			glNormalPointer(GL_FLOAT,sizeof(Vertex),(void*)(sizeof(vec3) * 1));
			glTexCoordPointer(2,GL_FLOAT,sizeof(Vertex),(void*)(sizeof(vec3) * 4));
			int *indices = surfaces[s]->indices;
			for(int i = 0; i < surfaces[s]->num_strips; i++) {
				glDrawElements(GL_TRIANGLE_STRIP,indices[0],GL_UNSIGNED_INT,indices + 1);
				indices += indices[0] + 1;
			}
			num_triangles += getNumTriangles(s);
		}
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
	}
	return num_triangles;
}

/*
 */
int MeshVBO::renderShadowVolume(int s) {
	int num_triangles = 0;
	glEnableVertexAttribArrayARB(0);
	if(s < 0) {
		for(int i = 0; i < num_surfaces; i++) {
			Surface *s = surfaces[i];
			if(!s->silhouette) continue;
			glVertexAttribPointerARB(0,4,GL_FLOAT,0,sizeof(vec4),s->silhouette->vertex);
			glDrawArrays(GL_QUADS,0,s->silhouette->num_vertex);
			num_triangles += s->silhouette->num_vertex / 2;
		}
	} else {
		Surface *surface = surfaces[s];
		Surface *s = surface;
		if(!s->silhouette) return 0;
		glVertexAttribPointerARB(0,4,GL_FLOAT,0,sizeof(vec4),s->silhouette->vertex);
		glDrawArrays(GL_QUADS,0,s->silhouette->num_vertex);
		num_triangles += s->silhouette->num_vertex / 2;
	}
	glDisableVertexAttribArrayARB(0);
	return num_triangles;
}
