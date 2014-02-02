/* ObjectParticles
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

#ifndef __OBJECT_PARTICLES_H__
#define __OBJECT_PARTICLES_H__

#include "object.h"

class Particles;

class ObjectParticles : public Object {
public:

	ObjectParticles(Particles *particles);
	virtual ~ObjectParticles();
	
	virtual void update(float ifps);
	
	void setOffTime(float time);
	
	virtual int render(int t = RENDER_ALL,int s = -1);
	
	virtual void findSilhouette(const vec4 &light,int s = -1);
	virtual int getNumIntersections(const vec3 &line0,const vec3 &line1,int s = -1);
	virtual int renderShadowVolume(int s = -1);
	
	virtual int intersection(const vec3 &line0,const vec3 &line1,vec3 &point,vec3 &normal,int s = -1);
	
	virtual int getNumSurfaces();
	virtual const char *getSurfaceName(int s);
	virtual int getSurface(const char *name);
	
	virtual const vec3 &getMin(int s = -1);
	virtual const vec3 &getMax(int s = -1);
	virtual const vec3 &getCenter(int s = -1);
	virtual float getRadius(int s = -1);
	
	vec3 min;
	vec3 max;
	vec3 center;
	
	Particles *particles;
	
	float off_time;
};

#endif /* __OBJECT_PARTICLES_H__ */
