/* Fog
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

#ifndef __FOG_H__
#define __FOG_H__

#include "mathlib.h"
#include "bsp.h"
#include "position.h"

#define FOG_DEPTH_TO_RGB	"fog_depth_to_rgb.shader"
#define FOG_PASS			"fog_pass.shader"
#define FOG_FAIL			"fog_fail.shader"
#define FOG_FINAL			"fog_final.shader"

class Mesh;
class PBuffer;
class Texture;
class Shader;

class Fog {
public:
	
	Fog(Mesh *mesh,const vec4 &color);
	~Fog();
	
	void enable();
	void disable();
	
	int render();
	
	float getDensity(const vec3 &point);
	int inside(const vec3 &point);
	
	const vec3 &getMin();
	const vec3 &getMax();
	const vec3 &getCenter();
	float getRadius();
	
	Position pos;	// position
	Mesh *mesh;		// mesh
	vec4 color;		// color
	
protected:
	
	static int counter;	// usage counter
	
	static PBuffer *pbuffer;
	static Texture *depth_tex;
	static Texture *fog_tex;
	
	static PBuffer *pbuffers[3];	// three different resolution 128/256/512
	static Texture *depth_texes[3];
	static Texture *fog_texes[3];
	
	static Shader *depth_to_rgb_shader;
	static Shader *pass_shader;
	static Shader *fail_shader;
	static Shader *final_shader;
};

#endif /* __FOG_H__ */
