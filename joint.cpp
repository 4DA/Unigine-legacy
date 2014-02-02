/* Joints
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
#include "physic.h"
#include "rigidbody.h"
#include "joint.h"

Joint::Joint(RigidBody *rigidbody_0,RigidBody *rigidbody_1) : rigidbody_0(rigidbody_0), rigidbody_1(rigidbody_1) {
	
	if(rigidbody_0->num_joints == RigidBody::NUM_JOINTS || rigidbody_1->num_joints == RigidBody::NUM_JOINTS) {
		rigidbody_0 = NULL;
		rigidbody_1 = NULL;
		fprintf(stderr,"Joint::Joint(): many joints in RigidBody\n");
		return;
	}
	
	rigidbody_0->joints[rigidbody_0->num_joints] = this;
	rigidbody_0->joined_rigidbodies[rigidbody_0->num_joints++] = rigidbody_1;
	
	rigidbody_1->joints[rigidbody_1->num_joints] = this;
	rigidbody_1->joined_rigidbodies[rigidbody_1->num_joints++] = rigidbody_0;
	
	Joint **j = new Joint*[++Physic::all_joints];
	memcpy(j,Physic::joints,sizeof(Joint*) * (Physic::all_joints - 1));
	if(Physic::joints) delete Physic::joints;
	Physic::joints = j;
}

Joint::~Joint() {
	
	if(!rigidbody_0 || !rigidbody_1) return;
	
	for(int i = 0, j = 0; i < rigidbody_0->num_joints; i++) {
		if(i != j) {
			rigidbody_0->joints[j] = rigidbody_0->joints[i];
			rigidbody_0->joined_rigidbodies[j] = rigidbody_0->joined_rigidbodies[i];
		}
		if(rigidbody_0->joints[i] != this) j++;
	}
	rigidbody_0->num_joints--;
	
	for(int i = 0, j = 0; i < rigidbody_1->num_joints; i++) {
		if(i != j) {
			rigidbody_1->joints[j] = rigidbody_1->joints[i];
			rigidbody_1->joined_rigidbodies[j] = rigidbody_1->joined_rigidbodies[i];
		}
		if(rigidbody_1->joints[i] != this) j++;
	}
	rigidbody_1->num_joints--;
	
	if(Physic::all_joints) Physic::all_joints--;
	else {
		delete Physic::joints;
		Physic::joints = NULL;
	}
}

/*
 */
int Joint::response(float) {
	fprintf(stderr,"Joint::response()\n");
	return 1;
}

/*
 */
int Joint::restriction_response(float ifps,const vec3 &point_0,const vec3 &point_1,float min_dist) {
	
	if(min_dist == JOINT_DIST * 2.0f) return 1;
	
	vec3 p0 = rigidbody_0->transform * point_0;
	vec3 p1 = rigidbody_1->transform * point_1;
	
	vec3 r0 = p0 - rigidbody_0->pos;
	vec3 r1 = p1 - rigidbody_1->pos;
	
	vec3 vel0 = cross(rigidbody_0->angularVelocity,r0) + rigidbody_0->velocity;
	vec3 vel1 = cross(rigidbody_1->angularVelocity,r1) + rigidbody_1->velocity;
	
	vec3 normal = p0 - p1 + vel0 * ifps - vel1 * ifps;
	float dist = normal.length();
	if(dist >= min_dist) return 1;
	
	normal.normalize();
	
	vec3 vel = vel0 - vel1;
	float normal_vel = normal * vel;
	
	float impulse_numerator = -normal_vel + (min_dist - dist) * Physic::penetration_speed / ifps;
	float impulse_denominator = 1.0f / rigidbody_0->mass + 1.0f / rigidbody_1->mass
		+ normal * cross(rigidbody_0->iWorldInertiaTensor * cross(r0,normal),r0)
		+ normal * cross(rigidbody_1->iWorldInertiaTensor * cross(r1,normal),r1);
	
	if(rigidbody_0->immovable == 0) rigidbody_0->addImpulse(p0,normal * impulse_numerator / impulse_denominator);
	if(rigidbody_1->immovable == 0) rigidbody_1->addImpulse(p1,-normal * impulse_numerator / impulse_denominator);
	
	if(rigidbody_0->immovable && rigidbody_1->immovable == 0) rigidbody_1->addImpulse(p1,-normal * impulse_numerator / impulse_denominator);
	if(rigidbody_1->immovable && rigidbody_0->immovable == 0) rigidbody_0->addImpulse(p0,normal * impulse_numerator / impulse_denominator);
	
	return 0;
}

/*****************************************************************************/
/*                                                                           */
/* JointBall                                                                 */
/*                                                                           */
/*****************************************************************************/

JointBall::JointBall(RigidBody *rigidbody_0,RigidBody *rigidbody_1,const vec3 &point,const vec3 &restriction_axis_0,const vec3 &restriction_axis_1,float restriction_angle) : Joint(rigidbody_0,rigidbody_1) {
	
	point_0 = rigidbody_0->itransform * point;
	point_1 = rigidbody_1->itransform * point;
	
	vec3 a;
	a = restriction_axis_0;
	a.normalize();
	restriction_point_0 = rigidbody_0->itransform * (point + a * JOINT_DIST);
	a = restriction_axis_1;
	a.normalize();
	restriction_point_1 = rigidbody_1->itransform * (point - a * JOINT_DIST);
	restriction_min_dist = sqrt(2.0f * JOINT_DIST * JOINT_DIST * (1.0f - cos(restriction_angle * DEG2RAD)));
}

JointBall::~JointBall() {
	
}

/*
 */
int JointBall::response(float ifps) {
	
	vec3 p0 = rigidbody_0->transform * point_0;
	vec3 p1 = rigidbody_1->transform * point_1;
	
	vec3 r0 = p0 - rigidbody_0->pos;
	vec3 r1 = p1 - rigidbody_1->pos;
	
	vec3 vel0 = cross(rigidbody_0->angularVelocity,r0) + rigidbody_0->velocity;
	vec3 vel1 = cross(rigidbody_1->angularVelocity,r1) + rigidbody_1->velocity;
	
	vec3 vel = (p0 - p1) * Physic::penetration_speed / ifps + vel0 - vel1;
	
	float normal_vel = vel.length();
	if(normal_vel < EPSILON) return 1;
	
	if(normal_vel > Physic::velocity_max) normal_vel = Physic::velocity_max;
	
	vec3 normal = vel;
	normal.normalize();
	
	float impulse_numerator = -normal_vel;
	float impulse_denominator = 1.0f / rigidbody_0->mass + 1.0f / rigidbody_1->mass
		+ normal * cross(rigidbody_0->iWorldInertiaTensor * cross(r0,normal),r0)
		+ normal * cross(rigidbody_1->iWorldInertiaTensor * cross(r1,normal),r1);
	
	if(rigidbody_0->immovable == 0) rigidbody_0->addImpulse(p0,normal * impulse_numerator / impulse_denominator);
	if(rigidbody_1->immovable == 0) rigidbody_1->addImpulse(p1,-normal * impulse_numerator / impulse_denominator);
	
	if(rigidbody_0->immovable && rigidbody_1->immovable == 0) rigidbody_1->addImpulse(p1,-normal * impulse_numerator / impulse_denominator);
	if(rigidbody_1->immovable && rigidbody_0->immovable == 0) rigidbody_0->addImpulse(p0,normal * impulse_numerator / impulse_denominator);
	
	// angle restriction
	restriction_response(ifps,restriction_point_0,restriction_point_1,restriction_min_dist);
	
	return 0;
}

/*****************************************************************************/
/*                                                                           */
/* JointHinge                                                                 */
/*                                                                           */
/*****************************************************************************/

JointHinge::JointHinge(RigidBody *rigidbody_0,RigidBody *rigidbody_1,const vec3 &point,const vec3 &axis,const vec3 &restriction_axis_0,const vec3 &restriction_axis_1,float restriction_angle) : Joint(rigidbody_0,rigidbody_1) {
	
	itransform_0 = rigidbody_0->itransform;
	itransform_1 = rigidbody_1->itransform;
	this->point = point;
	
	point_00 = itransform_0 * (point);
	point_01 = itransform_1 * (point);
	
	setAxis0(axis);
	setAxis1(axis);
	
	vec3 a;
	a = restriction_axis_0;
	a.normalize();
	restriction_point_0 = rigidbody_0->itransform * (point + a * JOINT_DIST);
	a = restriction_axis_1;
	a.normalize();
	restriction_point_1 = rigidbody_1->itransform * (point - a * JOINT_DIST);
	restriction_min_dist = sqrt(2.0f * JOINT_DIST * JOINT_DIST * (1.0f - cos(restriction_angle * DEG2RAD)));
}

JointHinge::~JointHinge() {
	
}

/*
 */
void JointHinge::setAxis0(const vec3 &axis) {
	axis_0 = axis;
	axis_0.normalize();
	
	point_10 = itransform_0 * (point + axis_0 * JOINT_DIST);
}

/*
 */
void JointHinge::setAxis1(const vec3 &axis) {
	axis_1 = axis;
	axis_1.normalize();
	
	point_11 = itransform_1 * (point + axis_1 * JOINT_DIST);
}

/*
 */
void JointHinge::setAngularVelocity0(float velocity) {
	rigidbody_0->angularMomentum = rigidbody_0->orienation * (axis_0 * velocity);
	rigidbody_0->angularVelocity = rigidbody_0->iWorldInertiaTensor * rigidbody_0->angularMomentum;
}

/*
 */
void JointHinge::setAngularVelocity1(float velocity) {
	rigidbody_1->angularMomentum = rigidbody_1->orienation * (axis_1 * velocity);
	rigidbody_1->angularVelocity = rigidbody_1->iWorldInertiaTensor * rigidbody_1->angularMomentum;
}

/*
 */
int JointHinge::response(float ifps) {
	
	for(int i = 0; i < 2; i++) {
		vec3 p0,p1;
		if(i == 0) {
			p0 = rigidbody_0->transform * point_00;
			p1 = rigidbody_1->transform * point_01;
		} else {
			p0 = rigidbody_0->transform * point_10;
			p1 = rigidbody_1->transform * point_11;
		}
		
		vec3 r0 = p0 - rigidbody_0->pos;
		vec3 r1 = p1 - rigidbody_1->pos;
		
		vec3 vel0 = cross(rigidbody_0->angularVelocity,r0) + rigidbody_0->velocity;
		vec3 vel1 = cross(rigidbody_1->angularVelocity,r1) + rigidbody_1->velocity;
		
		vec3 vel = (p0 - p1) * Physic::penetration_speed / ifps + vel0 - vel1;
		
		float normal_vel = vel.length();
		if(normal_vel < EPSILON) continue;
		
		if(normal_vel > Physic::velocity_max) normal_vel = Physic::velocity_max;
		
		vec3 normal = vel;
		normal.normalize();
		
		float impulse_numerator = -normal_vel;
		float impulse_denominator = 1.0f / rigidbody_0->mass + 1.0f / rigidbody_1->mass
			+ normal * cross(rigidbody_0->iWorldInertiaTensor * cross(r0,normal),r0)
			+ normal * cross(rigidbody_1->iWorldInertiaTensor * cross(r1,normal),r1);
		
		if(rigidbody_0->immovable == 0) rigidbody_0->addImpulse(p0,normal * impulse_numerator / impulse_denominator);
		if(rigidbody_1->immovable == 0) rigidbody_1->addImpulse(p1,-normal * impulse_numerator / impulse_denominator);
		
		if(rigidbody_0->immovable && rigidbody_1->immovable == 0) rigidbody_1->addImpulse(p1,-normal * impulse_numerator / impulse_denominator);
		if(rigidbody_1->immovable && rigidbody_0->immovable == 0) rigidbody_0->addImpulse(p0,normal * impulse_numerator / impulse_denominator);
	}
	
	// angle restriction
	restriction_response(ifps,restriction_point_0,restriction_point_1,restriction_min_dist);
	
	return 0;
}

/*****************************************************************************/
/*                                                                           */
/* JointUniversal                                                            */
/*                                                                           */
/*****************************************************************************/

JointUniversal::JointUniversal(RigidBody *rigidbody_0,RigidBody *rigidbody_1,const vec3 &point,const vec3 &axis_0,const vec3 &axis_1,const vec3 &restriction_axis_0,const vec3 &restriction_axis_1,float restriction_angle) : Joint(rigidbody_0,rigidbody_1) {
	
	this->point = point;
	
	this->axis_0 = axis_0;
	this->axis_0.normalize();
	
	this->axis_1 = axis_1;
	this->axis_1.normalize();
	
	point_00 = rigidbody_0->itransform * point;
	point_01 = rigidbody_1->itransform * point;
	
	point_10 = rigidbody_0->itransform * (point + this->axis_0 * JOINT_DIST);
	point_11 = rigidbody_1->itransform * (point + this->axis_0 * JOINT_DIST);
	
	this->axis_0 = rigidbody_0->itransform.rotation() * this->axis_0;
	this->axis_1 = rigidbody_1->itransform.rotation() * this->axis_1;
	
	vec3 a;
	a = restriction_axis_0;
	a.normalize();
	restriction_point_0 = rigidbody_0->itransform * (point + a * JOINT_DIST);
	a = restriction_axis_1;
	a.normalize();
	restriction_point_1 = rigidbody_1->itransform * (point - a * JOINT_DIST);
	restriction_min_dist = sqrt(2.0f * JOINT_DIST * JOINT_DIST * (1.0f - cos(restriction_angle * DEG2RAD)));
}

JointUniversal::~JointUniversal() {
	
}

/*
 */
int JointUniversal::response(float ifps) {
	
	for(int i = 0; i < 2; i++) {
		vec3 p0,p1;
		if(i == 0) {
			p0 = rigidbody_0->transform * point_00;
			p1 = rigidbody_1->transform * point_01;
		} else {
			p0 = rigidbody_0->transform * point_10;
			p1 = rigidbody_1->transform * point_11;
		}
		
		vec3 r0 = p0 - rigidbody_0->pos;
		vec3 r1 = p1 - rigidbody_1->pos;
		
		vec3 vel0 = cross(rigidbody_0->angularVelocity,r0) + rigidbody_0->velocity;
		vec3 vel1 = cross(rigidbody_1->angularVelocity,r1) + rigidbody_1->velocity;
		
		vec3 vel = (p0 - p1) * Physic::penetration_speed / ifps + vel0 - vel1;
	
		if(i == 1) {
			vec3 ax0 = rigidbody_0->transform.rotation() * axis_1;
			vec3 ax1 = rigidbody_1->transform.rotation() * axis_1;
			
			vec3 a = ax0 + ax1;
			a.normalize();
			
			vec3 d = a * ((p0 - p1) * a);
			vel = d * Physic::penetration_speed / ifps + a * ((vel0 - vel1) * a);
		}
		
		float normal_vel = vel.length();
		if(normal_vel < EPSILON) continue;
		
		if(normal_vel > Physic::velocity_max) normal_vel = Physic::velocity_max;
		
		vec3 normal = vel;
		normal.normalize();
		
		float impulse_numerator = -normal_vel;
		float impulse_denominator = 1.0f / rigidbody_0->mass + 1.0f / rigidbody_1->mass
			+ normal * cross(rigidbody_0->iWorldInertiaTensor * cross(r0,normal),r0)
			+ normal * cross(rigidbody_1->iWorldInertiaTensor * cross(r1,normal),r1);
		
		if(rigidbody_0->immovable == 0) rigidbody_0->addImpulse(p0,normal * impulse_numerator / impulse_denominator);
		if(rigidbody_1->immovable == 0) rigidbody_1->addImpulse(p1,-normal * impulse_numerator / impulse_denominator);
		
		if(rigidbody_0->immovable && rigidbody_1->immovable == 0) rigidbody_1->addImpulse(p1,-normal * impulse_numerator / impulse_denominator);
		if(rigidbody_1->immovable && rigidbody_0->immovable == 0) rigidbody_0->addImpulse(p0,normal * impulse_numerator / impulse_denominator);
	}
	
	// angle restriction
	restriction_response(ifps,restriction_point_0,restriction_point_1,restriction_min_dist);
	
	return 0;
}
