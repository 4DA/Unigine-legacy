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

#include "mesh.h"

Mesh::Mesh() : num_surfaces(0) {
	min = vec3(1000000,1000000,1000000);
	max = vec3(-1000000,-1000000,-1000000);
	center = vec3(0,0,0);
	radius = 1000000;
}

Mesh::Mesh(const char *name) : num_surfaces(0) {
	load(name);
}

Mesh::Mesh(const Mesh *mesh) : num_surfaces(0) {
	min = mesh->min;
	max = mesh->max;
	center = mesh->center;
	radius = mesh->radius;
	for(int i = 0; i < mesh->num_surfaces; i++) {
		Surface *s = new Surface;
		memcpy(s,mesh->surfaces[i],sizeof(Surface));
		s->vertex = new Vertex[s->num_vertex];
		memcpy(s->vertex,mesh->surfaces[i]->vertex,sizeof(Vertex) * s->num_vertex);
		if(mesh->surfaces[i]->edges) {
			s->edges = new Edge[s->num_edges];
			memcpy(s->edges,mesh->surfaces[i]->edges,sizeof(Edge) * s->num_edges);
		}
		if(mesh->surfaces[i]->triangles) {
			s->triangles = new Triangle[s->num_triangles];
			memcpy(s->triangles,mesh->surfaces[i]->triangles,sizeof(Triangle) * s->num_triangles);
		}
		if(mesh->surfaces[i]->indices) {
			s->indices = new int[s->num_indices];
			memcpy(s->indices,mesh->surfaces[i]->indices,sizeof(int) * s->num_indices);
		}
		if(mesh->surfaces[i]->silhouettes[0].vertex) {
			for(int j = 0; j < NUM_SILHOUETTES; j++) {
				s->silhouettes[j].vertex = new vec4[s->num_edges * 4];
				s->silhouettes[j].flags = new char[s->num_triangles];
			}
		}
		if(num_surfaces == NUM_SURFACES) {
			fprintf(stderr,"Mesh::Mesh(): many surfaces\n");
			num_surfaces--;
		}
		surfaces[num_surfaces++] = s;
	}
}

Mesh::~Mesh() {
	for(int i = 0; i < num_surfaces; i++) {
		Surface *s = surfaces[i];
		if(s->vertex) delete s->vertex;
		if(s->indices) delete s->indices;
		if(s->edges) delete s->edges;
		if(s->triangles) delete s->triangles;
		if(s->silhouettes[0].vertex) {
			for(int j = 0; j < NUM_SILHOUETTES; j++) {
				delete s->silhouettes[j].vertex;
				delete s->silhouettes[j].flags;
			}
		}
		delete s;
	}
	num_surfaces = 0;
}

/*****************************************************************************/
/*                                                                           */
/* render                                                                    */
/*                                                                           */
/*****************************************************************************/

/*
 */
int Mesh::render(int,int) {
	fprintf(stderr,"Mesh::render()\n");
	return 0;
}

/*****************************************************************************/
/*                                                                           */
/* stencil shadows                                                           */
/*                                                                           */
/*****************************************************************************/

/*
 */
void Mesh::findSilhouette(const vec4 &light,int s) {
	if(s < 0) {
		for(int i = 0; i < num_surfaces; i++) {
			Surface *s = surfaces[i];
			for(int j = 0; j < NUM_SILHOUETTES; j++) {
				if(s->silhouettes[j].light == light) {
					s->silhouette = &s->silhouettes[j];
					continue;
				}
			}
			s->silhouette = &s->silhouettes[s->last_silhouette++];
			if(s->last_silhouette == NUM_SILHOUETTES) s->last_silhouette = 0;
			for(int j = 0; j < s->num_edges; j++) s->edges[j].flag = -1;
			for(int j = 0; j < s->num_triangles; j++) {
				if(s->triangles[j].plane * light > 0.0) {
					s->silhouette->flags[j] = 1;
					Triangle *t = &s->triangles[j];
					s->edges[t->e[0]].reverse = t->reverse[0];
					s->edges[t->e[1]].reverse = t->reverse[1];
					s->edges[t->e[2]].reverse = t->reverse[2];
					s->edges[t->e[0]].flag++;
					s->edges[t->e[1]].flag++;
					s->edges[t->e[2]].flag++;
				} else s->silhouette->flags[j] = 0;
			}
			s->silhouette->light = light;
			s->silhouette->num_vertex = 0;
			vec4 *vertex = s->silhouette->vertex;
			for(int j = 0; j < s->num_edges; j++) {
				if(s->edges[j].flag != 0) continue;
				Edge *e = &s->edges[j];
				if(e->reverse) {
					*vertex++ = vec4(e->v[0],1);
					*vertex++ = vec4(e->v[1],1);
					*vertex++ = vec4(e->v[1],0);
					*vertex++ = vec4(e->v[0],0);
				} else {
					*vertex++ = vec4(e->v[1],1);
					*vertex++ = vec4(e->v[0],1);
					*vertex++ = vec4(e->v[0],0);
					*vertex++ = vec4(e->v[1],0);
				}
				s->silhouette->num_vertex += 4;
			}
		}
	} else {
		Surface *surface = surfaces[s];
		Surface *s = surface;	// :)
		for(int i = 0; i < NUM_SILHOUETTES; i++) {
			if(s->silhouettes[i].light == light) {
				s->silhouette = &s->silhouettes[i];
				return;
			}
		}
		s->silhouette = &s->silhouettes[s->last_silhouette++];
		if(s->last_silhouette == NUM_SILHOUETTES) s->last_silhouette = 0;
		for(int i = 0; i < s->num_edges; i++) s->edges[i].flag = -1;
		for(int i = 0; i < s->num_triangles; i++) {
			if(s->triangles[i].plane * light > 0.0) {
				s->silhouette->flags[i] = 1;
				Triangle *t = &s->triangles[i];
				s->edges[t->e[0]].reverse = t->reverse[0];
				s->edges[t->e[1]].reverse = t->reverse[1];
				s->edges[t->e[2]].reverse = t->reverse[2];
				s->edges[t->e[0]].flag++;
				s->edges[t->e[1]].flag++;
				s->edges[t->e[2]].flag++;
			} else s->silhouette->flags[i] = 0;;
		}
		s->silhouette->light = light;
		s->silhouette->num_vertex = 0;
		vec4 *vertex = s->silhouette->vertex;
		for(int i = 0; i < s->num_edges; i++) {
			if(s->edges[i].flag != 0) continue;
			Edge *e = &s->edges[i];
			if(e->reverse) {
				*vertex++ = vec4(e->v[0],1);
				*vertex++ = vec4(e->v[1],1);
				*vertex++ = vec4(e->v[1],0);
				*vertex++ = vec4(e->v[0],0);
			} else {
				*vertex++ = vec4(e->v[1],1);
				*vertex++ = vec4(e->v[0],1);
				*vertex++ = vec4(e->v[0],0);
				*vertex++ = vec4(e->v[1],0);
			}
			s->silhouette->num_vertex += 4;
		}
	}
}

int Mesh::getNumIntersections(const vec3 &line0,const vec3 &line1,int s) {
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
				if(s->silhouette->flags[j] == 0) continue;
				Triangle *t = &s->triangles[j];
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
			if(s->silhouette->flags[i] == 0) continue;
			Triangle *t = &s->triangles[i];
			float dot = -(t->plane * vec4(line0,1)) / (vec3(t->plane) * dir);
			if(dot < 0.0 || dot > 1.0) continue;
			vec3 p = line0 + dir * dot;
			if(p * t->c[0] > 0.0 && p * t->c[1] > 0.0 && p * t->c[2] > 0.0) num_intersections++;
		}
		return num_intersections;
	}
}

int Mesh::renderShadowVolume(int) {
	fprintf(stderr,"Mesh::renderShadowVolume()\n");
	return 0;
}

/*****************************************************************************/
/*                                                                           */
/* line inersection                                                          */
/*                                                                           */
/*****************************************************************************/

int Mesh::intersection(const vec3 &line0,const vec3 &line1,vec3 &point,vec3 &normal,int s) {
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

void Mesh::transform(const mat4 &m,int s) {
	mat4 r = m.rotation();
	if(s < 0) {
		for(int i = 0; i < num_surfaces; i++) {
			Surface *s = surfaces[i];
			for(int j = 0; j < s->num_vertex; j++) {
				Vertex *v = &s->vertex[j];
				v->xyz = m * v->xyz;
				v->normal = r * v->normal;
				v->normal.normalize();
				v->tangent = r * v->tangent;
				v->tangent.normalize();
				v->binormal = r * v->binormal;
				v->binormal.normalize();
			}
			for(int j = 0; j < s->num_edges; j++) {
				Edge *e = &s->edges[j];
				e->v[0] = m * e->v[0];
				e->v[1] = m * e->v[1];
			}
			for(int j = 0; j < s->num_triangles; j++) {
				Triangle *t = &s->triangles[j];
				t->v[0] = m * t->v[0];
				t->v[1] = m * t->v[1];
				t->v[2] = m * t->v[2];
				vec3 normal;
				normal.cross(t->v[1] - t->v[0],t->v[2] - t->v[0]);
				normal.normalize();
				t->plane = vec4(normal,-t->v[0] * normal);
				normal.cross(t->plane,t->v[0] - t->v[2]);	// fast point in traingle
				normal.normalize();
				t->c[0] = vec4(normal,-t->v[0] * normal);
				normal.cross(t->plane,t->v[1] - t->v[0]);
				normal.normalize();
				t->c[1] = vec4(normal,-t->v[1] * normal);
				normal.cross(t->plane,t->v[2] - t->v[1]);
				normal.normalize();
				t->c[2] = vec4(normal,-t->v[2] * normal);
			}
		}
	} else {
		Surface *surface = surfaces[s];
		Surface *s = surface;
		for(int i = 0; i < s->num_vertex; i++) {
			Vertex *v = &s->vertex[i];
			v->xyz = m * v->xyz;
			v->normal = r * v->normal;
			v->normal.normalize();
			v->tangent = r * v->tangent;
			v->tangent.normalize();
			v->binormal = r * v->binormal;
			v->binormal.normalize();
		}
		for(int i = 0; i < s->num_edges; i++) {
			Edge *e = &s->edges[i];
			e->v[0] = m * e->v[0];
			e->v[1] = m * e->v[1];
		}
		for(int i = 0; i < s->num_triangles; i++) {
			Triangle *t = &s->triangles[i];
			t->v[0] = m * t->v[0];
			t->v[1] = m * t->v[1];
			t->v[2] = m * t->v[2];
			vec3 normal;
			normal.cross(t->v[1] - t->v[0],t->v[2] - t->v[0]);
			normal.normalize();
			t->plane = vec4(normal,-t->v[0] * normal);
			normal.cross(t->plane,t->v[0] - t->v[2]);	// fast point in traingle
			normal.normalize();
			t->c[0] = vec4(normal,-t->v[0] * normal);
			normal.cross(t->plane,t->v[1] - t->v[0]);
			normal.normalize();
			t->c[1] = vec4(normal,-t->v[1] * normal);
			normal.cross(t->plane,t->v[2] - t->v[1]);
			normal.normalize();
			t->c[2] = vec4(normal,-t->v[2] * normal);
		}
	}
	calculate_bounds();
}

/*****************************************************************************/
/*                                                                           */
/* IO functions                                                              */
/*                                                                           */
/*****************************************************************************/

/*
 */
int Mesh::getNumSurfaces() {
	return num_surfaces;
}

const char *Mesh::getSurfaceName(int s) {
	return surfaces[s]->name;
}

int Mesh::getSurface(const char *name) {
	for(int i = 0; i < num_surfaces; i++) if(!strcmp(name,surfaces[i]->name)) return i;
	fprintf(stderr,"Mesh::getSurface(): can`t find \"%s\" surface\n",name);
	return -1;
}

/*
 */
int Mesh::getNumVertex(int s) {
	return surfaces[s]->num_vertex;
}

Mesh::Vertex *Mesh::getVertex(int s) {
	return surfaces[s]->vertex;
}

int Mesh::getNumStrips(int s) {
	return surfaces[s]->num_strips;
}

int *Mesh::getIndices(int s) {
	return surfaces[s]->indices;
}

int Mesh::getNumEdges(int s) {
	return surfaces[s]->num_edges;
}

Mesh::Edge *Mesh::getEdges(int s) {
	return surfaces[s]->edges;
}

int Mesh::getNumTriangles(int s) {
	return surfaces[s]->num_triangles;
}

Mesh::Triangle *Mesh::getTriangles(int s) {
	return surfaces[s]->triangles;
}

/*
 */
const vec3 &Mesh::getMin(int s) {
	if(s < 0) return min;
	return surfaces[s]->min;
}

const vec3 &Mesh::getMax(int s) {
	if(s < 0) return max;
	return surfaces[s]->max;
}

const vec3 &Mesh::getCenter(int s) {
	if(s < 0) return center;
	return surfaces[s]->center;
}

float Mesh::getRadius(int s) {
	if(s < 0) return radius;
	return surfaces[s]->radius;
}

/* add surface
 */
void Mesh::addSurface(Mesh *mesh,int surface) {
	Surface *s = new Surface;
	memcpy(s,mesh->surfaces[surface],sizeof(Surface));
	s->vertex = new Vertex[s->num_vertex];
	memcpy(s->vertex,mesh->surfaces[surface]->vertex,sizeof(Vertex) * s->num_vertex);
	if(mesh->surfaces[surface]->edges) {
		s->edges = new Edge[s->num_edges];
		memcpy(s->edges,mesh->surfaces[surface]->edges,sizeof(Edge) * s->num_edges);
	}
	if(mesh->surfaces[surface]->triangles) {
		s->triangles = new Triangle[s->num_triangles];
		memcpy(s->triangles,mesh->surfaces[surface]->triangles,sizeof(Triangle) * s->num_triangles);
	}
	if(mesh->surfaces[surface]->indices) {
		s->indices = new int[s->num_indices];
		memcpy(s->indices,mesh->surfaces[surface]->indices,sizeof(int) * s->num_indices);
	}
	if(mesh->surfaces[surface]->silhouettes[0].vertex) {
		for(int j = 0; j < NUM_SILHOUETTES; j++) {
			s->silhouettes[j].vertex = new vec4[s->num_edges * 4];
			s->silhouettes[j].flags = new char[s->num_triangles];
		}
	}
	if(num_surfaces == NUM_SURFACES) {
		fprintf(stderr,"Mesh::addSurface(): many surfaces\n");
		num_surfaces--;
	}
	surfaces[num_surfaces++] = s;
	calculate_bounds();
}

void Mesh::addSurface(const char *name,Vertex *vertex,int num_vertex) {
	Surface *s = new Surface;
	memset(s,0,sizeof(Surface));
	strcpy(s->name,name);
	s->num_vertex = num_vertex;
	s->vertex = new Vertex[s->num_vertex];
	memcpy(s->vertex,vertex,sizeof(Vertex) * s->num_vertex);
	if(num_surfaces == NUM_SURFACES) {
		fprintf(stderr,"Mesh::addSurface(): many surfaces\n");
		num_surfaces--;
	}
	surfaces[num_surfaces++] = s;
	calculate_bounds();
}

/* load
 */
int Mesh::load(const char *name) {
	for(int i = 0; i < num_surfaces; i++) {
		Surface *s = surfaces[i];
		if(s->vertex) delete s->vertex;
		if(s->edges) delete s->edges;
		if(s->triangles) delete s->triangles;
		if(s->indices) delete s->indices;
		delete s;
	}
	if(strstr(name,".mesh")) {
		FILE *file = fopen(name,"rb");
		if(!file) {
			fprintf(stderr,"Mesh::load(): error open \"%s\" file\n",name);
			return 0;
		}
		int magic;
		fread(&magic,sizeof(int),1,file);
		if(magic == MESH_STRIP_MAGIC) {	// load mesh file
			load(file);
			fclose(file);
			return 1;
		}
		fclose(file);
	}
	if(strstr(name,".3ds")) load_3ds(name);
	else if(strstr(name,".mesh")) load_mesh(name);
	if(num_surfaces == 0) {
		fprintf(stderr,"Mesh::load(): error open \"%s\" file\n",name);
		return 0;
	}
	calculate_tangent();
	create_shadow_volumes();
	create_triangle_strips();
	calculate_bounds();
	return 1;
}

/* save
 */
int Mesh::save(const char *name) {
	FILE *file = fopen(name,"wb");
	if(!file) {
		fprintf(stderr,"Mesh::load(): error create \"%s\" file\n",name);
		return 0;
	}
	int magic = MESH_STRIP_MAGIC;
	fwrite(&magic,sizeof(int),1,file);
	save(file);
	fclose(file);
	return 1;
}

/* strip mesh loader
 */
void Mesh::load(FILE *file) {
	// number of surfaces
	int num_surfaces;
	fread(&num_surfaces,sizeof(int),1,file);
	for(int i = 0; i < num_surfaces; i++) {
		Surface *s = new Surface;
		memset(s,0,sizeof(Surface));
		// name
		fread(s->name,sizeof(s->name),1,file);
		// vertexes
		fread(&s->num_vertex,sizeof(int),1,file);
		s->vertex = new Vertex[s->num_vertex];
		fread(s->vertex,sizeof(Vertex),s->num_vertex,file);
		// edges
		fread(&s->num_edges,sizeof(int),1,file);
		s->edges = new Edge[s->num_edges];
		for(int j = 0; j < s->num_edges; j++) {
			Edge *e = &s->edges[j];
			fread(e->v,sizeof(vec3),2,file);
			e->reverse = 0;
			e->flag = 0;
		}
		// triangles
		fread(&s->num_triangles,sizeof(int),1,file);
		s->triangles = new Triangle[s->num_triangles];
		for(int j = 0; j < s->num_triangles; j++) {
			Triangle *t = &s->triangles[j];
			fread(t->v,sizeof(vec3),3,file);
			fread(t->e,sizeof(int),3,file);
			char reverse;
			fread(&reverse,sizeof(char),1,file);
			t->reverse[0] = reverse & (1 << 0);
			t->reverse[1] = reverse & (1 << 1);
			t->reverse[2] = reverse & (1 << 2);
			vec3 normal;
			normal.cross(t->v[1] - t->v[0],t->v[2] - t->v[0]);
			normal.normalize();
			t->plane = vec4(normal,-t->v[0] * normal);
			normal.cross(t->plane,t->v[0] - t->v[2]);
			normal.normalize();
			t->c[0] = vec4(normal,-t->v[0] * normal);
			normal.cross(t->plane,t->v[1] - t->v[0]);
			normal.normalize();
			t->c[1] = vec4(normal,-t->v[1] * normal);
			normal.cross(t->plane,t->v[2] - t->v[1]);
			normal.normalize();
			t->c[2] = vec4(normal,-t->v[2] * normal);
		}
		// indices
		fread(&s->num_indices,sizeof(int),1,file);
		fread(&s->num_strips,sizeof(int),1,file);
		s->indices = new int[s->num_indices];
		if(s->num_indices < 65536) {
			unsigned short *buf = new unsigned short[s->num_indices];
			fread(buf,sizeof(unsigned short),s->num_indices,file);
			for(int j = 0; j < s->num_indices; j++) s->indices[j] = buf[j];
			delete buf;
		} else fread(s->indices,sizeof(int),s->num_indices,file);
		// shadow volume vertexes
		for(int j = 0; j < NUM_SILHOUETTES; j++) {
			s->silhouettes[j].vertex = new vec4[s->num_edges * 4];
			s->silhouettes[j].flags = new char[s->num_triangles];
		}
		if(this->num_surfaces == NUM_SURFACES) {
			fprintf(stderr,"Mesh::load(): many surfaces\n");
			this->num_surfaces--;
		}
		surfaces[this->num_surfaces++] = s;
	}
	calculate_bounds();
}

/* strip mesh saver
 */
void Mesh::save(FILE *file) {
	fwrite(&num_surfaces,sizeof(int),1,file);
	for(int i = 0; i < num_surfaces; i++) {
		Surface *s = surfaces[i];
		fwrite(s->name,sizeof(s->name),1,file);
		// vertexes
		fwrite(&s->num_vertex,sizeof(int),1,file);
		fwrite(s->vertex,sizeof(Vertex),s->num_vertex,file);
		// edges
		fwrite(&s->num_edges,sizeof(int),1,file);
		for(int j = 0; j < s->num_edges; j++) {
			fwrite(s->edges[j].v,sizeof(vec3),2,file);
		}
		// triangles
		fwrite(&s->num_triangles,sizeof(int),1,file);
		for(int j = 0; j < s->num_triangles; j++) {
			fwrite(s->triangles[j].v,sizeof(vec3),3,file);
			fwrite(s->triangles[j].e,sizeof(int),3,file);
			char reverse = (s->triangles[j].reverse[2] << 2) | (s->triangles[j].reverse[1] << 1) | (s->triangles[j].reverse[0] << 0);
			fwrite(&reverse,sizeof(char),1,file);
		}
		// indices
		fwrite(&s->num_indices,sizeof(int),1,file);
		fwrite(&s->num_strips,sizeof(int),1,file);
		if(s->num_indices < 65536) {
			unsigned short *buf = new unsigned short[s->num_indices];
			for(int j = 0; j < s->num_indices; j++) buf[j] = (unsigned int)s->indices[j];
			fwrite(buf,sizeof(unsigned short),s->num_indices,file);
			delete buf;
		} else fwrite(s->indices,sizeof(int),s->num_indices,file);
	}
}

/*****************************************************************************/
/*                                                                           */
/* raw mesh loader                                                           */
/*                                                                           */
/*****************************************************************************/

struct load_mesh_Vertex {
	vec3 xyz;
	vec3 normal;
	vec2 texcoord;
};

int Mesh::load_mesh(const char *name) {
	FILE *file = fopen(name,"rb");
	if(!file) {
		fprintf(stderr,"Mesh::load_mesh(): error open \"%s\" file\n",name);
		return 0;
	}
	int magic;
	// mesh magic raw
	fread(&magic,sizeof(int),1,file);
	if(magic != MESH_RAW_MAGIC) {
		fprintf(stderr,"Mesh::load_mesh(): wrong magic 0x%04x in \"%s\" file\n",magic,name);
		fclose(file);
		return 0;
	}
	// number of surfaces
	int num_surfaces;
	fread(&num_surfaces,sizeof(int),1,file);
	for(int i = 0; i < num_surfaces; i++) {
		Surface *s = new Surface;
		memset(s,0,sizeof(Surface));
		// name
		fread(s->name,sizeof(s->name),1,file);
		// vertexes
		int num_vertex;
		fread(&num_vertex,sizeof(int),1,file);
		load_mesh_Vertex *vertex = new load_mesh_Vertex[num_vertex];
		fread(vertex,sizeof(load_mesh_Vertex),num_vertex,file);
		s->vertex = new Vertex[num_vertex];
		for(int j = 0; j < num_vertex; j += 3) {
			if(vertex[j + 0].xyz == vertex[j + 1].xyz) continue;	// skip degenerate triangles
			if(vertex[j + 1].xyz == vertex[j + 2].xyz) continue;
			if(vertex[j + 2].xyz == vertex[j + 0].xyz) continue;
			for(int k = 0; k < 3; k++) {
				s->vertex[s->num_vertex + k].xyz = vertex[j + k].xyz;
				s->vertex[s->num_vertex + k].normal = vertex[j + k].normal;
				s->vertex[s->num_vertex + k].texcoord = vertex[j + k].texcoord;
			}
			s->num_vertex += 3;
		}
		delete vertex;
		if(this->num_surfaces == NUM_SURFACES) {
			fprintf(stderr,"Mesh::load_mesh(): many surfaces\n");
			this->num_surfaces--;
		}
		surfaces[this->num_surfaces++] = s;
	}
	fclose(file);
	return 1;
}

/*****************************************************************************/
/*                                                                           */
/* 3ds loader                                                                */
/*                                                                           */
/*****************************************************************************/

enum {
	LOAD_3DS_CHUNK_MAIN = 0x4d4d,
	LOAD_3DS_CHUNK_OBJMESH = 0x3d3d,
	LOAD_3DS_CHUNK_OBJBLOCK = 0x4000,
	LOAD_3DS_CHUNK_TRIMESH = 0x4100,
	LOAD_3DS_CHUNK_VERTLIST = 0x4110,
	LOAD_3DS_CHUNK_FACELIST = 0x4120,
	LOAD_3DS_CHUNK_MAPLIST = 0x4140,
	LOAD_3DS_CHUNK_SMOOTHLIST = 0x4150
};

struct load_3ds_trimesh {
	vec3 *vertex;
	int num_vertex;
	vec2 *texcoord;
	int num_texcoord;
	int *face;
	int *smoothgroup;
	int num_face;
	vec3 *normal;
};

struct load_3ds_objblock {
	char *name;
	load_3ds_trimesh **trimesh;
	int num_trimesh;
};

struct load_3ds_mesh {
	load_3ds_objblock **objblock;
	int num_objblock;
};

typedef int (*load_3ds_process_chunk)(FILE *file,int type,int size,void *data);

static int load_3ds_skeep_bytes(FILE *file,int bytes) {
	fseek(file,bytes,SEEK_CUR);
	return bytes;
}

static int load_3ds_read_string(FILE *file,char *string) {
	int i = 0;
	char *s = string;
	while((*s++ = fgetc(file)) != '\0') i++;
	return ++i;
}

static int load_3ds_read_ushort(FILE *file) {
	unsigned short ret;
	fread(&ret,1,sizeof(unsigned short),file);
	return ret;
}

static int load_3ds_read_int(FILE *file) {
	int ret;
	fread(&ret,1,sizeof(int),file);
	return ret;
}

static float load_3ds_read_float(FILE *file) {
	float ret;
	fread(&ret,1,sizeof(float),file);
	return ret;
}

static int load_3ds_read_chunk(FILE *file,load_3ds_process_chunk func,void *data) {
	int type = load_3ds_read_ushort(file);
	int size = load_3ds_read_int(file);
	if(func(file,type,size - 6,data) == 0) load_3ds_skeep_bytes(file,size - 6);
	return size;
}

static int load_3ds_read_chunks(FILE *file,int bytes,load_3ds_process_chunk func,void *data) {
	int bytes_read = 0;
	while(bytes_read < bytes) bytes_read += load_3ds_read_chunk(file,func,data);
	if(bytes_read != bytes) fprintf(stderr,"Mesh::load_3ds(): expected %d bytes but read %d\n",bytes,bytes_read);
	return bytes_read;
}

static int load_3ds_process_smoothlist(FILE *file,int type,int size,void *data) {
	if(type == LOAD_3DS_CHUNK_SMOOTHLIST) {
		load_3ds_trimesh *t = (load_3ds_trimesh*)data;
		t->smoothgroup = new int[t->num_face];
		for(int i = 0; i < t->num_face; i++) t->smoothgroup[i] = load_3ds_read_int(file);
		return 1;
	}
	return 0;
}

static int load_3ds_process_trimesh(FILE *file,int type,int size,void *data) {
	load_3ds_trimesh *t = (load_3ds_trimesh*)data;
	if(type == LOAD_3DS_CHUNK_VERTLIST) {
		t->num_vertex = load_3ds_read_ushort(file);
		t->vertex = new vec3[t->num_vertex];
		for(int i = 0; i < t->num_vertex; i++) {
			t->vertex[i].x = load_3ds_read_float(file);
			t->vertex[i].y = load_3ds_read_float(file);
			t->vertex[i].z = load_3ds_read_float(file);
		}
		return 1;
	} else if(type == LOAD_3DS_CHUNK_MAPLIST) {
		t->num_texcoord = load_3ds_read_ushort(file);
		t->texcoord = new vec2[t->num_texcoord];
		for(int i = 0; i < t->num_texcoord; i++) {
			t->texcoord[i].x = load_3ds_read_float(file);
			t->texcoord[i].y = 1.0f - load_3ds_read_float(file);
		}
		return 1;
	} else if(type == LOAD_3DS_CHUNK_FACELIST) {
		int bytes_left;
		t->num_face = load_3ds_read_ushort(file);
		t->face = new int[t->num_face * 3];
		for(int i = 0; i < t->num_face * 3; i += 3) {
			t->face[i + 0] = load_3ds_read_ushort(file);
			t->face[i + 1] = load_3ds_read_ushort(file);
			t->face[i + 2] = load_3ds_read_ushort(file);
			load_3ds_read_ushort(file);
		}
		bytes_left = size - t->num_face * sizeof(unsigned short) * 4 - 2;
		if(bytes_left > 0) load_3ds_read_chunks(file,bytes_left,load_3ds_process_smoothlist,t);
		return 1;
	}
	return 0;
}

static int load_3ds_process_objblock(FILE *file,int type,int size,void *data) {
	if(type == LOAD_3DS_CHUNK_TRIMESH) {
		load_3ds_objblock *o = (load_3ds_objblock*)data;
		o->num_trimesh++;
		o->trimesh = (load_3ds_trimesh**)realloc(o->trimesh,sizeof(load_3ds_trimesh) * o->num_trimesh);
		o->trimesh[o->num_trimesh - 1] = new load_3ds_trimesh;
		load_3ds_trimesh *t = o->trimesh[o->num_trimesh - 1];
		memset(t,0,sizeof(load_3ds_trimesh));
		load_3ds_read_chunks(file,size,load_3ds_process_trimesh,t);
		return 1;
	}
	return 0;
}

static int load_3ds_process_objmesh(FILE *file,int type,int size,void *data) {
	if(type == LOAD_3DS_CHUNK_OBJBLOCK) {
		char name[256];
		size -= load_3ds_read_string(file,name);
		load_3ds_mesh *m = (load_3ds_mesh*)data;
		m->num_objblock++;
		m->objblock = (load_3ds_objblock**)realloc(m->objblock,sizeof(load_3ds_objblock) * m->num_objblock);
		m->objblock[m->num_objblock - 1] = new load_3ds_objblock;
		load_3ds_objblock *o = m->objblock[m->num_objblock - 1];
		memset(o,0,sizeof(load_3ds_objblock));
		o->name = strdup(name);
		load_3ds_read_chunks(file,size,load_3ds_process_objblock,o);
		return 1;
	}
	return 0;
}

static int load_3ds_process_main(FILE *file,int type,int size,void *data) {
	if(type == LOAD_3DS_CHUNK_OBJMESH) {
		load_3ds_read_chunks(file,size,load_3ds_process_objmesh,data);
		return 1;
	}
	return 0;
}

static void load_3ds_trimesh_calculate_normals(load_3ds_trimesh *t) {
	vec3 *normal_face = new vec3[t->num_face];
	int *vertex_count = new int[t->num_vertex];
	memset(vertex_count,0,sizeof(int) * t->num_vertex);
	int **vertex_face = new int*[t->num_vertex];
	t->normal = new vec3[t->num_face * 3];
	memset(t->normal,0,sizeof(vec3) * t->num_face * 3);
	if(t->texcoord == NULL) {
		t->num_texcoord = t->num_vertex;
		t->texcoord = new vec2[t->num_vertex];
		memset(t->texcoord,0,sizeof(vec2) * t->num_vertex);
	}
	for(int i = 0, j = 0; i < t->num_face; i++, j += 3) {
		int v0 = t->face[j + 0];
		int v1 = t->face[j + 1];
		int v2 = t->face[j + 2];
		vertex_count[v0]++;
		vertex_count[v1]++;
		vertex_count[v2]++;
		normal_face[i].cross(t->vertex[v1] - t->vertex[v0],t->vertex[v2] - t->vertex[v0]);
		normal_face[i].normalize();
	}
	for(int i = 0; i < t->num_vertex; i++) {
		vertex_face[i] = new int[vertex_count[i] + 1];
		vertex_face[i][0] = vertex_count[i];
	}
	for(int i = 0, j = 0; i < t->num_face; i++, j += 3) {
		int v0 = t->face[j + 0];
		int v1 = t->face[j + 1];
		int v2 = t->face[j + 2];
		vertex_face[v0][vertex_count[v0]--] = i;
		vertex_face[v1][vertex_count[v1]--] = i;
		vertex_face[v2][vertex_count[v2]--] = i;
	}
	for(int i = 0, j = 0; i < t->num_face; i++, j += 3) {
		int v0 = t->face[j + 0];
		int v1 = t->face[j + 1];
		int v2 = t->face[j + 2];
		for(int k = 1; k <= vertex_face[v0][0]; k++) {
			int l = vertex_face[v0][k];
			if(l == i || (t->smoothgroup && t->smoothgroup[i] & t->smoothgroup[l])) t->normal[j + 0] += normal_face[l];
		}
		for(int k = 1; k <= vertex_face[v1][0]; k++) {
			int l = vertex_face[v1][k];
			if(l == i || (t->smoothgroup && t->smoothgroup[i] & t->smoothgroup[l])) t->normal[j + 1] += normal_face[l];
		}
		for(int k = 1; k <= vertex_face[v2][0]; k++) {
			int l = vertex_face[v2][k];
			if(l == i || (t->smoothgroup && t->smoothgroup[i] & t->smoothgroup[l])) t->normal[j + 2] += normal_face[l];
		}
	}
	for(int i = 0; i < t->num_face * 3; i++) t->normal[i].normalize();
	for(int i = 0; i < t->num_vertex; i++) delete vertex_face[i];
	delete vertex_face;
	delete vertex_count;
	delete normal_face;
}

/*
 */
int Mesh::load_3ds(const char *name) {
	FILE *file;
	file = fopen(name,"rb");
	if(!file) {
		fprintf(stderr,"Mesh::load_3ds(): error open \"%s\" file\n",name);
		return 0;
	}
	int type = load_3ds_read_ushort(file);
	int size = load_3ds_read_int(file);
	if(type != LOAD_3DS_CHUNK_MAIN) {
		fprintf(stderr,"Mesh::load_3ds(): wrong main chunk in \"%s\" file\n",name);
		fclose(file);
		return 0;
	}
	load_3ds_mesh *m = new load_3ds_mesh;
	memset(m,0,sizeof(load_3ds_mesh));
	load_3ds_read_chunks(file,size - 6,load_3ds_process_main,m);
	fclose(file);
	for(int i = 0; i < m->num_objblock; i++) {
		load_3ds_objblock *o = m->objblock[i];
		Surface *s = new Surface;
		memset(s,0,sizeof(Surface));
		// name
		strcpy(s->name,m->objblock[i]->name);
		free(m->objblock[i]->name);
		// calculate number of vertexes
		int num_objblock_vertex = 0;
		for(int j = 0; j < o->num_trimesh; j++) {
			num_objblock_vertex += o->trimesh[j]->num_face * 3;
			load_3ds_trimesh_calculate_normals(o->trimesh[j]);
		}
		s->num_vertex = num_objblock_vertex;
		s->vertex = new Vertex[s->num_vertex];
		// copy vertexes
		num_objblock_vertex = 0;
		for(int j = 0; j < o->num_trimesh; j++) {
			load_3ds_trimesh *t = o->trimesh[j];
			for(int k = 0, l = 0; k < t->num_face; k++, l += 3) {
				Vertex *v = s->vertex + num_objblock_vertex + l;
				v[0].xyz = t->vertex[t->face[l + 0]];
				v[1].xyz = t->vertex[t->face[l + 1]];
				v[2].xyz = t->vertex[t->face[l + 2]];
				v[0].texcoord = t->texcoord[t->face[l + 0]];
				v[1].texcoord = t->texcoord[t->face[l + 1]];
				v[2].texcoord = t->texcoord[t->face[l + 2]];
				v[0].normal = t->normal[l + 0];
				v[1].normal = t->normal[l + 1];
				v[2].normal = t->normal[l + 2];
			}
			num_objblock_vertex += t->num_face * 3;
			delete t->vertex;
			delete t->texcoord;
			delete t->face;
			if(t->smoothgroup) delete t->smoothgroup;
			delete t->normal;
			delete t;
		}
		free(o->trimesh);
		delete o;
		if(num_surfaces == NUM_SURFACES) {
			fprintf(stderr,"Mesh::load_3ds(): many surfaces\n");
			num_surfaces--;
		}
		surfaces[num_surfaces++] = s;
	}
	free(m->objblock);
	delete m;
	return 1;
}

/*****************************************************************************/
/*                                                                           */
/* calculate tangent                                                         */
/*                                                                           */
/*****************************************************************************/

void Mesh::calculate_tangent() {
	for(int i = 0; i < num_surfaces; i++) {
		Surface *s = surfaces[i];
		for(int j = 0; j < s->num_vertex; j += 3) {
			Vertex *v0 = &s->vertex[j + 0];
			Vertex *v1 = &s->vertex[j + 1];
			Vertex *v2 = &s->vertex[j + 2];
			vec3 normal,tangent,binormal;
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
			
			v1->binormal.cross(v1->normal,tangent);
			v1->binormal.normalize();
			v1->tangent.cross(v1->binormal,v1->normal);
			if(normal * v1->normal < 0) v1->binormal = -v1->binormal;

			v2->binormal.cross(v2->normal,tangent);
			v2->binormal.normalize();
			v2->tangent.cross(v2->binormal,v2->normal);
			if(normal * v2->normal < 0) v2->binormal = -v2->binormal;
		}
	}
}

/*****************************************************************************/
/*                                                                           */
/* calculate bounds                                                          */
/*                                                                           */
/*****************************************************************************/

void Mesh::calculate_bounds() {
	min = vec3(1000000,1000000,1000000);
	max = vec3(-1000000,-1000000,-1000000);
	for(int i = 0; i < num_surfaces; i++) {
		Surface *s = surfaces[i];
		s->min = vec3(1000000,1000000,1000000);
		s->max = vec3(-1000000,-1000000,-1000000);
		for(int j = 0; j < s->num_vertex; j++) {
			if(s->max.x < s->vertex[j].xyz.x) s->max.x = s->vertex[j].xyz.x;
			if(s->min.x > s->vertex[j].xyz.x) s->min.x = s->vertex[j].xyz.x;
			if(s->max.y < s->vertex[j].xyz.y) s->max.y = s->vertex[j].xyz.y;
			if(s->min.y > s->vertex[j].xyz.y) s->min.y = s->vertex[j].xyz.y;
			if(s->max.z < s->vertex[j].xyz.z) s->max.z = s->vertex[j].xyz.z;
			if(s->min.z > s->vertex[j].xyz.z) s->min.z = s->vertex[j].xyz.z;
		}
		s->center = (s->min + s->max) / 2.0f;
		s->radius = 0;
		if(max.x < s->max.x) max.x = s->max.x;
		if(min.x > s->min.x) min.x = s->min.x;
		if(max.y < s->max.y) max.y = s->max.y;
		if(min.y > s->min.y) min.y = s->min.y;
		if(max.z < s->max.z) max.z = s->max.z;
		if(min.z > s->min.z) min.z = s->min.z;
	}
	center = (min + max) / 2.0f;
	radius = 0;
	for(int i = 0; i < num_surfaces; i++) {
		Surface *s = surfaces[i];
		for(int j = 0; j < s->num_vertex; j++) {
			float r = (s->center - s->vertex[j].xyz).length();
			if(r > s->radius) s->radius = r;
			r = (center - s->vertex[j].xyz).length();
			if(r > radius) radius = r;
		}
	}
}


/*****************************************************************************/
/*                                                                           */
/* create shadow volume                                                      */
/*                                                                           */
/*****************************************************************************/

struct csv_Vertex {
	vec3 xyz;
	int id;
};

struct csv_Edge {
	int v[2];
	int id;
};

static int csv_vertex_cmp(const void *a,const void *b) {
	csv_Vertex *v0 = (csv_Vertex*)a;
	csv_Vertex *v1 = (csv_Vertex*)b;
	float d;
	d = v0->xyz[0] - v1->xyz[0];
	if(d > EPSILON) return 1;
	if(d < -EPSILON) return -1;
	d = v0->xyz[1] - v1->xyz[1];
	if(d > EPSILON) return 1;
	if(d < -EPSILON) return -1;
	d = v0->xyz[2] - v1->xyz[2];
	if(d > EPSILON) return 1;
	if(d < -EPSILON) return -1;
	return 0;
}

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

void Mesh::create_shadow_volumes() {
	for(int i = 0; i < num_surfaces; i++) {
		Surface *s = surfaces[i];
		s->num_edges = s->num_vertex;
		s->num_triangles = s->num_vertex / 3;
		// cerate vertexes
		csv_Vertex *v = new csv_Vertex[s->num_vertex];
		for(int j = 0; j < s->num_vertex; j++) {
			v[j].xyz = s->vertex[j].xyz;
			v[j].id = j;
		}
		qsort(v,s->num_vertex,sizeof(csv_Vertex),csv_vertex_cmp);
		int num_optimized_vertex = 0;
		int *vbuf = new int[s->num_vertex];
		for(int j = 0; j < s->num_vertex; j++) {
			if(j == 0 || csv_vertex_cmp(&v[num_optimized_vertex - 1],&v[j])) v[num_optimized_vertex++] = v[j];
			vbuf[v[j].id] = num_optimized_vertex - 1;
		}
		// create edges
		csv_Edge *e = new csv_Edge[s->num_edges];
		for(int j = 0; j < s->num_edges; j += 3) {
			e[j + 0].v[0] = vbuf[j + 0];
			e[j + 0].v[1] = vbuf[j + 1];
			e[j + 1].v[0] = vbuf[j + 1];
			e[j + 1].v[1] = vbuf[j + 2];
			e[j + 2].v[0] = vbuf[j + 2];
			e[j + 2].v[1] = vbuf[j + 0];
			e[j + 0].id = j + 0;
			e[j + 1].id = j + 1;
			e[j + 2].id = j + 2;
		}
		qsort(e,s->num_edges,sizeof(csv_Edge),csv_edge_cmp);
		int num_optimized_edges = 0;
		int *ebuf = new int[s->num_edges];
		int *rbuf = new int[s->num_edges];
		for(int j = 0; j < s->num_edges; j++) {
			if(j == 0 || csv_edge_cmp(&e[num_optimized_edges - 1],&e[j])) e[num_optimized_edges++] = e[j];
			ebuf[e[j].id] = num_optimized_edges - 1;
			rbuf[e[j].id] = e[num_optimized_edges - 1].v[0] != e[j].v[0];
		}
		// final
		s->num_edges = num_optimized_edges;
		s->edges = new Edge[s->num_edges];
		for(int j = 0; j < s->num_edges; j++) {
			s->edges[j].v[0] = v[e[j].v[0]].xyz;
			s->edges[j].v[1] = v[e[j].v[1]].xyz;
			s->edges[j].reverse = 0;
			s->edges[j].flag = 0;
		}
		// triangles
		s->triangles = new Triangle[s->num_triangles];
		for(int j = 0, k = 0; j < s->num_triangles; j++, k += 3) {
			Triangle *t = &s->triangles[j];
			t->v[0] = v[vbuf[k + 0]].xyz;
			t->v[1] = v[vbuf[k + 1]].xyz;
			t->v[2] = v[vbuf[k + 2]].xyz;
			t->e[0] = ebuf[k + 0];
			t->e[1] = ebuf[k + 1];
			t->e[2] = ebuf[k + 2];
			t->reverse[0] = rbuf[k + 0];
			t->reverse[1] = rbuf[k + 1];
			t->reverse[2] = rbuf[k + 2];
			vec3 normal;
			normal.cross(t->v[1] - t->v[0],t->v[2] - t->v[0]);
			normal.normalize();
			t->plane = vec4(normal,-t->v[0] * normal);
			normal.cross(t->plane,t->v[0] - t->v[2]);	// fast point in traingle
			normal.normalize();
			t->c[0] = vec4(normal,-t->v[0] * normal);
			normal.cross(t->plane,t->v[1] - t->v[0]);
			normal.normalize();
			t->c[1] = vec4(normal,-t->v[1] * normal);
			normal.cross(t->plane,t->v[2] - t->v[1]);
			normal.normalize();
			t->c[2] = vec4(normal,-t->v[2] * normal);
		}
		// shadow volume vertexes
		for(int j = 0; j < NUM_SILHOUETTES; j++) {
			s->silhouettes[j].vertex = new vec4[s->num_edges * 4];
			s->silhouettes[j].flags = new char[s->num_triangles];
		}
		delete rbuf;
		delete ebuf;
		delete e;
		delete v;
	}
}

/*****************************************************************************/
/*                                                                           */
/* create triangle strips                                                    */
/*                                                                           */
/*****************************************************************************/

struct cts_Vertex {
	Mesh::Vertex v;
	int face;
	int num;
	int id;
};

struct cts_Edge {
	int v[2];
	int face;
	int num;
};

struct cts_Triangle {
	int v[3];
	int env[3];
	int flag;
};

static int cts_vertex_cmp(const void *a,const void *b) {
	cts_Vertex *ctsv0 = (cts_Vertex*)a;
	cts_Vertex *ctsv1 = (cts_Vertex*)b;
	Mesh::Vertex *v0 = &ctsv0->v;
	Mesh::Vertex *v1 = &ctsv1->v;
	float d;
#define CMP(a,b,e) { \
	d = (a)[0] - (b)[0]; \
	if(d > e) return 1; \
	if(d < -e) return -1; \
	d = (a)[1] - (b)[1]; \
	if(d > e) return 1; \
	if(d < -e) return -1; \
	d = (a)[2] - (b)[2]; \
	if(d > e) return 1; \
	if(d < -e) return -1; \
}
	CMP(v0->xyz,v1->xyz,EPSILON)
	CMP(v0->normal,v1->normal,EPSILON)
	//CMP(v0->tangent,v1->tangent,0.1)
	//CMP(v0->binormal,v1->binormal,0.1)
#undef CMP
	d = v0->texcoord[0] - v1->texcoord[0];
	if(d > EPSILON) return 1;
	if(d < -EPSILON) return -1;
	d = v0->texcoord[1] - v1->texcoord[1];
	if(d > EPSILON) return 1;
	if(d < -EPSILON) return -1;
	return 0;
}

static int cts_edge_cmp(const void *a,const void *b) {
	cts_Edge *e0 = (cts_Edge*)a;
	cts_Edge *e1 = (cts_Edge*)b;
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

void Mesh::create_triangle_strips() {
	for(int i = 0; i < num_surfaces; i++) {
		Surface *s = surfaces[i];
		s->num_indices = 0;	// already is zero
		s->num_strips = 0;
		s->indices = new int[s->num_vertex * 4 / 3];
		cts_Vertex *v = new cts_Vertex[s->num_vertex];
		cts_Edge *e = new cts_Edge[s->num_vertex];
		cts_Triangle *t = new cts_Triangle[s->num_vertex / 3];
		// create vertexes
		for(int j = 0; j < s->num_vertex; j++) {
			v[j].v = s->vertex[j];
			v[j].face = j / 3;
			v[j].num = j % 3;
			v[j].id = j;
		}
		qsort(v,s->num_vertex,sizeof(cts_Vertex),cts_vertex_cmp);
		int num_optimized_vertex = 0;
		int *vbuf = new int[s->num_vertex];
		for(int j = 0; j < s->num_vertex; j++) {
			if(j == 0 || cts_vertex_cmp(&v[num_optimized_vertex - 1],&v[j])) v[num_optimized_vertex++] = v[j];
			t[v[j].face].v[v[j].num] = num_optimized_vertex - 1;
			vbuf[v[j].id] = num_optimized_vertex - 1;
		}
		// create edges
		for(int j = 0; j < s->num_vertex; j += 3) {
			e[j + 0].v[0] = vbuf[j + 1];
			e[j + 0].v[1] = vbuf[j + 2];
			e[j + 1].v[0] = vbuf[j + 0];
			e[j + 1].v[1] = vbuf[j + 2];
			e[j + 2].v[0] = vbuf[j + 0];
			e[j + 2].v[1] = vbuf[j + 1];
			e[j + 0].face = e[j + 1].face = e[j + 2].face = j / 3;
			e[j + 0].num = 0;
			e[j + 1].num = 1;
			e[j + 2].num = 2;
		}
		delete vbuf;
		qsort(e,s->num_vertex,sizeof(cts_Edge),cts_edge_cmp);
		// create triangles
		for(int j = 0; j < s->num_vertex / 3; j++) {
			t[j].env[0] = t[j].env[1] = t[j].env[2] = -1;
			t[j].flag = 0;
		}
		// find triangle environment
		for(int j = 0, k = 0; j < s->num_vertex; j++) {
			if(j == 0 || cts_edge_cmp(&e[k],&e[j])) k = j;
			if(j != k) {
				t[e[j].face].env[e[j].num] = e[k].face;
				t[e[k].face].env[e[k].num] = e[j].face;
			}
		}
		delete e;
		// create triangle strips
		for(int j = 0; j < s->num_vertex / 3; j++) {
			if(t[j].v[0] == t[j].v[1] || t[j].v[1] == t[j].v[2] || t[j].v[2] == t[j].v[0]) t[j].flag = 1;
		}
		int *indices = s->indices;
		for(int j = 0; j < s->num_vertex / 3; j++) {
			if(!t[j].flag) {
				s->num_strips++;
				int env;
				for(env = 0; env < 3; env++) if(t[j].env[env] != -1 && t[t[j].env[env]].flag == 0) break;
				int *strip_length = indices++;
				*strip_length = 3;
				*indices++ = t[j].v[(0 + env) % 3];
				*indices++ = t[j].v[(1 + env) % 3];
				*indices++ = t[j].v[(2 + env) % 3];
				t[j].flag = 1;
				int k = j;
				while(1) {
					for(env = 0; env < 3; env++) {
						if(t[k].env[env] != -1 && t[t[k].env[env]].flag == 0) {
							int *v = t[t[k].env[env]].v;
							if(*(indices - 3) != v[0] && ((*(indices - 2) == v[2] && *(indices - 1) == v[1]) || (*(indices - 2) == v[1] && *(indices - 1) == v[2]))) {
								*indices++ = v[0];
								break;
							}
							if(*(indices - 3) != v[2] && ((*(indices - 2) == v[1] && *(indices - 1) == v[0]) || (*(indices - 2) == v[0] && *(indices - 1) == v[1]))) {
								*indices++ = v[2];
								break;
							}
							if(*(indices - 3) != v[1] && ((*(indices - 2) == v[0] && *(indices - 1) == v[2]) || (*(indices - 2) == v[2] && *(indices - 1) == v[0]))) {
								*indices++ = v[1];
								break;
							}
						}
					}
					if(env == 3) break;
					vec3 normal;
					normal.cross(v[*(indices - 3)].v.xyz - v[*(indices - 2)].v.xyz,v[*(indices - 4)].v.xyz - v[*(indices - 2)].v.xyz);
					normal.cross(normal,v[*(indices - 3)].v.xyz - v[*(indices - 2)].v.xyz);
					vec4 plane(normal,-(v[*(indices - 2)].v.xyz * normal));
					if((plane * v[*(indices - 4)].v.xyz) * (plane * v[*(indices - 1)].v.xyz) > 0.0) {
						indices--;
						break;
					};
					k = t[k].env[env];
					t[k].flag = 1;
					(*strip_length)++;
				}
				s->num_indices += (*strip_length) + 1;
			}
		}
		delete t;
		delete s->vertex;
		s->num_vertex = num_optimized_vertex;
		s->vertex = new Vertex[s->num_vertex];
		for(int j = 0; j < s->num_vertex; j++) s->vertex[j] = v[j].v;
		delete v;
		indices = new int[s->num_indices];
		for(int j = 0; j < s->num_indices; j++) indices[j] = s->indices[j];
		delete s->indices;
		s->indices = indices;
	}
}
