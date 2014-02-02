/* Object
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
#include "bsp.h"
#include "position.h"
#include "shader.h"
#include "material.h"
#include "rigidbody.h"
#include "object.h"

Object::Object(int type) : type(type), rigidbody(NULL), is_identity(1),
	num_opacities(0), opacities(NULL), num_transparents(0), transparents(NULL),
	shadows(1), time(0), frame(0) {
}

Object::~Object() {
	for(int i = 0; i < pos.num_sectors; i++) Bsp::sectors[pos.sectors[i]].removeObject(this);
	if(rigidbody) delete rigidbody;
}

/*
 */
void Object::update(float ifps) {
	time += ifps;
	if(rigidbody) {
		if(Engine::physic_toggle) rigidbody->simulate();
	} else {
		pos.update(time,this);
	}
}

/*
 */
void Object::updatePos(const vec3 &p) {
	for(int i = 0; i < pos.num_sectors; i++) Bsp::sectors[pos.sectors[i]].removeObject(this);
	pos.radius = getRadius();
	pos = p;
	for(int i = 0; i < pos.num_sectors; i++) Bsp::sectors[pos.sectors[i]].addObject(this);
}

/*
 */
int Object::bindMaterial(const char *name,Material *material) {
	int bind = 0;
	for(int i = 0; i < getNumSurfaces(); i++) {
		if(Engine::match(name,getSurfaceName(i))) {
			int j;
			materials[i] = material;
			if(materials[i]->blend) {
				for(j = 0; j < num_opacities; j++) if(opacities[j] == i) break;
				if(j != num_opacities) {
					for(; j < num_opacities - 1; j++) opacities[j] = opacities[j + 1];
					num_opacities--;
				}
				for(j = 0; j < num_transparents; j++) if(transparents[j] == i) break;
				if(j == num_transparents) transparents[num_transparents++] = i;
				else transparents[j] = i;
			} else {
				for(j = 0; j < num_transparents; j++) if(transparents[j] == i) break;
				if(j != num_transparents) {
					for(; j < num_transparents - 1; j++) transparents[j] = transparents[j + 1];
					num_transparents--;
				}
				for(j = 0; j < num_opacities; j++) if(opacities[j] == i) break;
				if(j == num_opacities) opacities[num_opacities++] = i;
				else opacities[j] = i;
			}
			bind = 1;
		}
	}
	return bind;
}

/*
 */
void Object::setRigidBody(RigidBody *rigidbody) {
	is_identity = 0;
	this->rigidbody = rigidbody;
}

/*
 */
void Object::setShadows(int shadows) {
	this->shadows = shadows;
}

/*
 */
void Object::set(const vec3 &p) {
	is_identity = 0;
	transform.translate(p);
	itransform = transform.inverse();
	updatePos(p);
	if(rigidbody) rigidbody->set(p);
}

void Object::set(const mat4 &m) {
	is_identity = 0;
	transform = m;
	itransform = transform.inverse();
	updatePos(m * vec3(0,0,0));
	if(rigidbody) rigidbody->set(m);
}

/*
 */
void Object::enable() {
	if(is_identity) return;
	old_modelview = Engine::modelview;
	old_imodelview = Engine::imodelview;
	old_transform = Engine::transform;
	old_itransform = Engine::itransform;
	Engine::modelview = Engine::modelview * transform;
	Engine::imodelview = itransform * Engine::imodelview;
	Engine::transform = Engine::transform * transform;
	Engine::itransform = itransform * Engine::itransform;
	glLoadMatrixf(Engine::modelview);
	// new transformation
	if(Shader::old_shader) Shader::old_shader->bind();
}

void Object::disable() {
	if(is_identity) return;
	Engine::modelview = old_modelview;
	Engine::imodelview = old_imodelview;
	Engine::transform = old_transform;
	Engine::itransform = old_itransform;
	glLoadMatrixf(Engine::modelview);
	if(Shader::old_shader) Shader::old_shader->bind();
}
