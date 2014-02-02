/* Frustum
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
#include "bsp.h"
#include "frustum.h"

Frustum::Frustum() : num_planes(6), depth(0) {
	planes = new vec4[6 + DEPTH * 5];
}

Frustum::~Frustum() {
	delete planes;
}

/*
 */
void Frustum::get() {
	set(Engine::projection * Engine::modelview);
}

void Frustum::set(const mat4 &m) {
#define PLANE(n,c0,c1,c2,c3) { \
	planes[n] = vec4(c0,c1,c2,c3); \
	planes[n] *= 1.0f / vec3(planes[n]).length(); \
}
	PLANE(0,m[3]-m[0],m[7]-m[4],m[11]-m[8],m[15]-m[12])
	PLANE(1,m[3]+m[0],m[7]+m[4],m[11]+m[8],m[15]+m[12])
	PLANE(2,m[3]-m[1],m[7]-m[5],m[11]-m[9],m[15]-m[13])
	PLANE(3,m[3]+m[1],m[7]+m[5],m[11]+m[9],m[15]+m[13])
	PLANE(4,m[3]-m[2],m[7]-m[6],m[11]-m[10],m[15]-m[14])
	PLANE(5,m[3]+m[2],m[7]+m[6],m[11]+m[10],m[15]+m[14])
#undef PLANE
	num_planes = 6;
}

/*
 */
void Frustum::addPortal(const vec3 &point,const vec3 *points) {
	depth++;
	if(depth > DEPTH) {
		fprintf(stderr,"Frustum::addPortal(): stack overflow\n");
		return;
	}
#define PLANE(n,v0,v1,v2) { \
	vec3 normal; \
	normal.cross(v1 - v0,v2 - v0); \
	normal.normalize(); \
	planes[n] = vec4(normal,-normal * v0); \
}
	if((Engine::camera - points[0]) * cross(points[1] - points[0],points[2] - points[0]) < 0.0) {
		PLANE(num_planes + 0,point,points[0],points[1]);
		PLANE(num_planes + 1,point,points[1],points[2]);
		PLANE(num_planes + 2,point,points[2],points[3]);
		PLANE(num_planes + 3,point,points[3],points[0]);
		PLANE(num_planes + 4,points[0],points[1],points[2]);
	} else {
		PLANE(num_planes + 0,point,points[0],points[3]);
		PLANE(num_planes + 1,point,points[3],points[2]);
		PLANE(num_planes + 2,point,points[2],points[1]);
		PLANE(num_planes + 3,point,points[1],points[0]);
		PLANE(num_planes + 4,points[2],points[1],points[0]);
	}
#undef PLANE
	num_planes += 5;
}

void Frustum::removePortal() {
	if(depth > 0) {
		depth--;
		num_planes -= 5;
	}
}

/*****************************************************************************/
/*                                                                           */
/* inside                                                                    */
/*                                                                           */
/*****************************************************************************/

/*
 */
int Frustum::inside(const vec3 &min,const vec3 &max) {
	for(int i = 0; i < num_planes; i++) {
		if(planes[i] * vec4(min[0],min[1],min[2],1) > 0) continue;
		if(planes[i] * vec4(min[0],min[1],max[2],1) > 0) continue;
		if(planes[i] * vec4(min[0],max[1],min[2],1) > 0) continue;
		if(planes[i] * vec4(min[0],max[1],max[2],1) > 0) continue;
		if(planes[i] * vec4(max[0],min[1],min[2],1) > 0) continue;
		if(planes[i] * vec4(max[0],min[1],max[2],1) > 0) continue;
		if(planes[i] * vec4(max[0],max[1],min[2],1) > 0) continue;
		if(planes[i] * vec4(max[0],max[1],max[2],1) > 0) continue;
		return 0;
	}
	return 1;
}

int Frustum::inside(const vec3 &center,float radius) {
	for(int i = 0; i < num_planes; i++) {
		if(planes[i] * vec4(center,1) < -radius) return 0;
	}
	return 1;
}

int Frustum::inside(const vec3 *points,int num) {
	for(int i = 0; i < num_planes; i++) {
		int j = 0;
		for(; j < num; j++) if(planes[i] * vec4(points[j],1) > 0) break;
		if(j == num) return 0;
	}
	return 1;
}

int Frustum::inside(const vec3 &light,float light_radius,const vec3 &center,float radius) {
	vec3 dir = center - light;
	float length = dir.length();
	if(length < radius) return 1;
	if(length > radius + light_radius) return 0;
	vec3 x,y,c,points[8];
	dir.normalize();
	if(fabs(dir.z) > 1.0 - EPSILON) {
		x.cross(dir,vec3(1,0,0));
		x.normalize();
		y.cross(dir,x);
		y.normalize();
	} else {
		x.cross(dir,vec3(0,0,1));
		x.normalize();
		y.cross(dir,x);
		y.normalize();
	}
	float size = radius * (length - radius) / length;
	x *= size;
	y *= size;
	c = light + dir * (length - radius);
	points[0] = x + y + c;
	points[1] = x - y + c;
	points[2] = -x + y + c;
	points[3] = -x - y + c;
	size = light_radius / (length - radius);
	x *= size;
	y *= size;
	c = light + dir * light_radius;
	points[4] = x + y + c;
	points[5] = x - y + c;
	points[6] = -x + y + c;
	points[7] = -x - y + c;
	return inside(points,8);
}

/*
 */
int Frustum::inside_all(const vec3 &min,const vec3 &max) {
	for(int i = 0; i < num_planes; i++) {
		if(planes[i] * vec4(min[0],min[1],min[2],1) < 0) return 0;
		if(planes[i] * vec4(min[0],min[1],max[2],1) < 0) return 0;
		if(planes[i] * vec4(min[0],max[1],min[2],1) < 0) return 0;
		if(planes[i] * vec4(min[0],max[1],max[2],1) < 0) return 0;
		if(planes[i] * vec4(max[0],min[1],min[2],1) < 0) return 0;
		if(planes[i] * vec4(max[0],min[1],max[2],1) < 0) return 0;
		if(planes[i] * vec4(max[0],max[1],min[2],1) < 0) return 0;
		if(planes[i] * vec4(max[0],max[1],max[2],1) < 0) return 0;
	}
	return 1;
}

int Frustum::inside_all(const vec3 &center,float radius) {
	for(int i = 0; i < num_planes; i++) {
		if(planes[i] * vec4(center,1) < radius) return 0;
	}
	return 1;
}
