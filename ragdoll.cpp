/* RagDoll
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
#include "mesh.h"
#include "meshvbo.h"
#include "object.h"
#include "objectmesh.h"
#include "rigidbody.h"
#include "joint.h"
#include "parser.h"
#include "ragdoll.h"

RagDoll::RagDoll(SkinnedMesh *skinnedmesh,const char *name) : skinnedmesh(skinnedmesh), num_bones(0), root(-1) {

	Parser *parser = new Parser(Engine::findFile(name));
	
	if(!parser->get("mesh")) {
		fprintf(stderr,"RagDoll::RagDoll(): can`t find mesh in \"%s\" file\n",name);
		return;
	}
	
	if(!parser->get("rigidbodies")) {
		fprintf(stderr,"RagDoll::RagDoll(): can`t find rigidbodies in \"%s\" file\n",name);
		return;
	}
	
	if(!parser->get("joints")) {
		fprintf(stderr,"RagDoll::RagDoll(): can`t find joints in \"%s\" file\n",name);
		return;
	}
	
	num_bones = skinnedmesh->getNumBones();
	bones = skinnedmesh->getBones();
	
	meshes = new Object*[num_bones];
	memset(meshes,0,sizeof(Object*) * num_bones);
	
	rigidbodies = new RigidBody*[num_bones];
	offsets = new mat4[num_bones];
	ioffsets = new mat4[num_bones];
	
	char buf[1024];
	sscanf(parser->get("mesh"),"%s",buf);
	Mesh *mesh = new Mesh(Engine::findFile(buf));
	
	// scale objects
	if(parser->get("scale")) {
		vec3 v;
		sscanf(parser->get("scale"),"%f %f %f",&v.x,&v.y,&v.z);
		mat4 scale;
		scale.scale(v);
		skinnedmesh->transform(scale);
		mesh->transform(scale);
	}
	
	// create bones
	for(int i = 0; i < num_bones; i++) {
		char bone_name[1024];
		float mass = 1.0f;
		float restitution = 0.3f;
		float friction = 0.3f;
		int flag = 0;
		
		bone_name[0] = '\0';
		
		// find bone description in rigidbodies section
		char *s = parser->get("rigidbodies");
		while(*s) {
			char *d = buf;	// get line
			while(*s && *s != '\n') *d++ = *s++;
			while(*s && strchr("\n\r",*s)) s++;
			*d = '\0';
			
			d = buf;
			d += Parser::read_string(d,bone_name);
			
			if(!strcmp(bone_name,bones[i].name)) {
				char body[1024];
				sscanf(d,"%s %f %f %f",body,&mass,&restitution,&friction);
				if(!strcmp(body,"box")) flag = RigidBody::BODY_BOX;
				else if(!strcmp(body,"sphere")) flag = RigidBody::BODY_SPHERE;
				else if(!strcmp(body,"cylinder")) flag = RigidBody::BODY_CYLINDER;
				break;
			}
		}
		
		if(flag == 0) {
			fprintf(stderr,"RigidBody::RigidBody(): can`t find attributes for \"%s\" bone in \"%s\" file\n",bones[i].name,name);
			return;
		}
		
		int surface = mesh->getSurface(bones[i].name);
		if(surface < 0) {
			fprintf(stderr,"RagDoll::RagDoll(): can`t find mesh for \"%s\" bone in \"%s\" file\n",bones[i].name,name);
			return;
		}
		
		// create new mesh
		Mesh *m = new Mesh;
		m->addSurface(mesh,surface);
		vec3 center = (m->getMax() + m->getMin()) / 2.0f;
		offsets[i].translate(-center);
		m->transform(offsets[i]);
		meshes[i] = new ObjectMesh(new MeshVBO(m));
		delete m;
		
		//meshes[i]->bindMaterial("*",Engine::loadMaterial("default.mat"));
		
		rigidbodies[i] = new RigidBody(meshes[i],mass,restitution,friction,RigidBody::COLLIDE_MESH | flag);
		meshes[i]->setRigidBody(rigidbodies[i]);
		
		offsets[i] = bones[i].transform.inverse() * offsets[i].inverse();
		ioffsets[i] = offsets[i].inverse();
		
		meshes[i]->set(bones[i].transform * offsets[i]);
		
		Engine::addObject(meshes[i]);
		
		if(bones[i].parent == -1) {
			if(root == -1) root = i;
			else fprintf(stderr,"RagDoll::RagDoll(): many roots bones in \"%s\" file\n",name);
		}
	}
	
	// create joints
	char *s = parser->get("joints");
	while(*s) {
		char bone_0_name[1024];
		char bone_1_name[1024];
		int bone_0 = -1;
		int bone_1 = -1;
		
		char *d = buf;	// get line
		while(*s && *s != '\n') *d++ = *s++;
		while(*s && strchr("\n\r",*s)) s++;
		*d = '\0';
		
		d = buf;
		d += Parser::read_string(d,bone_0_name);
		d += Parser::read_string(d,bone_1_name);
		
		for(int i = 0; i < num_bones; i++) {
			if(!strcmp(bone_0_name,bones[i].name)) {
				bone_0 = i;
				break;
			}
		}
		if(bone_0 == -1) {
			fprintf(stderr,"RagDoll::RagDoll(): unknown bone \"%s\" in \"%s\" file\n",bone_0_name,name);
			continue;
		}
		
		for(int i = 0; i < num_bones; i++) {
			if(!strcmp(bone_1_name,bones[i].name)) {
				bone_1 = i;
				break;
			}
		}
		if(bone_1 == -1) {
			fprintf(stderr,"RagDoll::RagDoll(): unknown bone \"%s\" in \"%s\" file\n",bone_1_name,name);
			continue;
		}
		
		if(bones[bone_1].parent == bone_0) {
			// bone_0 - parent
			// bone_1 - child
		} else if(bones[bone_0].parent == bone_1) {
			int i = bone_0;
			bone_0 = bone_1;
			bone_1 = i;
		} else {
			fprintf(stderr,"RagDoll::RagDoll(): can`t joint \"%s\" and \"%s\" bones in \"%s\" file\n",bone_0_name,bone_1_name,name);
			continue;
		}
		
		char type[1024];
		d += Parser::read_string(d,type);

		if(!strcmp(type,"ball")) {	// ball - socket joint
			vec3 restriction_axis_0,restriction_axis_1;
			float restriction_angle;
			int num = sscanf(d,"%f %f %f %f %f %f %f",
				&restriction_axis_0.x,&restriction_axis_0.y,&restriction_axis_0.z,
				&restriction_axis_1.x,&restriction_axis_1.y,&restriction_axis_1.z,
				&restriction_angle);
			if(num == 7) new JointBall(rigidbodies[bone_0],rigidbodies[bone_1],bones[bone_1].transform * vec3(0,0,0),restriction_axis_0,restriction_axis_1,restriction_angle);
			else if(num == 0) new JointBall(rigidbodies[bone_0],rigidbodies[bone_1],bones[bone_1].transform * vec3(0,0,0));
			else fprintf(stderr,"RagDoll::RagDoll(): bad arguments for \"%s\" \"%s\" bones in \"%s\" file\n",bone_0_name,bone_1_name,name);
		} else if(!strcmp(type,"hinge")) {	// hinge joint
			vec3 axis,restriction_axis_0,restriction_axis_1;
			float restriction_angle;
			int num = sscanf(d,"%f %f %f %f %f %f %f %f %f %f",
				&axis.x,&axis.y,&axis.z,
				&restriction_axis_0.x,&restriction_axis_0.y,&restriction_axis_0.z,
				&restriction_axis_1.x,&restriction_axis_1.y,&restriction_axis_1.z,
				&restriction_angle);
			if(num == 10) new JointHinge(rigidbodies[bone_0],rigidbodies[bone_1],bones[bone_1].transform * vec3(0,0,0),axis,restriction_axis_0,restriction_axis_1,restriction_angle);
			else if(num == 3) new JointHinge(rigidbodies[bone_0],rigidbodies[bone_1],bones[bone_1].transform * vec3(0,0,0),axis);
			else fprintf(stderr,"RagDoll::RagDoll(): bad arguments for \"%s\" \"%s\" bones in \"%s\" file\n",bone_0_name,bone_1_name,name);
		} else if(!strcmp(type,"universal")) {	// universal joint
			vec3 axis_0,axis_1,restriction_axis_0,restriction_axis_1;
			float restriction_angle;
			int num = sscanf(d,"%f %f %f %f %f %f %f %f %f %f %f %f %f",
				&axis_0.x,&axis_0.y,&axis_0.z,&axis_1.x,&axis_1.y,&axis_1.z,
				&restriction_axis_0.x,&restriction_axis_0.y,&restriction_axis_0.z,
				&restriction_axis_1.x,&restriction_axis_1.y,&restriction_axis_1.z,
				&restriction_angle);
			if(num == 13) new JointUniversal(rigidbodies[bone_0],rigidbodies[bone_1],bones[bone_1].transform * vec3(0,0,0),axis_0,axis_1,restriction_axis_0,restriction_axis_1,restriction_angle);
			else if(num == 6) new JointUniversal(rigidbodies[bone_0],rigidbodies[bone_1],bones[bone_1].transform * vec3(0,0,0),axis_0,axis_1);
			else fprintf(stderr,"RagDoll::RagDoll(): bad arguments for \"%s\" \"%s\" bones in \"%s\" file\n",bone_0_name,bone_1_name,name);
		} else {
			fprintf(stderr,"RagDoll::RagDoll(): unknown joint type \"%s\" in \"%s\" file\n",type,name);
			continue;
		}
	}
	
	delete mesh;
	delete parser;
}

RagDoll::~RagDoll() {
	for(int i = 0; i < num_bones; i++) {
		if(meshes[i]) {
			delete meshes[i];
		}
	}
	delete meshes;
	delete offsets;
	delete ioffsets;
}

/*
 */
void RagDoll::update() {
	for(int i = 0; i < num_bones; i++) {
		if(meshes[i]) {
			bones[i].transform = itransform * meshes[i]->transform * ioffsets[i];
			bones[i].rotation = bones[i].transform.rotation();
		}
	}
}

/*
 */
void RagDoll::setTransform(const mat4 &m) {
	transform = m;
	itransform = transform.inverse();
	for(int i = 0; i < num_bones; i++) {
		if(meshes[i]) {
			meshes[i]->set(m * bones[i].transform * offsets[i]);
		}
	}
}

/*
 */
void RagDoll::set(const mat4 &m) {
	if(root == -1) return;
	meshes[root]->set(m);
}
