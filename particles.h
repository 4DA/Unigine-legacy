/* Particles
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

#ifndef __PARTICLES_H__
#define __PARTICLES_H__

#include "mathlib.h"

class Particles {
public:
	
	Particles(int num,const vec3 &pos,float speed,float rotation,const vec3 &force,float time,float radius,const vec4 &color);
	~Particles();
	
	void update(float ifps);
	
	int render();
	
	void set(const vec3 &p);
	void setForce(const vec3 &f);
	void setColor(const vec4 &c);
	
	const vec3 &getMin();
	const vec3 &getMax();
	const vec3 &getCenter();
	float getRadius();
	
	static vec3 OFF;
	
protected:
	
	float rand();
	
	int num_particles;	// number of particles
	
	vec3 pos;
	float speed;		// speed
	float rotation;		// rotation
	vec3 force;			// force / mass
	float time;			// life time
	float radius;		// radius
	vec4 color;			// color
	
	vec3 *xyz;			// positions
	vec3 *speeds;		// speeds
	float *rotations;	// rotations
	float *times;		// times
	
	struct Vertex {
		vec3 xyz;		// coordinate
		vec4 attrib;	// attributes (texcoord + dx + dy)
		vec4 color;		// color
		vec2 sincos;	// sin(rotation) + cos(rotation)
	};
	
	int num_vertex;
	Vertex *vertex;
	
	vec3 min;			// bound box
	vec3 max;
	vec3 center;
};

#endif /* __PARTICLES_H__ */
