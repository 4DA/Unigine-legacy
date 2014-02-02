/* ObjectParticles
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
#include "material.h"
#include "particles.h"
#include "object.h"
#include "fog.h"
#include "objectparticles.h"

ObjectParticles::ObjectParticles(Particles *particles) : Object(OBJECT_PARTICLES), particles(particles), off_time(-1.0f) {
	materials = new Material*[getNumSurfaces()];
	opacities = new int[getNumSurfaces()];
	transparents = new int[getNumSurfaces()];
	pos = Particles::OFF;
}

ObjectParticles::~ObjectParticles() {
	delete materials;
	delete opacities;
	delete transparents;
}

/*
 */
void ObjectParticles::update(float ifps) {
	Object::update(ifps);
	
	if(off_time > 0.0 && time > off_time) particles->set(Particles::OFF);
	else particles->set(pos);
	
	particles->update(ifps);
	
	min = particles->getMin() - pos;
	max = particles->getMax() - pos;
	center = particles->getCenter() - pos;
	
	updatePos(pos);
}

/*
 */
void ObjectParticles::setOffTime(float time) {
	this->time = 0;
	off_time = time;
}

/*
 */
int ObjectParticles::render(int t,int s) {
	int num_triangles = 0;
	frame = Engine::frame;
	if(t == RENDER_TRANSPARENT) {
		if(num_transparents) {
			if(materials[0]->enable()) {
				float min_density = 1.0;
				Engine::fog_color = vec4(1.0,1.0,1.0,1.0);	// without fog
				for(int i = 0; i < Engine::num_visible_fogs; i++) {
					float density = Engine::visible_fogs[i]->getDensity(pos);
					if(min_density > density) {
						Engine::fog_color = vec4(Engine::visible_fogs[i]->color,density);
						min_density = density;
					}
				}
				materials[0]->bind();
				num_triangles = particles->render();
			}
		}
	}
	return num_triangles;
}

/*
 */
void ObjectParticles::findSilhouette(const vec4 &light,int s) {

}

int ObjectParticles::getNumIntersections(const vec3 &line0,const vec3 &line1,int s) {
	return 0;
}

int ObjectParticles::renderShadowVolume(int s) {
	return 0;
}

/*
 */
int ObjectParticles::intersection(const vec3 &line0,const vec3 &line1,vec3 &point,vec3 &normal,int s) {
	return 0;
}

/*
 */
int ObjectParticles::getNumSurfaces() {
	return 1;
}

const char *ObjectParticles::getSurfaceName(int s) {
	return "particles";
}

int ObjectParticles::getSurface(const char *name) {
	return 0;
}

/*
 */
const vec3 &ObjectParticles::getMin(int s) {
	return min;
}

const vec3 &ObjectParticles::getMax(int s) {
	return max;
}

const vec3 &ObjectParticles::getCenter(int s) {
	return center;
}

float ObjectParticles::getRadius(int s) {
	return particles->getRadius();
}
