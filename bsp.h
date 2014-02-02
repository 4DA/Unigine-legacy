/* Bsp
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

#ifndef __BSP_H__
#define __BSP_H__

#include <stdio.h>
#include "mathlib.h"

#define BSP_MAGIC ('B' | ('S' << 8) | ('P' << 16) | ('M' << 24))

class Mesh;
class Material;
class Object;
class ObjectMesh;

/*
 */
class Node {
public:
	
	Node();
	~Node();
	
	void create(Mesh *mesh);
	void load(FILE *file);
	void save(FILE *file);
	
	void bindMaterial(const char *name,Material *material);
	void render();
	
	enum {
		TRIANGLES_PER_NODE = 1024,
	};
	
	vec3 min;			// bound box
	vec3 max;
	vec3 center;		// bound sphere
	float radius;
	
	Node *left,*right;	// childrens
	
	ObjectMesh *object;	// object
};

/*
 */
class Portal {
public:
	
	Portal();
	~Portal();
	
	void create(Mesh *mesh,int s);
	
	void getScissor(int *scissor);
	
	void render();
	
	vec3 center;		// bond sphere
	float radius;
	
	int num_sectors;
	int *sectors;
	
	vec3 points[4];

	int frame;
};

/*
 */
class Sector {
public:
	
	Sector();
	~Sector();
	
	void create(Mesh *mesh,int s);
	void getNodeObjects(Node *node);
	void create();
	
	int inside(const vec3 &point);
	int inside(Portal *portal);
	int inside(const vec3 &center,float radius);
	int inside(Mesh *mesh,int s);
	
	void addObject(Object *object);
	void removeObject(Object *object);
	
	void bindMaterial(const char *name,Material *material);
	void render(Portal *portal = NULL);
	
	void saveState();
	void restoreState(int frame);
	
	enum {
		NUM_OBJECTS = 256,
	};
	
	vec3 center;					// bound sphere
	float radius;
	
	int num_planes;					// bound
	vec4 *planes;
	
	Node *root;						// binary tree
	
	int num_portals;				// portals
	int *portals;
	
	int num_objects;				// dynamic objects
	Object **objects;
	
	int num_node_objects;			// static object from the node
	Object **node_objects;
	
	int num_visible_objects;		// only visible objects
	Object **visible_objects;
	
	Portal *portal;					// sector is visible through this portal
	
	int frame;

	int old_num_visible_objects;	// save/restore state
	Object **old_visible_objects;
	Portal *old_portal;
	int old_frame;
};

/*
 */
class Bsp {
public:
	
	Bsp();
	~Bsp();
	
	void load(const char *name);
	void save(const char *name);
	
	void bindMaterial(const char *name,Material *material);
	void render();
	
	void saveState();
	void restoreState(int frame);
	
	static int num_portals;
	static Portal *portals;

	static int num_sectors;
	static Sector *sectors;
	
	static int num_visible_sectors;
	static Sector **visible_sectors;

	static int old_num_visible_sectors;
	static Sector **old_visible_sectors;
};

#endif /* __BSP_H__ */
