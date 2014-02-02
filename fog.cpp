/* Fog
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
#include "pbuffer.h"
#include "texture.h"
#include "shader.h"
#include "engine.h"
#include "fog.h"

int Fog::counter = 0;
PBuffer *Fog::pbuffer;
Texture *Fog::depth_tex;
Texture *Fog::fog_tex;
PBuffer *Fog::pbuffers[3];
Texture *Fog::depth_texes[3];
Texture *Fog::fog_texes[3];
Shader *Fog::depth_to_rgb_shader;
Shader *Fog::pass_shader;
Shader *Fog::fail_shader;
Shader *Fog::final_shader;

/*
 */
Fog::Fog(Mesh *mesh,const vec4 &color) : mesh(mesh), color(color) {
	
	pos.radius = getRadius();
	pos = getCenter();
	
	if(counter++ == 0) {	// loading global objects
		
		for(int i = 0; i < 3; i++) {
			pbuffers[i] = new PBuffer(128 << i,128 << i,PBuffer::RGBA | PBuffer::DEPTH | PBuffer::STENCIL);
			
			pbuffers[i]->enable();
			glClearColor(0,0,0,1);
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_CULL_FACE);
			pbuffers[i]->disable();
			
			depth_texes[i] = new Texture(128 << i,128 << i,Texture::TEXTURE_2D,Texture::RGB | Texture::NEAREST);
			fog_texes[i] = new Texture(128 << i,128 << i,Texture::TEXTURE_2D,Texture::RGBA | Texture::LINEAR);
		}
		
		depth_to_rgb_shader = Engine::loadShader(FOG_DEPTH_TO_RGB);
		pass_shader = Engine::loadShader(FOG_PASS);
		fail_shader = Engine::loadShader(FOG_FAIL);
		final_shader = Engine::loadShader(FOG_FINAL);
	}
}

Fog::~Fog() {
	if(--counter == 0) {
		for(int i = 0; i < 3; i++) {
			delete pbuffers[i];
			delete depth_texes[i];
			delete fog_texes[i];
		}
		delete depth_to_rgb_shader;
		delete pass_shader;
		delete fail_shader;
	}
}

/*
 */
void Fog::enable() {
	
	// give color to shader
	Engine::fog_color = color;
	
	// select best resolution
	// depend on viewport size
	if(Engine::viewport[3] > 512) {
		pbuffer = pbuffers[2];
		depth_tex = depth_texes[2];
		fog_tex = fog_texes[2];
	} else if(Engine::viewport[2] > 256) {
		pbuffer = pbuffers[1];
		depth_tex = depth_texes[1];
		fog_tex = fog_texes[1];
	} else {
		pbuffer = pbuffers[0];
		depth_tex = depth_texes[0];
		fog_tex = fog_texes[0];
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
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	depth_to_rgb_shader->enable();
	depth_to_rgb_shader->bind();
	
	// render scene without materials (one pass)
}

void Fog::disable() {
	
	// render back triangles
	glCullFace(GL_FRONT);
	Engine::num_triangles += mesh->render(true);
	glCullFace(GL_BACK);
	depth_to_rgb_shader->disable();
	
	// save depth values
	depth_tex->bind();
	depth_tex->copy();
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	
	// fog pass
	pass_shader->enable();
	pass_shader->bind();
	depth_tex->bind();
	Engine::num_triangles += mesh->render(true);
	pass_shader->disable();
	
	// find fog fail area
	glEnable(GL_STENCIL_TEST);
	glDepthFunc(GL_ALWAYS);
	
	glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
	glStencilFunc(GL_ALWAYS,1,~0);
	glStencilOp(GL_KEEP,GL_KEEP,GL_INCR);
	glCullFace(GL_FRONT);
	Engine::num_triangles += mesh->render();
	glStencilOp(GL_KEEP,GL_KEEP,GL_DECR);
	glCullFace(GL_BACK);
	Engine::num_triangles += mesh->render();
	glStencilFunc(GL_EQUAL,1,~0);
	glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);
	glCullFace(GL_FRONT);
	glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
	
	// fog fail
	fail_shader->enable();
	fail_shader->bind();
	depth_tex->bind();
	Engine::num_triangles += mesh->render(true);
	fail_shader->disable();
	
	glCullFace(GL_BACK);
	glDepthFunc(GL_LESS);
	glDisable(GL_STENCIL_TEST);
	
	fog_tex->bind();
	fog_tex->copy();
	pbuffer->disable();
	glGetIntegerv(GL_VIEWPORT,Engine::viewport);
}

/*
 */
int Fog::render() {
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE_MINUS_SRC_ALPHA,GL_SRC_ALPHA);
	
	final_shader->enable();
	final_shader->bind();
	fog_tex->bind();
	
	int ret = mesh->render(true);
	
	// find fog fail area
	glEnable(GL_STENCIL_TEST);
	glDepthFunc(GL_ALWAYS);
	
	glClear(GL_STENCIL_BUFFER_BIT);
	
	glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
	glStencilFunc(GL_ALWAYS,1,~0);
	glStencilOp(GL_KEEP,GL_KEEP,GL_INCR);
	glCullFace(GL_FRONT);
	ret += mesh->render(true);
	glStencilOp(GL_KEEP,GL_KEEP,GL_DECR);
	glCullFace(GL_BACK);
	ret += mesh->render(true);
	glStencilFunc(GL_EQUAL,1,~0);
	glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);
	glCullFace(GL_FRONT);
	glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
	
	// fog fail
	ret += mesh->render(false);
	
	glCullFace(GL_BACK);
	glDepthFunc(GL_LESS);
	glDisable(GL_STENCIL_TEST);
	
	final_shader->disable();
	
	glDisable(GL_BLEND);
	
	return ret;
}

/*
 */
float Fog::getDensity(const vec3 &point) {
	float dist = 0;
	vec3 p0,n0,p1,n1;
	vec3 dir = point - Engine::camera;
	dir.normalize();
	if(mesh->intersection(Engine::camera,point,p0,n0)) {
		if(mesh->intersection(p0 + dir * 0.001,point,p1,n1)) {	// two intersections
			dist = (p1 - p0).length();
		} else {	// one intersection
			if(inside(point)) dist = (point - p0).length();
			else dist = (p0 - Engine::camera).length();
		}
	} else {	// zero intersections
		// but camera and point in the fog
		if(inside(Engine::camera)) dist = (point - Engine::camera).length();
	}
	return pow(2.0,-dist * color.w / 256.0 * 64.0);
}

int Fog::inside(const vec3 &point) {
	for(int i = 0; i < mesh->getNumSurfaces(); i++) {
		int num_triangles = mesh->getNumTriangles(i);
		Mesh::Triangle *triangles = mesh->getTriangles(i);
		for(int j = 0; j < num_triangles; j++) {
			if(triangles[j].plane * vec4(point,1) > 0.0) return 0;
		}
	}
	return 1;
}

/*
 */
const vec3 &Fog::getMin() {
	return mesh->getMin();
}

const vec3 &Fog::getMax() {
	return mesh->getMax();
}

const vec3 &Fog::getCenter() {
	return mesh->getCenter();
}

float Fog::getRadius() {
	return mesh->getRadius();
}
