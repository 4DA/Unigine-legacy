/* Cube Map Normalize generator
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
#include "mathlib.h"
#include "texture.h"

/*
 */
vec3 getCubeVector(int i,int size,int x,int y) {
	float s = ((float)x + 0.5) / (float)size * 2.0 - 1.0;
	float t = ((float)y + 0.5) / (float)size * 2.0 - 1.0;
	vec3 v;
	switch(i) {
		case 0: v = vec3(1.0,-t,-s); break;
		case 1: v = vec3(-1.0,-t,s); break;
		case 2: v = vec3(s,1.0,t); break;
		case 3: v = vec3(s,-1.0,-t); break;
		case 4: v = vec3(s,-t,1.0); break;
		case 5: v = vec3(-s,-t,-1.0); break;
	}
	v.normalize();
	return v;
}

int main(int argc,char **argv) {
	
	char *names[] = {
		"data/normalize_px.tga",
		"data/normalize_nx.tga",
		"data/normalize_py.tga",
		"data/normalize_ny.tga",
		"data/normalize_pz.tga",
		"data/normalize_nz.tga",
	};
	
	int size = 128;
	
	unsigned char *data = new unsigned char[size * size * 4];
	for(int i = 0; i < 6; i++) {
		unsigned char *d = data;
		for(int y = 0; y < size; y++) {
			for(int x = 0; x < size; x++) {
				vec3 dir = getCubeVector(i,size,x,y);
				dir = (dir + vec3(1,1,1)) * 0.5;
				*d++ = (unsigned char)(dir.x * 255.0);
				*d++ = (unsigned char)(dir.y * 255.0);
				*d++ = (unsigned char)(dir.z * 255.0);
				*d++ = 255;
			}
		}
		Texture::save_tga(names[i],data,size,size);
	}
	
	delete data;
}
