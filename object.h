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

#ifndef __OBJECT_H__
#define __OBJECT_H__

#include "bsp.h"
#include "position.h"

class Material;
class RigidBody;

class Object {
public:
	
	Object(int type);
	virtual ~Object();
	
	virtual void update(float ifps);	// update function
	void updatePos(const vec3 &p);		// update position
	
	int bindMaterial(const char *name,Material *material);
	
	enum {
		RENDER_ALL = 0,
		RENDER_OPACITY,
		RENDER_TRANSPARENT
	};
	
	virtual int render(int t = RENDER_ALL,int s = -1) = 0;
	
	virtual void findSilhouette(const vec4 &light,int s = -1) = 0;
	virtual int getNumIntersections(const vec3 &line0,const vec3 &line1,int s = -1) = 0;
	virtual int renderShadowVolume(int s = -1)  = 0;
	
	virtual int intersection(const vec3 &line0,const vec3 &line1,vec3 &point,vec3 &normal,int s = -1) = 0;
	
	virtual int getNumSurfaces() = 0;
	virtual const char *getSurfaceName(int s) = 0;
	virtual int getSurface(const char *name) = 0;
	
	virtual const vec3 &getMin(int s = -1) = 0;
	virtual const vec3 &getMax(int s = -1) = 0;
	virtual const vec3 &getCenter(int s = -1) = 0;
	virtual float getRadius(int s = -1) = 0;
	
	void setRigidBody(RigidBody *rigidbody);
	
	void setShadows(int shadows);
	
	virtual void set(const vec3 &p);	// set position
	virtual void set(const mat4 &m);	// set transformation
	
	void enable();				// enable transformation
	void disable();				// disable
	
	enum {
		OBJECT_MESH = 0,
		OBJECT_SKINNEDMESH,
		OBJECT_PARTICLES
	};
	
	int type;					// type of the object
	
	Position pos;				// position of the object
	
	RigidBody *rigidbody;		// rigidbody dynamic
	
	int is_identity;
	mat4 transform;
	mat4 itransform;
	
	mat4 old_modelview;			// save old matrixes
	mat4 old_imodelview;
	mat4 old_transform;
	mat4 old_itransform;
	
	Material **materials;		// all materials
	
	int num_opacities;			// opacitie surfaces
	int *opacities;
	
	int num_transparents;		// transparent surfaces
	int *transparents;
	
	int shadows;
	
	float time;					// object time
	int frame;
};

#endif /* __OBJECT_H__ */
