/* Position
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

#ifndef __POSITION_H__
#define __POSITION_H__

#include "mathlib.h"

class Object;
class Spline;
class Expression;

class Position : public vec3 {
public:
	
	Position();
	~Position();
	
	Position &operator=(const Position &pos);
	Position &operator=(const vec3 &pos);
	void find(int sector,float r);
	
	void setSpline(Spline *spline);
	void setExpression(Expression *expression);
	
	void setRadius(float radius);
	
	void update(float time,mat4 &transform);
	void update(float time,Object *object = NULL);
	
	mat4 to_matrix(float time);
	
	Spline *spline;
	Expression *expression;
	
	enum {
		NUM_SECTORS = 32,
	};
	
	int sector;
	
	float radius;
	int num_sectors;
	int *sectors;
};

/*
 */
class Spline {
public:
	Spline(const char *name,float speed,int close,int follow);
	Spline(const Spline &spline);
	~Spline();
	
	mat4 to_matrix(float time);
	
protected:
	int num;
	vec3 *params;
	float speed;
	float length;
	int follow;
};

/*
 */
class Expression {
public:
	Expression(const char *str);
	Expression(const Expression &expression);
	~Expression();
	
	mat4 to_matrix(float time);
	
protected:
	
	char *exp[14];
};

#endif /* __POSITION_H__ */
