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

#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glext.h>

#include "engine.h"
#include "skinnedmesh.h"

SkinnedMesh::SkinnedMesh(const char *name) : num_bones(0), bones(NULL), num_frames(0), frames(NULL), num_surfaces(0) {
	min = vec3(0,0,0);
	max = vec3(0,0,0);
	center = vec3(0,0,0);
	radius = 0;
	load(name);
}

SkinnedMesh::~SkinnedMesh() {
	if(bones) delete bones;
	if(frames) {
		for(int i = 0; i < num_frames; i++) delete frames[i];
		delete frames;
	}
	for(int i = 0; i < num_surfaces; i++) {
		Surface *s = surfaces[i];
		for(int j = 0; j < s->num_vertex; j++) delete s->vertex[j].weights;
		if(s->vertex) delete s->vertex;
		if(s->edges) delete s->edges;
		if(s->triangles) delete s->triangles;
		if(s->shadow_volume_vertex) delete s->shadow_volume_vertex;
		delete s;
	}
	num_surfaces = 0;
}

/*****************************************************************************/
/*                                                                           */
/* animation                                                                 */
/*                                                                           */
/*****************************************************************************/

void SkinnedMesh::setFrame(float frame,int from,int to) {
	
	if(num_frames == 0) return;
	
	if(from < 0) from = 0;
	if(to < 0) to = num_frames;
	int frame0 = (int)frame;
	frame -= frame0;
	frame0 += from;
	if(frame0 >= to) frame0 = (frame0 - from) % (to - from) + from;
	int frame1 = frame0 + 1;
	if(frame1 >= to) frame1 = from;
	
	for(int i = 0; i < num_bones; i++) {	// calculate matrixes
		mat4 translate;
		translate.translate(frames[frame0][i].xyz * (1.0f - frame) + frames[frame1][i].xyz * frame);
		quat rot;
		rot.slerp(frames[frame0][i].rot,frames[frame1][i].rot,frame);
		bones[i].rotation = rot.to_matrix();
		bones[i].transform = translate * bones[i].rotation;
	}
}

/*
 */
void SkinnedMesh::calculateSkin() {
	
	min = vec3(1000000,1000000,1000000);	// bound box
	max = vec3(-1000000,-1000000,-1000000);
	
	for(int i = 0; i < num_surfaces; i++) {	// calculate vertexes
		Surface *s = surfaces[i];
		
		s->min = vec3(1000000,1000000,1000000);
		s->max = vec3(-1000000,-1000000,-1000000);
		
		for(int j = 0; j < s->num_vertex; j++) {
			Vertex *v = &s->vertex[j];
			v->xyz = vec3(0,0,0);
			v->normal = vec3(0,0,0);
			v->tangent = vec3(0,0,0);
			v->binormal = vec3(0,0,0);
			for(int k = 0; k < v->num_weights; k++) {
				Weight *w = &v->weights[k];
				v->xyz += bones[w->bone].transform * w->xyz * w->weight;
				v->normal += bones[w->bone].rotation * w->normal * w->weight;
				v->tangent += bones[w->bone].rotation * w->tangent * w->weight;
				v->binormal += bones[w->bone].rotation * w->binormal * w->weight;
			}
			if(s->max.x < v->xyz.x) s->max.x = v->xyz.x;
			if(s->min.x > v->xyz.x) s->min.x = v->xyz.x;
			if(s->max.y < v->xyz.y) s->max.y = v->xyz.y;
			if(s->min.y > v->xyz.y) s->min.y = v->xyz.y;
			if(s->max.z < v->xyz.z) s->max.z = v->xyz.z;
			if(s->min.z > v->xyz.z) s->min.z = v->xyz.z;
		}
		
		for(int j = 0; j < s->num_triangles; j++) {
			Triangle *t = &s->triangles[j];
			vec3 normal;
			normal.cross(s->vertex[t->v[1]].xyz - s->vertex[t->v[0]].xyz,s->vertex[t->v[2]].xyz - s->vertex[t->v[0]].xyz);
			normal.normalize();
			t->plane = vec4(normal,-s->vertex[t->v[0]].xyz * normal);
			normal.cross(t->plane,s->vertex[t->v[0]].xyz - s->vertex[t->v[2]].xyz);		// fast point in triangle
			normal.normalize();
			t->c[0] = vec4(normal,-s->vertex[t->v[0]].xyz * normal);
			normal.cross(t->plane,s->vertex[t->v[1]].xyz - s->vertex[t->v[0]].xyz);
			normal.normalize();
			t->c[1] = vec4(normal,-s->vertex[t->v[1]].xyz * normal);
			normal.cross(t->plane,s->vertex[t->v[2]].xyz - s->vertex[t->v[1]].xyz);
			normal.normalize();
			t->c[2] = vec4(normal,-s->vertex[t->v[2]].xyz * normal);
			t->flag = 0;
		}

		s->center = (s->min + s->max) / 2.0f;
		s->radius = (s->max - s->center).length();
		if(max.x < s->max.x) max.x = s->max.x;
		if(min.x > s->min.x) min.x = s->min.x;
		if(max.y < s->max.y) max.y = s->max.y;
		if(min.y > s->min.y) min.y = s->min.y;
		if(max.z < s->max.z) max.z = s->max.z;
		if(min.z > s->min.z) min.z = s->min.z;
	}
	
	center = (min + max) / 2.0f;
	radius = (max - center).length();
}

/*****************************************************************************/
/*                                                                           */
/* render                                                                    */
/*                                                                           */
/*****************************************************************************/

int SkinnedMesh::render(int ppl,int s) {
	int num_triangles = 0;
	if(ppl) {
		glEnableVertexAttribArrayARB(0);
		glEnableVertexAttribArrayARB(1);
		glEnableVertexAttribArrayARB(2);
		glEnableVertexAttribArrayARB(3);
		glEnableVertexAttribArrayARB(4);
		if(s < 0) {
			for(int i = 0; i < num_surfaces; i++) {
				Surface *s = surfaces[i];
				glVertexAttribPointerARB(0,3,GL_FLOAT,0,sizeof(Vertex),s->vertex->xyz);
				glVertexAttribPointerARB(1,3,GL_FLOAT,0,sizeof(Vertex),s->vertex->normal);
				glVertexAttribPointerARB(2,3,GL_FLOAT,0,sizeof(Vertex),s->vertex->tangent);
				glVertexAttribPointerARB(3,3,GL_FLOAT,0,sizeof(Vertex),s->vertex->binormal);
				glVertexAttribPointerARB(4,2,GL_FLOAT,0,sizeof(Vertex),s->vertex->texcoord);
				glDrawElements(GL_TRIANGLES,s->num_indices,GL_UNSIGNED_INT,s->indeices);
				num_triangles += s->num_triangles;
			}
		} else {
			Surface *surface = surfaces[s];
			Surface *s = surface;
			glVertexAttribPointerARB(0,3,GL_FLOAT,0,sizeof(Vertex),s->vertex->xyz);
			glVertexAttribPointerARB(1,3,GL_FLOAT,0,sizeof(Vertex),s->vertex->normal);
			glVertexAttribPointerARB(2,3,GL_FLOAT,0,sizeof(Vertex),s->vertex->tangent);
			glVertexAttribPointerARB(3,3,GL_FLOAT,0,sizeof(Vertex),s->vertex->binormal);
			glVertexAttribPointerARB(4,2,GL_FLOAT,0,sizeof(Vertex),s->vertex->texcoord);
			glDrawElements(GL_TRIANGLES,s->num_indices,GL_UNSIGNED_INT,s->indeices);
			num_triangles += s->num_triangles;
		}
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
				Surface *s = surfaces[i];
				glVertexPointer(3,GL_FLOAT,sizeof(Vertex),s->vertex->xyz);
				glNormalPointer(GL_FLOAT,sizeof(Vertex),s->vertex->normal);
				glTexCoordPointer(2,GL_FLOAT,sizeof(Vertex),s->vertex->texcoord);
				glDrawElements(GL_TRIANGLES,s->num_indices,GL_UNSIGNED_INT,s->indeices);
				num_triangles += s->num_triangles;
			}
		} else {
			Surface *surface = surfaces[s];
			Surface *s = surface;
			glVertexPointer(3,GL_FLOAT,sizeof(Vertex),s->vertex->xyz);
			glNormalPointer(GL_FLOAT,sizeof(Vertex),s->vertex->normal);
			glTexCoordPointer(2,GL_FLOAT,sizeof(Vertex),s->vertex->texcoord);
			glDrawElements(GL_TRIANGLES,s->num_indices,GL_UNSIGNED_INT,s->indeices);
			num_triangles += s->num_triangles;
		}
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
	}
	return num_triangles;
}

/*****************************************************************************/
/*                                                                           */
/* stencil shadows                                                           */
/*                                                                           */
/*****************************************************************************/

void SkinnedMesh::findSilhouette(const vec4 &light,int s) {
	if(s < 0) {
		for(int i = 0; i < num_surfaces; i++) {
			Surface *s = surfaces[i];
			for(int j = 0; j < s->num_edges; j++) s->edges[j].flag = -1;
			for(int j = 0; j < s->num_triangles; j++) {
				Triangle *t = &s->triangles[j];
				if(t->plane * light > 0.0) {
					t->flag = 1;
					s->edges[t->e[0]].reverse = t->reverse[0];
					s->edges[t->e[1]].reverse = t->reverse[1];
					s->edges[t->e[2]].reverse = t->reverse[2];
					s->edges[t->e[0]].flag++;
					s->edges[t->e[1]].flag++;
					s->edges[t->e[2]].flag++;
				} else t->flag = 0;
			}
			s->num_shadow_volume_vertex = 0;
			vec4 *shadow_volume_vertex = s->shadow_volume_vertex;
			for(int j = 0; j < s->num_edges; j++) {
				if(s->edges[j].flag != 0) continue;
				Edge *e = &s->edges[j];
				if(e->reverse) {
					*shadow_volume_vertex++ = vec4(s->vertex[e->v[0]].xyz,1);
					*shadow_volume_vertex++ = vec4(s->vertex[e->v[1]].xyz,1);
					*shadow_volume_vertex++ = vec4(s->vertex[e->v[1]].xyz,0);
					*shadow_volume_vertex++ = vec4(s->vertex[e->v[0]].xyz,0);
				} else {
					*shadow_volume_vertex++ = vec4(s->vertex[e->v[1]].xyz,1);
					*shadow_volume_vertex++ = vec4(s->vertex[e->v[0]].xyz,1);
					*shadow_volume_vertex++ = vec4(s->vertex[e->v[0]].xyz,0);
					*shadow_volume_vertex++ = vec4(s->vertex[e->v[1]].xyz,0);
				}
				s->num_shadow_volume_vertex += 4;
			}
		}
	} else {
		Surface *surface = surfaces[s];
		Surface *s = surface;	// :)
		for(int i = 0; i < s->num_edges; i++) s->edges[i].flag = -1;
		for(int i = 0; i < s->num_triangles; i++) {
			Triangle *t = &s->triangles[i];
			if(t->plane * light > 0.0) {
				t->flag = 1;
				s->edges[t->e[0]].reverse = t->reverse[0];
				s->edges[t->e[1]].reverse = t->reverse[1];
				s->edges[t->e[2]].reverse = t->reverse[2];
				s->edges[t->e[0]].flag++;
				s->edges[t->e[1]].flag++;
				s->edges[t->e[2]].flag++;
			} else t->flag = 0;
		}
		s->num_shadow_volume_vertex = 0;
		vec4 *shadow_volume_vertex = s->shadow_volume_vertex;
		for(int i = 0; i < s->num_edges; i++) {
			if(s->edges[i].flag != 0) continue;
			Edge *e = &s->edges[i];
			if(e->reverse) {
				*shadow_volume_vertex++ = vec4(s->vertex[e->v[0]].xyz,1);
				*shadow_volume_vertex++ = vec4(s->vertex[e->v[1]].xyz,1);
				*shadow_volume_vertex++ = vec4(s->vertex[e->v[1]].xyz,0);
				*shadow_volume_vertex++ = vec4(s->vertex[e->v[0]].xyz,0);
			} else {
				*shadow_volume_vertex++ = vec4(s->vertex[e->v[1]].xyz,1);
				*shadow_volume_vertex++ = vec4(s->vertex[e->v[0]].xyz,1);
				*shadow_volume_vertex++ = vec4(s->vertex[e->v[0]].xyz,0);
				*shadow_volume_vertex++ = vec4(s->vertex[e->v[1]].xyz,0);
			}
			s->num_shadow_volume_vertex += 4;
		}
	}
}

/*
 */
int SkinnedMesh::getNumIntersections(const vec3 &line0,const vec3 &line1,int s) {
	int num_intersections = 0;
	if(s < 0) {
		vec3 dir = line1 - line0;
		float dot = (dir * center - dir * line0) / (dir * dir);
		if(dot < 0.0 && (line0 - center).length() > radius) return 0;
		else if(dot > 1.0 && (line1 - center).length() > radius) return 0;
		else if((line0 + dir * dot - center).length() > radius) return 0;
		for(int i = 0; i < num_surfaces; i++) {
			Surface *s = surfaces[i];
			float dot = (dir * s->center - dir * line0) / (dir * dir);
			if(dot < 0.0 && (line0 - s->center).length() > s->radius) continue;
			else if(dot > 1.0 && (line1 - s->center).length() > s->radius) continue;
			else if((line0 + dir * dot - s->center).length() > s->radius) continue;
			for(int j = 0; j < s->num_triangles; j++) {
				Triangle *t = &s->triangles[j];
				if(t->flag == 0) continue;
				float dot = -(t->plane * vec4(line0,1)) / (vec3(t->plane) * dir);
				if(dot < 0.0 || dot > 1.0) continue;
				vec3 p = line0 + dir * dot;
				if(p * t->c[0] > 0.0 && p * t->c[1] > 0.0 && p * t->c[2] > 0.0) num_intersections++;
			}
		}
		return num_intersections;
	} else {
		Surface *surface = surfaces[s];
		Surface *s = surface;
		vec3 dir = line1 - line0;
		float dot = (dir * s->center - dir * line0) / (dir * dir);
		if(dot < 0.0 && (line0 - s->center).length() > s->radius) return 0;
		else if(dot > 1.0 && (line1 - s->center).length() > s->radius) return 0;
		else if((line0 + dir * dot - s->center).length() > s->radius) return 0;
		for(int i = 0; i < s->num_triangles; i++) {
			Triangle *t = &s->triangles[i];
			if(t->flag == 0) continue;
			float dot = -(t->plane * vec4(line0,1)) / (vec3(t->plane) * dir);
			if(dot < 0.0 || dot > 1.0) continue;
			vec3 p = line0 + dir * dot;
			if(p * t->c[0] > 0.0 && p * t->c[1] > 0.0 && p * t->c[2] > 0.0) num_intersections++;
		}
		return num_intersections;
	}
}

/*
 */
int SkinnedMesh::renderShadowVolume(int s) {
	int num_triangles = 0;
	glEnableVertexAttribArrayARB(0);
	if(s < 0) {
		for(int i = 0; i < num_surfaces; i++) {
			Surface *s = surfaces[i];
			glVertexAttribPointerARB(0,4,GL_FLOAT,0,sizeof(vec4),s->shadow_volume_vertex);
			glDrawArrays(GL_QUADS,0,s->num_shadow_volume_vertex);
			num_triangles += s->num_shadow_volume_vertex / 2;
		}
	} else {
		Surface *surface = surfaces[s];
		Surface *s = surface;
		glVertexAttribPointerARB(0,4,GL_FLOAT,0,sizeof(vec4),s->shadow_volume_vertex);
		glDrawArrays(GL_QUADS,0,s->num_shadow_volume_vertex);
		num_triangles += s->num_shadow_volume_vertex / 2;
	}
	glDisableVertexAttribArrayARB(0);
	return num_triangles;
}

/*****************************************************************************/
/*                                                                           */
/* line inersection                                                          */
/*                                                                           */
/*****************************************************************************/

int SkinnedMesh::intersection(const vec3 &line0,const vec3 &line1,vec3 &point,vec3 &normal,int s) {
	float nearest = 2.0;
	if(s < 0) {
		vec3 dir = line1 - line0;
		float dot = (dir * center - dir * line0) / (dir * dir);
		if(dot < 0.0 && (line0 - center).length() > radius) return 0;
		else if(dot > 1.0 && (line1 - center).length() > radius) return 0;
		else if((line0 + dir * dot - center).length() > radius) return 0;
		for(int i = 0; i < num_surfaces; i++) {
			Surface *s = surfaces[i];
			float dot = (dir * s->center - dir * line0) / (dir * dir);
			if(dot < 0.0 && (line0 - s->center).length() > s->radius) continue;
			else if(dot > 1.0 && (line1 - s->center).length() > s->radius) continue;
			else if((line0 + dir * dot - s->center).length() > s->radius) continue;
			for(int j = 0; j < s->num_triangles; j++) {
				Triangle *t = &s->triangles[j];
				float dot = -(t->plane * vec4(line0,1)) / (vec3(t->plane) * dir);
				if(dot < 0.0 || dot > 1.0) continue;
				vec3 p = line0 + dir * dot;
				if(nearest > dot && p * t->c[0] > 0.0 && p * t->c[1] > 0.0 && p * t->c[2] > 0.0) {
					nearest = dot;
					point = p;
					normal = t->plane;
				}
			}
		}
		return nearest < 2.0 ? 1 : 0;
	} else {
		Surface *surface = surfaces[s];
		Surface *s = surface;
		vec3 dir = line1 - line0;
		float dot = (dir * s->center - dir * line0) / (dir * dir);
		if(dot < 0.0 && (line0 - s->center).length() > s->radius) return 0;
		else if(dot > 1.0 && (line1 - s->center).length() > s->radius) return 0;
		else if((line0 + dir * dot - s->center).length() > s->radius) return 0;
		for(int i = 0; i < s->num_triangles; i++) {
			Triangle *t = &s->triangles[i];
			float dot = -(t->plane * vec4(line0,1)) / (vec3(t->plane) * dir);
			if(dot < 0.0 || dot > 1.0) continue;
			vec3 p = line0 + dir * dot;
			if(nearest > dot && p * t->c[0] > 0.0 && p * t->c[1] > 0.0 && p * t->c[2] > 0.0) {
				nearest = dot;
				point = p;
				normal = t->plane;
			}
		}
		return nearest < 2.0 ? 1 : 0;
	}
}

/*****************************************************************************/
/*                                                                           */
/* transform                                                                 */
/*                                                                           */
/*****************************************************************************/

void SkinnedMesh::transform(const mat4 &m) {
	mat4 r = m.rotation();
	for(int i = 0; i < num_surfaces; i++) {
		Surface *s = surfaces[i];
		for(int j = 0; j < s->num_vertex; j++) {
			Vertex *v = &s->vertex[j];
			for(int k = 0; k < v->num_weights; k++) {
				Weight *w = &v->weights[k];
				w->xyz = m * w->xyz;
				w->normal = r * w->normal;
				w->normal.normalize();
			}
		}
	}
	for(int i = 0; i < num_frames; i++) {
		for(int j = 0; j < num_bones; j++) {
			frames[i][j].xyz = m * frames[i][j].xyz;
		}
	}
	calculate_tangent();
	setFrame(0);
	calculateSkin();
}

/*****************************************************************************/
/*                                                                           */
/* IO functions                                                              */
/*                                                                           */
/*****************************************************************************/

/*
 */
int SkinnedMesh::getNumSurfaces() {
	return num_surfaces;
}

const char *SkinnedMesh::getSurfaceName(int s) {
	return surfaces[s]->name;
}

int SkinnedMesh::getSurface(const char *name) {
	for(int i = 0; i < num_surfaces; i++) if(!strcmp(name,surfaces[i]->name)) return i;
	fprintf(stderr,"SkinnedMesh::getSurface(): can`t find \"%s\" surface\n",name);
	return -1;
}

/*
 */
int SkinnedMesh::getNumBones() {
	return num_bones;
}

SkinnedMesh::Bone *SkinnedMesh::getBones() {
	return bones;
}

const char *SkinnedMesh::getBoneName(int b) {
	return bones[b].name;
}

int SkinnedMesh::getBone(const char *name) {
	for(int i = 0; i < num_bones; i++) if(!strcmp(name,bones[i].name)) return i;
	fprintf(stderr,"SkinnedMesh::getBone(): can`t find \"%s\" bone\n",name);
	return -1;
}

const mat4 &SkinnedMesh::getBoneTransform(int b) {
	if(b < 0) {
		static mat4 dummy;
		return dummy;
	}
	return bones[b].transform;
}

/*
 */
int SkinnedMesh::getNumVertex(int s) {
	return surfaces[s]->num_vertex;
}

SkinnedMesh::Vertex *SkinnedMesh::getVertex(int s) {
	return surfaces[s]->vertex;
}

int SkinnedMesh::getNumEdges(int s) {
	return surfaces[s]->num_edges;
}

SkinnedMesh::Edge *SkinnedMesh::getEdges(int s) {
	return surfaces[s]->edges;
}

int SkinnedMesh::getNumTriangles(int s) {
	return surfaces[s]->num_triangles;
}

SkinnedMesh::Triangle *SkinnedMesh::getTriangles(int s) {
	return surfaces[s]->triangles;
}

/*
 */
const vec3 &SkinnedMesh::getMin(int s) {
	if(s < 0) return min;
	return surfaces[s]->min;
}

const vec3 &SkinnedMesh::getMax(int s) {
	if(s < 0) return max;
	return surfaces[s]->max;
}

const vec3 &SkinnedMesh::getCenter(int s) {
	if(s < 0) return center;
	return surfaces[s]->center;
}

float SkinnedMesh::getRadius(int s) {
	if(s < 0) return radius;
	return surfaces[s]->radius;
}

/* load
 */
int SkinnedMesh::load(const char *name) {
	FILE *file = fopen(name,"rb");
	if(!file) {
		fprintf(stderr,"SkinnedMesh::load(): error open \"%s\" file\n",name);
		return 0;
	}
	int magic = 0;
	fread(&magic,sizeof(int),1,file);
	fclose(file);
	int ret = 0;
	if(magic == SKINNED_MESH_MAGIC) ret = load_binary(name);
	else ret = load_ascii(name);
	calculate_tangent();
	create_shadow_volumes();
	setFrame(0);
	calculateSkin();
	return ret;
}

/* save
 */
int SkinnedMesh::save(const char *name) {
	FILE *file = fopen(name,"wb");
	if(!file) {
		fprintf(stderr,"SkinnedMesh::save(): error create \"%s\" file\n",name);
		return 0;
	}
	int magic = SKINNED_MESH_MAGIC;
	fwrite(&magic,sizeof(int),1,file);
	
	fwrite(&num_bones,sizeof(int),1,file);	// bones
	for(int i = 0; i < num_bones; i++) {
		fwrite(bones[i].name,sizeof(bones[i].name),1,file);
		fwrite(&bones[i].parent,sizeof(int),1,file);
	}

	fwrite(&num_surfaces,sizeof(int),1,file);	// surfaces
	for(int i = 0; i < num_surfaces; i++) {
		Surface *s = surfaces[i];
		fwrite(s->name,sizeof(s->name),1,file);	// name

		fwrite(&s->num_vertex,sizeof(int),1,file);	// vertexes
		for(int j = 0; j < s->num_vertex; j++) {
			Vertex *v = &s->vertex[j];
			fwrite(&v->texcoord,sizeof(vec2),1,file);
			fwrite(&v->num_weights,sizeof(int),1,file);	// weights
			for(int k = 0; k < v->num_weights; k++) {
				Weight *w = &v->weights[k];
				fwrite(&w->bone,sizeof(int),1,file);
				fwrite(&w->weight,sizeof(float),1,file);
				fwrite(&w->xyz,sizeof(vec3),1,file);
				fwrite(&w->normal,sizeof(vec3),1,file);
			}
		}

		fwrite(&s->num_triangles,sizeof(int),1,file);	// triangles
		for(int j = 0; j < s->num_triangles; j++) fwrite(s->triangles[j].v,sizeof(int),3,file);
	}

	fwrite(&num_frames,sizeof(int),1,file);	// frames
	for(int i = 0; i < num_frames; i++) fwrite(frames[i],sizeof(Frame),num_bones,file);

	fclose(file);
	return 1;
}

/*****************************************************************************/
/*                                                                           */
/* binary skinned mesh loader                                                */
/*                                                                           */
/*****************************************************************************/

int SkinnedMesh::load_binary(const char *name) {
	FILE *file = fopen(name,"rb");
	if(!file) {
		fprintf(stderr,"SkinnedMesh::load_binary(): error open \"%s\" file\n",name);
		return 0;
	}
	
	int magic;	// magic
	fread(&magic,sizeof(int),1,file);
	if(magic != SKINNED_MESH_MAGIC) {
		fprintf(stderr,"SkinnedMesh::load_binary(): wrong magic 0x%04x in \"%s\" file",magic,name);
		fclose(file);
		return 0;
	}
	
	fread(&num_bones,sizeof(int),1,file);	// bones
	bones = new Bone[num_bones];
	for(int i = 0; i < num_bones; i++) {
		fread(bones[i].name,sizeof(bones[i].name),1,file);
		fread(&bones[i].parent,sizeof(int),1,file);
	}
	
	int num_surfaces;	// surfaces
	fread(&num_surfaces,sizeof(int),1,file);
	for(int i = 0; i < num_surfaces; i++) {
		Surface *s = new Surface;
		memset(s,0,sizeof(Surface));
		fread(s->name,sizeof(s->name),1,file);	// name
		
		fread(&s->num_vertex,sizeof(int),1,file);	// vertexes
		s->vertex = new Vertex[s->num_vertex];
		for(int j = 0; j < s->num_vertex; j++) {
			Vertex *v = &s->vertex[j];
			fread(&v->texcoord,sizeof(vec2),1,file);
			fread(&v->num_weights,sizeof(int),1,file);	// weights
			v->weights = new Weight[v->num_weights];
			for(int k = 0; k < v->num_weights; k++) {
				Weight *w = &v->weights[k];
				fread(&w->bone,sizeof(int),1,file);
				fread(&w->weight,sizeof(float),1,file);
				fread(&w->xyz,sizeof(vec3),1,file);
				fread(&w->normal,sizeof(vec3),1,file);
			}
		}
		
		fread(&s->num_triangles,sizeof(int),1,file);	// triangles
		s->triangles = new Triangle[s->num_triangles];
		s->num_indices = s->num_triangles * 3;
		s->indeices = new int[s->num_indices];
		for(int j = 0; j < s->num_triangles; j++) {
			Triangle *t = &s->triangles[j];
			fread(t->v,sizeof(int),3,file);
			s->indeices[j * 3 + 0] = t->v[0];
			s->indeices[j * 3 + 1] = t->v[1];
			s->indeices[j * 3 + 2] = t->v[2];
		}
		if(this->num_surfaces == NUM_SURFACES) {
			fprintf(stderr,"SkinnedMesh::load_binary(): many surfaces\n");
			this->num_surfaces--;
		}
		surfaces[this->num_surfaces++] = s;
	}

	fread(&num_frames,sizeof(int),1,file);	// frames
	frames = new Frame*[num_frames];
	for(int i = 0; i < num_frames; i++) {
		frames[i] = new Frame[num_bones];
		fread(frames[i],sizeof(Frame),num_bones,file);
	}

	fclose(file);
	return 1;
}

/*****************************************************************************/
/*                                                                           */
/* ascii skinned mesh loader                                                 */
/*                                                                           */
/*****************************************************************************/

void SkinnedMesh::read_string(FILE *file,char *string) {
	char c;
	char *s = string;
	int quoted = -1;
	while(fread(&c,sizeof(char),1,file) == 1) {
		if(quoted == -1) {
			if(c == '"') quoted = 1;
			else if(!strchr(" \t\n\r",c)) {
				*s++ = c;
				quoted = 0;
			}
			continue;
		}
		if(quoted == 1 && c == '"') break;
		if(quoted == 0 && strchr(" \t\n\r",c)) break;
		*s++ = c;
	}
	*s = '\0';
}

/*
 */
int SkinnedMesh::load_ascii(const char *name) {
	FILE *file = fopen(name,"r");
	if(!file) {
		fprintf(stderr,"SkinnedMesh::load_ascii(): error open \"%s\" file\n",name);
		return 0;
	}
	char buf[1024];
	while(fscanf(file,"%s",buf) != EOF) {
		if(!strcmp(buf,"bones")) {
			fscanf(file,"%d %s",&num_bones,buf);
			bones = new Bone[num_bones];
			for(int i = 0; i < num_bones; i++) {
				memset(bones[i].name,0,sizeof(bones[i].name));
				read_string(file,bones[i].name);
				fscanf(file,"%d",&bones[i].parent);
			}
			fscanf(file,"%s",buf);
		}
		else if(!strcmp(buf,"surface")) {
			Surface *s = new Surface;
			memset(s,0,sizeof(Surface));
			memset(s->name,0,sizeof(s->name));
			read_string(file,s->name);
			fscanf(file,"%s",buf);
			while(fscanf(file,"%s",buf) != EOF) {
				if(!strcmp(buf,"vertex")) {
					fscanf(file,"%d %s",&s->num_vertex,buf);
					s->vertex = new Vertex[s->num_vertex];
					for(int i = 0; i < s->num_vertex; i++) {
						Vertex *v = &s->vertex[i];
						fscanf(file,"%f %f",&v->texcoord.x,&v->texcoord.y);
						fscanf(file,"%s %d",buf,&v->num_weights);
						fscanf(file,"%s",buf);
						v->weights = new Weight[v->num_weights];
						for(int j = 0; j < v->num_weights; j++) {
							Weight *w = &v->weights[j];
							fscanf(file,"%d %f",&w->bone,&w->weight);
							fscanf(file,"%f %f %f",&w->xyz.x,&w->xyz.y,&w->xyz.z);
							fscanf(file,"%f %f %f",&w->normal.x,&w->normal.y,&w->normal.z);
						}
						fscanf(file,"%s",buf);
					}
					fscanf(file,"%s",buf);
				}
				else if(!strcmp(buf,"triangles")) {
					fscanf(file,"%d %s",&s->num_triangles,buf);
					s->triangles = new Triangle[s->num_triangles];
					s->num_indices = s->num_triangles * 3;
					s->indeices = new int[s->num_indices];
					for(int i = 0; i < s->num_triangles; i++) {
						Triangle *t = &s->triangles[i];
						fscanf(file,"%d %d %d",&t->v[0],&t->v[1],&t->v[2]);
						s->indeices[i * 3 + 0] = t->v[0];
						s->indeices[i * 3 + 1] = t->v[1];
						s->indeices[i * 3 + 2] = t->v[2];
					}
					fscanf(file,"%s",buf);
				}
				else if(!strcmp(buf,"}")) {
					break;
				}
				else {
					fprintf(stderr,"SkinnedMesh::load_ascii(): unknown token \"%s\"\n",buf);
					fclose(file);
					return 0;
				}
			}
			if(num_surfaces == NUM_SURFACES) {
				fprintf(stderr,"SkinnedMesh::load_ascii(): many surfaces\n");
				num_surfaces--;
			}
			surfaces[num_surfaces++] = s;
		}
		else if(!strcmp(buf,"animation")) {
			fscanf(file,"%d %s",&num_frames,buf);
			frames = new Frame*[num_frames];
			for(int i = 0; i < num_frames; i++) {
				frames[i] = new Frame[num_bones];
				for(int j = 0; j < num_bones; j++) {
					Frame *f = &frames[i][j];
					fscanf(file,"%f %f %f",&f->xyz.x,&f->xyz.y,&f->xyz.z);
					fscanf(file,"%f %f %f %f",&f->rot.x,&f->rot.y,&f->rot.z,&f->rot.w);
				}
			}
			fscanf(file,"%s",buf);
		}
		else {
			fprintf(stderr,"SkinnedMesh::load_ascii(): unknown token \"%s\"\n",buf);
			fclose(file);
			return 0;
		}
	}
	fclose(file);
	return 1;
}

/*****************************************************************************/
/*                                                                           */
/* calculate tangent                                                         */
/*                                                                           */
/*****************************************************************************/

void SkinnedMesh::calculate_tangent() {
	setFrame(0.0);
	calculateSkin();
	
	for(int i = 0; i < num_surfaces; i++) {
		Surface *s = surfaces[i];
		
		for(int j = 0; j < s->num_vertex; j++) {	// set tangents and binormals to zero
			Vertex *v = &s->vertex[j];
			for(int k = 0; k < v->num_weights; k++) {
				Weight *w = &v->weights[k];
				w->tangent = vec3(0,0,0);
				w->binormal = vec3(0,0,0);
			}
		}
		
		for(int j = 0; j < s->num_triangles; j++) {
			Triangle *t = &s->triangles[j];
			
			Vertex *v0 = &s->vertex[t->v[0]];
			Vertex *v1 = &s->vertex[t->v[1]];
			Vertex *v2 = &s->vertex[t->v[2]];
			
			vec3 normal,tangent,binormal;	// calculate tangent and bonormal for triangle
			vec3 e0 = vec3(0,v1->texcoord.x - v0->texcoord.x,v1->texcoord.y - v0->texcoord.y);
			vec3 e1 = vec3(0,v2->texcoord.x - v0->texcoord.x,v2->texcoord.y - v0->texcoord.y);
			for(int k = 0; k < 3; k++) {
				e0.x = v1->xyz[k] - v0->xyz[k];
				e1.x = v2->xyz[k] - v0->xyz[k];
				vec3 v;
				v.cross(e0,e1);
				if(fabs(v[0]) > EPSILON) {
					tangent[k] = -v[1] / v[0];
					binormal[k] = -v[2] / v[0];
				} else {
					tangent[k] = 0;
					binormal[k] = 0;
				}
			}
			tangent.normalize();
			binormal.normalize();
			normal.cross(tangent,binormal);
			normal.normalize();
			
			v0->binormal.cross(v0->normal,tangent);
			v0->binormal.normalize();
			v0->tangent.cross(v0->binormal,v0->normal);
			if(normal * v0->normal < 0) v0->binormal = -v0->binormal;
			
			for(int k = 0; k < v0->num_weights; k++) {	// add it in to vertex weights
				Weight *w = &v0->weights[k];
				mat4 transform = bones[w->bone].rotation.inverse();
				w->tangent += transform * v0->tangent;
				w->binormal += transform * v0->binormal;
			}
			
			v1->binormal.cross(v1->normal,tangent);
			v1->binormal.normalize();
			v1->tangent.cross(v1->binormal,v1->normal);
			if(normal * v1->normal < 0) v1->binormal = -v1->binormal;
			
			for(int k = 0; k < v1->num_weights; k++) {
				Weight *w = &v1->weights[k];
				mat4 transform = bones[w->bone].rotation.inverse();
				w->tangent += transform * v1->tangent;
				w->binormal += transform * v1->binormal;
			}
			
			v2->binormal.cross(v2->normal,tangent);
			v2->binormal.normalize();
			v2->tangent.cross(v2->binormal,v2->normal);
			if(normal * v2->normal < 0) v2->binormal = -v2->binormal;
			
			for(int k = 0; k < v2->num_weights; k++) {
				Weight *w = &v2->weights[k];
				mat4 transform = bones[w->bone].rotation.inverse();
				w->tangent += transform * v2->tangent;
				w->binormal += transform * v2->binormal;
			}
		}

		for(int j = 0; j < s->num_vertex; j++) {	// normalize tangents and binormals
			Vertex *v = &s->vertex[j];
			for(int k = 0; k < v->num_weights; k++) {
				Weight *w = &v->weights[k];
				w->tangent.normalize();
				w->binormal.normalize();
			}
		}
	}
}

/*****************************************************************************/
/*                                                                           */
/* create shadow volume                                                      */
/*                                                                           */
/*****************************************************************************/

struct csv_Edge {
	int v[2];
	int id;
};

static int csv_edge_cmp(const void *a,const void *b) {
	csv_Edge *e0 = (csv_Edge*)a;
	csv_Edge *e1 = (csv_Edge*)b;
	int v[2][2];
	if(e0->v[0] < e0->v[1]) { v[0][0] = e0->v[0]; v[0][1] = e0->v[1]; }
	else { v[0][0] = e0->v[1]; v[0][1] = e0->v[0]; }
	if(e1->v[0] < e1->v[1]) { v[1][0] = e1->v[0]; v[1][1] = e1->v[1]; }
	else { v[1][0] = e1->v[1]; v[1][1] = e1->v[0]; }
	if(v[0][0] > v[1][0]) return 1;
	if(v[0][0] < v[1][0]) return -1;
	if(v[0][1] > v[1][1]) return 1;
	if(v[0][1] < v[1][1]) return -1;
	return 0;
}

void SkinnedMesh::create_shadow_volumes() {
	for(int i = 0; i < num_surfaces; i++) {
		Surface *s = surfaces[i];
		s->num_edges = s->num_triangles * 3;
		
		csv_Edge *e = new csv_Edge[s->num_edges];	// create temporary edges
		for(int j = 0, k = 0; j < s->num_edges; j += 3, k++) {
			e[j + 0].v[0] = s->triangles[k].v[0];
			e[j + 0].v[1] = s->triangles[k].v[1];
			e[j + 1].v[0] = s->triangles[k].v[1];
			e[j + 1].v[1] = s->triangles[k].v[2];
			e[j + 2].v[0] = s->triangles[k].v[2];
			e[j + 2].v[1] = s->triangles[k].v[0];
			e[j + 0].id = j + 0;
			e[j + 1].id = j + 1;
			e[j + 2].id = j + 2;
		}
		qsort(e,s->num_edges,sizeof(csv_Edge),csv_edge_cmp);	// optimize it
		int num_optimized_edges = 0;
		int *ebuf = new int[s->num_edges];
		int *rbuf = new int[s->num_edges];
		for(int j = 0; j < s->num_edges; j++) {
			if(j == 0 || csv_edge_cmp(&e[num_optimized_edges - 1],&e[j])) e[num_optimized_edges++] = e[j];
			ebuf[e[j].id] = num_optimized_edges - 1;
			rbuf[e[j].id] = e[num_optimized_edges - 1].v[0] != e[j].v[0];
		}
		
		s->num_edges = num_optimized_edges;
		s->edges = new Edge[s->num_edges];
		for(int j = 0; j < s->num_edges; j++) {
			s->edges[j].v[0] = e[j].v[0];
			s->edges[j].v[1] = e[j].v[1];
			s->edges[j].reverse = 0;
			s->edges[j].flag = 0;
		}
		for(int j = 0, k = 0; j < s->num_triangles; j++, k += 3) {
			Triangle *t = &s->triangles[j];
			t->e[0] = ebuf[k + 0];
			t->e[1] = ebuf[k + 1];
			t->e[2] = ebuf[k + 2];
			t->reverse[0] = rbuf[k + 0];
			t->reverse[1] = rbuf[k + 1];
			t->reverse[2] = rbuf[k + 2];
		}
		s->shadow_volume_vertex = new vec4[s->num_edges * 4];
		delete rbuf;
		delete ebuf;
		delete e;
	}
}
