/* Shader
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

#ifndef __SHADER_H__
#define __SHADER_H__

#include "engine.h"
#include "mathlib.h"

class Shader {
public:
	
	Shader(const char *name);
	~Shader();
	
	void load(const char *name);
	
	static void setParameter(int num,const vec4 &parameter);
	
	void enable();
	void disable();
	void bind();
	
	void bindTexture(int unit,Texture *texture);
	
	enum {
		NUM_MATRIXES = 4,
		NUM_PARAMETERS = 2,
		NUM_LOCAL_PARAMETERS = 4,
		NUM_TEXTURES = 6,
	};
	
	static Shader *old_shader;
	
protected:
	
	enum {
		TIME = 1,
		SIN,
		COS,
		CAMERA,
		ICAMERA,
		LIGHT,
		ILIGHT,
		LIGHT_COLOR,
		FOG_COLOR,
		VIEWPORT,
		PARAMETER,
		PROJECTION,
		MODELVIEW,
		IMODELVIEW,
		TRANSFORM,
		ITRANSFORM,
		LIGHT_TRANSFORM,
	};
	
	struct Matrix {
		int num;	// matrix number
		int type;	// matrix type
	};
	
	struct LocalParameter {
		int num;		// parameter number
		int type;		// parameter type
		int parameter;
	};
	
	GLuint compileARBtec(const char *src);
	
	void getMatrix(const char *name,Matrix *m);
	void getLocalParameter(const char *name,LocalParameter *p);
	
	int num_matrixes;
	Matrix matrixes[NUM_MATRIXES];
	
	int num_vertex_parameters;
	LocalParameter vertex_parameters[NUM_LOCAL_PARAMETERS];
	
	int num_fragment_parameters;
	LocalParameter fragment_parameters[NUM_LOCAL_PARAMETERS];
	
	GLuint vertex_target;
	GLuint vertex_id;
	GLuint fragment_target;
	GLuint fragment_id;
	
	static vec4 parameters[NUM_PARAMETERS];
	static Texture *old_textures[Shader::NUM_TEXTURES];
};

#endif /* __SHADER_H__ */
