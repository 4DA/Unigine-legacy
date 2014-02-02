/* Skinned Mesh Export plugin for 3DStudio
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

#include <vector>
#include "max.h"
#include "iparamm2.h"
#include "modstack.h"
#include "iskin.h"

#pragma comment(lib, "core.lib")
#pragma comment(lib, "maxutil.lib")
#pragma comment(lib, "geom.lib")
#pragma comment(lib, "mesh.lib")

/*****************************************************************************/
/*                                                                           */
/* Mesh Enum Proc                                                            */
/*                                                                           */
/*****************************************************************************/

class SkinnedMeshEnumProc : public ITreeEnumProc {
public:
	
	SkinnedMeshEnumProc(const char *name,IScene *scene,Interface *i,DWORD options);
	~SkinnedMeshEnumProc() { }
	
	int callback(INode *node);
	void export(FILE *file);

	Interface *inf;
	int selected;
	
	std::vector<INode*> bones;
	
	std::vector<INode*> surfaces;
	std::vector<Modifier*> modifiers;
	
	int total_vertex;
	int total_triangles;

	enum {
		MAX_WEIGHTS	= 32,
	};
	
	struct Weight {
		int bone;
		float weight;
		Point3 xyz;
		Point3 normal;
	};
	
	struct Vertex {
		Point3 xyz;
		Point3 normal;
		Point3 texcoord;
		int num_weights;
		Weight weights[MAX_WEIGHTS];
		int flag;
	};

	struct Triangle {
		int v[3];
	};
};

SkinnedMeshEnumProc::SkinnedMeshEnumProc(const char *name,IScene *scene,Interface *i,DWORD options) {
	FILE *file = fopen(name,"wb");
	if(!file) {
		char error[1024];
		sprintf(error,"error create \"%s\" file",name);
		MessageBox(NULL,error,"Skinned Mesh Export error",MB_OK);
		return;
	}

	inf = i;
	selected = (options & SCENE_EXPORT_SELECTED) ? TRUE : FALSE;
	
	total_vertex = 0;
	total_triangles = 0;

	scene->EnumTree(this);
	
	export(file);
	
	fclose(file);
	
	char buffer[1024];
	sprintf(buffer,"OK\nbones %d\nsurfaces %d\nvertexes %d\ntriangles %d\n",bones.size(),surfaces.size(),total_vertex,total_triangles);
	MessageBox(NULL,buffer,"Skinned Mesh Export",MB_OK);
}

/*
 */
int SkinnedMeshEnumProc::callback(INode *node) {
	
	if(selected && node->Selected() == FALSE) return TREE_CONTINUE;
	
	Object *obj = node->GetObjectRef();
	if(!obj) return TREE_CONTINUE;
	if(obj->SuperClassID() != GEN_DERIVOB_CLASS_ID) return TREE_CONTINUE;
	
	IDerivedObject *dobj = static_cast<IDerivedObject*>(obj);
	for(int i = 0; i < dobj->NumModifiers(); i++) {
		Modifier *modifier = dobj->GetModifier(i);
		if(modifier->ClassID() == SKIN_CLASSID) {
			surfaces.push_back(node);
			modifiers.push_back(modifier);
			return TREE_CONTINUE;
		}
	}
	
	return TREE_CONTINUE;
}

/*
 */
void SkinnedMeshEnumProc::export(FILE *file) {
	
	// find all bones
	for(int i = 0; i < (int)modifiers.size(); i++) {
		ISkin *skin = (ISkin*)modifiers[i]->GetInterface(I_SKIN);
		if(skin == NULL) {
			MessageBox(NULL,"skin is NULL","Skinned Mesh Export error",MB_OK);
			return;
		}
		for(int i = 0; i < skin->GetNumBones(); i++) {
			INode *bone = skin->GetBone(i);
			int j;
			for(j = 0; j < (int)bones.size(); j++) if(bones[j] == bone) break;	// it`s very slowly, but works :)
			if(j != bones.size()) continue;
			bones.push_back(bone);
		}
	}
	
	// export bones
	fprintf(file,"bones %d {\n",bones.size());
	for(int i = 0; i < (int)bones.size(); i++) {
		fprintf(file,"\t\"%s\" ",bones[i]->GetName());
		INode *parent = bones[i]->GetParentNode();
		int j;
		for(j = 0; j < (int)bones.size(); j++) if(bones[j] == parent) break;
		if(j == bones.size()) fprintf(file,"-1\n");
		else fprintf(file,"%d\n",j);
	}
	fprintf(file,"}\n");
	
	// export surfaces
	for(int i = 0; i < (int)surfaces.size(); i++) {
		Object *obj = surfaces[i]->EvalWorldState(inf->GetTime()).obj;
		if(obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID,0)) == 0) continue;
		
		TriObject *tri = (TriObject*)obj->ConvertToType(inf->GetTime(),Class_ID(TRIOBJ_CLASS_ID,0));
		Mesh* mesh = &tri->GetMesh();
		
		ISkin *skin = (ISkin*)modifiers[i]->GetInterface(I_SKIN);
		ISkinContextData *skin_data = skin->GetContextInterface(surfaces[i]);
		
		mesh->buildNormals();
		Matrix3 transform = surfaces[i]->GetObjTMAfterWSM(inf->GetTime());
		Matrix3 rotate = transform;
		rotate.NoTrans();
		rotate.NoScale();
		
		// create geometry
		int num_vertex = mesh->numVerts;
		int num_triangles = mesh->numFaces;
		Vertex *vertex = new Vertex[num_triangles * 3];
		int *vertex_to_verts = new int[num_triangles * 3];
		Triangle *triangles = new Triangle[num_triangles];
		
		for(int j = 0; j < num_triangles * 3; j++) vertex_to_verts[j] = -1;

		// vertexes and normals
		for(int j = 0; j < num_vertex; j++) vertex[j].flag = 0;
		
		int start_num_vertex = num_vertex;

		for(int j = 0; j < num_triangles; j++) {
			Face *f = &mesh->faces[j];
			for(int k = 0; k < 3; k++) {
				Vertex v;
				// xyz
				v.xyz = transform * mesh->verts[f->v[k]];
				// normal
				RVertex *rv = mesh->getRVertPtr(f->v[k]);
				int num_normals;
				if(rv->rFlags & SPECIFIED_NORMAL) v.normal = rotate * rv->rn.getNormal();
				else if((num_normals = rv->rFlags & NORCT_MASK) && f->smGroup) {
					if(num_normals == 1) v.normal = rotate * rv->rn.getNormal();
					else for(int l = 0; l < num_normals; l++) {
						if(rv->ern[l].getSmGroup() & f->smGroup) v.normal = rotate * rv->ern[l].getNormal();
					}
				} else v.normal = rotate * mesh->getFaceNormal(j);
				// texture coords
				v.texcoord = Point3(0,0,0);
				v.num_weights = 0;
				v.flag = 1;
				// add vertex
				if(vertex[f->v[k]].flag == 0) {	// new vertex
					vertex[f->v[k]] = v;
					vertex_to_verts[f->v[k]] = f->v[k];
					triangles[j].v[k] = f->v[k];
				} else {
					if(vertex[f->v[k]].normal == v.normal) {	// same normals
						triangles[j].v[k] = f->v[k];
					} else {	// different normals
						int l;
						for(l = start_num_vertex; l < num_vertex; l++) {	// try to find same vertex
							if(vertex[l].xyz == v.xyz && vertex[l].normal == v.normal) break;
						}
						if(l == num_vertex) {	// add new vertex
							vertex[num_vertex] = v;
							vertex_to_verts[num_vertex] = f->v[k];
							triangles[j].v[k] = num_vertex++;
						} else {
							triangles[j].v[k] = l;
						}
					}
				}
			}
		}
		
		// texture coords
		if(mesh->numTVerts) {
			for(int j = 0; j < num_vertex; j++) vertex[j].flag = 0;
			
			start_num_vertex = num_vertex;

			for(int j = 0; j < num_triangles; j++) {
				TVFace *t = &mesh->tvFace[j];
				for(int k = 0; k < 3; k++) {
					Vertex *v = &vertex[triangles[j].v[k]];
					Point3 texcoord = mesh->tVerts[t->t[k]];
					texcoord.y = 1.0f - texcoord.y;
					if(v->flag == 0) {	// new texture coord
						v->texcoord = texcoord;
						v->flag = 1;
					} else {
						if(v->texcoord != texcoord) {	// different texture coords
							int l;
							for(l = start_num_vertex; l < num_vertex; l++) {	// try to find same vertex
								if(vertex[l].xyz == v->xyz && vertex[l].normal == v->normal && vertex[l].texcoord == texcoord) break;
							}
							if(l == num_vertex) {	// add new vertex
								vertex[num_vertex] = *v;
								vertex[num_vertex].texcoord = texcoord;
								vertex_to_verts[num_vertex] = vertex_to_verts[triangles[j].v[k]];
								triangles[j].v[k] = num_vertex++;
							} else {
								triangles[j].v[k] = l;
							}
						}
					}
				}
			}
		}
		
		if(mesh->numVerts > skin_data->GetNumPoints()) {
			char error[1024];
			sprintf(error,"bad surface \"%s\"\n\nnumVerts is %d\nskin num points is %d",
				surfaces[i]->GetName(),mesh->numVerts,skin_data->GetNumPoints());
			MessageBox(NULL,error,"Skinned Mesh Export error",MB_OK);
			continue;
		}
		
		// weights
		for(int j = 0; j < num_vertex; j++) {
			int v = vertex_to_verts[j];
			if(v == -1) {
				char error[1024];
				sprintf(error,"bad surface \"%s\"",surfaces[i]->GetName());
				MessageBox(NULL,error,"Skinned Mesh Export error",MB_OK);
				break;
			}
			vertex[j].num_weights = skin_data->GetNumAssignedBones(v);
			for(int k = 0; k < vertex[j].num_weights; k++) {
				INode *bone = skin->GetBone(skin_data->GetAssignedBone(v,k));
				// find number of the bone
				int l;
				for(l = 0; l < (int)bones.size(); l++) if(bones[l] == bone) break;
				vertex[j].weights[k].bone = l;
				// weights of the bone
				vertex[j].weights[k].weight = skin_data->GetBoneWeight(v,k);
				// xyz and normal
				Matrix3 transform = bone->GetObjTMAfterWSM(inf->GetTime());
				if(transform.Parity()) {
					transform.Invert();
					Matrix3 m;
					m = transform;
					transform.Zero();
					transform -= m;
				} else {
					transform.Invert();
				}
				Matrix3 rotate = transform;
				rotate.Orthogonalize();
				rotate.NoScale();
				rotate.NoTrans();
				vertex[j].weights[k].xyz = transform * vertex[j].xyz;
				vertex[j].weights[k].normal = rotate * vertex[j].normal;
			}
		}
		
		// mirrored traingles
		if(transform.Parity()) {
			for(int j = 0; j < num_triangles; j++) {
				int v;
				v = triangles[j].v[0];
				triangles[j].v[0] = triangles[j].v[2];
				triangles[j].v[2] = v;
			}
		}

		// new surface
		fprintf(file,"surface \"%s\" {\n\tvertex %d {\n",surfaces[i]->GetName(),num_vertex);
		
		for(int j = 0; j < num_vertex; j++) {
			fprintf(file,"\t\t%f %f\n\t\tweights %d {\n",vertex[j].texcoord.x,vertex[j].texcoord.y,vertex[j].num_weights);
			for(int k = 0; k < vertex[j].num_weights; k++) {
				Weight *w = &vertex[j].weights[k];
				fprintf(file,"\t\t\t%d %f %f %f %f %f %f %f\n",w->bone,w->weight,w->xyz.x,w->xyz.y,w->xyz.z,w->normal.x,w->normal.y,w->normal.z);
			}
			fprintf(file,"\t\t}\n");
		}

		fprintf(file,"\t}\n\ttriangles %d {\n",num_triangles);
		
		for(int j = 0; j < num_triangles; j++) {
			fprintf(file,"\t\t%d %d %d\n",triangles[j].v[0],triangles[j].v[1],triangles[j].v[2]);
		}
		
		fprintf(file,"\t}\n}\n");
		
		delete triangles;
		delete vertex_to_verts;
		delete vertex;
		
		if(obj != tri) delete tri;

		total_vertex += num_vertex;
		total_triangles += num_triangles;
	}

	// export animation
	Interval interval = inf->GetAnimRange();
	int start_time = interval.Start();
	int end_time = interval.End();
	int ticks_per_frame = GetTicksPerFrame();
	
	fprintf(file,"animation %d {\n",(end_time - start_time) / ticks_per_frame);
	for(int time = start_time; time < end_time; time += ticks_per_frame) {
		for(int i = 0; i < (int)bones.size(); i++) {
			INode *bone = bones[i];
			Matrix3 transform = bone->GetObjTMAfterWSM(time);
			Point3 xyz = Point3(0,0,0) * transform;
			transform.Orthogonalize();
			transform.NoScale();
			transform.NoTrans();
			transform.Invert();
			Quat rot(transform);
			/*
			Point3 axis;
			float ang;
			AngAxisFromQ(rot,&ang,axis);
			if(fabs(fabs(rot.x) - 1.0) < 1e-6) {
				axis.x = 0;
				axis.y = 1;
			}
			rot = rot * QFromAngAxis(PI,axis);
			*/
			fprintf(file,"\t%f %f %f %f %f %f %f\n",xyz.x,xyz.y,xyz.z,rot.x,rot.y,rot.z,rot.w);
		}
	}
	fprintf(file,"}\n");
}

/*****************************************************************************/
/*                                                                           */
/* Skinned Mesh Export                                                       */
/*                                                                           */
/*****************************************************************************/

class SkinnedMeshExport : public SceneExport {
public:
	SkinnedMeshExport() { }
	~SkinnedMeshExport() { }

	int ExtCount() { return 1; }
	const TCHAR *Ext(int i) { return (i == 0) ? "txt" : ""; }
	const TCHAR *LongDesc() { return "Skinned Mesh Export plugin http://frustum.org"; }
	const TCHAR *ShortDesc() { return "Skinned Mesh Export"; }
	const TCHAR *AuthorName() { return "Alexander Zaprjagaev"; }
	const TCHAR *CopyrightMessage()	{ return ""; }
	const TCHAR *OtherMessage1() { return ""; }
	const TCHAR *OtherMessage2() { return ""; }
	unsigned int Version() { return 100; }
	void ShowAbout(HWND hWnd) { MessageBox(hWnd,"Skinned Mesh Export plugin\nhttp://frustum.org","about",MB_OK); }
	BOOL SupportsOptions(int ext,DWORD options) { return (options == SCENE_EXPORT_SELECTED) ? TRUE : FALSE; }
	
	int DoExport(const TCHAR *name,ExpInterface *ei,Interface *i,BOOL suppressPrompts = FALSE,DWORD options = 0);
};

int SkinnedMeshExport::DoExport(const TCHAR *name,ExpInterface *ei,Interface *i,BOOL suppressPrompts,DWORD options) {
	
	SkinnedMeshEnumProc smesh(name,ei->theScene,i,options);

	return 1;
}

/*****************************************************************************/
/*                                                                           */
/* DllMain                                                                   */
/*                                                                           */
/*****************************************************************************/

HINSTANCE hInstance;
int controlsInit = FALSE;

BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID lpvReserved) {
	hInstance = hinstDLL;
	if(!controlsInit) {
		controlsInit = TRUE;
		InitCustomControls(hInstance);
		InitCommonControls();
	}
	return TRUE;
}

class SkinnedMeshClassDesc : public ClassDesc {
public:
	int IsPublic() { return 1; }
	void *Create(BOOL loading = FALSE) { return new SkinnedMeshExport; }
	const TCHAR *ClassName() { return "Skinned Mesh Export"; }
	SClass_ID SuperClassID() { return SCENE_EXPORT_CLASS_ID; }
	Class_ID ClassID() { return Class_ID(0xdeadbeef,0x7b5a93eb); }
	const TCHAR *Category() { return ""; }
};

static SkinnedMeshClassDesc SkinnedMeshDesc;

__declspec(dllexport) const TCHAR *LibDescription() {
	return "Skinned Mesh Export Plugin";
}

__declspec(dllexport) int LibNumberClasses() {
	return 1;
}

__declspec(dllexport) ClassDesc *LibClassDesc(int i) {
	return (i == 0) ? &SkinnedMeshDesc : 0;
}

__declspec(dllexport) ULONG LibVersion() {
	return VERSION_3DSMAX;
}

__declspec(dllexport )ULONG CanAutoDefer() {
	return 1;
}
