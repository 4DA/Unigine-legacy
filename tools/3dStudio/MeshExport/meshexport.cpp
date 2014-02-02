/* Mesh Export plugin for 3DStudio
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

#include "max.h"

#pragma comment(lib, "core.lib")
#pragma comment(lib, "maxutil.lib")
#pragma comment(lib, "geom.lib")
#pragma comment(lib, "mesh.lib")

/*****************************************************************************/
/*                                                                           */
/* Mesh Enum Proc                                                            */
/*                                                                           */
/*****************************************************************************/

#define MESH_RAW_MAGIC ('m' | ('r' << 8) | ('0' << 16) | ('2' << 24))

struct Vertex {
	float xyz[3];
	float normal[3];
	float texcoord[2];
};

class MeshEnumProc : public ITreeEnumProc {
public:
	
	MeshEnumProc(const char *name,IScene *scene,Interface *i,DWORD options);
	~MeshEnumProc();
	
	int callback(INode *node);
	void export(INode *node,TriObject *tri);
	
	FILE *file;
	IScene *scene;
	Interface *i;
	int selected;

	int num_surfaces;
};

MeshEnumProc::MeshEnumProc(const char *name,IScene *scene,Interface *i,DWORD options) {
	file = fopen(name,"wb");
	if(!file) {
		char error[1024];
		sprintf(error,"Mesh Export:\nerror create \"%s\" file",name);
		MessageBox(0,error,"error",MB_OK);
		return;
	}
	this->scene = scene;
	this->i = i;
	selected = (options & SCENE_EXPORT_SELECTED) ? TRUE : FALSE;

	num_surfaces = 0;
	
	int magic = MESH_RAW_MAGIC;
	fwrite(&magic,sizeof(int),1,file);
	fwrite(&num_surfaces,sizeof(int),1,file);
	
	scene->EnumTree(this);
}

MeshEnumProc::~MeshEnumProc() {
	// info
	char buffer[1024];
	sprintf(buffer,"OK\nsurfaces %d",num_surfaces);
	MessageBox(0,buffer,"Mesh Export",MB_OK);
	// write number of surfaces
	fseek(file,sizeof(int),SEEK_SET);
	fwrite(&num_surfaces,sizeof(int),1,file);
	fclose(file);
}

/*
 */
int MeshEnumProc::callback(INode *node) {
	if(selected && node->Selected() == FALSE) return TREE_CONTINUE;
	Object *obj = node->EvalWorldState(i->GetTime()).obj;
	if(obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID,0))) { 
		TriObject *tri = (TriObject*)obj->ConvertToType(i->GetTime(),Class_ID(TRIOBJ_CLASS_ID,0));
		export(node,tri);
		if(obj != tri) delete tri;
	}
	return TREE_CONTINUE;
}

/*
 */
void MeshEnumProc::export(INode *node,TriObject *tri) {
	
	Mesh* mesh = &tri->GetMesh();
	num_surfaces++;
	
	// name
	char name[128];
	memset(name,0,sizeof(name));
	strcpy(name,node->GetName());
	fwrite(name,sizeof(name),1,file);
	
	// number of vertexes
	int num_vertex = mesh->numFaces * 3;
	fwrite(&num_vertex,sizeof(int),1,file);
	
	// vertexes
	mesh->buildNormals();
	Matrix3 transform = node->GetObjTMAfterWSM(i->GetTime());
	Matrix3 rotate = transform;
	rotate.NoTrans();
	rotate.NoScale();
	
	// negative scale
	int index[3];
	if(DotProd(CrossProd(transform.GetRow(0),transform.GetRow(1)),transform.GetRow(2)) < 0.0) {
		index[0] = 2; index[1] = 1; index[2] = 0;
	} else {
		index[0] = 0; index[1] = 1; index[2] = 2;
	}
	
	Vertex *vertex = new Vertex[mesh->numFaces * 3];
	for(int i = 0, j = 0; i < mesh->numFaces; i++, j += 3) {
		Point3 v;
		Face *f = &mesh->faces[i];
		for(int k = 0; k < 3; k++) {
			v = transform * mesh->verts[f->v[index[k]]];
			vertex[j + k].xyz[0] = v.x;
			vertex[j + k].xyz[1] = v.y;
			vertex[j + k].xyz[2] = v.z;
			// get normal (see asciiexp plugin)
			RVertex *rv = mesh->getRVertPtr(f->v[index[k]]);
			int num_normals;
			if(rv->rFlags & SPECIFIED_NORMAL) v = rv->rn.getNormal();
			else if((num_normals = rv->rFlags & NORCT_MASK) && f->smGroup) {
				if(num_normals == 1) v = rv->rn.getNormal();
				else for(int l = 0; l < num_normals; l++) {
					if(rv->ern[l].getSmGroup() & f->smGroup) v = rv->ern[l].getNormal();
				}
			} else v = mesh->getFaceNormal(i);
			v = rotate * v;
			vertex[j + k].normal[0] = v.x;
			vertex[j + k].normal[1] = v.y;
			vertex[j + k].normal[2] = v.z;
			// texture coords
			if(mesh->numTVerts) {
				v = mesh->tVerts[mesh->tvFace[i].t[index[k]]];
				vertex[j + k].texcoord[0] = v.x;
				vertex[j + k].texcoord[1] = 1.0f - v.y;
			} else {
				vertex[j + k].texcoord[0] = 0;
				vertex[j + k].texcoord[1] = 0;
			}
		}
	}
	fwrite(vertex,sizeof(Vertex),mesh->numFaces * 3,file);
	delete vertex;
}

/*****************************************************************************/
/*                                                                           */
/* Mesh Export                                                               */
/*                                                                           */
/*****************************************************************************/

class MeshExport : public SceneExport {
public:
	MeshExport() { }
	~MeshExport() { }

	int ExtCount() { return 1; }
	const TCHAR *Ext(int i) { return (i == 0) ? "mesh" : ""; }
	const TCHAR *LongDesc() { return "Mesh Export plugin http://frustum.org"; }
	const TCHAR *ShortDesc() { return "Mesh Export"; }
	const TCHAR *AuthorName() { return "Alexander Zaprjagaev"; }
	const TCHAR *CopyrightMessage()	{ return ""; }
	const TCHAR *OtherMessage1() { return ""; }
	const TCHAR *OtherMessage2() { return ""; }
	unsigned int Version() { return 100; }
	void ShowAbout(HWND hWnd) { MessageBox(hWnd,"Mesh Export plugin\nhttp://frustum.org","about",MB_OK); }
	BOOL SupportsOptions(int ext,DWORD options) { return (options == SCENE_EXPORT_SELECTED) ? TRUE : FALSE; }
	
	int DoExport(const TCHAR *name,ExpInterface *ei,Interface *i,BOOL suppressPrompts = FALSE,DWORD options = 0);
};

int MeshExport::DoExport(const TCHAR *name,ExpInterface *ei,Interface *i,BOOL suppressPrompts,DWORD options) {
	
	MeshEnumProc mesh(name,ei->theScene,i,options);

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

class MeshClassDesc : public ClassDesc {
public:
	int IsPublic() { return 1; }
	void *Create(BOOL loading = FALSE) { return new MeshExport; }
	const TCHAR *ClassName() { return "Mesh Export"; }
	SClass_ID SuperClassID() { return SCENE_EXPORT_CLASS_ID; }
	Class_ID ClassID() { return Class_ID(0xdeadbeef,0x4ba8d931); }
	const TCHAR *Category() { return ""; }
};

static MeshClassDesc MeshDesc;

__declspec(dllexport) const TCHAR *LibDescription() {
	return "Mesh Export Plugin";
}

__declspec(dllexport) int LibNumberClasses() {
	return 1;
}

__declspec(dllexport) ClassDesc *LibClassDesc(int i) {
	return (i == 0) ? &MeshDesc : 0;
}

__declspec(dllexport) ULONG LibVersion() {
	return VERSION_3DSMAX;
}

__declspec(dllexport )ULONG CanAutoDefer() {
	return 1;
}
