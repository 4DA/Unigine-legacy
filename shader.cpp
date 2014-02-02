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

#include "engine.h"
#include "light.h"
#include "parser.h"
#include "shader.h"

/*
 */
Shader *Shader::old_shader;
vec4 Shader::parameters[NUM_PARAMETERS];
Texture *Shader::old_textures[NUM_TEXTURES];

/*
 */
Shader::Shader(const char *name) {
	load(name);
}

Shader::~Shader() {
	if(vertex_target == GL_VERTEX_PROGRAM_ARB) {
		if(vertex_id) glDeleteProgramsARB(1,&vertex_id);
	}
	if(fragment_target == GL_FRAGMENT_PROGRAM_ARB) {
		if(fragment_id) glDeleteProgramsARB(1,&fragment_id);
	} else if(fragment_target == GL_FRAGMENT_PROGRAM_NV) {
		if(fragment_id) glDeleteProgramsNV(1,&fragment_id);
	} else if(fragment_target == GL_COMBINE) {
		glDeleteLists(fragment_id,1);
	}
}

/*****************************************************************************/
/*                                                                           */
/* load shader                                                               */
/*                                                                           */
/*****************************************************************************/

/*
 */
void Shader::load(const char *name) {
	Parser *parser = new Parser(name);
	// matrixes
	num_matrixes = 0;
	for(int i = 0; i < NUM_MATRIXES; i++) {
		char buf[1024];
		sprintf(buf,"matrix%d",i);
		if(parser->get(buf)) {
			matrixes[num_matrixes].num = i;
			getMatrix(parser->get(buf),&matrixes[num_matrixes]);
			num_matrixes++;
		}
	}
	// vertex program local parameters
	num_vertex_parameters = 0;
	for(int i = 0; i < NUM_LOCAL_PARAMETERS; i++) {
		char buf[1024];
		sprintf(buf,"vertex_local%d",i);
		if(parser->get(buf)) {
			vertex_parameters[num_vertex_parameters].num = i;
			getLocalParameter(parser->get(buf),&vertex_parameters[num_vertex_parameters]);
			num_vertex_parameters++;
		}
	}
	// fragement program local parameters
	num_fragment_parameters = 0;
	for(int i = 0; i < NUM_LOCAL_PARAMETERS; i++) {
		char buf[1024];
		sprintf(buf,"fragment_local%d",i);
		if(parser->get(buf)) {
			fragment_parameters[num_fragment_parameters].num = i;
			getLocalParameter(parser->get(buf),&fragment_parameters[num_fragment_parameters]);
			num_fragment_parameters++;
		}
	}
	char *data;
	// vertex program
	vertex_target = 0;
	vertex_id = 0;
	if((data = parser->get("vertex"))) {
		int error = -1;
		if(!strncmp(data,"!!ARBvp1.0",10)) {
			vertex_target = GL_VERTEX_PROGRAM_ARB;
			glGenProgramsARB(1,&vertex_id);
			glBindProgramARB(vertex_target,vertex_id);
			glProgramStringARB(vertex_target,GL_PROGRAM_FORMAT_ASCII_ARB,strlen(data),data);
			glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB,&error);
		} else {
			char *s = data;
			while(*s != '\0' && *s != '\n') s++;
			*s = '\0';
			fprintf(stderr,"Shader::Shader(): unknown vertex program header \"%s\" in \"%s\" file\n",data,name);
		}
		if(error != -1) {
			int line = 0;
			char *s = data;
			while(error-- && *s) if(*s++ == '\n') line++;
			while(s >= data && *s != '\n') s--;
			char *e = ++s;
			while(*e != '\0' && *e != '\n') e++;
			*e = '\0';
			fprintf(stderr,"Shader::Shader(): vertex program error in \"%s\" file at line %d:\n\"%s\"\n",name,line,s);
		}
	}
	// fragment program
	fragment_target = 0;
	fragment_id = 0;
	if((data = parser->get("fragment"))) {
		int error = -1;
		if(!strncmp(data,"!!ARBfp1.0",10)) {
			fragment_target = GL_FRAGMENT_PROGRAM_ARB;
			glGenProgramsARB(1,&fragment_id);
			glBindProgramARB(fragment_target,fragment_id);
			glProgramStringARB(fragment_target,GL_PROGRAM_FORMAT_ASCII_ARB,strlen(data),data);
			glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB,&error);
		} else if(!strncmp(data,"!!FP1.0",7)) {
			fragment_target = GL_FRAGMENT_PROGRAM_NV;
			glGenProgramsNV(1,&fragment_id);
			glBindProgramNV(fragment_target,fragment_id);
			glLoadProgramNV(fragment_target,fragment_id,strlen(data),(GLubyte*)data);
			glGetIntegerv(GL_PROGRAM_ERROR_POSITION_NV,&error);
		} else if(!strncmp(data,"!!ARBtec1.0",11)) {	// arb texture env combine
			fragment_target = GL_COMBINE;
			fragment_id = compileARBtec(data);
		} else {
			char *s = data;
			while(*s != '\0' && *s != '\n') s++;
			*s = '\0';
			fprintf(stderr,"Shader::Shader(): unknown fragment program header \"%s\" in \"%s\" file\n",data,name);
		}
		if(error != -1) {
			int line = 0;
			char *s = data;
			while(error-- && *s) if(*s++ == '\n') line++;
			while(s >= data && *s != '\n') s--;
			char *e = ++s;
			while(*e != '\0' && *e != '\n') e++;
			*e = '\0';
			fprintf(stderr,"Shader::Shader(): fragment program error in \"%s\" file at line %d:\n\"%s\"\n",name,line,s);
		}
	}
	delete parser;
}

/*
 */
GLuint Shader::compileARBtec(const char *src) {
	const char *s = src;
	GLuint list = glGenLists(1);
	glNewList(list,GL_COMPILE);
	if(strncmp("!!ARBtec1.0",s,11)) {
		fprintf(stderr,"Shader::compileARBtec(): unknown header\n");
		glEndList();
		return list;
	}
	s += 11;
	int unit = -1;
	while(*s) {
		if(*s == '#') while(*s && *s != '\n') s++;
		else if(strchr(" \t\n\r",*s)) s++;
		else if(!strncmp("END",s,3)) break;
		else {
			int sources = 0;
			if(unit > 0) glActiveTexture(GL_TEXTURE0 + unit);
			while(*s) {
				if(strchr(" \t",*s)) s++;
				else if(!strncmp("nop",s,3) && (s += 3)) {
					if(++unit > 0) glActiveTexture(GL_TEXTURE0 + unit);
					glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
				}
				else if(!strncmp("replace",s,7) && (s += 7)) {
					if(++unit > 0) glActiveTexture(GL_TEXTURE0 + unit);
					glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE);
					glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB,GL_REPLACE);
				}
				else if(!strncmp("modulate",s,8) && (s += 8)) {
					if(++unit > 0) glActiveTexture(GL_TEXTURE0 + unit);
					glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE);
					glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB,GL_MODULATE);
				}
				else if(!strncmp("add",s,3) && (s += 3)) {
					if(++unit > 0) glActiveTexture(GL_TEXTURE0 + unit);
					glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE);
					glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB,GL_ADD);
				}
				else if(!strncmp("add_signed",s,10) && (s += 10)) {
					if(++unit > 0) glActiveTexture(GL_TEXTURE0 + unit);
					glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE);
					glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB,GL_ADD_SIGNED);
				}
				else if(!strncmp("sub",s,3) && (s += 3)) {
					if(++unit > 0) glActiveTexture(GL_TEXTURE0 + unit);
					glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE);
					glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB,GL_SUBTRACT);
				}
				else if(!strncmp("dot3",s,4) && (s += 4)) {
					if(++unit > 0) glActiveTexture(GL_TEXTURE0 + unit);
					glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE);
					glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB,GL_DOT3_RGB);
				}
				else if(!strncmp("primary",s,7) && (s += 7)) glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB + sources++,GL_PRIMARY_COLOR);
				else if(!strncmp("texture0",s,8) && (s += 8)) glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB + sources++,GL_TEXTURE0);
				else if(!strncmp("texture1",s,8) && (s += 8)) glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB + sources++,GL_TEXTURE1);
				else if(!strncmp("texture2",s,8) && (s += 8)) glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB + sources++,GL_TEXTURE2);
				else if(!strncmp("texture3",s,8) && (s += 8)) glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB + sources++,GL_TEXTURE3);
				else if(!strncmp("texture",s,7) && (s += 7)) glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB + sources++,GL_TEXTURE0 + unit);
				else if(!strncmp("previous",s,8) && (s += 8)) glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB + sources++,GL_PREVIOUS);
				else if(!strncmp("scale2",s,6) && (s += 6)) glTexEnvi(GL_TEXTURE_ENV,GL_RGB_SCALE,2);
				else if(!strncmp("scale4",s,6) && (s += 6)) glTexEnvi(GL_TEXTURE_ENV,GL_RGB_SCALE,4);
				else if(*s == '\n' && s++) break;
				else {
					printf("%s\n",s);
					char buf[1024];
					char *d = buf;
					while(*s && !strchr(" \t\n\r",*s)) *d++ = *s++;
					*d = '\0';
					fprintf(stderr,"Shader::compileARBtec(): unknown token \"%s\"\n",buf);
					glEndList();
					return list;
				}
			}
			for(int i = 0; i < sources; i++) glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_RGB + i,GL_SRC_COLOR);
		}
	}
	if(unit > 1) glActiveTexture(GL_TEXTURE0);
	glEndList();
	return list;
}

/*
 */
void Shader::getMatrix(const char *name,Matrix *m) {
	if(!strcmp(name,"projection")) m->type = PROJECTION;
	else if(!strcmp(name,"modelview")) m->type = MODELVIEW;
	else if(!strcmp(name,"imodelview")) m->type = IMODELVIEW;
	else if(!strcmp(name,"transform")) m->type = TRANSFORM;
	else if(!strcmp(name,"itransform")) m->type = ITRANSFORM;
	else if(!strcmp(name,"light_transform")) m->type = LIGHT_TRANSFORM;
	else fprintf(stderr,"Shader::getMatrix(): unknown matrix \"%s\"\n",name);
}

void Shader::getLocalParameter(const char *name,Shader::LocalParameter *p) {
	if(!strcmp(name,"time")) p->type = TIME;
	else if(!strcmp(name,"sin")) p->type = SIN;
	else if(!strcmp(name,"cos")) p->type = COS;
	else if(!strcmp(name,"camera")) p->type = CAMERA;
	else if(!strcmp(name,"icamera")) p->type = ICAMERA;
	else if(!strcmp(name,"light")) p->type = LIGHT;
	else if(!strcmp(name,"ilight")) p->type = ILIGHT;
	else if(!strcmp(name,"light_color")) p->type = LIGHT_COLOR;
	else if(!strcmp(name,"fog_color")) p->type = FOG_COLOR;
	else if(!strcmp(name,"viewport")) p->type = VIEWPORT;
	else if(!strncmp(name,"parameter",9)) {
		p->type = PARAMETER;
		sscanf(name + 9,"%d",&p->parameter);
		if(p->parameter >= NUM_PARAMETERS) {
			fprintf(stderr,"Shader::getLocalParameter(): number of parameters is big\n");
			p->parameter = 0;
		}
	}
	else fprintf(stderr,"Shader::getLocalParameter(): unknown parameter \"%s\"\n",name);
}

/*
 */
void Shader::setParameter(int num,const vec4 &parameter) {
	parameters[num] = parameter;
}

/*****************************************************************************/
/*                                                                           */
/* enable/disable/bind                                                       */
/*                                                                           */
/*****************************************************************************/

/*
 */
void Shader::enable() {
	if(old_shader == this) return;
	if(old_shader) {
		if(old_shader->vertex_target != vertex_target) {
			if(old_shader->vertex_target) glDisable(old_shader->vertex_target);
			if(vertex_target) glEnable(vertex_target);
		}
		if(old_shader->fragment_target != fragment_target) {
			if(old_shader->fragment_target == GL_FRAGMENT_PROGRAM_ARB) glDisable(old_shader->fragment_target);
			else if(old_shader->fragment_target == GL_FRAGMENT_PROGRAM_NV) glDisable(old_shader->fragment_target);
			else if(old_shader->fragment_target == GL_COMBINE) {
				glActiveTexture(GL_TEXTURE0);
				glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
				glActiveTexture(GL_TEXTURE1);
				glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
				glActiveTexture(GL_TEXTURE2);
				glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
				glActiveTexture(GL_TEXTURE3);
				glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
				glActiveTexture(GL_TEXTURE0);
			}
			if(fragment_target == GL_FRAGMENT_PROGRAM_ARB) glEnable(fragment_target);
			else if(fragment_target == GL_FRAGMENT_PROGRAM_NV) glEnable(fragment_target);
		}
	} else {
		if(vertex_target) glEnable(vertex_target);
		if(fragment_target == GL_FRAGMENT_PROGRAM_ARB) glEnable(fragment_target);
		else if(fragment_target == GL_FRAGMENT_PROGRAM_NV) glEnable(fragment_target);
	}
}

/*
 */
void Shader::disable() {
	if(vertex_target) glDisable(vertex_target);
	if(fragment_target == GL_FRAGMENT_PROGRAM_ARB) glDisable(fragment_target);
	else if(fragment_target == GL_FRAGMENT_PROGRAM_NV) glDisable(fragment_target);
	else if(fragment_target == GL_COMBINE) {
		glActiveTexture(GL_TEXTURE0);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
		glActiveTexture(GL_TEXTURE1);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
		glActiveTexture(GL_TEXTURE2);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
		glActiveTexture(GL_TEXTURE3);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
	}
	for(int i = 0; i < NUM_TEXTURES; i++) {
		if(old_textures[i]) {
			glActiveTexture(GL_TEXTURE0 + i);
			old_textures[i]->disable();
		}
		old_textures[i] = NULL;
	}
	glActiveTexture(GL_TEXTURE0);
	old_shader = NULL;
}

/*
 */
void Shader::bind() {
	if(old_shader != this) {
		if(vertex_target == GL_VERTEX_PROGRAM_ARB) glBindProgramARB(vertex_target,vertex_id);
		if(fragment_target == GL_FRAGMENT_PROGRAM_ARB) glBindProgramARB(fragment_target,fragment_id);
		else if(fragment_target == GL_FRAGMENT_PROGRAM_NV) glBindProgramNV(fragment_target,fragment_id);
		else if(fragment_target == GL_COMBINE) glCallList(fragment_id);
		old_shader = this;
	}
	for(int i = 0; i < num_matrixes; i++) {
		glMatrixMode(GL_MATRIX0_ARB + matrixes[i].num);
		if(matrixes[i].type == PROJECTION) glLoadMatrixf(Engine::projection);
		else if(matrixes[i].type == MODELVIEW) glLoadMatrixf(Engine::modelview);
		else if(matrixes[i].type == IMODELVIEW) glLoadMatrixf(Engine::imodelview);
		else if(matrixes[i].type == TRANSFORM) glLoadMatrixf(Engine::transform);
		else if(matrixes[i].type == ITRANSFORM) glLoadMatrixf(Engine::itransform);
		else if(matrixes[i].type == LIGHT_TRANSFORM && Engine::current_light) glLoadMatrixf(Engine::current_light->transform);
	}
	if(num_matrixes) glMatrixMode(GL_MODELVIEW);
	for(int i = 0; i < num_vertex_parameters; i++) {
		if(vertex_parameters[i].type == TIME) glProgramLocalParameter4fvARB(vertex_target,vertex_parameters[i].num,vec4(Engine::time,Engine::time / 2.0f,Engine::time / 3.0f,Engine::time / 5.0f));
		else if(vertex_parameters[i].type == SIN) glProgramLocalParameter4fvARB(vertex_target,vertex_parameters[i].num,vec4(sin(Engine::time),sin(Engine::time / 2.0f),sin(Engine::time / 3.0f),sin(Engine::time / 5.0f)));
		else if(vertex_parameters[i].type == COS) glProgramLocalParameter4fvARB(vertex_target,vertex_parameters[i].num,vec4(cos(Engine::time),cos(Engine::time / 2.0f),cos(Engine::time / 3.0f),cos(Engine::time / 5.0f)));
		else if(vertex_parameters[i].type == CAMERA) glProgramLocalParameter4fvARB(vertex_target,vertex_parameters[i].num,vec4(Engine::camera,1));
		else if(vertex_parameters[i].type == ICAMERA) glProgramLocalParameter4fvARB(vertex_target,vertex_parameters[i].num,vec4(Engine::itransform * Engine::camera,1));
		else if(vertex_parameters[i].type == LIGHT) glProgramLocalParameter4fvARB(vertex_target,vertex_parameters[i].num,Engine::light);
		else if(vertex_parameters[i].type == ILIGHT) glProgramLocalParameter4fvARB(vertex_target,vertex_parameters[i].num,vec4(Engine::itransform * vec3(Engine::light),Engine::light.w));
		else if(vertex_parameters[i].type == LIGHT_COLOR) glProgramLocalParameter4fvARB(vertex_target,vertex_parameters[i].num,Engine::light_color);
		else if(vertex_parameters[i].type == FOG_COLOR) glProgramLocalParameter4fvARB(vertex_target,vertex_parameters[i].num,Engine::fog_color);
		else if(vertex_parameters[i].type == VIEWPORT) glProgramLocalParameter4fvARB(vertex_target,vertex_parameters[i].num,vec4(Engine::viewport[0],Engine::viewport[1],Engine::viewport[2],Engine::viewport[3]));
		else if(vertex_parameters[i].type == PARAMETER) glProgramLocalParameter4fvARB(vertex_target,vertex_parameters[i].num,parameters[vertex_parameters[i].parameter]);
	}
	for(int i = 0; i < num_fragment_parameters; i++) {
		if(vertex_parameters[i].type == TIME) glProgramLocalParameter4fvARB(fragment_target,fragment_parameters[i].num,vec4(Engine::time,Engine::time / 2.0f,Engine::time / 3.0f,Engine::time / 5.0f));
		else if(vertex_parameters[i].type == SIN) glProgramLocalParameter4fvARB(fragment_target,fragment_parameters[i].num,vec4(sin(Engine::time),sin(Engine::time / 2.0f),sin(Engine::time / 3.0f),sin(Engine::time / 5.0f)));
		else if(vertex_parameters[i].type == COS) glProgramLocalParameter4fvARB(fragment_target,fragment_parameters[i].num,vec4(cos(Engine::time),cos(Engine::time / 2.0f),cos(Engine::time / 3.0f),cos(Engine::time / 5.0f)));
		else if(fragment_parameters[i].type == CAMERA) glProgramLocalParameter4fvARB(fragment_target,fragment_parameters[i].num,vec4(Engine::camera,1));
		else if(fragment_parameters[i].type == ICAMERA) glProgramLocalParameter4fvARB(fragment_target,fragment_parameters[i].num,vec4(Engine::itransform * Engine::camera,1));
		else if(fragment_parameters[i].type == LIGHT) glProgramLocalParameter4fvARB(fragment_target,fragment_parameters[i].num,Engine::light);
		else if(fragment_parameters[i].type == ILIGHT) glProgramLocalParameter4fvARB(fragment_target,fragment_parameters[i].num,vec4(Engine::itransform * vec3(Engine::light),Engine::light.w));
		else if(fragment_parameters[i].type == LIGHT_COLOR) glProgramLocalParameter4fvARB(fragment_target,fragment_parameters[i].num,Engine::light_color);
		else if(fragment_parameters[i].type == FOG_COLOR) glProgramLocalParameter4fvARB(fragment_target,fragment_parameters[i].num,Engine::fog_color);
		else if(fragment_parameters[i].type == VIEWPORT) glProgramLocalParameter4fvARB(fragment_target,fragment_parameters[i].num,vec4(Engine::viewport[0],Engine::viewport[1],Engine::viewport[2],Engine::viewport[3]));
		else if(fragment_parameters[i].type == PARAMETER) glProgramLocalParameter4fvARB(fragment_target,fragment_parameters[i].num,parameters[fragment_parameters[i].parameter]);
	}
}

/*****************************************************************************/
/*                                                                           */
/* bind texture                                                              */
/*                                                                           */
/*****************************************************************************/
/*
 */
void Shader::bindTexture(int unit,Texture *texture) {
	if(old_textures[unit] == texture) return;
	glActiveTexture(GL_TEXTURE0 + unit);
	if(texture) {
		if(old_textures[unit] == NULL) texture->enable();
		else if(texture->target != old_textures[unit]->target) {
			old_textures[unit]->disable();
			texture->enable();
		}
		texture->bind();
	} else {
		if(old_textures[unit]) old_textures[unit]->disable();
	}
	old_textures[unit] = texture;
}
