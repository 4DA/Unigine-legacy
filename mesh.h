/* Mesh
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

#ifndef __MESH_H__
#define __MESH_H__

#include <vector>
#include <stdio.h>
#include "mathlib.h"

#define MESH_STRIP_MAGIC ('m' | 's' << 8 | '0' << 16 | '2' << 24)
#define MESH_RAW_MAGIC ('m' | 'r' << 8 | '0' << 16 | '2' << 24)

class Mesh {
public:
	
	Mesh();
	Mesh(const char *name);
	Mesh(const Mesh *mesh);
	virtual ~Mesh();
	
	virtual int render(int ppl = 0,int s = -1);
	
	void findSilhouette(const vec4 &light,int s = -1);
	int getNumIntersections(const vec3 &line0,const vec3 &line1,int s = -1);
	virtual int renderShadowVolume(int s = -1);
	
	int intersection(const vec3 &line0,const vec3 &line1,vec3 &point,vec3 &normal,int s = -1);
	
	void transform(const mat4 &m,int s = -1);
	
	struct Vertex {
		vec3 xyz;
		vec3 normal;
		vec3 tangent;
		vec3 binormal;
		vec2 texcoord;
	};
	
	struct Edge {
		vec3 v[2];
		char reverse;
		char flag;
	};
	
	struct Triangle {
		vec3 v[3];			// vertexes
		int e[3];			// edges
		char reverse[3];	// edge reverse flag
		vec4 plane;			// plane
		vec4 c[3];			// fast point in triangle
	};
	
	int getNumSurfaces();
	const char *getSurfaceName(int s);
	int getSurface(const char *name);
	
	int getNumVertex(int s);
	Vertex *getVertex(int s);
	
	int getNumStrips(int s);
	int *getIndices(int s);
	
	int getNumEdges(int s);
	Edge *getEdges(int s);
	
	int getNumTriangles(int s);
	Triangle *getTriangles(int s);
	
	const vec3 &getMin(int s = -1);
	const vec3 &getMax(int s = -1);
	const vec3 &getCenter(int s = -1);
	float getRadius(int s = -1);
	
	// io
	void addSurface(Mesh *mesh,int surface);
	void addSurface(const char *name,Vertex *vertex,int num_vertex);
	
	int load(const char *name);
	int save(const char *name);
	
	// strip mesh format without header
	void load(FILE *file);
	void save(FILE *file);
	
	int load_mesh(const char *name);
	int load_3ds(const char *name);
	
	void calculate_tangent();
	void calculate_bounds();
	void create_shadow_volumes();
	void create_triangle_strips();
	
protected:
	
	vec3 min;
	vec3 max;
	vec3 center;
	float radius;
	
	struct Silhouette {
		vec4 light;
		vec4 *vertex;
		int num_vertex;
		char *flags;		// front/back triangle
	};
	
	enum {
		NUM_SURFACES = 512,
		NUM_SILHOUETTES = 4,
	};
	
	struct Surface {
		char name[128];								// surface name
		int num_vertex;								// number of vertexes
		Vertex *vertex;
		int num_edges;								// number of edges
		Edge *edges;
		int num_triangles;							// number of triangles
		Triangle *triangles;
		int num_indices;							// number of indices
		int num_strips;								// number of triangle strips
		int *indices;
		Silhouette silhouettes[NUM_SILHOUETTES];	// silhouette history
		int last_silhouette;
		Silhouette *silhouette;						// current silhouette
		vec3 min;									// bound box
		vec3 max;
		vec3 center;								// bound sphere
		float radius;
	};
	
	int num_surfaces;
	Surface *surfaces[NUM_SURFACES];
};

#endif /* __MESH_H__ */
