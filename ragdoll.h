/* RagDoll
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

#ifndef __RAGDOLL_H__
#define __RAGDOLL_H__

#include "mathlib.h"
#include "skinnedmesh.h"

class Object;
class RigidBody;

class RagDoll {
public:
	RagDoll(SkinnedMesh *skinnedmesh,const char *name);
	~RagDoll();
	
	void update();
	
	void setTransform(const mat4 &m);
	
	void set(const mat4 &m);

protected:
	
	SkinnedMesh *skinnedmesh;
	
	int num_bones;
	SkinnedMesh::Bone *bones;
	
	Object **meshes;
	RigidBody **rigidbodies;
	mat4 *offsets;
	mat4 *ioffsets;
	
	int root;
	
	mat4 transform;
	mat4 itransform;
};

#endif /* __RAGDOLL_H__ */
