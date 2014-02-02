/* Map
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

#include "parser.h"
#include "light.h"
#include "fog.h"
#include "mirror.h"
#include "mesh.h"
#include "objectmesh.h"
#include "rigidbody.h"
#include "particles.h"
#include "objectparticles.h"
#include "engine.h"
#include "console.h"
#include "map.h"

char *Map::data;

/*
 */
void Map::load(const char *name) {
	Parser *parser = new Parser(name);
	
	char file_name[1024];
	strcpy(file_name,name);
	
	if(parser->get("path")) {
		char *s = parser->get("path");
		char path[1024];
		char *d = path;
		while(*s) {
			if(!strchr(" \t\n\r",*s)) *d++ = *s;
			s++;
		}
		*d = '\0';
		Engine::addPath(path);
	}
	
	char *data_block = (char*)Parser::interpret(parser->get("data"));
	if(!data_block) {
		fprintf(stderr,"Map::load(): can`t get data block in \"%s\" file\n",file_name);
		delete parser;
		return;
	}
	
	data = data_block;
	
	try {
		while(*data) {
			const char *token = read_token();
			if(!token || !*token) break;
			else if(!strcmp(token,"bsp")) load_bsp();
			else if(!strcmp(token,"light")) load_light();
			else if(!strcmp(token,"fog")) load_fog();
			else if(!strcmp(token,"mirror")) load_mirror();
			else if(!strcmp(token,"mesh")) load_mesh();
			else if(!strcmp(token,"particles")) load_particles();
			else throw(error("unknown token \"%s\"",token));
		}
	}
	catch(const char *msg) {
		fprintf(stderr,"Map::load(): %s in \"%s\" file\n",msg,file_name);
		delete data_block;
		delete parser;
		return;
	}
	
	delete data_block;
	delete parser;
}

/*****************************************************************************/
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

/*
 */
const char *Map::error(const char *error,...) {
	static char buf[1024];
	va_list arg;
	va_start(arg,error);
	vsprintf(buf,error,arg);
	va_end(arg);
	return buf;
}

/*
 */
const char *Map::read_token(char *must) {
	if(!*data) return NULL;
	while(*data && strchr(" \t\n\r",*data)) data++;
	char *token = data;
	char *s = token;
	while(*data && !strchr(" \t\n\r",*data)) *s++ = *data++;
	if(*data) data++;
	*s = '\0';
	if(must) if(strcmp(token,must)) throw("unknown token");
	return token;
}

int Map::read_bool() {
	const char *token = read_token();
	if(token) {
		if(!strcmp(token,"false") || !strcmp(token,"0")) return 0;
		if(!strcmp(token,"true") || !strcmp(token,"1")) return 1;
		throw(error("unknown token \"%s\" in read_bool",token));
		return 1;
	}
	return 0;
}

int Map::read_int() {
	const char *token = read_token();
	if(token) {
		if(!strchr("-01234567890",*token)) throw(error("unknown token \"%s\" in read_int",token));
		return atoi(token);
	}
	return 0;
}

float Map::read_float() {
	const char *token = read_token();
	if(token) {
		if(!strchr("-01234567890.",*token)) throw(error("unknown token \"%s\" in read_float",token));
		return atof(token);
	}
	return 0.0;
}

vec3 Map::read_vec3() {
	vec3 ret;
	ret.x = read_float();
	ret.y = read_float();
	ret.z = read_float();
	return ret;
}

vec4 Map::read_vec4() {
	vec4 ret;
	ret.x = read_float();
	ret.y = read_float();
	ret.z = read_float();
	ret.w = read_float();
	return ret;
}

const char *Map::read_string() {
	if(!*data) return NULL;
	while(*data && strchr(" \t\n\r",*data)) data++;
	char *str = data;
	char *s = str;
	if(*str == '\"') {
		while(*data && !strchr(" \"",*data)) *s++ = *data++;
		if(*data) data++;
		*s = '\0';
		return str + 1;
	}
	while(*data && !strchr(" \t\n\r",*data)) *s++ = *data++;
	if(*data) data++;
	*s = '\0';
	return str;
}

/*****************************************************************************/
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

/*
 */
void Map::load_bsp() {
	read_token("{");
	Engine::bsp = new Bsp();
	while(1) {
		const char *token = read_token();
		if(!token || !strcmp(token,"}")) break;
		else if(!strcmp(token,"mesh")) Engine::bsp->load(Engine::findFile(read_string()));
		else if(!strcmp(token,"save")) Engine::bsp->save(read_string());
		else if(!strcmp(token,"material")) {
			const char *name = read_string();
			Engine::bsp->bindMaterial(name,Engine::loadMaterial(read_string()));
		}
		else throw(error("unknown token \"%s\" in bsp block",token));
	}
}

/*
 */
void Map::load_pos(Position &pos,mat4 &matrix) {
	read_token("{");
	const char *token = read_token();
	if(!strcmp(token,"spline")) {
		read_token("{");
		char name[1024];
		int close = 0;
		int follow = 0;
		float speed = 1;
		while(1) {
			const char *token = read_token();
			if(!token || !strcmp(token,"}")) break;
			else if(!strcmp(token,"path")) strcpy(name,read_string());
			else if(!strcmp(token,"speed")) speed = read_float();
			else if(!strcmp(token,"close")) close = read_bool();
			else if(!strcmp(token,"follow")) follow = read_bool();
			else throw(error("unknown token \"%s\" in spline block",token));
		}
		Spline *spline = new Spline(Engine::findFile(name),speed,close,follow);
		pos.setSpline(spline);
		matrix = spline->to_matrix(0.0);
		read_token("}");
	} else if(!strcmp(token,"exp")) {
		read_token("{");
		char str[1024];
		char *d = str;
		if(!*data) return;
		while(*data && *data != '}') *d++ = *data++;
		if(*data) data++;
		*d = '\0';
		Expression *expression = new Expression(str);
		pos.setExpression(expression);
		matrix = expression->to_matrix(0.0);
		read_token("}");
	} else {
		float m[16];
		m[0] = atof(token);
		m[1] = read_float();
		m[2] = read_float();
		const char *token = read_token();
		if(!strcmp(token,"}")) {
			matrix.translate(vec3(m[0],m[1],m[2]));
		} else {
			m[3] = atof(token);
			m[4] = read_float();
			m[5] = read_float();
			m[6] = read_float();
			const char *token = read_token();
			if(!strcmp(token,"}")) {
				quat rot(m[3],m[4],m[5],m[6]);
				mat4 translate;
				translate.translate(vec3(m[0],m[1],m[2]));
				matrix = translate * rot.to_matrix();
			} else {
				m[7] = atof(token);
				for(int i = 8; i < 16; i++) m[i] = read_float();
				matrix = mat4(m);
				read_token("}");
			}
		}
		pos = matrix * vec3(0,0,0);
	}
}

/*
 */
void Map::load_light() {
	read_token("{");
	Position pos;
	mat4 matrix;
	float radius = 0.0;
	vec4 color(1.0,1.0,1.0,1.0);
	int shadows = 1;
	Material *material = NULL;
	while(1) {
		const char *token = read_token();
		if(!token || !strcmp(token,"}")) break;
		else if(!strcmp(token,"pos")) load_pos(pos,matrix);
		else if(!strcmp(token,"radius")) radius = read_float();
		else if(!strcmp(token,"color")) color = read_vec4();
		else if(!strcmp(token,"shadows")) shadows = read_bool();
		else if(!strcmp(token,"material")) {
			read_string();
			material = Engine::loadMaterial(read_string());
		}
		else throw(error("unknown token \"%s\" in light block",token));
	}
	Light *light = new Light(vec3(0,0,0),radius,color,shadows);
	if(material) light->bindMaterial("*",material);
	light->pos = pos;
	light->pos.radius = radius;
	light->set(matrix);
	Engine::addLight(light);
}

/*
 */
void Map::load_fog() {
	read_token("{");
	Mesh *mesh = NULL;
	vec4 color(1.0,1.0,1.0,1.0);
	while(1) {
		const char *token = read_token();
		if(!token || !strcmp(token,"}")) break;
		else if(!strcmp(token,"mesh")) mesh = Engine::loadMesh(read_string());
		else if(!strcmp(token,"color")) color = read_vec4();
		else throw(error("unknown token \"%s\" in fog block",token));
	}
	if(mesh) Engine::addFog(new Fog(mesh,color));
	else fprintf(stderr,"Map::load_fog(): can`t find mesh\n");
}

/*
 */
void Map::load_mirror() {
	read_token("{");
	Mirror *mirror = NULL;
	while(1) {
		const char *token = read_token();
		if(!token || !strcmp(token,"}")) break;
		else if(!strcmp(token,"mesh")) mirror = new Mirror(Engine::loadMesh(read_string()));
		else if(mirror && !strcmp(token,"material")) {
			const char *name = read_string();
			mirror->bindMaterial(name,Engine::loadMaterial(read_string()));
		}
		else throw(error("unknown token \"%s\" in mirror block",token));
	}
	if(mirror) Engine::addMirror(mirror);
	else fprintf(stderr,"Map::load_mirror(): can`t find mesh\n");
}

/*
 */
void Map::load_mesh() {
	read_token("{");
	ObjectMesh *mesh = NULL;
	int shadows = 1;
	while(1) {
		const char *token = read_token();
		if(!token || !strcmp(token,"}")) break;
		else if(!strcmp(token,"mesh")) mesh = new ObjectMesh(read_string());
		else if(!strcmp(token,"shadows")) shadows = read_bool();
		else if(mesh && !strcmp(token,"material")) {
			const char *name = read_string();
			mesh->bindMaterial(name,Engine::loadMaterial(read_string()));
		}
		else if(mesh && !strcmp(token,"pos")) {
			mat4 matrix;
			load_pos(mesh->pos,matrix);
			mesh->set(matrix);
		}
		else if(mesh && !strcmp(token,"rigidbody")) {
			read_token("{");
			mat4 matrix;
			float mass = 0;
			float restitution = 0;
			float friction = 0;
			int flag = 0;
			while(1) {
				const char *token = read_token();
				if(!token || !strcmp(token,"}")) break;
				else if(!strcmp(token,"pos")) load_pos(mesh->pos,matrix);
				else if(!strcmp(token,"mass")) mass = read_float();
				else if(!strcmp(token,"restitution")) friction = read_float();
				else if(!strcmp(token,"friction")) friction = read_float();
				else if(!strcmp(token,"collide")) {
					const char *collide = read_string();
					if(!strcmp(collide,"mesh")) flag |= RigidBody::COLLIDE_MESH;
					else if(!strcmp(collide,"sphere")) flag |= RigidBody::COLLIDE_SPHERE;
					else throw(error("unknown collide \"%s\" in rigidbody block",collide));
				} else if(!strcmp(token,"body")) {
					const char *body = read_string();
					if(!strcmp(body,"box")) flag |= RigidBody::BODY_BOX;
					else if(!strcmp(body,"sphere")) flag |= RigidBody::BODY_SPHERE;
					else if(!strcmp(body,"cylinder")) flag |= RigidBody::BODY_CYLINDER;
					else throw(error("unknown body \"%s\" in rigidbody block",body));
				} else throw(error("unknown token \"%s\" in rigidbody block",token));
			}
			mesh->setRigidBody(new RigidBody(mesh,mass,restitution,friction,flag));
			mesh->set(matrix);
		}
		else throw(error("unknown token \"%s\" in mesh block",token));
	}
	if(mesh) {
		mesh->setShadows(shadows);
		Engine::addObject(mesh);
	}
	else fprintf(stderr,"Map::load_mesh(): can`t find mesh\n");
}

/*
 */
void Map::load_particles() {
	read_token("{");
	Position pos;
	mat4 matrix;
	int num = 0;
	float speed = 0.0;
	float rotation = 0.0;
	vec3 force(0.0,0.0,0.0);
	float time = 0.0;
	float radius = 0.0;
	vec4 color(1.0,1.0,1.0,1.0);
	Material *material = NULL;
	while(1) {
		const char *token = read_token();
		if(!token || !strcmp(token,"}")) break;
		else if(!strcmp(token,"pos")) load_pos(pos,matrix);
		else if(!strcmp(token,"num")) num = read_int();
		else if(!strcmp(token,"speed")) speed = read_float();
		else if(!strcmp(token,"rotation")) rotation = read_float();
		else if(!strcmp(token,"force")) force = read_vec3();
		else if(!strcmp(token,"time")) time = read_float();
		else if(!strcmp(token,"radius")) radius = read_float();
		else if(!strcmp(token,"color")) color = read_vec4();
		else if(!strcmp(token,"material")) {
			read_string();
			material = Engine::loadMaterial(read_string());
		} else throw(error("unknown token \"%s\" in particles block",token));
	}
	ObjectParticles *particles = new ObjectParticles(new Particles(num,vec3(0,0,0),speed,rotation,force,time,radius,color));
	if(material) particles->bindMaterial("*",material);
	particles->pos = pos;
	particles->set(matrix);
	Engine::addObject(particles);
}
