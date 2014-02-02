/* Physic
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
#include <stdlib.h>
#include "rigidbody.h"
#include "collide.h"
#include "joint.h"
#include "physic.h"

float Physic::time = 0.0;
float Physic::time_step = 1.0f / 50.0f;
float Physic::gravitation = -9.8f * 0.5f;
float Physic::velocity_max = 20.0f;
float Physic::velocity_threshold = 0.1f * 0.1f;
float Physic::angularVelocity_threshold = (2.0f * DEG2RAD) * (2.0f * DEG2RAD);
float Physic::time_to_frost = 1.0f / 10.f;
float Physic::penetration_speed = 1.0f / 5.0f;
int Physic::num_first_iterations = 5;
int Physic::num_second_iterations = 15;

int Physic::all_joints = 0;
int Physic::num_joints = 0;
Joint **Physic::joints = NULL;

int Physic::all_rigidbodies = 0;
int Physic::num_rigidbodies = 0;
RigidBody **Physic::rigidbodies = NULL;

/*
 */
int rigidbody_cmp(const void *a,const void *b) {
	RigidBody *r0 = *(RigidBody**)a;
	RigidBody *r1 = *(RigidBody**)b;
	if(r0->pos.z > r1->pos.z) return 1;
	if(r0->pos.z < r1->pos.z) return -1;
	return 0;
}

/*
 */
void Physic::update(float ifps) {
	
	qsort(rigidbodies,num_rigidbodies,sizeof(RigidBody*),rigidbody_cmp);
	
	time += ifps;
	
	while(time > time_step) {
		
		time -= time_step;
		
		for(int i = 0; i < num_rigidbodies; i++) {
			RigidBody *rb = rigidbodies[i];
			
			rb->force = vec3(0,0,0);
			rb->torque = vec3(0,0,0);
			
			if(rb->frozen == 0) rb->findContacts(time_step);
			
			rb->frozen = 0;
		}
		
		for(int i = 0; i < num_first_iterations; i++) {
			int done = 1;
			for(int j = 0; j < num_rigidbodies; j++) {
				if(rigidbodies[j]->contactsResponse(time_step) == 0) done = 0;
			}
			for(int j = 0; j < num_joints; j++) {
				joints[j]->response(time_step);
			}
			if(done) break;
		}
		
		for(int i = 0; i < num_rigidbodies; i++) {
			RigidBody *rb = rigidbodies[i];
			
			if(rb->collide->num_contacts > 0 && rb->num_joints == 0 &&
				rb->velocity * rb->velocity < velocity_threshold &&
				rb->angularVelocity * rb->angularVelocity < angularVelocity_threshold) {
				
				rb->frozen_time += time_step;
				if(rb->frozen_time > time_to_frost) {
					if(rb->frozen_num_objects == rb->collide->num_objects) {
						rb->frozen = 1;
						rb->velocity = vec3(0,0,0);
						rb->angularVelocity = vec3(0,0,0);
					} else {
						rb->frozen = 0;
						rb->frozen_time = 0.0;
						rb->findContacts(time_step);
					}
				}
				rb->frozen_num_objects = rb->collide->num_objects;
			} else {
				rb->frozen = 0;
				rb->frozen_time = 0.0;
				rb->findContacts(time_step);
			}
			
			rb->calcForce(time_step);
			rb->integrateVelocity(time_step);
		}
		
		for(int i = 0; i < num_second_iterations; i++) {
			int done = 1;
			for(int j = 0; j < num_rigidbodies; j++) {
				if(rigidbodies[j]->contactsResponse(time_step,true) == 0) done = 0;
			}
			for(int j = 0; j < num_joints; j++) {
				joints[j]->response(time_step);
			}
			if(done) break;
		}
		
		for(int i = 0; i < num_rigidbodies; i++) {
			if(rigidbodies[i]->frozen == 0) rigidbodies[i]->integratePos(time_step);
		}
	}
	
	// new simulate - new objects
	for(int i = 0; i < num_rigidbodies; i++) {
		rigidbodies[i]->simulated = 0;
		rigidbodies[i]->immovable = 0;
	}
	
	num_rigidbodies = 0;
	num_joints = 0;
}
