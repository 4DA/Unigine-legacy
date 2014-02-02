/* Mirror
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
#include "frustum.h"
#include "mesh.h"
#include "material.h"
#include "pbuffer.h"
#include "texture.h"
#include "mirror.h"

int Mirror::counter = 0;
PBuffer *Mirror::pbuffers[3];

/*
 */
Mirror::Mirror(Mesh *mesh) : mesh(mesh), material(NULL) {
	
	pos = getCenter();
	
	if(mesh->getNumSurfaces() == 0) {
		fprintf(stderr,"Mirror::Mirror: bad mesh\n");
	} else {
		Mesh::Triangle *triangles = mesh->getTriangles(0);
		plane = triangles[0].plane;
	}
	
	for(int i = 0; i < 3; i++) mirror_texes[i] = new Texture(128 << i,128 << i,Texture::TEXTURE_2D,Texture::RGB | Texture::LINEAR | Texture::CLAMP);
	
	if(counter++ == 0) {
		for(int i =  0; i < 3; i++) {
			pbuffers[i] = new PBuffer(128 << i,128 << i,PBuffer::RGB | PBuffer::DEPTH | PBuffer::STENCIL);
			
			pbuffers[i]->enable();
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LESS);
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			pbuffers[i]->disable();
		}	
	}
}

Mirror::~Mirror() {
	if(--counter == 0) {
		for(int i = 0; i < 3; i++) delete pbuffers[i];
	}
}

/*
 */
int Mirror::bindMaterial(const char *name,Material *material) {
	this->material = material;
	return 1;
}

/*
 */
void Mirror::enable() {
	
	old_camera = Engine::camera;
	old_modelview = Engine::modelview;
	old_imodelview = Engine::imodelview;
	
	mat4 transform;
	transform.reflect(plane);
	
	Engine::modelview = Engine::modelview * transform;
	Engine::imodelview = Engine::modelview.inverse();
	
	// save modelview
	modelview = Engine::modelview;
	
	// select best resolution
	if(Engine::viewport[3] > 512) {
		pbuffer = pbuffers[2];
		mirror_tex = mirror_texes[2];
	} else if(Engine::viewport[3] > 256) {
		pbuffer = pbuffers[1];
		mirror_tex = mirror_texes[1];
	} else {
		pbuffer = pbuffers[0];
		mirror_tex = mirror_texes[0];
	}
	
	// activate pbuffer
	pbuffer->enable();
	glGetIntegerv(GL_VIEWPORT,Engine::viewport);
	
	// load matrixes
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(Engine::projection);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(Engine::modelview);
	
	if(Engine::modelview.det() < 0.0) glFrontFace(GL_CW);
	else glFrontFace(GL_CCW);
	
	// get camera
	Engine::camera = Engine::imodelview * vec3(0,0,0);
	
	// hehe :)
	Engine::camera.sector = pos.sector;
	
	// set frustum
	Engine::frustum->set(Engine::projection * Engine::modelview);
	
	// render scene
}

void Mirror::disable() {
	
	// mirror texture
	mirror_tex->bind();
	mirror_tex->copy();
	
	pbuffer->disable();
	glGetIntegerv(GL_VIEWPORT,Engine::viewport);
	
	// restore matrixes
	Engine::camera = old_camera;
	Engine::modelview = old_modelview;
	Engine::imodelview = old_imodelview;
	
	// restore frustum
	Engine::frustum->set(Engine::projection * Engine::modelview);
}

/*
 */
void Mirror::render() {
	
	if(!material) return;
	
	if(!material->enable()) return;
	
	if(material->blend) {
		if(Engine::num_visible_lights == 0) glDepthMask(GL_FALSE);
		else glDepthFunc(GL_LESS);
	}
	
	material->bind();
	
	// <texture0> - mirror texture
	material->bindTexture(0,mirror_tex);
	
	glMatrixMode(GL_TEXTURE);
	glTranslatef(0.5,0.5,0);
	glScalef(0.5,0.5,0);
	glMultMatrixf(Engine::projection);
	glMultMatrixf(modelview);
	
	glDisable(GL_CULL_FACE);
	mesh->render(true);
	glEnable(GL_CULL_FACE);
	
	if(material->blend) {
		if(Engine::num_visible_lights == 0) glDepthMask(GL_TRUE);
		else glDepthFunc(GL_EQUAL);
	}
	
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
}

/*
 */
const vec3 &Mirror::getMin() {
	return mesh->getMin();
}

const vec3 &Mirror::getMax() {
	return mesh->getMax();
}

const vec3 &Mirror::getCenter() {
	return mesh->getCenter();
}

float Mirror::getRadius() {
	return mesh->getRadius();
}
