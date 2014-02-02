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

#include "console.h"
#include "engine.h"
#include "frustum.h"
#include "material.h"
#include "mesh.h"
#include "meshvbo.h"
#include "object.h"
#include "objectmesh.h"
#include "bsp.h"

/*****************************************************************************/
/*                                                                           */
/* Node                                                                      */
/*                                                                           */
/*****************************************************************************/

Node::Node() : left(NULL), right(NULL), object(NULL) {
	min = vec3(1000000,1000000,1000000);
	max = vec3(-1000000,-1000000,-1000000);
	center = vec3(0,0,0);
	radius = 1000000;
}

Node::~Node() {
	if(left) delete left;
	if(right) delete right;
	if(object) {
		delete object->mesh;
		delete object;
	}
}

/*
 */
void Node::create(Mesh *mesh) {
	min = mesh->getMin();
	max = mesh->getMax();
	center = mesh->getCenter();
	radius = mesh->getRadius();
	int num_vertex = 0;
	for(int i = 0; i < mesh->getNumSurfaces(); i++) num_vertex += mesh->getNumVertex(i);
	if(num_vertex / 3 > TRIANGLES_PER_NODE) {
		vec4 plane;
		vec3 size = max - min;
		if(size.x > size.y) {	// find clip plane
			if(size.x > size.z) plane = vec4(1,0,0,1);
			else plane = vec4(0,0,1,1);
		} else{
			if(size.y > size.z) plane = vec4(0,1,0,1);
			else plane = vec4(0,0,1,1);
		}
		vec3 center(0,0,0);
		int num_vertex = 0;	// find center of node
		for(int i = 0; i < mesh->getNumSurfaces(); i++) {
			Mesh::Vertex *vertex = mesh->getVertex(i);
			for(int j = 0; j < mesh->getNumVertex(i); j++) {
				center += vertex[j].xyz;
				num_vertex++;
			}
		}
		center /= (float)num_vertex;
		plane.w = -(plane * vec4(center,1));
		Mesh *left_mesh = new Mesh();
		Mesh *right_mesh = new Mesh();
		for(int i = 0; i < mesh->getNumSurfaces(); i++) {
			int num_vertex = mesh->getNumVertex(i);
			Mesh::Vertex *vertex = mesh->getVertex(i);
			int left_mesh_num_vertex = 0;
			Mesh::Vertex *left_mesh_vertex = new Mesh::Vertex[num_vertex * 2];
			int right_mesh_num_vertex = 0;
			Mesh::Vertex *right_mesh_vertex = new Mesh::Vertex[num_vertex * 2];
			for(int j = 0; j < num_vertex; j += 3) {
				/*int pos = 0;
				if(plane * vec4(vertex[j + 0].xyz,1) > 0.0) pos++;
				if(plane * vec4(vertex[j + 1].xyz,1) > 0.0) pos++;
				if(plane * vec4(vertex[j + 2].xyz,1) > 0.0) pos++;
				if(pos > 1) {
					left_mesh_vertex[left_mesh_num_vertex++] = vertex[j + 0];
					left_mesh_vertex[left_mesh_num_vertex++] = vertex[j + 1];
					left_mesh_vertex[left_mesh_num_vertex++] = vertex[j + 2];
				} else {
					right_mesh_vertex[right_mesh_num_vertex++] = vertex[j + 0];
					right_mesh_vertex[right_mesh_num_vertex++] = vertex[j + 1];
					right_mesh_vertex[right_mesh_num_vertex++] = vertex[j + 2];
				}*/
				int num_left = 0;
				int num_right = 0;
				Mesh::Vertex *left = &left_mesh_vertex[left_mesh_num_vertex];
				Mesh::Vertex *right = &right_mesh_vertex[right_mesh_num_vertex];
				float cur_dot = plane * vec4(vertex[j + 0].xyz,1);
				for(int cur = 0; cur < 3; cur++) {
					int next = (cur + 1) % 3;
					float next_dot = plane * vec4(vertex[j + next].xyz,1);
					if(cur_dot <= 0.0) left[num_left++] = vertex[j + cur];
					if(cur_dot > 0.0) right[num_right++] = vertex[j + cur];
					if((cur_dot <= 0.0) != (next_dot <= 0.0)) {
						float k = -cur_dot / (next_dot - cur_dot);
						Mesh::Vertex v;
						v.xyz = vertex[j + cur].xyz * (1.0f - k) + vertex[j + next].xyz * k;
						v.normal = vertex[j + cur].normal * (1.0f - k) + vertex[j + next].normal * k;
						v.tangent = vertex[j + cur].tangent * (1.0f - k) + vertex[j + next].tangent * k;
						v.binormal = vertex[j + cur].binormal * (1.0f - k) + vertex[j + next].binormal * k;
						v.texcoord = vertex[j + cur].texcoord * (1.0f - k) + vertex[j + next].texcoord * k;
						left[num_left++] = v;
						right[num_right++] = v;
					}
					cur_dot = next_dot;
				}
				if(num_left == 3) left_mesh_num_vertex += 3;
				else if(num_left == 4) {
					left[4] = left[0];
					left[5] = left[2];
					left_mesh_num_vertex += 6;
				}
				if(num_right == 3) right_mesh_num_vertex += 3;
				else if(num_right == 4) {
					right[4] = right[0];
					right[5] = right[2];
					right_mesh_num_vertex += 6;
				}
			}
			if(left_mesh_num_vertex > 0) left_mesh->addSurface(mesh->getSurfaceName(i),left_mesh_vertex,left_mesh_num_vertex);
			if(right_mesh_num_vertex > 0) right_mesh->addSurface(mesh->getSurfaceName(i),right_mesh_vertex,right_mesh_num_vertex);
			delete right_mesh_vertex;
			delete left_mesh_vertex;
		}
		int left_mesh_num_vertex = 0;
		for(int i = 0; i < left_mesh->getNumSurfaces(); i++) left_mesh_num_vertex += left_mesh->getNumVertex(i);
		int right_mesh_num_vertex = 0;
		for(int i = 0; i < right_mesh->getNumSurfaces(); i++) right_mesh_num_vertex += right_mesh->getNumVertex(i);
		if(left_mesh_num_vertex > 0 && right_mesh_num_vertex > 0) {
			left_mesh->calculate_bounds();
			left = new Node();
			left->create(left_mesh);
			right_mesh->calculate_bounds();
			right = new Node();
			right->create(right_mesh);
		} else {
			mesh->create_shadow_volumes();
			mesh->create_triangle_strips();
			object = new ObjectMesh(new MeshVBO(mesh));
			delete right_mesh;
			delete left_mesh;
		}
	} else {
		mesh->create_shadow_volumes();
		mesh->create_triangle_strips();
		object = new ObjectMesh(new MeshVBO(mesh));
	}
	delete mesh;
}

/*
 */
void Node::load(FILE *file) {
	int header;
	fread(&header,sizeof(int),1,file);
	if(header == 0x7fffffff) {
		fread(&min,sizeof(vec3),1,file);
		fread(&max,sizeof(vec3),1,file);
		fread(&center,sizeof(vec3),1,file);
		fread(&radius,sizeof(float),1,file);
		left = new Node();
		left->load(file);
		right = new Node();
		right->load(file);
	} else {
		fseek(file,-sizeof(int),SEEK_CUR);
		Mesh *mesh = new Mesh();
		mesh->load(file);
		min = mesh->getMin();
		max = mesh->getMax();
		center = mesh->getCenter();
		radius = mesh->getRadius();
		object = new ObjectMesh(new MeshVBO(mesh));
		delete mesh;
	}
}

/*
 */
void Node::save(FILE *file) {
	if(left && right) {
		int header = 0x7fffffff;
		fwrite(&header,sizeof(int),1,file);
		fwrite(&min,sizeof(vec3),1,file);
		fwrite(&max,sizeof(vec3),1,file);
		fwrite(&center,sizeof(vec3),1,file);
		fwrite(&radius,sizeof(float),1,file);
		left->save(file);
		right->save(file);
	} else {
		object->mesh->save(file);
	}
}

/*
 */
void Node::bindMaterial(const char *name,Material *material) {
	if(left) left->bindMaterial(name,material);
	if(right) right->bindMaterial(name,material);
	if(object) object->bindMaterial(name,material);
}

/*
 */
void Node::render() {
	if(left && right) {
		int check_left = Engine::frustum->inside(left->center,left->radius);
		int check_right = Engine::frustum->inside(right->center,right->radius);
		if(check_left && check_right) {
			if((left->center - Engine::camera).length() < (right->center - Engine::camera).length()) {
				left->render();
				right->render();
			} else {
				right->render();
				left->render();
			}
			return;
		}
		if(check_left) left->render();
		else if(check_right) right->render();
		return;
	}
	if(object && object->frame != Engine::frame) {
		Sector *s = Bsp::visible_sectors[Bsp::num_visible_sectors - 1];
		s->visible_objects[s->num_visible_objects++] = object;
		Engine::num_triangles += object->render(Object::RENDER_OPACITY);
	}
}

/*****************************************************************************/
/*                                                                           */
/* Portal                                                                    */
/*                                                                           */
/*****************************************************************************/

Portal::Portal() : center(0,0,0), radius(1000000.0), num_sectors(0), sectors(NULL), frame(0) {
	
}

Portal::~Portal() {
	if(sectors) delete sectors;
}

/*
 */
void Portal::create(Mesh *mesh,int s) {
	center = mesh->getCenter(s);
	radius = mesh->getRadius(s);
	int num_vertex = mesh->getNumVertex(s);
	if(num_vertex != 6) {
		fprintf(stderr,"Portal::create(): portal mesh must have only two triangle\n");
		return;
	}
	Mesh::Vertex *v = mesh->getVertex(s);
	int flag[6];	// create quad from the six vertexes
	for(int i = 0; i < 6; i++) {
		flag[i] = 0;
		for(int j = 0; j < 6; j++) {
			if(i == j) continue;
			if(v[i].xyz == v[j].xyz) flag[i] = 1;
		}
	}
	for(int i = 0, j = 0; i < 6; i++) {
		if(flag[i] == 0) points[j++] = v[i].xyz;
		if(i == 5 && j != 2) {
			fprintf(stderr,"Portal::create(): can`t find two similar vertexes for create quad\n");
			return;
		}
	}
	points[2] = points[1];
	for(int i = 0, j = 1; i < 3; i++) {
		if(flag[i] != 0) {
			points[j] = v[i].xyz;
			j += 2;
		}
	}
}

void Portal::getScissor(int *scissor) {
	if((center - Engine::camera).length() < radius) {
		scissor[0] = Engine::viewport[0];
		scissor[1] = Engine::viewport[1];
		scissor[2] = Engine::viewport[2];
		scissor[3] = Engine::viewport[3];
		return;
	}
	mat4 mvp = Engine::projection * Engine::modelview;
	vec4 p[4];
	p[0] = mvp * vec4(points[0],1);
	p[1] = mvp * vec4(points[1],1);
	p[2] = mvp * vec4(points[2],1);
	p[3] = mvp * vec4(points[3],1);
	p[0] /= p[0].w;
	p[1] /= p[1].w;
	p[2] /= p[2].w;
	p[3] /= p[3].w;
	vec3 min = vec3(1000000,1000000,1000000);
	vec3 max = vec3(-1000000,-1000000,-1000000);
	for(int i = 0; i < 4; i++) {
		if(min.x > p[i].x) min.x = p[i].x;
		if(max.x < p[i].x) max.x = p[i].x;
		if(min.y > p[i].y) min.y = p[i].y;
		if(max.y < p[i].y) max.y = p[i].y;
	}
	scissor[0] = Engine::viewport[0] + (int)((float)Engine::viewport[2] * (min.x + 1.0) / 2.0) - 4;	// remove it
	scissor[1] = Engine::viewport[1] + (int)((float)Engine::viewport[3] * (min.y + 1.0) / 2.0) - 4;
	scissor[2] = Engine::viewport[0] + (int)((float)Engine::viewport[2] * (max.x + 1.0) / 2.0) + 4;
	scissor[3] = Engine::viewport[1] + (int)((float)Engine::viewport[3] * (max.y + 1.0) / 2.0) + 4;
	if(scissor[0] < Engine::viewport[0]) scissor[0] = Engine::viewport[0];
	else if(scissor[0] > Engine::viewport[0] + Engine::viewport[2]) scissor[0] = Engine::viewport[0] + Engine::viewport[2];
	if(scissor[1] < Engine::viewport[1]) scissor[1] = Engine::viewport[1];
	else if(scissor[1] > Engine::viewport[1] + Engine::viewport[3]) scissor[1] = Engine::viewport[1] + Engine::viewport[3];
	if(scissor[2] < Engine::viewport[0]) scissor[2] = Engine::viewport[0];
	else if(scissor[2] > Engine::viewport[2] + Engine::viewport[3]) scissor[2] = Engine::viewport[0] + Engine::viewport[2];
	if(scissor[3] < Engine::viewport[1]) scissor[3] = Engine::viewport[1];
	else if(scissor[3] > Engine::viewport[1] + Engine::viewport[3]) scissor[3] = Engine::viewport[1] + Engine::viewport[3];
	scissor[2] -= scissor[0];
	scissor[3] -= scissor[1];
}

/*
 */
void Portal::render() {
	glBegin(GL_QUADS);
	glVertex3fv(points[0]);
	glVertex3fv(points[1]);
	glVertex3fv(points[2]);
	glVertex3fv(points[3]);
	glEnd();
}

/*****************************************************************************/
/*                                                                           */
/* Sector                                                                    */
/*                                                                           */
/*****************************************************************************/

Sector::Sector() : center(0,0,0), radius(1000000.0), num_planes(0), planes(NULL), root(NULL),
	num_portals(0), portals(NULL), num_objects(0), objects(NULL), num_node_objects(0), node_objects(NULL),
	num_visible_objects(0), visible_objects(NULL), portal(NULL), frame(0),
	old_num_visible_objects(0), old_visible_objects(NULL), old_portal(NULL), old_frame(0) {
	
}

Sector::~Sector() {
	if(portals) delete portals;
	if(planes) delete planes;
	if(root) delete root;
	if(objects) delete objects;
	if(node_objects) delete node_objects;
	if(visible_objects) delete visible_objects;
	if(old_visible_objects) delete old_visible_objects;
}

/*
 */
void Sector::create(Mesh *mesh,int s) {
	center = mesh->getCenter(s);
	radius = mesh->getRadius(s);
	num_planes = mesh->getNumVertex(s) / 3;
	planes = new vec4[num_planes];
	Mesh::Vertex *v = mesh->getVertex(s);
	for(int i = 0; i < num_planes; i++) {
		vec3 normal;
		normal.cross(v[i * 3 + 1].xyz - v[i * 3 + 0].xyz,v[i * 3 + 2].xyz - v[i * 3 + 0].xyz);
		normal.normalize();
		planes[i] = vec4(normal,-normal * v[i * 3 + 0].xyz);
	}
}

/* get all objects from the tree
 */
void Sector::getNodeObjects(Node *node) {
	if(node->left && node->right) {
		getNodeObjects(node->left);
		getNodeObjects(node->right);
	}
	if(node->object) {
		if(node_objects) node_objects[num_node_objects] = node->object;
		num_node_objects++;
	}
}

/*
 */
void Sector::create() {
	
	objects = new Object*[NUM_OBJECTS];
	
	num_node_objects = 0;
	getNodeObjects(root);
	
	node_objects = new Object*[num_node_objects];
	visible_objects = new Object*[num_node_objects + NUM_OBJECTS];
	old_visible_objects = new Object*[num_node_objects + NUM_OBJECTS];
	
	num_node_objects = 0;
	getNodeObjects(root);
}

/*
 */
int Sector::inside(const vec3 &point) {
	for(int i = 0; i < num_planes; i++) {
		if(planes[i] * vec4(point,1) > 0.0) return 0;
	}
	return 1;
}

int Sector::inside(Portal *portal) {
	if(inside(portal->points[0]) == 0) return 0;
	if(inside(portal->points[1]) == 0) return 0;
	if(inside(portal->points[2]) == 0) return 0;
	if(inside(portal->points[3]) == 0) return 0;
	return 1;
}

int Sector::inside(const vec3 &center,float radius) {
	for(int i = 0; i < num_planes; i++) {
		if(planes[i] * vec4(center,1) > radius) return 0;
	}
	return 1;
}

int Sector::inside(Mesh *mesh,int s) {
	Mesh::Vertex *v = mesh->getVertex(s);
	for(int i = 0; i < mesh->getNumVertex(s); i++) {
		if(inside(v[i].xyz) == 0) return 0;
	}
	return 1;
}

/*
 */
void Sector::addObject(Object *object) {
	if(!objects) return;
	int i = 0;
	for(; i < num_objects; i++) if(objects[i] == object) return;
	objects[num_objects++] = object;
}

/*
 */
void Sector::removeObject(Object *object) {
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
void Sector::bindMaterial(const char *name,Material *material) {
	root->bindMaterial(name,material);
}

/*
 */
void Sector::render(Portal *portal) {
	
	if(frame == Engine::frame) return;
	frame = Engine::frame;
	
	this->portal = portal;
	Bsp::visible_sectors[Bsp::num_visible_sectors++] = this;
	
	num_visible_objects = 0;
	
	root->render();
	
	for(int i = 0; i < num_objects; i++) {
		Object *o = objects[i];
		if(o->frame == Engine::frame) continue;
		if(Engine::frustum->inside(o->pos + o->getCenter(),o->getRadius())) {
			Engine::num_triangles += o->render(Object::RENDER_OPACITY);
			visible_objects[num_visible_objects++] = o;
		}
	}
	
	for(int i = 0; i < num_portals; i++) {
		Portal *p = &Bsp::portals[portals[i]];
		if(p->frame == Engine::frame) continue;
		p->frame = Engine::frame;
		if(Engine::frustum->inside(p->center,p->radius)) {
			float dist = (Engine::camera - p->center).length();
			
			if(Engine::have_occlusion && dist > p->radius) {
				
				if(Material::old_material) Material::old_material->disable();
				
				glDisable(GL_CULL_FACE);
				glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
				glDepthMask(GL_FALSE);
				glBeginQueryARB(GL_SAMPLES_PASSED_ARB,Engine::query_id);
				p->render();
				glEndQueryARB(GL_SAMPLES_PASSED_ARB);
				glDepthMask(GL_TRUE);
				glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
				glEnable(GL_CULL_FACE);
					
				GLuint samples;
				glGetQueryObjectuivARB(Engine::query_id,GL_QUERY_RESULT_ARB,&samples);
				if(samples == 0) continue;
			}
			
			if(dist > p->radius) Engine::frustum->addPortal(Engine::camera,p->points);
			for(int j = 0; j < p->num_sectors; j++) {
				Bsp::sectors[p->sectors[j]].render(dist > p->radius ? p : NULL);
			}
			if(dist > p->radius) Engine::frustum->removePortal();
		}
	}
}

/*
 */
void Sector::saveState() {
	old_num_visible_objects = num_visible_objects;
	for(int i = 0; i < num_visible_objects; i++) {
		old_visible_objects[i] = visible_objects[i];
	}
	old_portal = portal;
	old_frame = frame;
}

void Sector::restoreState(int frame) {
	num_visible_objects = old_num_visible_objects;
	for(int i = 0; i < num_visible_objects; i++) {
		visible_objects[i] = old_visible_objects[i];
	}
	portal = old_portal;
	this->frame = frame;
}

/*****************************************************************************/
/*                                                                           */
/* Bsp                                                                       */
/*                                                                           */
/*****************************************************************************/

int Bsp::num_portals;
Portal *Bsp::portals;
int Bsp::num_sectors;
Sector *Bsp::sectors;
int Bsp::num_visible_sectors;
Sector **Bsp::visible_sectors;
int Bsp::old_num_visible_sectors;
Sector **Bsp::old_visible_sectors;

Bsp::Bsp() {
	num_portals = 0;
	portals = NULL;
	num_sectors = 0;
	sectors = NULL;
	num_visible_sectors = 0;
	visible_sectors = NULL;
	old_num_visible_sectors = 0;
	old_visible_sectors = NULL;
}

Bsp::~Bsp() {
	if(num_portals) delete [] portals;
	num_portals = 0;
	portals = NULL;
	if(num_sectors) delete [] sectors;
	num_sectors = 0;
	sectors = NULL;
	if(visible_sectors) delete visible_sectors;
	num_visible_sectors = 0;
	visible_sectors = NULL;
	if(old_visible_sectors) delete old_visible_sectors;
	old_num_visible_sectors = 0;
	old_visible_sectors = NULL;
}

/*****************************************************************************/
/*                                                                           */
/* Bsp IO                                                                    */
/*                                                                           */
/*****************************************************************************/

/*
 */
void Bsp::load(const char *name) {
	
	if(strstr(name,".bsp")) {	// read own binary bsp format
		
		FILE *file = fopen(name,"rb");
		if(!file) {
			fprintf(stderr,"Bsp::load(): error open \"%s\" file\n",name);
			return;
		}
		
		int magic;
		fread(&magic,sizeof(int),1,file);
		if(magic != BSP_MAGIC) {
			fprintf(stderr,"Bsp::load(): wrong magic in \"%s\" file\n",name);
			fclose(file);
			return;
		}
		
		fread(&num_portals,sizeof(int),1,file);
		portals = new Portal[num_portals];
		for(int i = 0; i < num_portals; i++) {
			Portal *p = &portals[i];
			fread(&p->center,sizeof(vec3),1,file);
			fread(&p->radius,sizeof(float),1,file);
			fread(&p->num_sectors,sizeof(int),1,file);
			p->sectors = new int[p->num_sectors];
			fread(p->sectors,sizeof(int),p->num_sectors,file);
			fread(p->points,sizeof(vec3),4,file);
		}
		
		fread(&num_sectors,sizeof(int),1,file);
		sectors = new Sector[num_sectors];
		for(int i = 0; i < num_sectors; i++) {
			Sector *s = &sectors[i];
			fread(&s->center,sizeof(vec3),1,file);
			fread(&s->radius,sizeof(float),1,file);
			fread(&s->num_portals,sizeof(int),1,file);
			s->portals = new int[s->num_portals];
			fread(s->portals,sizeof(int),s->num_portals,file);
			fread(&s->num_planes,sizeof(int),1,file);
			s->planes = new vec4[s->num_planes];
			fread(s->planes,sizeof(vec4),s->num_planes,file);
			s->root = new Node();
			s->root->load(file);
			s->create();
		}
		
		fclose(file);
		
		visible_sectors = new Sector*[num_sectors];
		old_visible_sectors = new Sector*[num_sectors];
		
		Engine::console->printf("sectors %d\nportals %d\n",num_sectors,num_portals);
		
		return;
	}
	
	// else generate bsp tree with portals and sectors
	Mesh *mesh = new Mesh();
	if(strstr(name,".3ds")) mesh->load_3ds(name);
	else if(strstr(name,".mesh")) mesh->load_mesh(name);
	
	mesh->calculate_bounds();
	mesh->calculate_tangent();

	for(int i = 0; i < mesh->getNumSurfaces(); i++) {
		const char *name = mesh->getSurfaceName(i);
		if(!strncmp(name,"portal",6)) num_portals++;
		else if(!strncmp(name,"sector",6)) num_sectors++;
	}
	
	if(num_portals == 0 || num_sectors == 0) {
		
		num_sectors = 1;
		sectors = new Sector[1];
		sectors[0].root = new Node();
		sectors[0].root->create(mesh);
		sectors[0].create();
		
	} else {
		
		portals = new Portal[num_portals];
		sectors = new Sector[num_sectors];
		
		int *usage_flag = new int[mesh->getNumSurfaces()];
		
		num_portals = 0;
		num_sectors = 0;
		for(int i = 0; i < mesh->getNumSurfaces(); i++) {
			const char *name = mesh->getSurfaceName(i);
			if(!strncmp(name,"portal",6)) {
				portals[num_portals++].create(mesh,i);
				usage_flag[i] = 1;
			} else if(!strncmp(name,"sector",6)) {
				sectors[num_sectors++].create(mesh,i);
				usage_flag[i] = 1;
			} else usage_flag[i] = 0;
		}
		
		for(int i = 0; i < num_portals; i++) portals[i].sectors = new int[num_sectors];
		for(int i = 0; i < num_sectors; i++) sectors[i].portals = new int[num_portals];
		
		for(int i = 0; i < num_sectors; i++) {
			Sector *s = &sectors[i];
			for(int j = 0; j < num_portals; j++) {
				Portal *p = &portals[j];
				if(s->inside(p)) {
					p->sectors[p->num_sectors++] = i;
					s->portals[s->num_portals++] = j;
				}
			}
		}
		
		for(int i = 0; i < num_sectors; i++) {
			Sector *s = &sectors[i];
			Mesh *m = new Mesh();
			for(int j = 0; j < mesh->getNumSurfaces(); j++) {
				if(usage_flag[j]) continue;
				if(s->inside(mesh,j)) {
					m->addSurface(mesh,j);
					usage_flag[j] = 1;
				}
			}
			s->root = new Node();
			s->root->create(m);
			s->create();
		}
		
		delete usage_flag;
		delete mesh;
	}
	
	visible_sectors = new Sector*[num_sectors];
	old_visible_sectors = new Sector*[num_sectors];
	
	Engine::console->printf("sectors %d\nportals %d\n",num_sectors,num_portals);
}

/*
 */
void Bsp::save(const char *name) {
	FILE *file = fopen(name,"wb");
	if(!file) {
		fprintf(stderr,"Bsp::save(): can`t create \"%s\" file\n",name);
		return;
	}
	int magic = BSP_MAGIC;
	fwrite(&magic,sizeof(int),1,file);
	fwrite(&num_portals,sizeof(int),1,file);
	for(int i = 0; i < num_portals; i++) {
		Portal *p = &portals[i];
		fwrite(&p->center,sizeof(vec3),1,file);
		fwrite(&p->radius,sizeof(float),1,file);
		fwrite(&p->num_sectors,sizeof(int),1,file);
		fwrite(p->sectors,sizeof(int),p->num_sectors,file);
		fwrite(p->points,sizeof(vec3),4,file);
	}
	fwrite(&num_sectors,sizeof(int),1,file);
	for(int i = 0; i < num_sectors; i++) {
		Sector *s = &sectors[i];
		fwrite(&s->center,sizeof(vec3),1,file);
		fwrite(&s->radius,sizeof(float),1,file);
		fwrite(&s->num_portals,sizeof(int),1,file);
		fwrite(s->portals,sizeof(int),s->num_portals,file);
		fwrite(&s->num_planes,sizeof(int),1,file);
		fwrite(s->planes,sizeof(vec4),s->num_planes,file);
		s->root->save(file);
	}
	fclose(file);
}

/*****************************************************************************/
/*                                                                           */
/* Bsp Render                                                                */
/*                                                                           */
/*****************************************************************************/

/*
 */
void Bsp::bindMaterial(const char *name,Material *material) {
	for(int i = 0; i < num_sectors; i++) sectors[i].bindMaterial(name,material);
}

/*
 */
void Bsp::render() {
	num_visible_sectors = 0;
	if(Engine::camera.sector != -1) {
		sectors[Engine::camera.sector].render();
	} else {
		int sector = -1;
		float dist = 1000000.0;
		for(int i = 0; i < num_sectors; i++) {
			float d = (sectors[i].center - Engine::camera).length();
			if(d < dist) {
				dist = d;
				sector = i;
			}
		}
		if(sector != -1) sectors[sector].render();
	}
}

/*
 */
void Bsp::saveState() {
	old_num_visible_sectors = num_visible_sectors;
	for(int i = 0; i < num_visible_sectors; i++) {
		old_visible_sectors[i] = visible_sectors[i];
		old_visible_sectors[i]->saveState();
	}
}

void Bsp::restoreState(int frame) {
	num_visible_sectors = old_num_visible_sectors;
	for(int i = 0; i < num_visible_sectors; i++) {
		visible_sectors[i] = old_visible_sectors[i];
		visible_sectors[i]->restoreState(frame);
	}
}
