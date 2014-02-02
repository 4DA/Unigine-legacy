/* Light
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
#include "flare.h"
#include "light.h"

Light::Light(const vec3 &pos,float radius,const vec4 &color,int shadows) : radius(radius), color(color), shadows(shadows), material(NULL), flare(NULL), time(0.0) {
	set(pos);
}

Light::~Light() {
	if(flare) delete flare;
}

/*
 */
void Light::update(float ifps) {
	time += ifps;
	pos.update(time,transform);
}

/*
 */
int Light::bindMaterial(const char *name,Material *material) {
	this->material = material;
	return 1;
}

/*
 */
void Light::setFlare(Flare *flare) {
	this->flare = flare;
}

/*
 */
void Light::renderFlare() {
	if(flare) flare->render(pos,color);
}

/*
 */
void Light::set(const vec3 &p) {
	transform.translate(p);
	pos.radius = radius;
	pos = p;
}

/*
 */
void Light::set(const mat4 &m) {
	transform = m;
	pos.radius = radius;
	pos = m * vec3(0,0,0);
}

/*
 */
void Light::setColor(const vec4 &c) {
	color = c;
}

/*
 */
void Light::getScissor(int *scissor) {
	if((pos - Engine::camera).length() < radius * 1.5) {
		scissor[0] = Engine::viewport[0];
		scissor[1] = Engine::viewport[1];
		scissor[2] = Engine::viewport[2];
		scissor[3] = Engine::viewport[3];
		return;
	}
	mat4 tmodelview = Engine::modelview.transpose();
	mat4 mvp = Engine::projection * Engine::modelview;
	vec3 x = tmodelview * vec3(radius,0,0);
	vec3 y = tmodelview * vec3(0,radius,0);
	vec4 p[4];
	p[0] = mvp * vec4(pos - x,1);
	p[1] = mvp * vec4(pos + x,1);
	p[2] = mvp * vec4(pos - y,1);
	p[3] = mvp * vec4(pos + y,1);
	p[0] /= p[0].w;
	p[1] /= p[1].w;
	p[2] /= p[2].w;
	p[3] /= p[3].w;
	if(p[0].x < p[2].x) {
		scissor[0] = Engine::viewport[0] + (int)((float)Engine::viewport[2] * (p[0].x + 1.0) / 2.0);
		scissor[2] = Engine::viewport[0] + (int)((float)Engine::viewport[2] * (p[1].x + 1.0) / 2.0);
	} else {
		scissor[0] = Engine::viewport[0] + (int)((float)Engine::viewport[2] * (p[1].x + 1.0) / 2.0);
		scissor[2] = Engine::viewport[0] + (int)((float)Engine::viewport[2] * (p[0].x + 1.0) / 2.0);
	}
	if(p[1].y < p[3].y) {
		scissor[1] = Engine::viewport[1] + (int)((float)Engine::viewport[3] * (p[2].y + 1.0) / 2.0);
		scissor[3] = Engine::viewport[1] + (int)((float)Engine::viewport[3] * (p[3].y + 1.0) / 2.0);
	} else {
		scissor[1] = Engine::viewport[1] + (int)((float)Engine::viewport[3] * (p[3].y + 1.0) / 2.0);
		scissor[3] = Engine::viewport[1] + (int)((float)Engine::viewport[3] * (p[2].y + 1.0) / 2.0);
	}
	if(scissor[0] < Engine::viewport[0]) scissor[0] = Engine::viewport[0];
	else if(scissor[0] > Engine::viewport[0] + Engine::viewport[2]) scissor[0] = Engine::viewport[0] + Engine::viewport[2];
	if(scissor[1] < Engine::viewport[1]) scissor[1] = Engine::viewport[1];
	else if(scissor[1] > Engine::viewport[1] + Engine::viewport[3]) scissor[1] = Engine::viewport[1] + Engine::viewport[3];
	if(scissor[2] < Engine::viewport[0]) scissor[2] = Engine::viewport[0];
	else if(scissor[2] > Engine::viewport[2] + Engine::viewport[3]) scissor[2] = Engine::viewport[0] + Engine::viewport[2];
	if(scissor[3] < Engine::viewport[1]) scissor[3] = Engine::viewport[1];
	else if(scissor[3] > Engine::viewport[1] + Engine::viewport[3]) scissor[3] = Engine::viewport[1] + Engine::viewport[3];
	scissor[2] -= scissor[0];
	scissor[3] -= scissor[1];
}
