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

#ifndef __JOINT_H__
#define __JOINT_H__

#include "mathlib.h"

#define JOINT_DIST 10.0f

class Physic;
class RigidBody;

/*
 */
class Joint {
public:
	
	Joint(RigidBody *rigidbody_0,RigidBody *rigidbody_1);
	virtual ~Joint();
	
protected:
	
	friend class Physic;
	
	virtual int response(float ifps);
	
	int restriction_response(float ifps,const vec3 &point_0,const vec3 &point_1,float min_dist);
	
	RigidBody *rigidbody_0;
	RigidBody *rigidbody_1;
};

/*
 */
class JointBall : public Joint {
public:
	
	JointBall(RigidBody *rigidbody_0,RigidBody *rigidbody_1,const vec3 &point,const vec3 &restriction_axis_0 = vec3(1,0,0),const vec3 &restriction_axis_1 = vec3(1,0,0),float restriction_angle = 180.0f);
	virtual ~JointBall();
	
protected:
	
	virtual int response(float ifps);
	
	vec3 point_0;
	vec3 point_1;

	vec3 restriction_point_0;
	vec3 restriction_point_1;
	float restriction_min_dist;
};

/*
 */
class JointHinge : public Joint {
public:
	
	JointHinge(RigidBody *rigidbody_0,RigidBody *rigidbody_1,const vec3 &point,const vec3 &hinge_axis,const vec3 &restriction_axis_0 = vec3(1,0,0),const vec3 &restriction_axis_1 = vec3(1,0,0),float restriction_angle = 180.0f);
	virtual ~JointHinge();
	
	void setAxis0(const vec3 &axis);
	void setAxis1(const vec3 &axis);
	void setAngularVelocity0(float velocity);
	void setAngularVelocity1(float velocity);
	
protected:
	
	virtual int response(float ifps);
	
	vec3 point_00;
	vec3 point_01;
	
	vec3 point_10;
	vec3 point_11;
	
	vec3 point;
	vec3 axis_0;
	vec3 axis_1;
	mat4 itransform_0;
	mat4 itransform_1;
	
	vec3 restriction_point_0;
	vec3 restriction_point_1;
	float restriction_min_dist;	
};

/*
 */
class JointUniversal : public Joint {
public:
	
	JointUniversal(RigidBody *rigidbody_0,RigidBody *rigidbody_1,const vec3 &point,const vec3 &axis_0,const vec3 &axis_1,const vec3 &restiction_axis_0 = vec3(1,0,0),const vec3 &restriction_axis_1 = vec3(1,0,0),float restriction_angle = 180.0f);
	virtual ~JointUniversal();
	
protected:
	
	virtual int response(float ifps);
	
	vec3 point_00;
	vec3 point_01;
	
	vec3 point_10;
	vec3 point_11;
	
	vec3 point;
	vec3 axis_0;
	vec3 axis_1;
	
	vec3 restriction_point_0;
	vec3 restriction_point_1;
	float restriction_min_dist;
};

#endif /* __JOINT_H__ */
