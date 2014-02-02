/* Skinned Mesh Viewer
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

#include <stdio.h>
#include "glapp.h"
#include "font.h"
#include "mathlib.h"
#include "skinnedmesh.h"
#include "texture.h"

#define WIDTH	1024
#define HEIGHT	768

/*
 */
class GLAppMain : public GLApp {
public:
	
	int init(const char *name);
	void idle();
	void render();
	
	int pause;
	int wireframe;
	float time;
	float psi,phi;
	vec3 camera;
	vec3 speed;
	mat4 modelview;
	mat4 projection;
	
	Font *font;
	
	SkinnedMesh *mesh;
	float radius;
	vec3 center;
	
	Texture *texture;
};

/*
 */
int GLAppMain::init(const char *name) {
	
	time = 0;
	pause = 0;
	wireframe = 0;

	psi = 90;
	phi = 20;
	camera = vec3(0,5,2);
	speed = vec3(0,0,0);
	
	// opengl
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_NORMALIZE);
	
	font = new Font();
	
	if(name) {
		mesh = new SkinnedMesh(name);
		radius = mesh->getRadius();
		center = mesh->getCenter();
	} else {
		char name[1024];
		if(selectFile("select Skinned Mesh file",name)) {
			mesh = new SkinnedMesh(name);
			radius = mesh->getRadius();
			center = mesh->getCenter();
		} else {
			exit();
		}
	}
	
	texture = NULL;
	
	error();
	
	return 1;
}

/*
 */
void GLAppMain::render() {
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(projection);
    glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(modelview);
	
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0,GL_POSITION,vec4(camera,1));
	if(texture) {
		texture->enable();
		texture->bind();
	}
	
	glScalef(2.0 / radius,2.0 / radius,2.0 / radius);
	glTranslatef(-center.x,-center.y,-center.z);
	glColor3f(1,1,1);
	
	if(wireframe) glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	
	mesh->render();
	
	if(wireframe) glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);

	if(texture) texture->disable();
	glDisable(GL_LIGHTING);
	
	float step = pow(10,(int)log10(radius));
	for(int i = 0; i <= 20; i++) {
		if(i == 10) glColor3f(0.6,0.6,0.6);
		else glColor3f(0.9,0.9,0.9);
		glBegin(GL_LINES);
		glVertex3fv(vec3(i - 10,-10,0) * step);
		glVertex3fv(vec3(i - 10,10,0) * step);
		glEnd();
		glBegin(GL_LINES);
		glVertex3fv(vec3(-10,i - 10,0) * step);
		glVertex3fv(vec3(10,i - 10,0) * step);
		glEnd();
	}
	
	error();
	
	// info
	glColor3f(0.25,1.0,0.01);
	font->enable(800,600);
	font->printf(10,10,"fps: %.0f",fps);
	font->disable();
	glColor3f(0.81,1.0,0.11);
	font->enable(1024,768);
	font->printf(15,45,"'f' - load skinned mesh\n't' - load texture\n'l' - wireframe\ncenter %.1f %.1f %.1f\nradius %0.1f",
		center.x,center.y,center.z,radius);
	font->disable();
	glColor3f(1,1,1);
}

/*
 */
void GLAppMain::idle() {
	
	if(!pause) time += ifps;
	mesh->setFrame(time * 15.0);
	
	// keyboard events
	if(keys[KEY_ESC]) exit();
	
	if(keys[(int)' ']) {
		pause = !pause;
		keys[(int)' '] = 0;
	}
	
	if(keys[(int)'l']) {
		wireframe = !wireframe;
		keys[(int)'l'] = 0;
	}

	if(keys[(int)'f']) {
		char name[1024];
		if(selectFile("select Skinned Mesh file",name)) {
			delete mesh;
			mesh = new SkinnedMesh(name);
			radius = mesh->getRadius();
			center = mesh->getCenter();
			if(texture) delete texture;
			texture = NULL;
			time = 0;
		}
		keys[(int)'f'] = 0;
	}
	
	if(keys[(int)'t']) {
		char name[1024];
		if(selectFile("select texture",name)) {
			if(texture) delete texture;
			texture = new Texture(name);
		}
		keys[(int)'t'] = 0;
	}
	
	// camera movement
	static int look = 0;
	
	if(!look && mouseButton & BUTTON_LEFT) {
		setCursor(windowWidth / 2,windowHeight / 2);
		mouseX = windowWidth / 2;
		mouseY = windowHeight / 2;
		look = 1;
	}
	if(mouseButton & BUTTON_RIGHT) look = 0;
	
	if(look) {
		showCursor(0);
		static float count = 0;
		count += ifps;
		if(count > 1.0 / 60.0) {
			psi += (mouseX - windowWidth / 2) * 0.2;
			phi += (mouseY - windowHeight / 2) * 0.2;
			if(phi < -89) phi = -89;
			if(phi > 89) phi = 89;
			setCursor(windowWidth / 2,windowHeight / 2);
			count -= 1.0 / 60.0;
		}
	} else showCursor(1);
	
	if(keys[KEY_UP] || keys[(int)'w']) speed.x += 40 * ifps;
	if(keys[KEY_DOWN] || keys[(int)'s']) speed.x -= 40 * ifps;
	if(keys[KEY_LEFT] || keys[(int)'a']) speed.y -= 40 * ifps;
	if(keys[KEY_RIGHT] || keys[(int)'d']) speed.y += 40 * ifps;
	if(keys[KEY_SHIFT]) speed.z += 40 * ifps;
	if(keys[KEY_CTRL]) speed.z -= 40 * ifps;
	speed -= speed * 5 * ifps;
	
	vec3 dir = (quat(vec3(0,0,1),-psi) * quat(vec3(0,1,0),phi)).to_matrix() * vec3(1,0,0);
	vec3 x,y,z;
	x = dir;
	y.cross(dir,vec3(0,0,1));
	y.normalize();
	z.cross(y,x);
	camera += (x * speed.x + y * speed.y + z * speed.z) * ifps;
	modelview.look_at(camera,camera + dir,vec3(0,0,1));
	
	projection.perspective(45,(float)windowWidth / (float)windowHeight,0.1,100);
}

/*
 */
int main(int argc,char **argv) {
	
	GLAppMain *glApp = new GLAppMain;
	
	int w = WIDTH;
	int h = HEIGHT;
	int fs = 0;
	for(int i = 1; i < argc; i++) {
		if(!strcmp(argv[i],"-fs")) fs = 1;
		else if(!strcmp(argv[i],"-w")) sscanf(argv[++i],"%d",&w);
		else if(!strcmp(argv[i],"-h")) sscanf(argv[++i],"%d",&h);
	}

	if(!glApp->setVideoMode(w,h,fs)) return 0;

	glApp->setTitle("Skinned Mesh Viewer http://frustum.org");

	if(!glApp->init(argc > 1 ? argv[1] : NULL)) return 0;
	
	glApp->main();
	
	delete glApp;
	
	return 0;
}
