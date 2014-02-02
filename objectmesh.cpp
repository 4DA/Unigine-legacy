/* ObjectMesh
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

#include "frustum.h"
#include "material.h"
#include "mesh.h"
#include "engine.h"
#include "object.h"
#include "objectmesh.h"

ObjectMesh::ObjectMesh(Mesh *mesh) : Object(OBJECT_MESH), mesh(mesh) {
	materials = new Material*[getNumSurfaces()];
	opacities = new int[getNumSurfaces()];
	transparents = new int[getNumSurfaces()];
	frames = new int[getNumSurfaces()];
}

ObjectMesh::ObjectMesh(const char *name) : Object(OBJECT_MESH) {
	mesh = Engine::loadMesh(name);
	materials = new Material*[getNumSurfaces()];
	opacities = new int[getNumSurfaces()];
	transparents = new int[getNumSurfaces()];
	frames = new int[getNumSurfaces()];
}

ObjectMesh::~ObjectMesh() {
	delete materials;
	delete opacities;
	delete transparents;
	delete frames;
}

/*
 */
int ObjectMesh::render(int t,int s) {
	int num_triangles = 0;
	enable();
	if(frame != Engine::frame) {
		if(is_identity) {
			for(int i = 0; i < getNumSurfaces(); i++) {
				frames[i] = Engine::frustum->inside(pos + getMin(i),pos + getMax(i)) ? Engine::frame : 0;
			}
		} else {
			for(int i = 0; i < getNumSurfaces(); i++) {
				frames[i] = Engine::frustum->inside(pos + getCenter(i),getRadius(i)) ? Engine::frame : 0;
			}
		}
		frame = Engine::frame;
	}
	if(t == RENDER_ALL) {
		for(int i = 0; i < getNumSurfaces(); i++) {
			if(frames[i] != Engine::frame) continue;
			num_triangles += mesh->render(false,i);
		}
	} else if(t == RENDER_OPACITY) {
		for(int i = 0; i < num_opacities; i++) {
			int j = opacities[i];
			if(frames[j] != Engine::frame) continue;
			if(!materials[j]->enable()) continue;
			materials[j]->bind();
			num_triangles += mesh->render(true,j);
		}
	} else if(t == RENDER_TRANSPARENT) {
		for(int i = 0; i < num_transparents; i++) {
			int j = transparents[i];
			if(frames[j] != Engine::frame) continue;
			if(!materials[j]->enable()) continue;
			materials[j]->bind();
			num_triangles += mesh->render(true,j);
		}
	}
	disable();
	return num_triangles;
}

/*
 */
void ObjectMesh::findSilhouette(const vec4 &light,int s) {
	mesh->findSilhouette(light,s);
}

int ObjectMesh::getNumIntersections(const vec3 &line0,const vec3 &line1,int s) {
	return mesh->getNumIntersections(line0,line1,s);
}

int ObjectMesh::renderShadowVolume(int s) {
	return mesh->renderShadowVolume(s);
}

/*
 */
int ObjectMesh::intersection(const vec3 &line0,const vec3 &line1,vec3 &point,vec3 &normal,int s) {
	return mesh->intersection(line0,line1,point,normal,s);
}

/*
 */
int ObjectMesh::getNumSurfaces() {
	return mesh->getNumSurfaces();
}

const char *ObjectMesh::getSurfaceName(int s) {
	return mesh->getSurfaceName(s);
}

int ObjectMesh::getSurface(const char *name) {
	return mesh->getSurface(name);
}

/*
 */
const vec3 &ObjectMesh::getMin(int s) {
	return mesh->getMin(s);
}

const vec3 &ObjectMesh::getMax(int s) {
	return mesh->getMax(s);
}

const vec3 &ObjectMesh::getCenter(int s) {
	return mesh->getCenter(s);
}

float ObjectMesh::getRadius(int s) {
	return mesh->getRadius(s);
}
