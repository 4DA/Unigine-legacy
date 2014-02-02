/* RigidBody dynamics
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
#include <string.h>
#include "collide.h"
#include "physic.h"
#include "joint.h"
#include "object.h"
#include "collide.h"
#include "engine.h"
#include "rigidbody.h"

RigidBody::RigidBody(Object *object,float mass,float restitution,float friction,int flag) :
	object(object), mass(mass), restitution(restitution), friction(friction),
	frozen(0), frozen_time(0.0), frozen_num_objects(0), num_joints(0), simulated(0), immovable(0) {
	
	if(flag & COLLIDE_MESH) collide_type = COLLIDE_MESH;
	else if(flag & COLLIDE_SPHERE) collide_type = COLLIDE_SPHERE;
	collide = new Collide();
	
	if(flag && BODY_BOX) {
		vec3 min = object->getMin();
		vec3 max = object->getMax();
		vec3 v = (max - min) / 2.0;
		mat3 inertiaTensor;
		inertiaTensor[0] = 1.0f / 12.0f * mass * (v.y * v.y + v.z * v.z);
		inertiaTensor[4] = 1.0f / 12.0f * mass * (v.x * v.x + v.z * v.z);
		inertiaTensor[8] = 1.0f / 12.0f * mass * (v.x * v.x + v.y * v.y);
		iBodyInertiaTensor = inertiaTensor.inverse();
	}
	else if(flag && BODY_SPHERE) {
		float radius = object->getRadius();
		mat3 inertiaTensor;
		inertiaTensor[0] = 2.0f / 5.0f * mass * radius * radius;
		inertiaTensor[4] = 2.0f / 5.0f * mass * radius * radius;
		inertiaTensor[8] = 2.0f / 5.0f * mass * radius * radius;
		iBodyInertiaTensor = inertiaTensor.inverse();
	}
	else if(flag && BODY_CYLINDER) {
		vec3 min = object->getMin();
		vec3 max = object->getMax();
		float radius = max.x - min.x;
		float height = max.z - min.z;
		mat3 inertiaTensor;
		inertiaTensor[0] = 1.0f / 12.0f * mass * height * height;
		inertiaTensor[4] = 1.0f / 12.0f * mass * height * height;
		inertiaTensor[8] = 1.0f / 2.0f * mass * radius * radius;
		iBodyInertiaTensor = inertiaTensor.inverse();
	}
	
	set(object->transform);
		
	RigidBody **rb = new RigidBody*[++Physic::all_rigidbodies];
	memcpy(rb,Physic::rigidbodies,sizeof(RigidBody*) * (Physic::all_rigidbodies - 1));
	if(Physic::rigidbodies) delete Physic::rigidbodies;
	Physic::rigidbodies = rb;
}

RigidBody::~RigidBody() {
	delete collide;
	
	if(num_joints) delete joints[0];
	
	if(Physic::all_rigidbodies) Physic::all_rigidbodies--;
	else {
		delete Physic::rigidbodies;
		Physic::rigidbodies = NULL;
	}
}

/*
 */
void RigidBody::set(const vec3 &p) {
	mat4 m;
	m.translate(p);
	set(m);
}

/*
 */
void RigidBody::set(const mat4 &m) {
	
	vec3 old_pos = pos;
	
	pos = m * vec3(0,0,0);
	orienation = mat3(m);
	
	mat4 old_transform = transform;
	
	transform = m;
	itransform = transform.inverse();
	
	//velocity = (pos - old_pos) * Engine::ifps * 100.0f;
	
	velocity = vec3(0,0,0);
	angularVelocity = vec3(0,0,0);
	angularMomentum = vec3(0,0,0);
	
	iWorldInertiaTensor.identity();
	
	frozen = 0;
	frozen_time = 0.0f;
	immovable = 1;
}

/*
 */
void RigidBody::simulate() {
	if(simulated) return;
	simulated = 1;
	Physic::rigidbodies[Physic::num_rigidbodies++] = this;
	for(int i = 0; i < num_joints; i++) {	// add all joined rigid bodies
		RigidBody *rb = joined_rigidbodies[i];
		if(rb->simulated == 0) {
			Physic::joints[Physic::num_joints++] = joints[i];
			rb->simulate();
		}
	}
}

/*
 */
void RigidBody::addImpulse(const vec3 &point,const vec3 &impulse) {
	velocity += impulse / mass;
	angularMomentum += cross(point - pos,impulse);
	angularVelocity = iWorldInertiaTensor * angularMomentum;
}

/*****************************************************************************/
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

/*
 */
void RigidBody::calcForce(float ifps) {
	
	force = vec3(0,0,0);
	torque = vec3(0,0,0);	
	
	if(!frozen) force.z += Physic::gravitation * mass;

	force -= velocity * 0.1f;
}

/*
 */
void RigidBody::findContacts(float ifps) {
	
	static vec3 old_velicity;
	static vec3 old_angularMomentum;
	static vec3 old_angularVelocity;
	static Position old_pos;
	static mat3 old_orientation;
	static mat3 old_iWorldInertiaTensor;
	
	old_velicity = velocity;
	old_angularMomentum = angularMomentum;
	old_angularVelocity = angularVelocity;
	old_pos = pos;
	old_orientation = orienation;
	old_iWorldInertiaTensor = iWorldInertiaTensor;
	
	// predicted new position
	integrateVelocity(ifps);
	
	integratePos(ifps);
	
	// find conact points
	if(collide_type == COLLIDE_MESH) collide->collide(object);
	else if(collide_type == COLLIDE_SPHERE) collide->collide(object,object->pos,object->getRadius());
	
	velocity = old_velicity;	// restore old values
	angularMomentum = old_angularMomentum;
	angularVelocity = old_angularVelocity;
	pos = old_pos;
	orienation = old_orientation;
	iWorldInertiaTensor = old_iWorldInertiaTensor;
}

/*
 */
void RigidBody::integrateVelocity(float ifps) {
	
	velocity += force * ifps / mass;
	
	float vel = velocity.length();
	if(vel > Physic::velocity_max) velocity *= Physic::velocity_max / vel;
	
	angularMomentum += torque * ifps;
	angularVelocity = iWorldInertiaTensor * angularMomentum;
}

/*
 */
void RigidBody::integratePos(float ifps) {
	
	pos += velocity * ifps;
	
	mat3 m;
	m[0] = 0.0; m[3] = -angularVelocity[2]; m[6] = angularVelocity[1];
	m[1] = angularVelocity[2]; m[4] = 0.0; m[7] = -angularVelocity[0];
	m[2] = -angularVelocity[1]; m[5] = angularVelocity[0]; m[8] = 0.0;
	orienation += (m * orienation) * ifps;
	orienation.orthonormalize();
	
	iWorldInertiaTensor = orienation * iBodyInertiaTensor * orienation.transpose();
	
	transform = mat4(orienation);
	transform[12] = pos.x;
	transform[13] = pos.y;
	transform[14] = pos.z;
	itransform = transform.inverse();
	
	object->transform = transform;
	object->itransform = itransform;
	
	object->updatePos(pos);
}

/*
 */
int RigidBody::contactsResponse(float ifps,int zero_restitution) {
	
	int done = 1;
	
	for(int i = 0; i < collide->num_contacts; i++) {
		Collide::Contact *c = &collide->contacts[i];
		
		if(c->object->rigidbody) {	// rigidbody - rigidbody contact
			
			RigidBody *rb = c->object->rigidbody;
			vec3 r0 = c->point - pos;
			vec3 r1 = c->point - rb->pos;
			
			vec3 vel = (cross(angularVelocity,r0) + velocity) - (cross(rb->angularVelocity,r1) + rb->velocity);
			
			float normal_vel = c->normal * vel;
			if(normal_vel > -EPSILON) continue;
			
			float impulse_numerator;
			if(!zero_restitution) impulse_numerator = -(1.0f + restitution) * normal_vel;
			else impulse_numerator = -normal_vel + c->depth * Physic::penetration_speed / ifps;
			
			if(impulse_numerator < EPSILON) continue;
			
			vec3 tangent = -(vel - c->normal * normal_vel);
			
			done = 0;
			
			float impulse_denominator = 1.0f / mass + 1.0f / rb->mass
				+ c->normal * cross(iWorldInertiaTensor * cross(r0,c->normal),r0)
				+ c->normal * cross(rb->iWorldInertiaTensor * cross(r1,c->normal),r1);
			
			vec3 impulse = c->normal * impulse_numerator / impulse_denominator;
			
			if(frozen == 0 && immovable == 0) {
				velocity += impulse / mass;
				angularMomentum += cross(r0,impulse);
				angularVelocity = iWorldInertiaTensor * angularMomentum;
			}
			if(rb->frozen == 0 && rb->immovable == 0) {
				rb->velocity -= impulse / rb->mass;
				rb->angularMomentum -= cross(r1,impulse);
				rb->angularVelocity = rb->iWorldInertiaTensor * rb->angularMomentum;
			}
			
			// friction
			if(tangent.normalize() < EPSILON) continue;
			
			vel = (cross(angularVelocity,r0) + velocity) - (cross(rb->angularVelocity,r1) + rb->velocity);
			
			float tangent_vel = tangent * vel;
			if(tangent_vel > -EPSILON) continue;
			
			float friction_numerator = -tangent_vel * friction;
			float friction_denominator = 1.0f / mass + 1.0f / rb->mass +
				tangent * cross(iWorldInertiaTensor * cross(r0,tangent),r0) +
				tangent * cross(rb->iWorldInertiaTensor * cross(r1,tangent),r1);
			
			impulse = tangent * friction_numerator / friction_denominator;
			
			if(!frozen) {
				velocity += impulse / mass;
				angularMomentum += cross(r0,impulse);
				angularVelocity = iWorldInertiaTensor * angularMomentum;
			}
			
			if(!rb->frozen) {
				rb->velocity -= impulse / rb->mass;
				rb->angularMomentum -= cross(r1,impulse);
				rb->angularVelocity = rb->iWorldInertiaTensor * rb->angularMomentum;
			}
		} else {	// rigidbody - scene contact
			
			if(frozen) continue;
			
			vec3 r = c->point - pos;
			
			vec3 vel = cross(angularVelocity,r) + velocity;

			float normal_vel = c->normal * vel;
			if(normal_vel > -EPSILON) continue;
			
			float impulse_numerator;
			if(!zero_restitution) impulse_numerator = -(1.0f + restitution) * normal_vel;
			else impulse_numerator = -normal_vel + c->depth * Physic::penetration_speed / ifps;
			
			if(impulse_numerator < EPSILON) continue;
			
			vec3 tangent = -(vel - c->normal * normal_vel);
			
			done = 0;
			
			float impulse_denominator = 1.0f / mass + c->normal * cross(iWorldInertiaTensor * cross(r,c->normal),r);
			
			vec3 impulse = c->normal * impulse_numerator / impulse_denominator;
			
			velocity += impulse / mass;
			angularMomentum += cross(r,impulse);
			angularVelocity = iWorldInertiaTensor * angularMomentum;
			
			// friction
			if(tangent.normalize() < EPSILON) continue;
			
			vel = cross(angularVelocity,r) + velocity;

			float tangent_vel = tangent * vel;
			if(tangent_vel > -EPSILON) continue;
			
			float friction_numerator = -tangent_vel * friction;
			float friction_denominator = 1.0f / mass + tangent * cross(iWorldInertiaTensor * cross(r,tangent),r);
			
			impulse = tangent * friction_numerator / friction_denominator;
			
			velocity += impulse / mass;
			angularMomentum += cross(r,impulse);
			angularVelocity = iWorldInertiaTensor * angularMomentum;
		}
	}
	return done;
}
