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

#include "pbuffer.h"
#include "console.h"
#include "frustum.h"
#include "meshvbo.h"
#include "bsp.h"
#include "position.h"
#include "object.h"
#include "light.h"
#include "fog.h"
#include "mirror.h"
#include "shader.h"
#include "texture.h"
#include "material.h"
#include "physic.h"
#include "map.h"
#include "engine.h"

char *Engine::vendor;
char *Engine::renderer;
char *Engine::version;
char *Engine::extensions;

int Engine::screen_width = 1024;
int Engine::screen_height = 512;
int Engine::screen_multisample = 0;
PBuffer *Engine::screen;
Texture *Engine::screen_texture;
Material *Engine::screen_material;

Console *Engine::console;

Position Engine::camera;
Frustum *Engine::frustum;

Bsp *Engine::bsp;

int Engine::num_objects;
Object **Engine::objects;

int Engine::num_lights;
Light **Engine::lights;
int Engine::num_visible_lights;
Light **Engine::visible_lights;
Light *Engine::current_light;

int Engine::num_fogs;
Fog **Engine::fogs;
int Engine::num_visible_fogs;
Fog **Engine::visible_fogs;
Fog *Engine::current_fog;

int Engine::num_mirrors;
Mirror **Engine::mirrors;
int Engine::num_visible_mirrors;
Mirror **Engine::visible_mirrors;
Mirror *Engine::current_mirror;

GLuint Engine::query_id;
Mesh *Engine::sphere_mesh;

Shader *Engine::shadow_volume_shader;

float Engine::time;
vec4 Engine::light;
vec4 Engine::light_color;
vec4 Engine::fog_color;
int Engine::viewport[4];
mat4 Engine::projection;
mat4 Engine::modelview;
mat4 Engine::imodelview;
mat4 Engine::transform;
mat4 Engine::itransform;

float Engine::ifps;

int Engine::frame;

int Engine::num_triangles;

int Engine::have_occlusion;
int Engine::have_stencil_two_side;

int Engine::wareframe_toggle;
int Engine::scissor_toggle;
int Engine::shadows_toggle;
int Engine::show_shadows_toggle;
int Engine::fog_toggle;
int Engine::mirror_toggle;
int Engine::physic_toggle;

std::vector<char*> Engine::path;
std::vector<char*> Engine::defines;

int Engine::texture_filter = Texture::LINEAR_MIPMAP_LINEAR;

std::map<std::string,Shader*> Engine::shaders;
std::map<std::string,Texture*> Engine::textures;
std::map<std::string,Material*> Engine::materials;
std::map<std::string,Mesh*> Engine::meshes;

void (*Engine::extern_load)(void*) = NULL;
void *Engine::extern_load_data;

int Engine::stderr_fd;

/*****************************************************************************/
/*                                                                           */
/* console commands                                                          */
/*                                                                           */
/*****************************************************************************/

static void define(int argc,char **argv,void*) {
	if(argc == 1) {
		Engine::console->printf("defines: ");
		for(int i = 0; i < (int)Engine::defines.size(); i++) {
			Engine::console->printf("%s%s",Engine::defines[i],i == (int)Engine::defines.size() - 1 ? "" : ", ");
		}
		Engine::console->printf("\n");
	} else {
		for(int i = 1; i < argc; i++) Engine::define(argv[i]);
	}
}

static void undef(int argc,char **argv,void*) {
	if(argc == 1) Engine::console->printf("undef: missing argument\n");
	else {
		for(int i = 1; i < argc; i++) Engine::undef(argv[i]);
	}
}

static void load(int argc,char **argv,void*) {
	if(argc == 1) Engine::console->printf("load: missing argument\n");
	else Engine::load(argv[1]);
}

static void reload(int argc,char **argv,void*) {
	if(argc == 1) Engine::reload();
	else {
		if(!strcmp(argv[1],"shaders")) Engine::reload_shaders();
		else if(!strcmp(argv[1],"textures")) Engine::reload_textures();
		else if(!strcmp(argv[1],"materials")) Engine::reload_materials();
		else Engine::console->printf("reload: unknown argument (shaders, textures, materials)\n");
	}
}

static void status(int,char**,void*) {
	Engine::console->printf("lights: %d\nfogs: %d\nmirrors: %d\nobjects: %d\n",
		Engine::num_lights,Engine::num_fogs,Engine::num_mirrors,Engine::num_objects);
}

static void extensions(int,char**,void*) {
	Engine::console->printf("%s\n",Engine::extensions);
}

/*****************************************************************************/
/*                                                                           */
/* Engine                                                                    */
/*                                                                           */
/*****************************************************************************/

/*
 */
int Engine::init(const char *config) {
	
	// bind system stderr to Engine::console
#ifndef _WIN32
	int fd[2];
	pipe(fd);
	stderr_fd = fd[0];
	fcntl(stderr_fd,F_SETFL,O_NONBLOCK);
	stderr = fdopen(fd[1],"w");
	setbuf(stderr,NULL);
#else
	HANDLE read,write;
	SECURITY_ATTRIBUTES security;
	
	security.nLength = sizeof(security);
	security.bInheritHandle = TRUE;
	security.lpSecurityDescriptor = NULL;
	
	CreatePipe(&read,&write,&security,1024 - 1);
	
	DWORD mode = PIPE_READMODE_BYTE | PIPE_NOWAIT;
	SetNamedPipeHandleState(read,&mode,NULL,NULL);
	SetNamedPipeHandleState(write,&mode,NULL,NULL);
	
	stderr_fd = _open_osfhandle((long)read,O_RDONLY | O_BINARY);
	FILE *file = fdopen(_open_osfhandle((long)write,O_WRONLY | O_BINARY),"w");
	*stderr = *file;
	setbuf(stderr,NULL);
#endif
	
	// load config
	if(config) {
		FILE *file = fopen(config,"rb");
		if(!file) fprintf(stderr,"Engine::init(): error open \"%s\" file\n",config);
		else {
			char buf[1024];
			while(fscanf(file,"%s",buf) == 1) {
				if(buf[0] == '#' || (buf[0] == '/' && buf[1] == '/')) while(fread(buf,1,1,file) == 1 && buf[0] != '\n');
				else if(!strcmp(buf,"screen_width")) fscanf(file,"%d",&screen_width);
				else if(!strcmp(buf,"screen_height")) fscanf(file,"%d",&screen_height);
				else if(!strcmp(buf,"screen_multisample")) fscanf(file,"%d",&screen_multisample);
				else if(!strcmp(buf,"texture_filter")) {
					texture_filter = 0;
					fgets(buf,sizeof(buf),file);
					char *s = buf;
					while(*s) {
						if(isalpha(*s)) {
							char *d = s;
							while(*d && !strchr(" \t\n",*d)) d++;
							if(*d) *d++ = '\0';
							if(!strcmp(s,"nearest")) texture_filter |= Texture::NEAREST;
							else if(!strcmp(s,"linear")) texture_filter |= Texture::LINEAR;
							else if(!strcmp(s,"nearest_mipmap_nearest")) texture_filter |= Texture::NEAREST_MIPMAP_NEAREST;
							else if(!strcmp(s,"linear_mipmap_nearest")) texture_filter |= Texture::LINEAR_MIPMAP_NEAREST;
							else if(!strcmp(s,"linear_mipmap_linear")) texture_filter |= Texture::LINEAR_MIPMAP_LINEAR;
							else if(!strcmp(s,"anisotropy_1")) texture_filter |= Texture::ANISOTROPY_1;
							else if(!strcmp(s,"anisotropy_2")) texture_filter |= Texture::ANISOTROPY_2;
							else if(!strcmp(s,"anisotropy_4")) texture_filter |= Texture::ANISOTROPY_4;
							else if(!strcmp(s,"anisotropy_8")) texture_filter |= Texture::ANISOTROPY_8;
							else if(!strcmp(s,"anisotropy_16")) texture_filter |= Texture::ANISOTROPY_16;
							s = d;
						}
						else if(strchr(" \t",*s)) s++;
						else {
							fprintf(stderr,"Engine::init(): unknown token \"%s\" in \"%s\" file\n",s,config);
							break;
						}
					}
				}
				else fprintf(stderr,"Engine::init(): unknown token \"%s\" in \"%s\" file\n",buf,config);
			}
		}
	}
	
	// init OpenGL extensions win32 only
#ifdef _WIN32
	glext_init();
#endif
	
	// console
	FILE *log = fopen(ENGINE_LOG_NAME,"wb");
	console = new Console(findFile(ENGINE_FONT_NAME),log);
	
	console->printf(1,1,1,"3D Engine\nhttp://frustum.org\n\n");
	
	// renderer abilities
	vendor = (char*)glGetString(GL_VENDOR);
	renderer = (char*)glGetString(GL_RENDERER);
	version = (char*)glGetString(GL_VERSION);
	extensions = (char*)glGetString(GL_EXTENSIONS);
	
	console->printf("vendor: %s\nrenderer: %s\nversion: %s\n\n",vendor,renderer,version);
	
	have_occlusion = 0;
	have_stencil_two_side = 0;
	
	wareframe_toggle = 0;
	scissor_toggle = 1;
	shadows_toggle = 1;
	show_shadows_toggle = 0;
	fog_toggle = 1;
	mirror_toggle = 1;
	physic_toggle = 1;
	
	if(!strstr(extensions,"GL_ARB_vertex_program")) {	// fatal error
		console->printf("can`t find GL_ARB_vertex_program extension");
		return 0;
	}
	
	if(strstr(extensions,"GL_ARB_occlusion_query")) {
		have_occlusion = 1;
		console->printf("find GL_ARB_occlusion_query extension\n");
	}
	if(strstr(extensions,"GL_EXT_stencil_two_side")) {
		have_stencil_two_side = 1;
		console->printf("find GL_EXT_stencil_two_side extension\n");
	}
	if(strstr(extensions,"GL_NV_depth_clamp")) {
		define("DEPTH_CLAMP");
		console->printf("find GL_NV_depth_clamp extension\n");
	}
	
	if(strstr(extensions,"GL_NV_fragment_program")) {			// nv3x cards
		define("NV3X");
		define("TEYLOR");
		define("OFFSET");
		define("HORIZON");
		console->printf("using GL_NV_fragment_program shaders code\n");
	}
	else if(strstr(extensions,"GL_ARB_fragment_program")) {		// radeons
		define("RADEON");
		define("TEYLOR");
		define("OFFSET");
		define("HORIZON");
		console->printf("using GL_ARB_fragment_program shaders code\n");
	}
	else if(strstr(extensions,"GL_ARB_texture_env_combine")) {	// hm
		define("VERTEX");
		if(!isDefine("DEPTH_CLAMP")) shadows_toggle = 0;
		fog_toggle = 0;
		mirror_toggle = 0;
		console->printf("using GL_ARB_texture_env_combine \"shaders\" code\n");
	} else {
		console->printf("pls upgrade you video card...\n");
		return 0;
	}
	
	console->addBool("wareframe",&wareframe_toggle);
	console->addBool("scissor",&scissor_toggle);
	console->addBool("shadows",&shadows_toggle);
	console->addBool("show_shadows",&show_shadows_toggle);
	console->addBool("fog",&fog_toggle);
	console->addBool("mirror",&mirror_toggle);
	console->addBool("physic",&physic_toggle);	
	
	console->addCommand("define",::define,NULL);
	console->addCommand("undef",::undef,NULL);
	console->addCommand("load",::load,NULL);
	console->addCommand("reload",::reload,NULL);
	console->addCommand("extensions",::extensions,NULL);
	
	// screen
	if(screen_multisample == 1) screen = new PBuffer(screen_width,screen_height,PBuffer::RGB | PBuffer::DEPTH | PBuffer::STENCIL | PBuffer::MULTISAMPLE_2);
	else if(screen_multisample == 2) screen = new PBuffer(screen_width,screen_height,PBuffer::RGB | PBuffer::DEPTH | PBuffer::STENCIL | PBuffer::MULTISAMPLE_4);
	else screen = new PBuffer(screen_width,screen_height,PBuffer::RGB | PBuffer::DEPTH | PBuffer::STENCIL);
	screen->enable();
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	screen->disable();	
	screen_texture = new Texture(screen_width,screen_height,Texture::TEXTURE_2D,Texture::RGB | Texture::CLAMP | Texture::LINEAR);
	screen_material = loadMaterial(ENGINE_SCREEN_MATERIAL);
	
	// create new objects
	if(!frustum) frustum = new Frustum;
	
	bsp = NULL;
	
	num_objects = 0;
	objects = NULL;

	num_lights = 0;
	lights = NULL;
	num_visible_lights = 0;
	lights = NULL;
	
	num_fogs = 0;
	fogs = NULL;
	num_visible_fogs = 0;
	visible_fogs = NULL;
	
	num_mirrors = 0;
	mirrors = NULL;
	num_visible_mirrors = 0;
	visible_mirrors = NULL;
	
	glGenQueriesARB(1,&query_id);
	sphere_mesh = loadMesh(ENGINE_SPHERE_MESH);
	
	shadow_volume_shader = loadShader(ENGINE_SHADOW_VOLUME_SHADER);
	
	time = 0;
	frame = 0;
	
	console->printf("\ninit ok\n");

	return 1;
}

/*
 */
void Engine::clear() {
	
	// objects
	if(num_objects) {
		for(int i = 0; i < num_objects; i++) delete objects[i];
		delete objects;
		objects = NULL;
		num_objects = 0;
	}
	
	// lights
	if(num_lights) {
		for(int i = 0; i < num_lights; i++) delete lights[i];
		delete lights;
		lights = NULL;
		num_lights = 0;
		delete visible_lights;
		visible_lights = NULL;
		num_visible_lights = 0;
	}
	
	// fogs
	if(num_fogs) {
		for(int i = 0; i < num_fogs; i++) delete fogs[i];
		delete fogs;
		fogs = NULL;
		num_fogs = 0;
		delete visible_fogs;
		visible_fogs = NULL;
		num_visible_fogs = 0;
	}
	
	// mirrors
	if(num_mirrors) {
		for(int i = 0; i < num_fogs; i++) delete mirrors[i];
		delete mirrors;
		mirrors = NULL;
		num_mirrors = 0;
		delete  visible_mirrors;
		visible_mirrors = NULL;
		num_visible_mirrors = 0;
	}
	
	// bsp at the end
	if(bsp) delete bsp;
	bsp = NULL;
	
	// shaders
	std::map<std::string,Shader*>::iterator shaders_it;
	for(shaders_it = shaders.begin(); shaders_it != shaders.end(); shaders_it++) delete shaders_it->second;
	shaders.clear();
	
	// textures
	std::map<std::string,Texture*>::iterator textures_it;
	for(textures_it = textures.begin(); textures_it != textures.end(); textures_it++) delete textures_it->second;
	textures.clear();
	
	// materials
	std::map<std::string,Material*>::iterator materials_it;
	for(materials_it = materials.begin(); materials_it != materials.end(); materials_it++) delete materials_it->second;
	materials.clear();
	
	// meshes
	std::map<std::string,Mesh*>::iterator meshes_it;
	for(meshes_it = meshes.begin(); meshes_it != meshes.end(); meshes_it++) delete meshes_it->second;
	meshes.clear();
	
	screen_material = loadMaterial(ENGINE_SCREEN_MATERIAL);
	sphere_mesh = loadMesh(ENGINE_SPHERE_MESH);
	shadow_volume_shader = loadShader(ENGINE_SHADOW_VOLUME_SHADER);
}

/*****************************************************************************/
/*                                                                           */
/* loaders                                                                   */
/*                                                                           */
/*****************************************************************************/

/* support '|' as "OR" operation, '?' as ANY symbol and '*' as ANY string
 */
int Engine::match(const char *mask,const char *name) {
	char *m = (char*)mask;
	char *n = (char*)name;
	int match = 1;
	while(1) {
		if((match && *m == '*')|| *m == '\0') break;
		if(*m == '|') {
			if(match) break;
			m++;
			n = (char*)name;
			match = 1;
		} else {
			if(*m != '?' && *m != *n) match = 0;
			if(*n) n++;
			m++;
		}
	}
	return match;
}

/*
 */
void Engine::addPath(const char *path) {
	char *s = (char*)path;
	while(1) {
		char *p = new char[strlen(s) + 1];
		char *d = p;
		while(*s != '\0' && *s != ',') *d++ = *s++;
		*d = '\0';
		Engine::path.push_back(p);
		if(*s == '\0') break;
		else s++;
	}
}

const char *Engine::findFile(const char *name) {
	static char buf[1024];
	for(char *s = (char*)name; *s != '\0'; s++) {
		if(*s == '%' && *(s + 1) == 's') {
			static char complex_name[1024];
			sprintf(complex_name,name,"px");
			s = (char*)findFile(complex_name);
			if(strcmp(s,name)) {
				for(s = buf + strlen(buf); s > buf; s--) {
					if(*s == 'x' && *(s - 1) == 'p') {
						*s = 's';
						*(s - 1) = '%';
						break;
					}
				}
				return buf;	// hm
			}
			break;
		}
	}
	for(int i = (int)path.size() - 1; i >= -1; i--) {
		if(i == -1) sprintf(buf,"%s",name);
		else sprintf(buf,"%s%s",path[i],name);
		FILE *file = fopen(buf,"rb");
		if(file) {
			fclose(file);
			return buf;
		}
	}
	fprintf(stderr,"Engine::findFile(): can`t find \"%s\" file\n",name);
	return name;
}

/*
 */
void Engine::define(const char *define) {
	int i;
	for(i = 0; i < (int)defines.size(); i++) if(!strcmp(defines[i],define)) break;
	if(i == (int)defines.size()) {
		char *d = new char[strlen(define) + 1];
		strcpy(d,define);
		defines.push_back(d);
	}
}

void Engine::undef(const char *define) {
	for(int i = 0; i < (int)defines.size(); i++) {
		if(!strcmp(defines[i],define)) {
			delete defines[i];
			defines.erase(defines.begin() + i);
		}
	}
}

int Engine::isDefine(const char *define) {
	for(int i = 0; i < (int)defines.size(); i++) if(!strcmp(defines[i],define)) return 1;
	return 0;
}

/*
 */
void Engine::load(const char *name) {
	clear();
	Map::load(findFile(name));
	if(extern_load) extern_load(extern_load_data);
	console->printf("load \"%s\" ok\n",name);
}

/*
 */
Shader *Engine::loadShader(const char *name) {
	std::map<std::string,Shader*>::iterator it = shaders.find(name);
	if(it == shaders.end()) {
		Shader *shader = new Shader(findFile(name));
		shaders[name] = shader;
		return shader;
	}
	return it->second;
}

Texture *Engine::loadTexture(const char *name,GLuint target,int flag) {
	std::map<std::string,Texture*>::iterator it = textures.find(name);
	if(it == textures.end()) {
		Texture *texture = new Texture(findFile(name),target,flag);
		textures[name] = texture;
		return texture;
	}
	return it->second;
}

Material *Engine::loadMaterial(const char *name) {
	std::map<std::string,Material*>::iterator it = materials.find(name);
	if(it == materials.end()) {
		Material *material = new Material(findFile(name));
		materials[name] = material;
		return material;
	}
	return it->second;
}

Mesh *Engine::loadMesh(const char *name) {
	std::map<std::string,Mesh*>::iterator it = meshes.find(name);
	if(it == meshes.end()) {
		Mesh *mesh = new MeshVBO(findFile(name));
		meshes[name] = mesh;
		return mesh;
	}
	return it->second;
}

/* reload functions
 */
void Engine::reload() {
	reload_shaders();
	reload_textures();
	reload_materials();
}

void Engine::reload_shaders() {
	std::map<std::string,Shader*>::iterator it;
	for(it = shaders.begin(); it != shaders.end(); it++) {
		it->second->~Shader();
		it->second->load(findFile(it->first.c_str()));
	}
}

void Engine::reload_textures() {
	std::map<std::string,Texture*>::iterator it;
	for(it = textures.begin(); it != textures.end(); it++) {
		it->second->~Texture();
		it->second->load(findFile(it->first.c_str()),it->second->target,it->second->flag);
	}
}

void Engine::reload_materials() {
	std::map<std::string,Material*>::iterator it;
	for(it = materials.begin(); it != materials.end(); it++) {
		it->second->~Material();
		it->second->load(findFile(it->first.c_str()));
	}
}

void Engine::setExternLoad(void (*func)(void*),void *data) {
	extern_load = func;
	extern_load_data = data;
}

/* objects managment
 */
void Engine::addObject(Object *object) {
	if(num_objects % ENGINE_GAP_SIZE == 0) {
		Object **objects = new Object*[num_objects + ENGINE_GAP_SIZE];
		for(int i = 0; i < num_objects; i++) objects[i] = Engine::objects[i];
		if(Engine::objects) delete Engine::objects;
		Engine::objects = objects;
	}
	object->update(0.0);
	objects[num_objects++] = object;
}

void Engine::removeObject(Object *object) {
	if(object->pos.sector != -1) Bsp::sectors[object->pos.sector].removeObject(object);
	for(int i = 0; i < num_objects; i++) {
		if(objects[i] == object) {
			num_objects--;
			for(; i < num_objects; i++) objects[i] = objects[i + 1];
			return;
		}
	}
}

/*
 */
void Engine::addLight(Light *light) {
	if(num_lights % ENGINE_GAP_SIZE == 0) {
		Light **lights = new Light*[num_lights + ENGINE_GAP_SIZE];
		for(int i = 0; i < num_lights; i++) lights[i] = Engine::lights[i];
		if(Engine::lights) delete Engine::lights;
		Engine::lights = lights;
		if(visible_lights) delete visible_lights;
		visible_lights = new Light*[num_lights + ENGINE_GAP_SIZE];
	}
	light->update(0.0);
	lights[num_lights++] = light;
}

void Engine::removeLight(Light *light) {
	for(int i = 0; i < num_lights; i++) {
		if(lights[i] == light) {
			num_lights--;
			for(; i < num_lights; i++) lights[i] = lights[i + 1];
			return;
		}
	}
}

/*
 */
void Engine::addFog(Fog *fog) {
	if(num_fogs % ENGINE_GAP_SIZE == 0) {
		Fog **fogs = new Fog*[num_fogs + ENGINE_GAP_SIZE];
		for(int i = 0; i < num_fogs; i++) fogs[i] = Engine::fogs[i];
		if(Engine::fogs) delete Engine::fogs;
		Engine::fogs = fogs;
		if(visible_fogs) delete visible_fogs;
		visible_fogs = new Fog*[num_lights + ENGINE_GAP_SIZE];
	}
	fogs[num_fogs++] = fog;
}

void Engine::removeFog(Fog *fog) {
	for(int i = 0; i < num_fogs; i++) {
		if(fogs[i] == fog) {
			num_fogs--;
			for(; i < num_fogs; i++) fogs[i] = fogs[i + 1];
			return;
		}
	}
}

/*
 */
void Engine::addMirror(Mirror *mirror) {
	if(num_mirrors % ENGINE_GAP_SIZE == 0) {
		Mirror **mirrors = new Mirror*[num_mirrors + ENGINE_GAP_SIZE];
		for(int i = 0; i < num_mirrors; i++) mirrors[i] = Engine::mirrors[i];
		if(Engine::mirrors) delete Engine::mirrors;
		Engine::mirrors = mirrors;
		if(visible_mirrors) delete visible_mirrors;
		visible_mirrors = new Mirror*[num_mirrors + ENGINE_GAP_SIZE];
	}
	mirrors[num_mirrors++] = mirror;
}

void Engine::removeMirror(Mirror *mirror) {
	for(int i = 0; i < num_mirrors; i++) {
		if(mirrors[i] == mirror) {
			num_mirrors--;
			for(; i < num_mirrors; i++) mirrors[i] = mirrors[i + 1];
			return;
		}
	}
}

/*****************************************************************************/
/*                                                                           */
/* update                                                                    */
/*                                                                           */
/*****************************************************************************/

void Engine::update(float ifps) {
	
	Engine::ifps = ifps;
	
	// new frame
	frame++;
	
	// read stderr
	char buf[1024];
	while(1) {
		int ret = read(stderr_fd,buf,sizeof(buf));
		if(ret > 0) {
			buf[ret] = '\0';
			console->printf(1,0,0,"%s",buf);
			if(console->getActivity() == 0) console->keyPress('`');
		} else break;
	}
	
	// update time
	time += ifps;
	
	// update lights
	for(int i = 0; i < num_visible_lights; i++) {
		visible_lights[i]->update(ifps);
	}
	
	// update objects
	Object *objects[Sector::NUM_OBJECTS];
	for(int i = 0; i < Bsp::num_visible_sectors; i++) {
		Sector *s = Bsp::visible_sectors[i];
		for(int j = 0; j < s->num_objects; j++) objects[j] = s->objects[j];
		int num_objects = s->num_objects;
		for(int j = 0; j < num_objects; j++) {
			Object *o = objects[j];
			if(o->frame == -Engine::frame) continue;
			o->update(ifps);
			o->frame = -Engine::frame;
		}
	}
}

/*****************************************************************************/
/*                                                                           */
/* renderers                                                                 */
/*                                                                           */
/*****************************************************************************/

/* main bottleneck :)
 */
void Engine::render_light() {
	
	glDepthMask(GL_FALSE);
	
	if(have_occlusion) {
		glDisable(GL_CULL_FACE);
		glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
	}
	
	// find visible lights
	num_visible_lights = 0;
	for(int i = 0; i < num_lights; i++) {
		Light *l = lights[i];
		for(int j = 0; j < l->pos.num_sectors; j++) {
			if(Bsp::sectors[l->pos.sectors[j]].frame != frame) continue;
			
			if(frustum->inside(l->pos,l->radius) == 0) continue;
			
			if(have_occlusion && (l->pos - camera).length() > l->radius) {
				glPushMatrix();
				glTranslatef(l->pos.x,l->pos.y,l->pos.z);
				glScalef(l->radius,l->radius,l->radius);
				glBeginQueryARB(GL_SAMPLES_PASSED_ARB,query_id);
				sphere_mesh->render();
				glEndQueryARB(GL_SAMPLES_PASSED_ARB);
				glPopMatrix();
				
				GLuint samples;
				glGetQueryObjectuivARB(query_id,GL_QUERY_RESULT_ARB,&samples);
				if(samples == 0) continue;
			}
			
			visible_lights[num_visible_lights++] = l;
			break;
		}
	}
	
	// find visible fogs
	num_visible_fogs = 0;
	if(fog_toggle) {
		for(int i = 0; i < num_fogs; i++) {
			Fog *fog = fogs[i];
			for(int j = 0; j < fog->pos.num_sectors; j++) {
				if(Bsp::sectors[fog->pos.sectors[j]].frame != frame) continue;
				
				if(frustum->inside(fog->getMin(),fog->getMax()) == 0) continue;
				
				if(have_occlusion && !fog->inside(camera)) {
					glBeginQueryARB(GL_SAMPLES_PASSED_ARB,query_id);
					fog->mesh->render();
					glEndQueryARB(GL_SAMPLES_PASSED_ARB);
					
					GLuint samples;
					glGetQueryObjectuivARB(query_id,GL_QUERY_RESULT_ARB,&samples);
					if(samples == 0) continue;
				}
				
				visible_fogs[num_visible_fogs++] = fog;
				break;
			}
		}
	}
	
	if(have_occlusion) {
		glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
		glEnable(GL_CULL_FACE);
	}
	
	if(scissor_toggle) glEnable(GL_SCISSOR_TEST);
	
	if(shadows_toggle) glEnable(GL_STENCIL_TEST);
	
	// render lights
	for(int i = 0; i < num_visible_lights; i++) {
		
		Light *l = visible_lights[i];
		current_light = l;
		
		light = vec4(l->pos,1.0 / (l->radius * l->radius));
		light_color = l->color;
		
		int stencil_value = 0;
		
		// scissor
		int scissor[4];
		l->getScissor(scissor);
		if(scissor_toggle) glScissor(scissor[0],scissor[1],scissor[2],scissor[3]);
		
		// shadows
		if(shadows_toggle && l->shadows) {
			
			if(show_shadows_toggle) {
				glEnable(GL_BLEND);
				glBlendFunc(GL_ONE,GL_ONE);
			} else {
				glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
			}
			
			// shadow volume extrusion and depth clamp on ATI
			shadow_volume_shader->enable();
			shadow_volume_shader->bind();
			
			// depth clamp extension
			if(isDefine("NV3X")) glEnable(GL_DEPTH_CLAMP_NV);
			
			if(have_stencil_two_side) {
				glDisable(GL_CULL_FACE);
				glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);
				
				glActiveStencilFaceEXT(GL_BACK);
				glStencilFunc(GL_ALWAYS,0,~0);
				glStencilOp(GL_KEEP,GL_KEEP,GL_INCR_WRAP);
				glActiveStencilFaceEXT(GL_FRONT);
				glStencilFunc(GL_ALWAYS,0,~0);
				glStencilOp(GL_KEEP,GL_KEEP,GL_DECR_WRAP);
			}
			
			// shadow volumes
			for(int i = 0; i < l->pos.num_sectors; i++) {
				Sector *s = &Bsp::sectors[l->pos.sectors[i]];
				if(s->portal) frustum->addPortal(camera,s->portal->points);
				
				// static objects
				for(int j = 0; j < s->num_node_objects; j++) {
					Object *o = s->node_objects[j];
					if(o->shadows == 0) continue;
					for(int i = 0; i < o->num_opacities; i++) {
						int j = o->opacities[i];
						if(o->materials[j]->alpha_test) continue;
						if(frustum->inside(light,l->radius,o->getCenter(j),o->getRadius(j)) == 0) continue;
						
						glFlush();
						
						o->findSilhouette(vec4(light,1),j);
						stencil_value += o->getNumIntersections(light,camera,j);
						
						if(have_stencil_two_side) {
							num_triangles += o->renderShadowVolume(j);
						} else {
							glCullFace(GL_FRONT);
							glStencilFunc(GL_ALWAYS,0,~0);
							glStencilOp(GL_KEEP,GL_KEEP,GL_INCR_WRAP);
							num_triangles += o->renderShadowVolume(j);
							
							glCullFace(GL_BACK);
							glStencilFunc(GL_ALWAYS,0,~0);
							glStencilOp(GL_KEEP,GL_KEEP,GL_DECR_WRAP);
							num_triangles += o->renderShadowVolume(j);
						}
					}
				}
				
				// dynamic objects
				for(int j = 0; j < s->num_objects; j++) {
					Object *o = s->objects[j];
					if(o->shadows == 0) continue;
					// object may be present in many sectors
					if(o->pos.sector != l->pos.sectors[i]) continue;
					o->enable();
					vec3 icamera = itransform * camera;
					vec3 ilight = itransform * vec3(light);
					for(int i = 0; i < o->num_opacities; i++) {
						int j = o->opacities[i];
						if(o->materials[j]->alpha_test) continue;
						if(frustum->inside(light,l->radius,o->pos + o->getCenter(j),o->getRadius(j)) == 0) continue;
						
						glFlush();
						
						o->findSilhouette(vec4(ilight,1),j);
						stencil_value += o->getNumIntersections(ilight,icamera,j);

						if(have_stencil_two_side) {
							num_triangles += o->renderShadowVolume(j);
						} else {
							glCullFace(GL_FRONT);
							glStencilFunc(GL_ALWAYS,0,~0);
							glStencilOp(GL_KEEP,GL_KEEP,GL_INCR_WRAP);
							num_triangles += o->renderShadowVolume(j);
							
							glCullFace(GL_BACK);
							glStencilFunc(GL_ALWAYS,0,~0);
							glStencilOp(GL_KEEP,GL_KEEP,GL_DECR_WRAP);
							num_triangles += o->renderShadowVolume(j);
						}
					}
					o->disable();
				}
				
				if(s->portal) frustum->removePortal();
			}
			
			if(have_stencil_two_side) {
				glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
				glEnable(GL_CULL_FACE);
			}
			
			if(isDefine("NV3X")) glDisable(GL_DEPTH_CLAMP_NV);
			
			shadow_volume_shader->disable();
			
			if(show_shadows_toggle) {
				glDisable(GL_BLEND);
			} else {
				glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
			}
			
			if((camera.sector == -1 || Bsp::sectors[camera.sector].inside(camera) == 0) && --stencil_value < 0) stencil_value = 0;
			
			glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);
			glStencilFunc(GL_EQUAL,stencil_value,~0);
		}
		
		if(shadows_toggle && l->shadows == 0) glDisable(GL_STENCIL_TEST);
		
		glDepthFunc(GL_EQUAL);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE,GL_ONE);
		
		// all visible opacitie objects
		for(int i = 0; i < Bsp::num_visible_sectors; i++) {
			Sector *s = Bsp::visible_sectors[i];
			Portal *p = (s->frame == Engine::frame) ? s->portal : NULL;
			if(p) {
				int portal_scissor[4];
				p->getScissor(portal_scissor);
				portal_scissor[2] += portal_scissor[0];
				portal_scissor[3] += portal_scissor[1];
				if(portal_scissor[0] < scissor[0]) portal_scissor[0] = scissor[0];
				if(portal_scissor[1] < scissor[1]) portal_scissor[1] = scissor[1];
				if(portal_scissor[2] > scissor[0] + scissor[2]) portal_scissor[2] = scissor[0] + scissor[2];
				if(portal_scissor[3] > scissor[1] + scissor[3]) portal_scissor[3] = scissor[1] + scissor[3];
				portal_scissor[2] -= portal_scissor[0];
				portal_scissor[3] -= portal_scissor[1];
				if(portal_scissor[2] < 0 || portal_scissor[3] < 0) continue;
				if(scissor_toggle) glScissor(portal_scissor[0],portal_scissor[1],portal_scissor[2],portal_scissor[3]);
			} else {
				if(scissor_toggle) glScissor(scissor[0],scissor[1],scissor[2],scissor[3]);
			}
			for(int j = 0; j < s->num_visible_objects; j++) {
				Object *o = s->visible_objects[j];
				if((o->pos + o->getCenter() - light).length() < o->getRadius() + l->radius) {
					num_triangles += o->render(Object::RENDER_OPACITY);
				}
			}
		}
		
		if(scissor_toggle) glScissor(viewport[0],viewport[1],viewport[2],viewport[3]);
		
		// mirrors
		for(int i = 0; i < num_visible_mirrors; i++) {
			if(visible_mirrors[i]->material && visible_mirrors[i]->material->blend == 0) {
				visible_mirrors[i]->render();
			}
		}
		
		// disable material
		if(Material::old_material) Material::old_material->disable();
		
		glDisable(GL_BLEND);
		glDepthFunc(GL_LEQUAL);
		
		if(shadows_toggle && l->shadows == 0) glEnable(GL_STENCIL_TEST);
		
		// clear stensil only if it is needed
		if(l != visible_lights[num_visible_lights - 1]) glClear(GL_STENCIL_BUFFER_BIT);		
	}
	current_light = NULL;
	
	if(shadows_toggle) glDisable(GL_STENCIL_TEST);
	
	if(scissor_toggle) {
		glScissor(viewport[0],viewport[1],viewport[2],viewport[3]);
		glDisable(GL_SCISSOR_TEST);
	}
	
	glDepthMask(GL_TRUE);
}

/*
 */
void Engine::render_transparent() {
	
	glDepthMask(GL_FALSE);
	
	// volume fogs
	if(fog_toggle) {
		for(int i = 0; i < num_visible_fogs; i++) {
			Fog *fog = visible_fogs[i];
			current_fog = fog;
			
			fog->enable();
			// all visible objects
			for(int i = 0; i < Bsp::num_visible_sectors; i++) {
				Sector *s = Bsp::visible_sectors[i];
				for(int j = 0; j < s->num_visible_objects; j++) {
					Object *o = s->visible_objects[j];
					if((o->pos + o->getCenter() - fog->getCenter()).length() < o->getRadius() + fog->getRadius()) {
						num_triangles += o->render(Object::RENDER_ALL);
					}
				}
			}
			fog->disable();
			
			num_triangles += fog->render();
		}
	}
	current_fog = NULL;
	
	glEnable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	
	// transparent objects
	// ambient shader
	int save_num_visible_lights = num_visible_lights;
	num_visible_lights = 0;
	
	for(int i = 0; i < num_visible_mirrors; i++) {
		if(visible_mirrors[i]->material && visible_mirrors[i]->material->blend == 1) {
			visible_mirrors[i]->render();
		}
	}
	
	for(int i = Bsp::num_visible_sectors - 1; i >= 0; i--) {
		Sector *s = Bsp::visible_sectors[i];
		for(int j = 0; j < s->num_visible_objects; j++) {
			Object *o = s->visible_objects[j];
			num_triangles += o->render(Object::RENDER_TRANSPARENT);
		}
	}
	
	// light shader
	num_visible_lights = save_num_visible_lights;
	
	for(int i = 0; i < num_visible_mirrors; i++) {
		if(visible_mirrors[i]->material && visible_mirrors[i]->material->blend == 1) {
			visible_mirrors[i]->render();
		}
	}
	
	for(int i = 0; i < num_visible_lights; i++) {
		Light *l = visible_lights[i];
		current_light = l;
		
		light = vec4(l->pos,1.0 / (l->radius * l->radius));
		light_color = l->color;
		
		for(int i = Bsp::num_visible_sectors - 1; i >= 0; i--) {
			Sector *s = Bsp::visible_sectors[i];
			for(int j = 0; j < s->num_visible_objects; j++) {
				Object *o = s->visible_objects[j];
				if((o->pos + o->getCenter() - light).length() < o->getRadius() + l->radius) {
					num_triangles += o->render(Object::RENDER_TRANSPARENT);
				}
			}
		}
	}
	current_light = NULL;
	
	// disable all materials
	if(Material::old_material) Material::old_material->disable();
	
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	
	glDepthMask(GL_TRUE);
}

/*
 */
void Engine::render(float ifps) {
	
	// get matrixes
	glGetFloatv(GL_PROJECTION_MATRIX,projection);
	glGetFloatv(GL_MODELVIEW_MATRIX,modelview);
	imodelview = modelview.inverse();
	
	transform.identity();
	itransform.identity();
	
	// get camera
	camera = imodelview * vec3(0,0,0);

	// set frustum
	frustum->set(projection * modelview);
	
	// global triangle counter
	num_triangles = 0;
	
	/* render to pbuffer
	 */
	screen->enable();
	
	// get viewport
	glGetIntegerv(GL_VIEWPORT,viewport);
	
	// set matrixes
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(projection);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(modelview);
	
	// ambient pass
	// clear depth buffer
	glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glDepthFunc(GL_LESS);
	
	// number of visible lights
	// zero - ambient shader
	num_visible_lights = 0;
	
	// render bsp
	if(bsp) bsp->render();
	
	// disable material
	if(Material::old_material) Material::old_material->disable();
	
	// find visible mirrors
	num_visible_mirrors = 0;
	if(mirror_toggle) {
		
		if(have_occlusion) {
			glDisable(GL_CULL_FACE);
			glDepthMask(GL_FALSE);
			glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
		}
		
		for(int i = 0; i < num_mirrors; i++) {
			Mirror *mirror = mirrors[i];
			if(mirror->pos.sector == -1 || Bsp::sectors[mirror->pos.sector].frame != frame) continue;
			
			if(frustum->inside(mirror->getMin(),mirror->getMax()) == 0) continue;
			
			if(have_occlusion) {
				glBeginQueryARB(GL_SAMPLES_PASSED_ARB,query_id);
				mirror->mesh->render();
				glEndQueryARB(GL_SAMPLES_PASSED_ARB);
				
				GLuint samples;
				glGetQueryObjectuivARB(query_id,GL_QUERY_RESULT_ARB,&samples);
				if(samples == 0) continue;
			}
			
			visible_mirrors[num_visible_mirrors++] = mirror;
			break;
		}
		
		if(have_occlusion) {
			glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
			glDepthMask(GL_TRUE);
			glEnable(GL_CULL_FACE);
		}
	}
	
	// render mirrors
	if(num_visible_mirrors) {
		
		// save state
		if(bsp) bsp->saveState();
		
		// without any mirrors
		int save_num_visible_mirrors = num_visible_mirrors;
		num_visible_mirrors = 0;
		
		for(int i = 0; i < save_num_visible_mirrors; i++) {
			
			frame++;	// new frame
			
			Mirror *mirror = visible_mirrors[i];
			current_mirror = mirror;
			
			mirror->enable();
			
			// clear
			glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			glDepthFunc(GL_LESS);
			
			// ambient mirror pass
			num_visible_lights = 0;
			if(bsp) bsp->render();
			
			// light mirror pass
			render_light();
			
			// transparent mirror pass
			render_transparent();
			
			mirror->disable();
		}
		
		num_visible_mirrors = save_num_visible_mirrors;
		
		// restore state
		if(bsp) bsp->restoreState(++frame);
	}
	current_mirror = NULL;
	
	// ambient mirror pass
	num_visible_lights = 0;
	for(int i = 0; i < num_visible_mirrors; i++) {
		if(visible_mirrors[i]->material && visible_mirrors[i]->material->blend == 0) {
			visible_mirrors[i]->render();
		}
	}
	
	// light pass
	render_light();
	
	// transparent pass
	render_transparent();
	
	// disable material
	if(Material::old_material) Material::old_material->disable();
	
	// wareframe
	if(wareframe_toggle) {
		glColor3f(0,1,0);
		glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
		glEnable(GL_POLYGON_OFFSET_LINE);
		glPolygonOffset(-0.5,0.0);
		for(int i = 0; i < Bsp::num_visible_sectors; i++) {
			Sector *s = Bsp::visible_sectors[i];
			for(int j = 0; j < s->num_visible_objects; j++) {
				Object *o = s->visible_objects[j];
					num_triangles += o->render(Object::RENDER_ALL);
			}
		}
		glDisable(GL_POLYGON_OFFSET_LINE);
		glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	}
	
	glFlush();
	
	/* calculate physics
	 */
	if(physic_toggle) Physic::update(Engine::ifps);	
	
	/* render light flares
	 */
	for(int i = 0; i < num_visible_lights; i++) {
		visible_lights[i]->renderFlare();
	}
	
	/* disable pbuffer
	 */
	screen_texture->bind();
	screen_texture->copy();
	screen->disable();
	
	// get viewport
	glGetIntegerv(GL_VIEWPORT,viewport);
	
	// postprocessing
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1,1,-1,1,-1,1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	
	screen_material->enable();
	screen_material->bind();
	screen_material->bindTexture(0,screen_texture);
	screen_texture->render();
	screen_material->disable();
	
	// console
	console->enable(viewport[2],viewport[3]);
	console->render(ifps,40);
	console->disable();
}

/*****************************************************************************/
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

/*
 */
Object *Engine::intersection(const vec3 &line0,const vec3 &line1,vec3 &point,vec3 &normal) {
	vec3 p,n;
	point = line1;
	Object *object = NULL;
	for(int i = 0; i < Bsp::num_visible_sectors; i++) {
		Sector *s = Bsp::visible_sectors[i];
		for(int j = 0; j < s->num_visible_objects; j++) {
			Object *o = s->visible_objects[j];
			if(o->is_identity) {
				if(o->intersection(line0,point,p,n)) {
					point = p;
					normal = n;
					object = o;
				}
			} else {
				vec3 l0 = o->itransform * line0;
				vec3 l1 = o->itransform * point;
				if(o->intersection(l0,l1,p,n)) {
					point = o->transform * p;
					normal = o->transform.rotation() * n;
					object = o;
				}
			}
		}
	}
	return object;
}
