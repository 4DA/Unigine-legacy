/* 3D Engine_v0.2 physic demo
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
#include "alapp.h"

#include "engine.h"
#include "console.h"
#include "light.h"
#include "flare.h"
#include "particles.h"
#include "object.h"
#include "objectmesh.h"
#include "objectskinnedmesh.h"
#include "skinnedmesh.h"
#include "ragdoll.h"
#include "objectparticles.h"
#include "texture.h"
#include "rigidbody.h"
#include "collide.h"
#include "joint.h"


#ifdef GRAB
	#include "video.h"
	#define WIDTH	400
	#define HEIGHT	300
#else
	#define WIDTH	1024
	#define HEIGHT	768
#endif

/*
 */
class GLAppMain : public GLApp, ALApp {
public:
	
	int init();
	void idle();
	void render();
	
	void keyPress(int key);
	
	float time;
	
	int pause_toggle;				// state toggles
	int info_toggle;
	int flash_light_toggle;
	int robot_toggle;
	
	float phi,psi;					// free camera parameters
	vec3 camera,speed,dir;
	Collide *collide;
	
	mat4 modelview;					// view matrixes
	mat4 projection;
	
	Object *robot;					// robot body
	JointHinge *j0,*j1,*j2,*j3;		// wheel - robot body joints
	float angle;					// direction
	float velocity;					// velocity of the robot
	
	Light *flash_light;				// flash light
	
	Light *shoot_light;				// shoot light
	Sound *shoot_sound;				// shoot sound
	ObjectParticles *shoot_particles;	// particles
	Texture *cross_tex;					// cross
	
	Object *lamp;					// lamp
	Light *lamp_light;				// lamp light
	
#ifdef GRAB
	Video *video;					// video grabber
	unsigned char *buffer;
#endif
};

/*
 */
static void quit(int,char**,void*) {
	GLApp::exit();
}

static void help(int,char**,void*) {
	Engine::console->printf(0.77,1.0,0.15,
		"w,a,s,d - camera control\n"
		"cursors - robot control\n"
		"left mouse button - shoot\n"
		"\n"
		"other keys:\n"
		"'`' - console toggle\n"
		"\n"
		"'h' - help toggle\n"
		"'i' - info toggle\n"
		"'f' - flash light toggle\n"
		"'r' - robot view toggle\n"
		"' ' - pause toggle"
		"\n"
		"'1' - vertex lighting toggle //%s\n"
		"'2' - Teylor toggle //%s\n"
		"'3' - half angle specular toggle //%s\n"
		"\n"
		"('z'|'v'|'c')&middle button - add rigid body object\n"
		"\n"
		"console commands:\n"
		"\"ls\" - prints all variables and commands\n"
		"\"set\" - sets value of variable\n"
		"\"clear\" - clears console\n",
		Engine::isDefine("VERTEX") ? "on" : "off",
		Engine::isDefine("TEYLOR") ? "on" : "off",
		Engine::isDefine("HALF") ? "on" : "off");
}

/*****************************************************************************/
/*                                                                           */
/* extern load                                                               */
/*                                                                           */
/*****************************************************************************/

/*
 */
void extern_load(void *data) {
	GLAppMain *glapp = (GLAppMain*)data;
	
	{	// robot
	
		ObjectMesh *mesh0,*mesh1,*mesh2,*mesh3,*mesh4;
		RigidBody *rb0,*rb1,*rb2,*rb3,*rb4;
		
		mesh0 = new ObjectMesh(Engine::loadMesh("robot_wheel.mesh"));
		mesh0->bindMaterial("*",Engine::loadMaterial("robot_wheel.mat"));
		mesh1 = new ObjectMesh(Engine::loadMesh("robot_wheel.mesh"));
		mesh1->bindMaterial("*",Engine::loadMaterial("robot_wheel.mat"));
		mesh2 = new ObjectMesh(Engine::loadMesh("robot_wheel.mesh"));
		mesh2->bindMaterial("*",Engine::loadMaterial("robot_wheel.mat"));
		mesh3 = new ObjectMesh(Engine::loadMesh("robot_wheel.mesh"));
		mesh3->bindMaterial("*",Engine::loadMaterial("robot_wheel.mat"));
		mesh4 = new ObjectMesh(Engine::loadMesh("robot.mesh"));
		mesh4->bindMaterial("robot",Engine::loadMaterial("robot_body.mat"));
		mesh4->bindMaterial("camera",Engine::loadMaterial("robot_camera.mat"));
		
		rb0 = new RigidBody(mesh0,10,0.1,0.6,RigidBody::COLLIDE_SPHERE | RigidBody::BODY_SPHERE);
		rb1 = new RigidBody(mesh1,10,0.1,0.6,RigidBody::COLLIDE_SPHERE | RigidBody::BODY_SPHERE);
		rb2 = new RigidBody(mesh2,10,0.1,0.6,RigidBody::COLLIDE_SPHERE | RigidBody::BODY_SPHERE);
		rb3 = new RigidBody(mesh3,10,0.1,0.6,RigidBody::COLLIDE_SPHERE | RigidBody::BODY_SPHERE);
		rb4 = new RigidBody(mesh4,10,0.2,0.2,RigidBody::COLLIDE_MESH | RigidBody::BODY_BOX);
		
		mesh0->setRigidBody(rb0);
		mesh1->setRigidBody(rb1);
		mesh2->setRigidBody(rb2);
		mesh3->setRigidBody(rb3);
		mesh4->setRigidBody(rb4);
		
		vec3 pos = vec3(-21,26,0.5);
		
		mesh0->set(vec3(0.549,-0.565,0) + pos);
		mesh1->set(vec3(-0.549,-0.565,0) + pos);
		mesh2->set(vec3(0.549,0.562,0) + pos);
		mesh3->set(vec3(-0.549,0.562,0) + pos);
		mesh4->set(vec3(0,0,0.15) + pos);
		
		glapp->j0 = new JointHinge(rb4,rb0,vec3(0.549,-0.565,0) + pos,vec3(1,0,0));
		glapp->j1 = new JointHinge(rb4,rb1,vec3(-0.549,-0.565,0) + pos,vec3(1,0,0));
		glapp->j2 = new JointHinge(rb4,rb2,vec3(0.549,0.562,0) + pos,vec3(1,0,0));
		glapp->j3 = new JointHinge(rb4,rb3,vec3(-0.549,0.562,0) + pos,vec3(1,0,0));
		
		Engine::addObject(mesh0);
		Engine::addObject(mesh1);
		Engine::addObject(mesh2);
		Engine::addObject(mesh3);
		Engine::addObject(mesh4);
	
		glapp->angle = 0;
		glapp->velocity = 0;
		glapp->robot = mesh4;
	}
	
	// flash light
	glapp->flash_light = new Light(vec3(0,0,0),15,vec4(1.0,1.0,1.0,1.0),0);
	glapp->flash_light->bindMaterial("*",Engine::loadMaterial("flash_light.mat"));
	
	// shoot light
	glapp->shoot_light = new Light(vec3(0,0,0),20,vec4(1.0,0.4,0.1,1.0));
	glapp->shoot_particles = new ObjectParticles(new Particles(128,Particles::OFF,0.3,0.3,vec3(0,0,-3),0.5,0.1,vec4(0.8,0.3,0.2,1.0)));
	glapp->shoot_particles->bindMaterial("*",Engine::loadMaterial("fire.mat"));
	Engine::addObject(glapp->shoot_particles);
	
	{	// lamp 0
		ObjectSkinnedMesh *mesh = new ObjectSkinnedMesh("lamp/lamp.txt");
		mesh->bindMaterial("*",Engine::loadMaterial("default.mat"));
		mesh->setShadows(0);
		mesh->set(vec3(-4.975,31.403,2.92));
		
		Engine::addObject(mesh);
		
		RagDoll *ragdoll = new RagDoll(mesh->skinnedmesh,"lamp/lamp.conf");
		mesh->setRagDoll(ragdoll);
		
		glapp->lamp = mesh;
		glapp->lamp_light = new Light(vec3(0,0,1000),6,vec4(0.9,0.9,0.9,1.0));
		glapp->lamp_light->bindMaterial("*",Engine::loadMaterial("lamp_light.mat"));
		Engine::addLight(glapp->lamp_light);
		
		glapp->lamp_light->setFlare(new Flare(0,0.6,0.12));
	}
	
	{	// ufo
		ObjectSkinnedMesh *mesh = new ObjectSkinnedMesh("ufo/ragdoll.txt");
		mesh->bindMaterial("*",Engine::loadMaterial("ufo_body.mat"));
		mesh->set(mat4(2,40,2) * mat4(1,0,0,45));
		
		Engine::addObject(mesh);
		
		RagDoll *ragdoll = new RagDoll(mesh->skinnedmesh,"ufo/ragdoll.conf");
		mesh->setRagDoll(ragdoll);
	}
}

/*****************************************************************************/
/*                                                                           */
/* init                                                                      */
/*                                                                           */
/*****************************************************************************/

/*
 */
int GLAppMain::init() {
	
	// absolute minimum
	checkExtension("GL_ARB_vertex_program");
	checkExtension("GL_ARB_vertex_buffer_object");
	
	// toggles
	pause_toggle = 0;
	info_toggle = 0;
	flash_light_toggle = 0;
	robot_toggle = 0;
	
	// camera
	psi = 35.0;
	phi = 10.0;
	camera = vec3(-0.79,2.58,0.67);
	speed = vec3(0,0,0);
	collide = new Collide();
	
	// shoot
	shoot_sound = new Sound("data/fire.wav");
	cross_tex = new Texture("data/cross.tga",Texture::TEXTURE_2D,Texture::RGBA | Texture::LINEAR_MIPMAP_LINEAR);
	
	// engine
	Engine::addPath("data/,"
		"data/engine/,"
		"data/textures/,"
		"data/textures/cube/,"
		"data/materials/,"
		"data/shaders/,"
		"data/meshes/,"
		"data/physic/");
	
	if(Engine::init("data/engine.conf") == 0) return 0;
	
	Engine::console->addCommand("quit",quit);
	Engine::console->addCommand("help",help);
	Engine::setExternLoad(extern_load,this);
	
	Engine::load("physic.map");
	
	GLApp::error();	// errors
	ALApp::error();
	
#ifdef GRAB
	video = new Video("video.mpg",WIDTH,HEIGHT,16000000);
	buffer = new unsigned char[WIDTH * HEIGHT * 4];
#endif

    // GLenum err = glewInit();
	// if (GLEW_OK != err) {
	// 	/* Problem: glewInit failed, something is seriously wrong. */
	// 	fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
	// 	exit("no glew");
	// }
	
	return 1;
}

/*****************************************************************************/
/*                                                                           */
/* render                                                                    */
/*                                                                           */
/*****************************************************************************/

/*
 */
void GLAppMain::render() {
	
	glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(projection);
    glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(modelview);
	
	// engine render
	Engine::update(pause_toggle ? 0 : ifps);
	Engine::render(ifps);
	// it`s all :)
	
	GLApp::error();
	ALApp::error();
	
	// cross
	if(!Engine::console->getActivity() && robot_toggle == 0) {
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-30,30,-22.5,22.5,-1,1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		
		glDepthFunc(GL_ALWAYS);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE);
		
		cross_tex->enable();
		cross_tex->bind();
		cross_tex->render();
		cross_tex->disable();
		
		glDisable(GL_BLEND);
	}
	
#ifndef GRAB
	
	if(!Engine::console->getActivity()) {
	
		// fps
		Engine::console->enable(800,600);
		glColor3f(0.19,0.0,0.82);
		Engine::console->printf(9,9,"fps: %.0f",fps);
		glColor3f(0.77,1.0,0.15);
		Engine::console->printf(8,8,"fps: %.0f",fps);
		Engine::console->disable();
		
		// info
		if(info_toggle) {
			Engine::console->enable(1024,768);
			vec3 c = modelview.inverse() * vec3(0,0,0);
			glColor3f(0.19,0.0,0.82);
			Engine::console->printf(13,35,"triangles: %d\nsectors %d\nlights %d\nrun \"help\" for more information\n%f %f %f",
				Engine::num_triangles,Bsp::num_visible_sectors,Engine::num_visible_lights,c.x,c.y,c.z);
			glColor3f(0.77,1.0,0.15);
			Engine::console->printf(12,34,"triangles: %d\nsectors %d\nlights %d\nrun \"help\" for more information\n%f %f %f",
				Engine::num_triangles,Bsp::num_visible_sectors,Engine::num_visible_lights,c.x,c.y,c.z);
			Engine::console->disable();
		}
		
		glColor3f(1,1,1);
	}
#endif
	
#ifdef GRAB
	static float count = 0;
	count += ifps;
	if(count > 1.0 / 25.0) {
		glReadPixels(0,0,WIDTH,HEIGHT,GL_RGB,GL_UNSIGNED_BYTE,buffer);
		video->save(buffer,true);
		count -= 1.0 / 25.0;
	}
#endif
}

/*****************************************************************************/
/*                                                                           */
/* idle                                                                      */
/*                                                                           */
/*****************************************************************************/

/*
 */
void GLAppMain::idle() {
	
	time += ifps;
	
	// toggles
	if(!Engine::console->getActivity()) {
		if(keys[(int)' ']) {
			pause_toggle = !pause_toggle;
			keys[(int)' '] = 0;
		}
		
		if(keys[(int)'i']) {
			info_toggle = !info_toggle;
			keys[(int)'i'] = 0;
		}
		
		if(keys[(int)'f']) {
			flash_light_toggle = !flash_light_toggle;
			keys[(int)'f'] = 0;
		}
		
		if(keys[(int)'r']) {
			robot_toggle = !robot_toggle;
			keys[(int)'r'] = 0;
		}
	}
	
	// from camera
	static int look = 0;
	
	// shoot before camera
	if(robot_toggle == 0) {
		static int shoot = 0;
		static float shoot_time = 0;
		
		shoot_light->set(modelview.inverse());
		
		GLint state;
		alGetSourcei(shoot_sound->source,AL_SOURCE_STATE,&state);
		alSourcef(shoot_sound->source,AL_GAIN,0.1);
		
		if(state == AL_PLAYING) {
			float k = 1.0 - (time - shoot_time) * 4.0;
			if(k < 0) {
				Engine::removeLight(shoot_light);
				shoot_light->setColor(vec4(1.0,0.4,0.1,1.0));
			} else {
				shoot_light->setColor(vec4(1.0,0.4,0.1,1.0) * k);
			}
		}
		
		if(look && shoot == 0 && state != AL_PLAYING && mouseButton & BUTTON_LEFT) {
			vec3 point,normal;
			Object *object = Engine::intersection(camera,camera + dir * 1000,point,normal);
			shoot_sound->play();
			if(object) {
				if(object->rigidbody) {
					object->rigidbody->addImpulse(point,-normal * 100 * fabs(normal * dir));
				}
				shoot_particles->set(point - dir * 0.2);
				shoot_particles->setOffTime(0.2);
				shoot_time = time;
				Engine::addLight(shoot_light);
			}
			shoot = 1;
		} else {
			if(state != AL_PLAYING) shoot = 1;
			else shoot = 0;
		}
		if(!(mouseButton & BUTTON_LEFT)) shoot = 0;
	}
	
	// camera movement
	if(robot_toggle) {	// robot view
		
		look = 0;
		modelview = mat4(0,0,1,180) * mat4(1,0,0,70) * mat4(0.0,0.25,-0.60) * robot->transform.inverse();
		projection.perspective(89,(float)windowWidth / (float)windowHeight,0.1,500);
		
	} else {	// free camera
		mat4 m0,m1,m2;
		
		if(!look && mouseButton & BUTTON_LEFT) {
			setCursor(windowWidth / 2,windowHeight / 2);
			mouseX = windowWidth / 2;
			mouseY = windowHeight / 2;
			look = 1;
		}
		if(mouseButton & BUTTON_RIGHT) look = 0;
		
		if(look) {
			showCursor(0);
			psi += (mouseX - windowWidth / 2) * 0.2;
			phi += (mouseY - windowHeight / 2) * 0.2;
			if(phi < -89) phi = -89;
			if(phi > 89) phi = 89;
			setCursor(windowWidth / 2,windowHeight / 2);
		} else showCursor(1);
		
		float vel = 20;
		if(!Engine::console->getActivity()) {
			if(keys[(int)'w']) speed.x += vel * ifps;
			if(keys[(int)'s']) speed.x -= vel * ifps;
			if(keys[(int)'a']) speed.y -= vel * ifps;
			if(keys[(int)'d']) speed.y += vel * ifps;
			if(keys[KEY_SHIFT]) speed.z += vel * ifps;
			if(keys[KEY_CTRL]) speed.z -= vel * ifps;
		}
		
		speed -= speed * 5 * ifps;
		
		quat q0,q1;
		q0.set(vec3(0,0,1),-psi);
		q1.set(vec3(0,1,0),phi);
		dir = (q0 * q1).to_matrix() * vec3(1,0,0);
		vec3 x,y,z;
		x = dir;
		y.cross(dir,vec3(0,0,1));
		y.normalize();
		z.cross(y,x);
		camera += (x * speed.x + y * speed.y + z * speed.z) * ifps;
		
		for(int i = 0; i < 4; i++) {
			collide->collide(NULL,camera + (x * speed.x + y * speed.y + z * speed.z) * ifps / 4.0f,0.25);
			for(int j = 0; j < collide->num_contacts; j++) {
				camera += collide->contacts[j].normal * collide->contacts[j].depth / (float)collide->num_contacts;
			}
		}
		
		modelview.look_at(camera,camera + dir,vec3(0,0,1));
		projection.perspective(89,(float)windowWidth / (float)windowHeight,0.1,500);
	}
	
	// robot controls
	if(keys[KEY_UP]) velocity += ifps * 1;
	if(keys[KEY_DOWN]) velocity -= ifps * 1;
	velocity -= velocity * ifps;
	if(velocity > 1) velocity = 0.5;
	if(velocity < -1) velocity = -0.5;
	
	if(fabs(velocity) > 0.1) {
		j0->setAngularVelocity1(velocity);
		j1->setAngularVelocity1(velocity);
		j2->setAngularVelocity1(velocity);
		j3->setAngularVelocity1(velocity);
	}
	
	if(keys[KEY_LEFT]) angle += 80 * ifps;
	if(keys[KEY_RIGHT]) angle -= 80 * ifps;
	if(angle > 30) angle = 30;
	if(angle < -30) angle = -30;
	
	float rad,radius = 1.127f / tan(angle * DEG2RAD);
	
	rad = atan(1.127f / (radius - 0.549f));
	j0->setAxis0(vec3(cos(rad),sin(rad),0));
	j2->setAxis0(vec3(cos(rad),-sin(rad),0));
	
	rad = atan(1.127f / (radius + 0.549f));
	j1->setAxis0(vec3(cos(rad),sin(rad),0));
	j3->setAxis0(vec3(cos(rad),-sin(rad),0));
	
	// flash light
	static int flash = 0;
	flash_light->set(modelview.inverse());
	
	if(flash_light_toggle) {
		if(flash == 0) {
			Engine::addLight(flash_light);
			flash = 1;
		}
	} else {
		if(flash == 1) {
			Engine::removeLight(flash_light);
			flash = 0;
		}
	}
	
	// lamp
	ObjectSkinnedMesh *sm = reinterpret_cast<ObjectSkinnedMesh*>(lamp);
	lamp_light->set(sm->transform * sm->getBoneTransform(sm->getBone("end")) * mat4(0,0,-0.25));
	lamp->set(vec3(-4.975,31.403,3.1));
	
	// add object
	if(robot_toggle == 0) {
		
		mat4 m0,m1,m2,m3;
		m0.rotate(0,0,1,90);
		m1.rotate(0,1,0,-90);
		m2.translate(-2,0,0);
		m3.rotate(0,1,0,180);
		
		static Object *mesh = NULL;
		
		static int press = 0;
		if(mouseButton & BUTTON_MIDDLE) {
			if(!press) {
				if(keys[(int)'z']) {
					mesh = new ObjectMesh(Engine::loadMesh("box.mesh"));
					mesh->bindMaterial("*",Engine::loadMaterial("box.mat"));
					mesh->setRigidBody(new RigidBody(mesh,10,0.3,0.5,RigidBody::COLLIDE_MESH | RigidBody::BODY_BOX));
					Engine::addObject(mesh);
					mesh->set(modelview.inverse() * m0 * m1 * m2 * m3);
					press = 1;
				}
				if(keys[(int)'x']) {
					mesh = new ObjectMesh(Engine::loadMesh("barrel.mesh"));
					mesh->bindMaterial("*",Engine::loadMaterial("barrel.mat"));
					mesh->setRigidBody(new RigidBody(mesh,40,0.3,0.5,RigidBody::COLLIDE_MESH | RigidBody::BODY_CYLINDER));
					Engine::addObject(mesh);
					mesh->set(modelview.inverse() * m0 * m1 * m2 * m3);
					press = 1;
				}
				if(keys[(int)'c']) {
					mesh = new ObjectMesh(Engine::loadMesh("chair.mesh"));
					mesh->bindMaterial("*",Engine::loadMaterial("chair.mat"));
					mesh->setRigidBody(new RigidBody(mesh,20,0.3,0.5,RigidBody::COLLIDE_MESH | RigidBody::BODY_BOX));
					Engine::addObject(mesh);
					mesh->set(modelview.inverse() * m0 * m1 * m2 * m3);
					press = 1;
				}
			}
		}
		
		if(!(mouseButton & BUTTON_MIDDLE)) press = 0;
		else if(mesh) mesh->set(modelview.inverse() * m0 * m1 * m2 * m3);
	}
	
	// sound update
	ALApp::update();
}

/*
 */
void GLAppMain::keyPress(int key) {
	if(key == KEY_UP) Engine::console->keyPress('\1');
	else if(key == KEY_DOWN) Engine::console->keyPress('\2');
	else if(key == KEY_RETURN) Engine::console->keyPress('\n');
	else if(key == KEY_BACKSPACE) Engine::console->keyPress('\b');
	else if(key < 256) Engine::console->keyPress(key);
	
	if(Engine::console->getActivity()) return;
	
	if(key == '1') {
		if(Engine::isDefine("VERTEX")) Engine::undef("VERTEX");
		else Engine::define("VERTEX");
		Engine::reload();
	} else if(key == '2') {
		if(Engine::isDefine("TEYLOR")) Engine::undef("TEYLOR");
		else Engine::define("TEYLOR");
		Engine::reload();
	} else if(key == '3') {
		if(Engine::isDefine("HALF")) Engine::undef("HALF");
		else Engine::define("HALF");
		Engine::reload();
	}

}

/*****************************************************************************/
/*                                                                           */
/* main                                                                      */
/*                                                                           */
/*****************************************************************************/

/*
 */
int main(int argc,char **argv) {
	
	GLAppMain *glApp = new GLAppMain();
	
	int w = WIDTH;
	int h = HEIGHT;
	int fs = 0;
	for(int i = 1; i < argc; i++) {
		if(!strcmp(argv[i],"-fs")) fs = 1;
		else if(!strcmp(argv[i],"-w")) sscanf(argv[++i],"%d",&w);
		else if(!strcmp(argv[i],"-h")) sscanf(argv[++i],"%d",&h);
	}
	
	if(glApp->setVideoMode(w,h,fs) == 0) return 0;
	
	glApp->setTitle("3D Engine demo v0.2 http://frustum.org");
	
	if(glApp->init() == 0) {
		delete glApp;
		return 0;
	}
	
	glApp->main();
	
	Engine::clear();
	delete glApp;
	
	return 0;
}
