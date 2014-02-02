/* Map
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

#ifndef __MAP__
#define __MAP__

#include "mathlib.h"

class Position;

class Map {
public:
	
	static void load(const char *name);
	
protected:
	
	static char *data;	
	
	static const char *error(const char *error,...);
	
	static const char *read_token(char *must = NULL);
	static int read_bool();
	static int read_int();
	static float read_float();
	static vec3 read_vec3();
	static vec4 read_vec4();
	static const char *read_string();
	
	static void load_bsp();
	static void load_pos(Position &pos,mat4 &matrix);
	static void load_light();
	static void load_fog();
	static void load_mirror();
	static void load_mesh();
	static void load_skinnedmesh();
	static void load_particles();
};

#endif /* __MAP__ */
