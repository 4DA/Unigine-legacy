/* Light Flares
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

#ifndef __FLARE_H__
#define __FLARE_H__

#define FLARE_TEX	"flare.png"

class Texture;

class Flare {
public:
	Flare(float min_radius,float max_radius,float sphere_radius);
	~Flare();
	
	void render(const vec3 &pos,const vec4 &color);
	
protected:
	float min_radius;
	float max_radius;
	float sphere_radius;
	
	float time;
	
	static int counter;
	
	static Texture *flare_tex;
};

#endif /* __FLARE_H__ */
