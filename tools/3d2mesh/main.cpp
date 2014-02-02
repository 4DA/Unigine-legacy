/* 3d2mesh utile
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
int main(int argc,char **argv) {
	
	if(argc < 2) {
		printf("3d (.3ds or raw .mesh) to strip .mesh utile\n");
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
		if(s) strcpy(s,".mesh");
		else strcat(name,".mesh");
		Mesh *mesh = new Mesh(argv[i]);
		if(mesh->getNumSurfaces() != 0) {
			printf("%s -> %s\n",argv[i],name);
			mesh->save(name);
		}
		delete mesh;
	}
	
	return 0;
}
