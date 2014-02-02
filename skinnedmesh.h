/* SkinnedMesh
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

#ifndef __SKINNED_MESH_H__
#define __SKINNED_MESH_H__

#include <vector>
#include "mathlib.h"

#define SKINNED_MESH_MAGIC ('s' | 'm' << 8 | '0' << 16 | '2' << 24)

class SkinnedMesh {
public:
	
	SkinnedMesh(const char *name);
	virtual ~SkinnedMesh();
	
	void setFrame(float frame,int from = -1,int to = -1);
	void calculateSkin();
	
	virtual int render(int ppl = 0,int s = -1);
	
	void findSilhouette(const vec4 &light,int s = -1);
	int getNumIntersections(const vec3 &line0,const vec3 &line1,int s = -1);
	virtual int renderShadowVolume(int s = -1);
	
	int intersection(const vec3 &line0,const vec3 &line1,vec3 &point,vec3 &normal,int s = -1);
	
	void transform(const mat4 &m);
	
	struct Bone {
		char name[128];
		mat4 transform;
		mat4 rotation;
		int parent;
	};
	
	struct Weight {
		int bone;
		float weight;
		vec3 xyz;
		vec3 normal;
		vec3 tangent;
		vec3 binormal;
	};
	
	struct Vertex {
		vec3 xyz;
		vec3 normal;
		vec3 tangent;
		vec3 binormal;
		vec2 texcoord;
		int num_weights;
		Weight *weights;
	};
	
	struct Edge {
		int v[2];
		char reverse;
		char flag;
	};
	
	struct Triangle {
		int v[3];			// vertexes
		int e[3];			// edges
		char reverse[3];	// edge reverse flag
		vec4 plane;			// plane
		vec4 c[3];			// fast point in triangle
		char flag;
	};
	
	int getNumSurfaces();
	const char *getSurfaceName(int s);
	int getSurface(const char *name);

	int getNumBones();
	Bone *getBones();
	const char *getBoneName(int b);
	int getBone(const char *name);
	const mat4 &getBoneTransform(int b);
	
	int getNumVertex(int s);
	Vertex *getVertex(int s);
	
	int getNumEdges(int s);
	Edge *getEdges(int s);
	
	int getNumTriangles(int s);
	Triangle *getTriangles(int s);
	
	const vec3 &getMin(int s = -1);
	const vec3 &getMax(int s = -1);
	const vec3 &getCenter(int s = -1);
	float getRadius(int s = -1);
	
	int load(const char *name);
	int save(const char *name);
	
protected:
	
	int load_binary(const char *name);
	void read_string(FILE *file,char *string);
	int load_ascii(const char *name);
	
	void calculate_tangent();
	void create_shadow_volumes();
	
	vec3 min;
	vec3 max;
	vec3 center;
	float radius;
	
	struct Frame {
		vec3 xyz;			// pivot
		quat rot;			// rotation
	};
	
	int num_bones;
	Bone *bones;
	
	int num_frames;
	Frame **frames;
	
	enum {
		NUM_SURFACES = 32,
	};
	
	struct Surface {
		char name[128];					// name
		int num_vertex;					// no comment :)
		Vertex *vertex;
		int num_edges;
		Edge *edges;
		int num_triangles;
		Triangle *triangles;
		int num_indices;
		int *indeices;
		int num_shadow_volume_vertex;	// number of last silhouette vertexes
		vec4 *shadow_volume_vertex;
		vec3 min;						// bound box
		vec3 max;
		vec3 center;					// bound sphere
		float radius;
	};
	
	int num_surfaces;
	Surface *surfaces[NUM_SURFACES];
};

#endif /* __SKINNED_MESH_H__ */
