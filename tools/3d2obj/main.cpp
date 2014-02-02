/* 3d2obj utile
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

#include <stdio.h>
#include "mesh.h"

/*
 */
void exportObj(const char *name,Mesh *mesh) {
	FILE *file = fopen(name,"wb");
	if(!file) {
		fprintf(stderr,"exportObj(): error create \"%s\" file\n",name);
		return;
	}
	
	fprintf(file,"#####                           #####\n");
	fprintf(file,"#                                   #\n");
	fprintf(file,"# file was produced by 3d2obj utile #\n");
	fprintf(file,"# http://frustum.org                #\n");
	fprintf(file,"#                                   #\n");
	fprintf(file,"#####                           #####\n");
	
	int num_vertex = 0;
	
	for(int i = 0; i < mesh->getNumSurfaces(); i++) {
		Mesh::Vertex *vertex = mesh->getVertex(i);
		int *indices = mesh->getIndices(i);
		
		fprintf(file,"\ng %s\n",*mesh->getSurfaceName(i) == '\0' ? name : mesh->getSurfaceName(i));
		
		for(int j = 0; j < mesh->getNumVertex(i); j++) {
			fprintf(file,"v %f %f %f\n",vertex[j].xyz.x,vertex[j].xyz.z,vertex[j].xyz.y);
		}
		for(int j = 0; j < mesh->getNumVertex(i); j++) {
			fprintf(file,"vn %f %f %f\n",vertex[j].normal.x,vertex[j].normal.z,vertex[j].normal.y);
		}
		for(int j = 0; j < mesh->getNumVertex(i); j++) {
			fprintf(file,"vt %f %f\n",vertex[j].texcoord.x,1.0f - vertex[j].texcoord.y);
		}
		
		for(int j = 0; j < mesh->getNumStrips(i); j++) {
			for(int k = 2; k < indices[0]; k++) {
				int v0 = indices[k - 2 + 1] + 1 + num_vertex;
				int v1 = indices[k - 1 + 1] + 1 + num_vertex;
				int v2 = indices[k - 0 + 1] + 1 + num_vertex;
				if(k % 2 == 1) fprintf(file,"f %d/%d/%d %d/%d/%d %d/%d/%d\n",v0,v0,v0,v1,v1,v1,v2,v2,v2);
				else fprintf(file,"f %d/%d/%d %d/%d/%d %d/%d/%d\n",v1,v1,v1,v0,v0,v0,v2,v2,v2);
			}
			indices += indices[0] + 1;
		}

		num_vertex += mesh->getNumVertex(i);
	}
	
	fclose(file);
}

/*
 */
int main(int argc,char **argv) {
	
	if(argc < 2) {
		printf("3d (.3ds or .mesh) to Maya OBJ format\n");
		printf("usage: %s <3d file> ...\n",argv[0]);
		printf("written by Alexander Zaprjagaev\n");
		printf("frustum@frustum.org\n");
		printf("http://frustum.org\n");
		return 0;
	}
	
	for(int i = 1; i < argc; i++) {
		char name[1024];
		strcpy(name,argv[i]);
		char *s = strrchr(name,'.');
		if(s) strcpy(s,".obj");
		else strcat(name,".obj");
		Mesh *mesh = new Mesh(argv[i]);
		if(mesh->getNumSurfaces() != 0) {
			printf("%s -> %s\n",argv[i],name);
			exportObj(name,mesh);
		}
		delete mesh;
	}
	
	return 0;
}
