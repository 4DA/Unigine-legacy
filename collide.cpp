/* Collide
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
#include "object.h"
#include "objectmesh.h"
#include "objectskinnedmesh.h"
#include "mesh.h"
#include "skinnedmesh.h"
#include "rigidbody.h"
#include "collide.h"

int Collide::counter;
Position Collide::position;
int Collide::num_surfaces;
Collide::Surface *Collide::surfaces;

/*
 */
Collide::Collide() : num_contacts(0), num_objects(0) {
	
	contacts = new Contact[NUM_CONTACTS];
	objects = new Object*[NUM_OBJECTS];
	
	if(counter++ == 0) {
		surfaces = new Surface[NUM_SURFACES];
		for(int i = 0; i < NUM_SURFACES; i++) {
			surfaces[i].triangles = new Triangle[NUM_TRIANGLES];
		}
	}
}

Collide::~Collide() {
	
	delete contacts;
	delete objects;
	
	if(--counter == 0) {
		for(int i = 0; i < NUM_SURFACES; i++) {
			delete surfaces[i].triangles;
		}
		delete surfaces;
	}
}

/*****************************************************************************/
/*                                                                           */
/* add contact                                                               */
/*                                                                           */
/*****************************************************************************/

int Collide::addContact(Object *object,Material *material,const vec3 &point,const vec3 &normal,float depth,int min_depth) {
	Contact *c = &contacts[num_contacts++];
	if(min_depth) {
		vec3 p = object->is_identity ? point : object->transform * point;
		for(int i = 0; i < num_contacts; i++) {
			if(contacts[i].point == p) {
				num_contacts--;
				if(contacts[i].depth < depth) return 1;
				c = &contacts[i];
				break;
			}
		}
	}
	if(num_contacts == NUM_CONTACTS) {
		num_contacts--;
		return 0;
	}
	if(num_objects == 0) {
		objects[num_objects++] = object;
	} else {
		for(int i = 0; i < num_objects; i++) {
			if(objects[i] == object) break;
			else if(i == num_objects - 1) objects[num_objects++] = object;
		}
	}
	c->object = object;
	c->material = material;
	c->point = object->is_identity ? point : object->transform * point;
	c->normal = object->is_identity ? normal : object->transform.rotation() * normal;
	c->depth = depth;
	
	/*glDepthFunc(GL_ALWAYS);
	glPointSize(5.0);
	glColor3f(0,1,0);
	glBegin(GL_POINTS);
	glVertex3fv(c->point);
	glEnd();
	glColor3f(0,0,1);
	glBegin(GL_LINES);
	glVertex3fv(c->point);
	glVertex3fv(c->point + c->normal * c->depth * 20);
	glEnd();*/
	
	return 1;
}

/*****************************************************************************/
/*                                                                           */
/* collide with sphere                                                       */
/*                                                                           */
/*****************************************************************************/

int Collide::collide(Object *object,const vec3 &pos,float radius) {
	position = pos;
	return collide(object,position,radius);
}

int Collide::collide(Object *object,const Position &pos,float radius) {
	num_contacts = 0;
	num_objects = 0;
	
	if(Bsp::num_sectors == 0) return 0;
	if(pos.sector == -1) return 0;
	
	for(int i = 0; i < pos.num_sectors; i++) {
		Sector *s = &Bsp::sectors[pos.sectors[i]];
		for(int j = 0; j < s->num_node_objects; j++) {	// static objects
			Object *o = s->node_objects[j];
			if((o->getCenter() - pos).length() >= o->getRadius() + radius) continue;
			collideObjectSphere(o,pos,radius);
		}
		for(int j = 0; j < s->num_objects; j++) {	// dynamic objects
			Object *o = s->objects[j];
			if(object == o) continue;
			vec3 p = o->is_identity ? pos : o->itransform * pos;
			if((o->getCenter() - p).length() >= o->getRadius() + radius) continue;
			if(object && object->rigidbody && object->rigidbody->num_joints > 0 && o->rigidbody) {
				RigidBody *rb = object->rigidbody;
				int k;
				for(k = 0; k < rb->num_joints; k++) {
					if(rb->joined_rigidbodies[k] == o->rigidbody) break;
				}
				if(k == rb->num_joints) collideObjectSphere(o,p,radius);
			} else {
				collideObjectSphere(o,p,radius);
			}
		}
	}
	return num_contacts;
}

/*
 */
void Collide::collideObjectSphere(Object *object,const vec3 &pos,float radius) {
	
	static int next[3] = { 1, 2, 0 };
	
	// collide sphere-Mesh
	if(object->type == Object::OBJECT_MESH) {
		
		// collide sphere-sphere
		if(object->rigidbody && object->rigidbody->collide_type == RigidBody::BODY_SPHERE) {
			float r = object->getRadius();
			if(pos.length() < radius + r) {
				vec3 normal = pos;
				normal.normalize();
				if(!addContact(object,NULL,normal * radius,normal,radius + r - pos.length())) return;
			}
			return;
		}
		
		// collide sphere-Mesh
		Mesh *mesh = reinterpret_cast<ObjectMesh*>(object)->mesh;
		
		for(int i = 0; i < mesh->getNumSurfaces(); i++) {
			if((mesh->getCenter(i) - pos).length() > mesh->getRadius(i) + radius) continue;
			
			Mesh::Triangle *triangles = mesh->getTriangles(i);
			
			int num_triangles = mesh->getNumTriangles(i);
			for(int j = 0; j < num_triangles; j++) {
				Mesh::Triangle *t = &triangles[j];
				
				// distance from the center of sphere to the plane of triangle
				float dist = vec4(pos,1) * t->plane;
				if(dist >= radius || dist < 0) continue;
				
				vec3 normal = t->plane;
				vec3 point = pos - normal * radius;
				float depth = radius - dist;
				
				int k;	// point in traingle
				for(k = 0; k < 3; k++) if(vec4(point,1) * t->c[k] < 0.0) break;
				
				// collide sphere with edges
				if(k != 3) {
					point = pos - normal * dist;	// point on triangle plane
					for(k = 0; k < 3; k++) {
						vec3 edge = t->v[next[k]] - t->v[k];
						vec3 dir = cross(edge,normal);
						dir.normalize();
						
						float d = (point - t->v[k]) * dir;
						if(d >= radius || d <= 0) continue;
						
						vec3 p = point - dir * d;	// point on edge
						
						float dot = p * edge;	// clamp point
						if(dot > t->v[next[k]] * edge) p = t->v[next[k]];
						else if(dot < t->v[k] * edge) p = t->v[k];
						
						d = (point - p).length();
						if(d > radius) continue;
						
						depth = sqrt(radius * radius - d * d) - dist;
						if(depth <= 0.0) continue;
						
						point = p - normal * depth;
						
						break;	// ok
					}
					if(k == 3) continue;	// next triangle
				}
				
				// add new contact
				if(!addContact(object,object->materials[i],point,t->plane,depth)) return;
			}
		}
		return;
	}
	
	// collide sphere-SkinnedMesh
	/*if(object->type == Object::OBJECT_SKINNEDMESH) {
		
		ObjectSkinnedMesh *objectskinnedmesh = reinterpret_cast<ObjectSkinnedMesh*>(object);
		if(objectskinnedmesh->ragdoll) return;
		
		SkinnedMesh *skinnedmesh = objectskinnedmesh->skinnedmesh;
		
		for(int i = 0; i < skinnedmesh->getNumSurfaces(); i++) {
			if((skinnedmesh->getCenter(i) - pos).length() > skinnedmesh->getRadius(i) + radius) continue;
			
			SkinnedMesh::Vertex *vertex = skinnedmesh->getVertex(i);
			SkinnedMesh::Triangle *triangles = skinnedmesh->getTriangles(i);
			
			int num_triangles = skinnedmesh->getNumTriangles(i);
			for(int j = 0; j < num_triangles; j++) {
				SkinnedMesh::Triangle *t = &triangles[j];
				
				// distance from the center of sphere to the plane of triangle
				float dist = vec4(pos,1) * t->plane;
				if(dist >= radius || dist < 0) continue;
				
				vec3 normal = vec3(t->plane);
				vec3 point = pos - normal * radius;
				float depth = radius - dist;
				
				int k;	// point in traingle
				for(k = 0; k < 3; k++) if(vec4(point,1) * t->c[k] < 0.0) break;
				
				// collide sphere with edges
				if(k != 3) {
					point = pos - normal * dist;	// point on triangle plane
					for(k = 0; k < 3; k++) {
						float d = (point - vertex[t->v[k]].xyz) * t->c[k];
						if(d >= radius || d <= 0) continue;
						
						vec3 edge = vertex[t->v[k]].xyz - vertex[t->v[next[k]]].xyz;
						vec3 p = point - t->c[k] * d;	// point on edge
						
						float dot = p * edge;	// clamp point
						if(dot > vertex[t->v[next[k]]].xyz * edge) p = vertex[t->v[next[k]]].xyz;
						else if(dot < vertex[t->v[k]].xyz * edge) p = vertex[t->v[k]].xyz;
						
						d = (point - p).length();
						if(d > radius) continue;
						
						depth = sqrt(radius * radius - d * d) - dist;
						if(depth <= 0.0) continue;
						
						point = p - normal * depth;
						
						break;	// ok
					}
					if(k == 3) continue;	// next triangle
				}
				
				// add new contact
				if(!addContact(object,object->materials[i],point,normal,depth)) return;
			}
		}
		return;
	}*/
}

/*****************************************************************************/
/*                                                                           */
/* collide with mesh                                                         */
/*                                                                           */
/*****************************************************************************/

int Collide::collide(Object *object) {
	
	num_contacts = 0;
	num_objects = 0;
	
	if(Bsp::num_sectors == 0) return 0;
	if(object->pos.sector == -1) return 0;
	
	num_surfaces = 0;
	
	// transform and copy triangles from the mesh in to collide surface
	if(object->type == Object::OBJECT_MESH) {
		
		Mesh *mesh = reinterpret_cast<ObjectMesh*>(object)->mesh;
		
		for(int i = 0; i < mesh->getNumSurfaces(); i++) {
			
			surfaces[num_surfaces].num_triangles = mesh->getNumTriangles(i);
			
			if(surfaces[num_surfaces].num_triangles > NUM_TRIANGLES) {
				fprintf(stderr,"Collide::collide(): many vertex %d\n",surfaces[num_surfaces].num_triangles);
				continue;
			}
			
			Mesh::Triangle *t = mesh->getTriangles(i);
			Triangle *ct = surfaces[num_surfaces].triangles;
			
			vec3 min = vec3(1000000,1000000,1000000);	// calculate bound box
			vec3 max = vec3(-1000000,-1000000,-1000000);
			
			for(int j = 0; j < surfaces[num_surfaces].num_triangles; j++) {
				ct[j].v[0] = object->transform * t[j].v[0];
				ct[j].v[1] = object->transform * t[j].v[1];
				ct[j].v[2] = object->transform * t[j].v[2];
				mat4 rotate = object->transform.rotation();
				vec3 normal = rotate * vec3(t[j].plane);
				ct[j].plane = vec4(normal,-ct[j].v[0] * normal);
				normal = rotate * vec4(t[j].c[0]);
				ct[j].c[0] = vec4(normal,-ct[j].v[0] * normal);
				normal = rotate * vec4(t[j].c[1]);
				ct[j].c[1] = vec4(normal,-ct[j].v[1] * normal);
				normal = rotate * vec4(t[j].c[2]);
				ct[j].c[2] = vec4(normal,-ct[j].v[2] * normal);
				for(int k = 0; k < 3; k++) {
					if(min.x > ct[j].v[k].x) min.x = ct[j].v[k].x;
					else if(max.x < ct[j].v[k].x) max.x = ct[j].v[k].x;
					if(min.y > ct[j].v[k].y) min.y = ct[j].v[k].y;
					else if(max.y < ct[j].v[k].y) max.y = ct[j].v[k].y;
					if(min.z > ct[j].v[k].z) min.z = ct[j].v[k].z;
					else if(max.z < ct[j].v[k].z) max.z = ct[j].v[k].z;
				}
			}
			// bound sphere
			surfaces[num_surfaces].center = (max + min) / 2.0f;
			surfaces[num_surfaces].radius = (max - min).length() / 2.0f;
			// bound box
			surfaces[num_surfaces].min = min;
			surfaces[num_surfaces].max = max;
		
			// new surface
			num_surfaces++;
			if(num_surfaces == NUM_SURFACES) {
				fprintf(stderr,"Collide::collide(): many surfaces %d\n",NUM_SURFACES);
				break;
			}
		}
	} else {
		fprintf(stderr,"Collide::collide(): %d format isn`t supported\n",object->type);
	}
	
	if(!num_surfaces) return 0;
	
	// static objects
	for(int i = 0; i < object->pos.num_sectors; i++) {
		Sector *s = &Bsp::sectors[object->pos.sectors[i]];
		for(int j = 0; j < s->num_node_objects; j++) {
			Object *o = s->node_objects[j];
			if((o->getCenter() - object->pos - object->getCenter()).length() >= o->getRadius() + object->getRadius()) continue;
			collideObjectMesh(o);
		}
	}
	
	// dynamic objects
	for(int i = 0; i < object->pos.num_sectors; i++) {
		
		Sector *s = &Bsp::sectors[object->pos.sectors[i]];
		
		for(int j = 0; j < s->num_objects; j++) {
			
			Object *o = s->objects[j];
			if(object == o) continue;
			
			if((o->pos + o->getCenter() - object->pos - object->getCenter()).length() >= o->getRadius() + object->getRadius()) continue;
			mat4 transform = o->itransform * object->transform;
			
			if(object->rigidbody && object->rigidbody->num_joints > 0 && o->rigidbody) {
				RigidBody *rb = object->rigidbody;
				int k;
				for(k = 0; k < rb->num_joints; k++) {
					if(rb->joined_rigidbodies[k] == o->rigidbody) break;
				}
				if(k != rb->num_joints) continue;
			}
			
			// object bound box
			vec3 object_min = vec3(1000000,1000000,1000000);
			vec3 object_max = vec3(-1000000,-1000000,-1000000);
			
			// transform vertex and calcualte bound box
			num_surfaces = 0;
			if(object->type == Object::OBJECT_MESH) {
				Mesh *mesh = reinterpret_cast<ObjectMesh*>(object)->mesh;
				for(int k = 0; k < mesh->getNumSurfaces(); k++) {
					
					surfaces[num_surfaces].num_triangles = mesh->getNumTriangles(k);
					if(surfaces[num_surfaces].num_triangles > NUM_TRIANGLES) continue;
					
					Mesh::Triangle *t = mesh->getTriangles(k);
					Triangle *ct = surfaces[num_surfaces].triangles;
					
					vec3 min = vec3(1000000,1000000,1000000);	// calculate bound box
					vec3 max = vec3(-1000000,-1000000,-1000000);
					for(int l = 0; l < surfaces[num_surfaces].num_triangles; l++) {
						ct[l].v[0] = transform * t[l].v[0];
						ct[l].v[1] = transform * t[l].v[1];
						ct[l].v[2] = transform * t[l].v[2];
						for(int m = 0; m < 3; m++) {
							if(min.x > ct[l].v[m].x) min.x = ct[l].v[m].x;
							else if(max.x < ct[l].v[m].x) max.x = ct[l].v[m].x;
							if(min.y > ct[l].v[m].y) min.y = ct[l].v[m].y;
							else if(max.y < ct[l].v[m].y) max.y = ct[l].v[m].y;
							if(min.z > ct[l].v[m].z) min.z = ct[l].v[m].z;
							else if(max.z < ct[l].v[m].z) max.z = ct[l].v[m].z;
						}
					}
					// bound sphere
					surfaces[num_surfaces].center = (max + min) / 2.0f;
					surfaces[num_surfaces].radius = (max - min).length() / 2.0f;
					// bound box
					surfaces[num_surfaces].min = min;
					surfaces[num_surfaces].max = max;
					// object bound box
					if(object_min.x > min.x) object_min.x = min.x;
					if(object_max.x < max.x) object_max.x = max.x;
					if(object_min.y > min.y) object_min.y = min.y;
					if(object_max.y < max.y) object_max.y = max.y;
					if(object_min.z > min.z) object_min.z = min.z;
					if(object_max.z < max.z) object_max.z = max.z;
					
					num_surfaces++;
					if(num_surfaces == NUM_SURFACES) break;
				}
			}
			
			if(num_surfaces == 0) continue;
			
			// bound box test
			const vec3 &min = o->getMin();
			const vec3 &max = o->getMax();
			
			if(min.x >= object_max.x) continue;
			if(max.x <= object_min.x) continue;
			if(min.y >= object_max.y) continue;
			if(max.y <= object_min.y) continue;
			if(min.z >= object_max.z) continue;
			if(max.z <= object_min.z) continue;
			
			// recalculate normals and fast point and triangle
			num_surfaces = 0;
			if(object->type == Object::OBJECT_MESH) {
				Mesh *mesh = reinterpret_cast<ObjectMesh*>(object)->mesh;
				for(int k = 0; k < mesh->getNumSurfaces(); k++) {
					
					surfaces[num_surfaces].num_triangles = mesh->getNumTriangles(k);
					if(surfaces[num_surfaces].num_triangles > NUM_TRIANGLES) continue;
					
					Mesh::Triangle *t = mesh->getTriangles(k);
					Triangle *ct = surfaces[num_surfaces].triangles;
					
					for(int l = 0; l < surfaces[num_surfaces].num_triangles; l++) {
						mat4 rotate = transform.rotation();
						vec3 normal = rotate * vec3(t[l].plane);
						ct[l].plane = vec4(normal,-ct[l].v[0] * normal);
						normal = rotate * vec4(t[l].c[0]);
						ct[l].c[0] = vec4(normal,-ct[l].v[0] * normal);
						normal = rotate * vec4(t[l].c[1]);
						ct[l].c[1] = vec4(normal,-ct[l].v[1] * normal);
						normal = rotate * vec4(t[l].c[2]);
						ct[l].c[2] = vec4(normal,-ct[l].v[2] * normal);
					}
					
					num_surfaces++;
					if(num_surfaces == NUM_SURFACES) break;
				}
			}
			if(num_surfaces == 0) continue;
			
			// collide it
			collideObjectMesh(o);
		}
	}
	
	return num_contacts;
}

/*
 */
void Collide::collideObjectMesh(Object *object) {
	
	static int next[3] = { 1, 2, 0 };
	
	// collide Mesh-Mesh
	if(object->type == Object::OBJECT_MESH) {
		
		Mesh *mesh = reinterpret_cast<ObjectMesh*>(object)->mesh;
		
		for(int i = 0; i < mesh->getNumSurfaces(); i++) {
			for(int j = 0; j < num_surfaces; j++) {
				
				if((mesh->getCenter(i) - surfaces[j].center).length() >= mesh->getRadius(i) + surfaces[j].radius) continue;
				
				const vec3 &min = mesh->getMin(i);
				const vec3 &max = mesh->getMax(i);
				
				if(min.x >= surfaces[j].max.x) continue;
				if(max.x <= surfaces[j].min.x) continue;
				if(min.y >= surfaces[j].max.y) continue;
				if(max.y <= surfaces[j].min.y) continue;
				if(min.z >= surfaces[j].max.z) continue;
				if(max.z <= surfaces[j].min.z) continue;
				
				Mesh::Triangle *triangles = mesh->getTriangles(i);
				Surface *s = &surfaces[j];
				
				int num_triangles = mesh->getNumTriangles(i);
				for(int k = 0; k < num_triangles; k++) {
					
					Mesh::Triangle *t = &triangles[k];
					
					for(int l = 0; l < s->num_triangles; l++) {
						
						Triangle *ct = &s->triangles[l];
						
						// fast triangle-triangle intersection test
						float dist0[3];
						int collide0 = 0;
						for(int m = 0; m < 3; m++) {
							dist0[m] = ct->v[m] * t->plane;
							if(fabs(dist0[m]) > s->radius * 2) {
								collide0 = 3;
								break;
							}
							if(dist0[m] > 0.0) collide0++;
							else if(dist0[m] < 0.0) collide0--;
						}
						if(collide0 == 3 || collide0 == -3) continue;
						
						for(int m = 0; m < 3; m++) {
							if(!((dist0[m] <= 0.0) ^ (dist0[next[m]] <= 0.0))) continue;
							
							vec3 &v0 = ct->v[m];
							vec3 &v1 = ct->v[next[m]];
							vec3 edge = v1 - v0;
							vec3 p = v0 - edge * dist0[m] / (dist0[next[m]] - dist0[m]);	// intersect ct edge with t plane
							
							int p_inside = p * t->c[0] > 0.0 && p * t->c[1] > 0.0 && p * t->c[2] > 0.0;
							if(!p_inside) continue;
							
							int v0_inside = v0 * t->c[0] > 0.0 && v0 * t->c[1] > 0.0 && v0 * t->c[2] > 0.0;
							int v1_inside = v1 * t->c[0] > 0.0 && v1 * t->c[1] > 0.0 && v1 * t->c[2] > 0.0;
							
							if(v0_inside && v1_inside) {
								float length = edge.length() / 2.0f;
								if(dist0[m] < 0.0 && dist0[m] > -length) {
									if(!addContact(object,object->materials[i],v0,t->plane,-dist0[m],true)) return;
								} else if(dist0[next[m]] < 0.0 && dist0[next[m]] > -length) {
									if(!addContact(object,object->materials[i],v1,t->plane,-dist0[next[m]],true)) return;
								}
							}
							
							if(v0_inside != v1_inside) {
								float dist = 1000000;
								for(int n = 0; n < 3; n++) {
									//float d = t->v[n] * t->c[n];
									float d = p * t->c[n];
									if(dist > d) dist = d;
								}
								if(dist0[m] < 0.0) {
									float d = (v0 - p).length();
									if(dist > d) dist = 0;
									else dist = -dist0[m] * dist / d;
								} else {
									float d = (v1 - p).length();
									if(dist > d) dist = 0;
									else dist = -dist0[next[m]] * dist / d;
								}
								if(!addContact(object,object->materials[i],p,-ct->plane,dist)) return;
							}
						}
						
						// collide with scene
						if(!object->rigidbody) {
							
							float dist1[3];
							int collide1 = 0;
							for(int m = 0; m < 3; m++) {
								dist1[m] = t->v[m] * ct->plane;
								if(dist1[m] > 0.0) collide1++;
								else if(dist1[m] < 0.0) collide1--;
							}
							if(collide1 == 3 || collide1 == -3) continue;
						
							for(int m = 0; m < 3; m++) {
								if(dist1[m] >= 0) continue;
								if(dist1[next[m]] <= 0) continue;

								vec3 &v0 = t->v[m];
								vec3 &v1 = t->v[next[m]];
								vec3 edge = v1 - v0;
								vec3 p = v0 - edge * dist1[m] / (dist1[next[m]] - dist1[m]);	// intersect ct edge with t plane
								
								int p_inside = p * ct->c[0] > 0.0 && p * ct->c[1] > 0.0 && p * ct->c[2] > 0.0;
								if(!p_inside) continue;
								
								int v0_inside = v0 * ct->c[0] > 0.0 && v0 * ct->c[1] > 0.0 && v0 * ct->c[2] > 0.0;
								int v1_inside = v1 * ct->c[0] > 0.0 && v1 * ct->c[1] > 0.0 && v1 * ct->c[2] > 0.0;
								
								if(v0_inside != v1_inside) {
									float dist = 1000000;
									for(int n = 0; n < 3; n++) {
										float d = p * ct->c[n];
										if(dist > d) dist = d;
									}
									float d = (v0 - p).length();
									if(dist > d) dist = 0;
									else dist = -dist1[m] * dist / d;
									if(!addContact(object,object->materials[i],p,t->plane,dist)) return;
								}
							}
						}
					}
				}
			}
		}
		return;
	}
}

/*****************************************************************************/
/*                                                                           */
/* sorting contacts                                                          */
/*                                                                           */
/*****************************************************************************/

static int contact_cmp(const void *a,const void *b) {
	Collide::Contact *c0 = (Collide::Contact*)a;
	Collide::Contact *c1 = (Collide::Contact*)b;
	if(c0->object->pos.z > c1->object->pos.z) return 1;
	if(c0->object->pos.z < c1->object->pos.z) return -1;
	if(c0->point.z > c1->point.z) return 1;
	if(c0->point.z < c1->point.z) return -1;
	return 0;
}

void Collide::sort() {
	qsort(contacts,num_contacts,sizeof(Contact),contact_cmp);
}
