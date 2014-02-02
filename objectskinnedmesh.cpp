/* ObjectSkinnedMesh
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
#include "material.h"
#include "skinnedmesh.h"
#include "ragdoll.h"
#include "engine.h"
#include "object.h"
#include "objectskinnedmesh.h"

float ObjectSkinnedMesh::skin_time_step = 1.0f / 25.0f;

/*
 */
ObjectSkinnedMesh::ObjectSkinnedMesh(SkinnedMesh *skinnedmesh) : Object(OBJECT_SKINNEDMESH), skinnedmesh(skinnedmesh), ragdoll(NULL), skin_time(0.0) {
	materials = new Material*[getNumSurfaces()];
	opacities = new int[getNumSurfaces()];
	transparents = new int[getNumSurfaces()];
	frames = new int[getNumSurfaces()];
}

ObjectSkinnedMesh::ObjectSkinnedMesh(const char *name) : Object(OBJECT_SKINNEDMESH), ragdoll(NULL), skin_time(0.0)  {
	skinnedmesh = new SkinnedMesh(Engine::findFile(name));
	materials = new Material*[getNumSurfaces()];
	opacities = new int[getNumSurfaces()];
	transparents = new int[getNumSurfaces()];
	frames = new int[getNumSurfaces()];
}

ObjectSkinnedMesh::~ObjectSkinnedMesh() {
	delete materials;
	delete opacities;
	delete transparents;
	delete frames;
}

/*
 */
void ObjectSkinnedMesh::update(float ifps) {
	Object::update(ifps);
	
	if(ragdoll) {
		ragdoll->update();
	} else {
		skinnedmesh->setFrame(time * 10);
	}
	
	skin_time += ifps;
	
	if(skin_time > skin_time_step) {
		skinnedmesh->calculateSkin();
		while(skin_time > skin_time_step) skin_time -= skin_time_step;
	}
}

/*
 */
int ObjectSkinnedMesh::render(int t,int s) {
	int num_triangles = 0;
	enable();
	if(frame != Engine::frame) {
		for(int i = 0; i < getNumSurfaces(); i++) {
			frames[i] = Engine::frustum->inside(pos + getMin(i),pos + getMax(i)) ? Engine::frame : 0;
		}
		frame = Engine::frame;
	}
	if(t == RENDER_ALL) {
		for(int i = 0; i < getNumSurfaces(); i++) {
			if(frames[i] != Engine::frame) continue;
			num_triangles += skinnedmesh->render(false,i);
		}
	} else if(t == RENDER_OPACITY) {
		for(int i = 0; i < num_opacities; i++) {
			int j = opacities[i];
			if(frames[j] != Engine::frame) continue;
			if(!materials[j]->enable()) continue;
			materials[j]->bind();
			num_triangles += skinnedmesh->render(true,j);
		}
	} else if(t == RENDER_TRANSPARENT) {
		for(int i = 0; i < num_transparents; i++) {
			int j = transparents[i];
			if(frames[j] != Engine::frame) continue;
			if(!materials[j]->enable()) continue;
			materials[j]->bind();
			num_triangles += skinnedmesh->render(true,j);
		}
	}
	disable();
	return num_triangles;
}

/*
 */
void ObjectSkinnedMesh::findSilhouette(const vec4 &light,int s) {
	skinnedmesh->findSilhouette(light,s);
}

int ObjectSkinnedMesh::getNumIntersections(const vec3 &line0,const vec3 &line1,int s) {
	return skinnedmesh->getNumIntersections(line0,line1,s);
}

int ObjectSkinnedMesh::renderShadowVolume(int s) {
	return skinnedmesh->renderShadowVolume(s);
}

/*
 */
int ObjectSkinnedMesh::intersection(const vec3 &line0,const vec3 &line1,vec3 &point,vec3 &normal,int s) {
	if(ragdoll) return 0;
	return skinnedmesh->intersection(line0,line1,point,normal,s);
}

/*
 */
int ObjectSkinnedMesh::getNumSurfaces() {
	return skinnedmesh->getNumSurfaces();
}

const char *ObjectSkinnedMesh::getSurfaceName(int s) {
	return skinnedmesh->getSurfaceName(s);
}

int ObjectSkinnedMesh::getSurface(const char *name) {
	return skinnedmesh->getSurface(name);
}

/*
 */
int ObjectSkinnedMesh::getNumBones() {
	return skinnedmesh->getNumBones();
}

const char *ObjectSkinnedMesh::getBoneName(int b) {
	return skinnedmesh->getBoneName(b);
}

int ObjectSkinnedMesh::getBone(const char *name) {
	return skinnedmesh->getBone(name);
}

const mat4 &ObjectSkinnedMesh::getBoneTransform(int b) {
	return skinnedmesh->getBoneTransform(b);
}

/*
 */
const vec3 &ObjectSkinnedMesh::getMin(int s) {
	return skinnedmesh->getMin(s);
}

const vec3 &ObjectSkinnedMesh::getMax(int s) {
	return skinnedmesh->getMax(s);
}

const vec3 &ObjectSkinnedMesh::getCenter(int s) {
	return skinnedmesh->getCenter(s);
}

float ObjectSkinnedMesh::getRadius(int s) {
	return skinnedmesh->getRadius(s);
}

/*
 */
void ObjectSkinnedMesh::setRagDoll(RagDoll *ragdoll) {
	ragdoll->setTransform(transform);
	this->ragdoll = ragdoll;
}

/*
 */
void ObjectSkinnedMesh::set(const vec3 &p) {
	mat4 m;
	m.translate(p);
	if(ragdoll) ragdoll->set(m);
	else Object::set(m);
}

void ObjectSkinnedMesh::set(const mat4 &m) {
	if(ragdoll) ragdoll->set(m);
	else Object::set(m);
}
