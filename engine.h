/* 3D Engine
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

#ifndef __ENGINE_H__
#define __ENGINE_H__

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#pragma warning(disable: 4244)
#pragma warning(disable: 4305)
#else
#include <unistd.h>
#include <fcntl.h>
#endif
#include <string.h>

#include <GL/gl.h>
#include <GL/glext.h>
#ifdef _WIN32
#include "win32/glext.h"
#endif

#include <map>
#include <string>
#include <vector>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

class PBuffer;
class Console;
class Frustum;
class Bsp;
class Mesh;
class Light;
class Fog;
class Mirror;
class Shader;
class Texture;
class Material;

#include "mathlib.h"
#include "bsp.h"
#include "position.h"
#include "texture.h"

#define ENGINE_SCREEN_MATERIAL		"screen.mat"
#define ENGINE_FONT_NAME			"font.png"
#define ENGINE_SPHERE_MESH			"sphere.mesh"
#define ENGINE_SHADOW_VOLUME_SHADER	"shadow_volume.shader"
#define ENGINE_LOG_NAME				"Engine.log"
#define ENGINE_GAP_SIZE				256

class Engine {
public:
	
	static int init(const char *config = NULL);
	static void clear();
	
	static int match(const char *mask,const char *name);
	
	static void addPath(const char *path);
	static const char *findFile(const char *name);
	
	static void define(const char *define);
	static void undef(const char *define);
	static int isDefine(const char *define);
	
	// loaders
	static void load(const char *name);
	
	static Shader *loadShader(const char *name);
	static Texture *loadTexture(const char *name,GLuint target = Texture::TEXTURE_2D,int flag = texture_filter);
	static Material *loadMaterial(const char *name);
	static Mesh *loadMesh(const char *name);
	
	static void reload();
	static void reload_shaders();
	static void reload_textures();
	static void reload_materials();
	static void setExternLoad(void (*func)(void*),void *data);
	
	static void addObject(Object *object);
	static void removeObject(Object *object);
	
	static void addLight(Light *light);
	static void removeLight(Light *light);
	
	static void addFog(Fog *fog);
	static void removeFog(Fog *fog);
	
	static void addMirror(Mirror *mirror);
	static void removeMirror(Mirror *mirror);
	
	static void update(float ifps);
	
	// renderers
	static void render_light();
	static void render_transparent();
	static void render(float ifps);
	
	// intersection line with scene
	static Object *intersection(const vec3 &line0,const vec3 &line1,vec3 &point,vec3 &normal);
	
	// renderer info
	static char *vendor;
	static char *renderer;
	static char *version;
	static char *extensions;
	
	// screen
	static int screen_width;
	static int screen_height;
	static int screen_multisample;
	static PBuffer *screen;
	static Texture *screen_texture;
	static Material *screen_material;
	
	// console
	static Console *console;
	
	// objects
	static Position camera;
	static Frustum *frustum;
	
	static Bsp *bsp;
	
	static int num_objects;
	static Object **objects;
	
	// lights
	static int num_lights;
	static Light **lights;
	static int num_visible_lights;
	static Light **visible_lights;
	static Light *current_light;
	
	// fogs
	static int num_fogs;
	static Fog **fogs;
	static int num_visible_fogs;
	static Fog **visible_fogs;
	static Fog *current_fog;
	
	// mirrors
	static int num_mirrors;
	static Mirror **mirrors;
	static int num_visible_mirrors;
	static Mirror **visible_mirrors;
	static Mirror *current_mirror;
	
	// occlusion test
	static GLuint query_id;
	static Mesh *sphere_mesh;
	
	// shadow volume
	static Shader *shadow_volume_shader;
	
	// current state
	static float time;
	static vec4 light;
	static vec4 light_color;
	static vec4 fog_color;
	static int viewport[4];
	static mat4 projection;
	static mat4 modelview;
	static mat4 imodelview;
	static mat4 transform;
	static mat4 itransform;
	
	// 1.0f / fps
	static float ifps;
	
	// current frame
	static int frame;

	// all triangles
	static int num_triangles;
	
	// renderer abilities
	static int have_occlusion;
	static int have_stencil_two_side;
	
	// engine toggles
	static int wareframe_toggle;
	static int scissor_toggle;
	static int shadows_toggle;
	static int show_shadows_toggle;
	static int fog_toggle;
	static int mirror_toggle;
	static int physic_toggle;
	
	// path
	static std::vector<char*> path;
	
	// defines
	static std::vector<char*> defines;
	
	// default texture filter
	static int texture_filter;
	
	// object managment
	static std::map<std::string,Shader*> shaders;
	static std::map<std::string,Texture*> textures;
	static std::map<std::string,Material*> materials;
	static std::map<std::string,Mesh*> meshes;
	
	static void (*extern_load)(void*);
	static void *extern_load_data;
	
	// new stderr descriptor
	static int stderr_fd;
};

#endif /* __ENGINE_H__ */
