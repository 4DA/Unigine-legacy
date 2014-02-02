/* Collide
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

#ifndef __COLLIDE_H__
#define __COLLIDE_H__

#include "mathlib.h"
#include "bsp.h"
#include "position.h"

class Object;

class Collide {
public:
	
	Collide();
	~Collide();
	
	int collide(Object *object,const vec3 &pos,float radius);
	int collide(Object *object,const Position &pos,float radius);
	
	int collide(Object *object);
	
	void sort();
	
	enum {
		NUM_CONTACTS = 32,
		NUM_OBJECTS = 16,
	};
	
	struct Contact {
		Object *object;
		Material *material;
		vec3 point;
		vec3 normal;
		float depth;
	};
	
	int num_contacts;
	Contact *contacts;
	
	int num_objects;
	Object **objects;
	
protected:
	
	int addContact(Object *object,Material *material,const vec3 &point,const vec3 &normal,float depth,int min_depth = 0);
	void collideObjectSphere(Object *object,const vec3 &pos,float radius);
	void collideObjectMesh(Object *object);
	
	enum {
		NUM_TRIANGLES = 1024,
		NUM_SURFACES = 16,
	};
	
	struct Triangle {
		vec3 v[3];			// vertexes
		vec4 plane;			// plane
		vec4 c[3];			// fast point in triangle
	};
	
	struct Surface {
		int num_triangles;		// triangles
		Triangle *triangles;
		vec3 center;			// bound sphere
		float radius;
		vec3 min;				// bound box
		vec3 max;
	};
	
	static int counter;
	
	static Position position;
	
	static int num_surfaces;
	static Surface *surfaces;
};

#endif /* __COLLIDE_H__ */
