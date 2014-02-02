/* Mesh Export plugin for Maya
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

#include <maya/MFnPlugin.h>
#include <maya/MObject.h>
#include <maya/MFnMesh.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MItMeshVertex.h>
#include <maya/MPxFileTranslator.h>
#include <maya/MFnDagNode.h>
#include <maya/MDagPath.h>
#include <maya/MItDag.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MItSelectionList.h>

#define MESH_RAW_MAGIC ('m' | ('r' << 8) | ('0' << 16) | ('2' << 24))

struct Vertex {
	float xyz[3];
	float normal[3];
	float texcoord[2];
};

/*****************************************************************************/
/*                                                                           */
/* MeshTranslator                                                            */
/*                                                                           */
/*****************************************************************************/

class MeshTranslator : public MPxFileTranslator {
public:
	MeshTranslator() { };
	virtual ~MeshTranslator() { };
	
	static void *creator() { return new MeshTranslator(); }
	
	MStatus reader(const MFileObject &name,const MString &optionsString,FileAccessMode mode) { return MS::kFailure; }
	MStatus writer(const MFileObject &name,const MString &optionsString,FileAccessMode mode);
	
	bool haveReadMethod() const { return false; }
	bool haveWriteMethod() const { return true; }
	MString defaultExtension() const { return "mesh"; }
	
	MFileKind identifyFile(const MFileObject& fileName,const char* buffer,short size) const;

private:
	
	void exportMesh(MDagPath &path);
	
	FILE *file;
	int num_surfaces;
};

/*****************************************************************************/
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

/*
 */
void MeshTranslator::exportMesh(MDagPath &path) {
	
	MFnMesh mesh(path);
	MItMeshPolygon polyIt(path);
	
	// name
	char name[128];
	memset(name,0,sizeof(name));
	strcpy(name,mesh.name().asChar());
	int l = strlen(name);
	if(l > 5 && !strcmp(name + l - 5,"Shape")) memset(name + l - 5,0,5);
	fwrite(name,sizeof(name),1,file);
	
	// number of vertexes
	int num_vertex = 0;
	for(; !polyIt.isDone(); polyIt.next()) {
		for(int i = 0; i < polyIt.polygonVertexCount(); i++) {
			if(i > 2) num_vertex += 2;
			num_vertex++;
		}
	}
	polyIt.reset();
	
	Vertex *vertex = new Vertex[num_vertex];
	
#define EXPORT(i) { \
	MPoint v; \
	v = polyIt.point(i,MSpace::kWorld); \
	vertex[num_vertex].xyz[0] = v.x; \
	vertex[num_vertex].xyz[1] = v.z; \
	vertex[num_vertex].xyz[2] = v.y; \
	MVector n; \
	polyIt.getNormal(i,n,MSpace::kWorld); \
	vertex[num_vertex].normal[0] = n.x; \
	vertex[num_vertex].normal[1] = n.z; \
	vertex[num_vertex].normal[2] = n.y; \
	float2 texcoord; \
	polyIt.getUV(i,texcoord); \
	vertex[num_vertex].texcoord[0] = texcoord[0]; \
	vertex[num_vertex].texcoord[1] = 1.0f - texcoord[1]; \
	num_vertex++; \
}
	
	// export
	num_vertex = 0;
	for(; !polyIt.isDone(); polyIt.next()) {
		for(int i = 0; i < polyIt.polygonVertexCount(); i++) {
			if(i > 2) {
				EXPORT(0)
				EXPORT(i - 1)
			}
			EXPORT(i)
		}
	}
	
#undef EXPORT
	
	// flip triangles
	for(int i = 0; i < num_vertex; i += 3)	 {
		Vertex v = vertex[i];
		vertex[i] = vertex[i + 1];
		vertex[i + 1] = v;
	}
	
	fwrite(&num_vertex,sizeof(int),1,file);
	fwrite(vertex,sizeof(Vertex),num_vertex,file);
	delete vertex;
	
	num_surfaces++;
}

/*
 */
MStatus MeshTranslator::writer(const MFileObject &name,const MString &optionsString,FileAccessMode mode) {
	
	file = fopen(name.fullName().asChar(),"wb");
	if(!file) {
		fprintf(stderr,"MeshTranslator::writer(): can`t create \"%s\" file\n",name.fullName().asChar());
		return MS::kFailure;
	}
	
	int magic = MESH_RAW_MAGIC;
	fwrite(&magic,sizeof(int),1,file);
	
	num_surfaces = 0;
	fwrite(&num_surfaces,sizeof(int),1,file);
	
	if(mode == MPxFileTranslator::kExportAccessMode || mode == MPxFileTranslator::kSaveAccessMode) {
		MItDag it(MItDag::kBreadthFirst,MFn::kInvalid);
		for(; !it.isDone(); it.next()) {
			MDagPath path;
			it.getPath(path);
			MFnDagNode node(path);
			if(node.isIntermediateObject()) continue;
			if(path.hasFn(MFn::kNurbsSurface)) {
				fprintf(stderr,"MeshTranslator::writer(): skipping nurbs surface\n");
				continue;
			}
			if(path.hasFn(MFn::kMesh) && path.hasFn(MFn::kTransform)) continue;
			if(path.hasFn(MFn::kMesh)) exportMesh(path);
		}
	} else if(mode == MPxFileTranslator::kExportActiveAccessMode) {
		MSelectionList slist;
		MGlobal::getActiveSelectionList(slist);
		MItSelectionList selection(slist);
		if(selection.isDone()) {
			fprintf(stderr,"MeshTranslator::writer(): nothing is selected\n");
			return MS::kFailure;
		}
		MItDag it(MItDag::kDepthFirst,MFn::kInvalid);
		for(; !selection.isDone(); selection.next()) {
			MDagPath path;
			selection.getDagPath(path);
			it.reset(path.node(),MItDag::kDepthFirst,MFn::kInvalid);
			for(; !it.isDone(); it.next()) {
				MDagPath path;
				it.getPath(path);
				MFnDagNode node(path);
				if(node.isIntermediateObject()) continue;
				if(path.hasFn(MFn::kNurbsSurface)) {
					fprintf(stderr,"MeshTranslator::writer(): skipping nurbs surface\n");
					continue;
				}
				if(path.hasFn(MFn::kMesh) && path.hasFn(MFn::kTransform)) continue;
				if(path.hasFn(MFn::kMesh)) exportMesh(path);
			}
		}
	}
	
	fseek(file,sizeof(int),SEEK_SET);
	fwrite(&num_surfaces,sizeof(int),1,file);
	
	fclose(file);
	
	return MS::kSuccess;
}

/*
 */
MPxFileTranslator::MFileKind MeshTranslator::identifyFile(const MFileObject& fileName,const char* buffer,short size) const {
	const char *name = fileName.name().asChar();
	int length = strlen(name);
	if(length > 5 && !strcasecmp(name + length - 5,".mesh")) return kCouldBeMyFileType;
	return kNotMyFileType;
}

/*****************************************************************************/
/*                                                                           */
/* register/deregister                                                       */
/*                                                                           */
/*****************************************************************************/

MStatus initializePlugin(MObject obj) {
	MFnPlugin plugin(obj,"http://frustum.org/","1.0","Any");
	return plugin.registerFileTranslator("MeshExport","none",MeshTranslator::creator);
}

MStatus uninitializePlugin(MObject obj) {
	MFnPlugin plugin(obj);
	return plugin.deregisterFileTranslator("MeshExport");
}
