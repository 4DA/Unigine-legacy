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

#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glext.h>

#include "engine.h"
#include "particles.h"

vec3 Particles::OFF = vec3(0,0,1000000.0);

/*
 */
Particles::Particles(int num,const vec3 &pos,float speed,float rotation,const vec3 &force,float time,float radius,const vec4 &color) :
	num_particles(num), pos(pos), speed(speed), rotation(rotation), force(force), time(time), radius(radius), color(color) {
	
	xyz = new vec3[num_particles];
	speeds = new vec3[num_particles];
	rotations = new float[num_particles];
	times = new float[num_particles];
	
	for(int i = 0; i < num_particles; i++) {
		xyz[i] = OFF;
		speeds[i] = vec3(0,0,0);
		rotations[i] = 0;
		times[i] = (float)i / (float)num_particles * time;
	}
	
	num_vertex = num_particles * 4;
	vertex = new Vertex[num_vertex];
	
	for(int i = 0; i < num_vertex; i++) {
		vertex[i].xyz = xyz[i / 4];
		int j = i % 4;
		if(j == 0) vertex[i].attrib = vec4(-0.5,0.5,-radius,-radius);
		else if(j == 1) vertex[i].attrib = vec4(0.5,0.5,radius,-radius);
		else if(j == 2) vertex[i].attrib = vec4(0.5,-0.5,radius,radius);
		else if(j == 3) vertex[i].attrib = vec4(-0.5,-0.5,-radius,radius);
		vertex[i].color = vec4(0,0,0,0);
		vertex[i].sincos = vec2(0,0);
	}
}

Particles::~Particles() {
	delete xyz;
	delete speeds;
	delete rotations;
	delete times;
	delete vertex;
}

/*****************************************************************************/
/*                                                                           */
/* render                                                                    */
/*                                                                           */
/*****************************************************************************/

void Particles::update(float ifps) {
	min = vec3(1000000,1000000,1000000);
	max = vec3(-1000000,-1000000,-1000000);
	for(int i = 0; i < num_particles; i++) {
		int j = i * 4;
		speeds[i] += force * ifps;
		xyz[i] += speeds[i] * ifps;
		times[i] -= ifps;
		if(times[i] < 0) {
			xyz[i] = pos;
			speeds[i] = vec3(rand(),rand(),rand()) * speed;
			rotations[i] = rand() * rotation;
			times[i] += time;

			speeds[i] += force * ifps * 2.0f;
			xyz[i] += speeds[i] * ifps * 2.0f;
		}
		// update position
		vertex[j + 0].xyz = xyz[i];
		vertex[j + 1].xyz = vertex[j + 0].xyz;
		vertex[j + 2].xyz = vertex[j + 0].xyz;
		vertex[j + 3].xyz = vertex[j + 0].xyz;
		// update color
		vertex[j + 0].color = color * times[i] / time;
		vertex[j + 1].color = vertex[j + 0].color;
		vertex[j + 2].color = vertex[j + 0].color;
		vertex[j + 3].color = vertex[j + 0].color;
		// update rotation
		vertex[j + 0].sincos = vec2(sin(rotations[i] * times[i] * PI * 2.0),cos(rotations[i] * times[i] * PI * 2.0));
		vertex[j + 1].sincos = vertex[j + 0].sincos;
		vertex[j + 2].sincos = vertex[j + 0].sincos;
		vertex[j + 3].sincos = vertex[j + 0].sincos;
		// bound box
		if(xyz[i].z < OFF.z - 1000.0) {
			if(max.x < xyz[i].x) max.x = xyz[i].x;
			if(min.x > xyz[i].x) min.x = xyz[i].x;
			if(max.y < xyz[i].y) max.y = xyz[i].y;
			if(min.y > xyz[i].y) min.y = xyz[i].y;
			if(max.z < xyz[i].z) max.z = xyz[i].z;
			if(min.z > xyz[i].z) min.z = xyz[i].z;
		}
	}
	if(min.z > OFF.z - 1000.0) {
		max = OFF;
		min = OFF;
	} else {
		max += vec3(radius,radius,radius);
		min -= vec3(radius,radius,radius);
	}
	center = (min + max) / 2.0f;
}

/*
 */
void Particles::set(const vec3 &p) {
	pos = p;
}

void Particles::setForce(const vec3 &f) {
	force = f;
}

void Particles::setColor(const vec4 &c) {
	color = c;
}

/*****************************************************************************/
/*                                                                           */
/* render                                                                    */
/*                                                                           */
/*****************************************************************************/

int Particles::render() {
	glEnableVertexAttribArrayARB(0);
	glEnableVertexAttribArrayARB(1);
	glEnableVertexAttribArrayARB(2);
	glEnableVertexAttribArrayARB(3);
	glVertexAttribPointerARB(0,3,GL_FLOAT,0,sizeof(Vertex),vertex->xyz);
	glVertexAttribPointerARB(1,4,GL_FLOAT,0,sizeof(Vertex),vertex->attrib);
	glVertexAttribPointerARB(2,4,GL_FLOAT,0,sizeof(Vertex),vertex->color);
	glVertexAttribPointerARB(3,2,GL_FLOAT,0,sizeof(Vertex),vertex->sincos);
	glDrawArrays(GL_QUADS,0,num_vertex);
	glDisableVertexAttribArrayARB(3);
	glDisableVertexAttribArrayARB(2);
	glDisableVertexAttribArrayARB(1);
	glDisableVertexAttribArrayARB(0);
	return num_particles * 2;
}

/*****************************************************************************/
/*                                                                           */
/* IO functions                                                              */
/*                                                                           */
/*****************************************************************************/

const vec3 &Particles::getMin() {
	return min;
}

const vec3 &Particles::getMax() {
	return max;
}

const vec3 &Particles::getCenter() {
	return center;
}

float Particles::getRadius() {
	return (max - center).length();
}

/*****************************************************************************/
/*                                                                           */
/* normal law                                                                */
/*                                                                           */
/*****************************************************************************/

float Particles::rand() {
	return sqrt(-2.0 * log((float)::rand() / RAND_MAX)) * sin(2.0 * PI * (float)::rand() / RAND_MAX);
}
