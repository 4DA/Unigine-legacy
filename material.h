/* Material
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

#ifndef __MATERIAL_H__
#define __MATERIAL_H__

#include "engine.h"
#include "shader.h"

class Texture;

class Material {
public:
	
	Material(const char *name);
	~Material();
	
	void load(const char *name);
	
	int enable();
	void disable();
	void bind();
	
	void bindTexture(int unit,Texture *texture);
	
	int blend;
	
	GLuint sfactor;
	GLuint dfactor;
	
	int alpha_test;
	
	GLuint alpha_func;
	float alpha_ref;
	
	GLuint getBlendFactor(const char *factor);
	GLuint getAlphaFunc(const char *func);
	
	Shader *light_shader;
	Shader *ambient_shader;
	vec4 parameters[Shader::NUM_PARAMETERS];
	Texture *textures[Shader::NUM_TEXTURES];
	
	static Material *old_material;
};

#endif /* __MATERIAL_H__ */
