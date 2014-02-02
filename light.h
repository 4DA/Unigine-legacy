/* Light
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

#ifndef __LIGHT_H__
#define __LIGHT_H__

#include "mathlib.h"
#include "bsp.h"
#include "position.h"

class Material;
class Flare;

class Light {
public:
	
	Light(const vec3 &pos,float radius,const vec4 &color,int shadows = 1);
	~Light();
	
	void update(float ifps);
	
	int bindMaterial(const char *name,Material *material);
	void setFlare(Flare *flare);
	
	void renderFlare();
	
	void set(const vec3 &p);
	void set(const mat4 &m);
	void setColor(const vec4 &c);
	
	void getScissor(int *scissor);
	
	Position pos;
	mat4 transform;
	
	float radius;
	vec4 color;
	
	int shadows;
	
	Material *material;
	
	Flare *flare;
	
	float time;
};

#endif /* __LIGHT_H__ */
