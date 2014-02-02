/* Horizon Texture generator
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
#include <string.h>
#include <stdlib.h>
#include "mathlib.h"
#include "texture.h"

/*
 */
class Horizon {
public:
	Horizon(const char *name,float size,float elevation) : hmap(NULL) {
		unsigned char *h = Texture::load(name,width,height);
		if(!h) return;
		hmap = new float[width * height];
		float *dest = hmap;
		unsigned char *src = h;
		for(int i = 0; i < width * height; i++) {
			*dest++ = (float)*src / 255.0 * elevation / size * (float)width;
			src += 4;
		}
		delete h;
	}
	~Horizon() {
		if(hmap) delete hmap;
	}
	
	// height
	float height_wrap(int x,int y) {
		while(x < 0) x += width;
		while(y < 0) y += height;
		if(x >= width) x = x % width;
		if(y >= height) y = y % height;
		return hmap[y * width + x];
	}
	
	// create one layer of 3d texture
	void create_layer(unsigned char *dest,const vec2 &dir) {
		vec2 d = dir;
		float step = fabs(d.x) > fabs(d.y) ? fabs(d.x) : fabs(d.y);
		d /= step;
		int length = (int)sqrt((float)width * d.x * (float)width * d.x + (float)height * d.y * (float)height * d.y);
		for(int y = 0; y < height; y++) {
			for(int x = 0; x < width; x++) {
				vec2 v(x,y);
				*dest = 0;
				float height_start = height_wrap(x,y);
				float len = 0;
				float dlen =  d.length();
				float angle = 0;
				for(int i = 0; i < length; i++) {
					v += d;
					len += dlen;
					int nx = (int)(v.x + 0.5);
					int ny = (int)(v.y + 0.5);
					float a = (height_wrap(nx,ny) - height_start) / len;
					if(a > angle) angle = a;
				}
				*dest++ = (unsigned char)((1.0 - cos(atan(angle))) * 255.0);
			}
		}
	}
	
	// create horizon texture
	void create(const char *name,int depth) {
		if(!hmap) return;
		unsigned char *data = new unsigned char[width * height * depth];
		for(int i = 0; i < depth; i++) {
			vec2 dir;
			dir.x = sin((float)i / (float)depth * 2 * PI);
			dir.y = cos((float)i / (float)depth * 2 * PI);
			create_layer(data + width * height * i,dir);
			fprintf(stdout,"%.1f prescent complete\r",(float)(i + 1) / (float)depth * 100.0);
			fflush(stdout);
		}
		printf("\n");
		Texture::save_3d(name,data,width,height,depth,Texture::LUMINANCE);
		delete data;
	}
	
	int width,height;
	float *hmap;
};

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
	
	if(argc > 4) {
		char name[1024];
		strcpy(name,argv[1]);
		char *s = strrchr(name,'.');
		if(s) strcpy(s,".3d");
		else strcat(name,".3d");
		
		Horizon *horizon = new Horizon(argv[1],atof(argv[2]),atof(argv[3]));
		horizon->create(name,atoi(argv[4]));
		delete horizon;

		printf("%s -> %s\n",argv[1],name);
	} else {
		printf("horizon texture generator\n");
		printf("usage: %s <heightmap> <size> <elevation> <depth>\n",argv[0]);
		printf("written by Alexander Zaprjagaev\n");
		printf("frustum@frustum.org\n");
		printf("http://frustum.org\n");
	}
	
	// create lookup cubemap texture
	/*char *names[] = {
		"data/horizon_px.tga",
		"data/horizon_nx.tga",
		"data/horizon_py.tga",
		"data/horizon_ny.tga",
		"data/horizon_pz.tga",
		"data/horizon_nz.tga",
	};
	
	int size = 128;
	
	unsigned char *data = new unsigned char[size * size * 4];
	for(int i = 0; i < 6; i++) {
		unsigned char *d = data;
		for(int y = 0; y < size; y++) {
			for(int x = 0; x < size; x++) {
				vec3 dir = getCubeVector(i,size,x,y);
				vec3 vec(dir.x,dir.y,0);
				vec.normalize();
				float h = acos(vec3(0,1,0) * vec) / (2.0 * PI);
				if(vec.x < 0) h = 1.0 - h;
				*d++ = (unsigned char)(h * 255.0);
				*d++ = (unsigned char)((1.0 - vec * dir) * 255.0);
				*d++ = 0;
				*d++ = 255;
			}
		}
		Texture::save_tga(names[i],data,size,size);
	}
	delete data;*/

	return 0;
}
