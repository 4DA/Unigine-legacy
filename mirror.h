/* Mirror
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

#ifndef __MIRROR_H__
#define __MIRROR_H__

#include "mathlib.h"
#include "bsp.h"
#include "position.h"

class Mesh;
class Material;
class PBuffer;
class Texture;

class Mirror {
public:
	
	Mirror(Mesh *mesh);
	~Mirror();
	
	int bindMaterial(const char *name,Material *material);
	
	void enable();
	void disable();
	
	void render();
	
	const vec3 &getMin();
	const vec3 &getMax();
	const vec3 &getCenter();
	float getRadius();
	
	Position pos;			// position
	Mesh *mesh;				// mesh
	vec4 plane;				// plane
	
	Material *material;		// material
	
protected:
	
	mat4 modelview;			// new modelview matrix
	
	PBuffer *pbuffer;
	
	Texture *mirror_tex;
	Texture *mirror_texes[3];	// three different resolution 128/256/512
	
	Position old_camera;
	mat4 old_modelview;
	mat4 old_imodelview;
	
	static int counter;
	static PBuffer *pbuffers[3];
};

#endif /* __MIRROR_H__ */
