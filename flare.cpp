/* Light Flares
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
#include "texture.h"
#include "mesh.h"
#include "flare.h"

int Flare::counter = 0;
Texture *Flare::flare_tex;

/*
 */
Flare::Flare(float min_radius,float max_radius,float sphere_radius) : min_radius(min_radius), max_radius(max_radius), sphere_radius(sphere_radius), time(0) {
	
	if(counter++ == 0) {
		flare_tex = new Texture(Engine::findFile(FLARE_TEX),Texture::TEXTURE_2D,Texture::RGB | Texture::LINEAR_MIPMAP_LINEAR | Texture::CLAMP);
	}
}

Flare::~Flare() {
	if(--counter == 0) {
		delete flare_tex;
	}
}

/*
 */
void Flare::render(const vec3 &pos,const vec4 &color) {
	if(Engine::have_occlusion == 0) return;
	
	time += Engine::ifps;	// manualy update time
	
	glDepthMask(GL_FALSE);
	glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
	
	glDepthFunc(GL_ALWAYS);
	
	glPushMatrix();
	glTranslatef(pos.x,pos.y,pos.z);
	glScalef(sphere_radius,sphere_radius,sphere_radius);
	glBeginQueryARB(GL_SAMPLES_PASSED_ARB,Engine::query_id);
	Engine::sphere_mesh->render(false);
	glEndQueryARB(GL_SAMPLES_PASSED_ARB);
	glPopMatrix();
	
	GLuint samples_0;
	glGetQueryObjectuivARB(Engine::query_id,GL_QUERY_RESULT_ARB,&samples_0);
	
	glDepthFunc(GL_LESS);
	
	if(samples_0 == 0) {
		glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
		glDepthMask(GL_TRUE);
		return;
	}
	
	glPushMatrix();
	glTranslatef(pos.x,pos.y,pos.z);
	glScalef(sphere_radius,sphere_radius,sphere_radius);
	glBeginQueryARB(GL_SAMPLES_PASSED_ARB,Engine::query_id);
	Engine::sphere_mesh->render(false);
	glEndQueryARB(GL_SAMPLES_PASSED_ARB);
	glPopMatrix();
	
	GLuint samples_1;
	glGetQueryObjectuivARB(Engine::query_id,GL_QUERY_RESULT_ARB,&samples_1);
	
	glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
	glDepthMask(GL_TRUE);
	
	if(samples_1 == 0) return;
	
	glDepthFunc(GL_ALWAYS);
	
	float radius = min_radius + (max_radius - min_radius) * (float)samples_1 / (float)samples_0;
	if(radius < 0) radius = 0;
	
	vec3 dx = Engine::imodelview.rotation() * vec3(1,0,0);
	vec3 dy = Engine::imodelview.rotation() * vec3(0,1,0);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE,GL_ONE);
	vec3 c(color);
	c.normalize();
	glColor3fv(c * (float)samples_1 / (float)samples_0 / 1.5f);
	flare_tex->enable();
	flare_tex->bind();
	
	glMatrixMode(GL_TEXTURE);
	
	glTranslatef(0.5,0.5,0);
	glRotatef(time * 360.0f / 16.0f,0,0,1);
	glTranslatef(-0.5,-0.5,0);
	
	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2f(0,1);
	glVertex3fv(pos - (dx + dy) * radius);
	glTexCoord2f(1,1);
	glVertex3fv(pos + (dx - dy) * radius);
	glTexCoord2f(0,0);
	glVertex3fv(pos - (dx - dy) * radius);
	glTexCoord2f(1,0);
	glVertex3fv(pos + (dx + dy) * radius);
	glEnd();
	
	glLoadIdentity();
	
	glTranslatef(0.5,0.5,0);
	glRotatef(-time * 360.0f / 16.0f,0,0,1);
	glTranslatef(-0.5,-0.5,0);
	
	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2f(0,1);
	glVertex3fv(pos - (dx + dy) * radius);
	glTexCoord2f(1,1);
	glVertex3fv(pos + (dx - dy) * radius);
	glTexCoord2f(0,0);
	glVertex3fv(pos - (dx - dy) * radius);
	glTexCoord2f(1,0);
	glVertex3fv(pos + (dx + dy) * radius);
	glEnd();
	
	glLoadIdentity();
	
	glMatrixMode(GL_MODELVIEW);
	
	flare_tex->disable();
	glDisable(GL_BLEND);
	
	glDepthFunc(GL_LESS);
	
}
