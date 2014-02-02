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

#include "engine.h"
#include "object.h"
#include "rigidbody.h"
#include "parser.h"
#include "position.h"

Position::Position() : spline(NULL), expression(NULL), sector(-1), radius(0.0), num_sectors(0) {
	sectors = new int[NUM_SECTORS];
}

Position::~Position() {
	if(spline) delete spline;
	if(expression) delete expression;
	delete sectors;
}

/*
 */
Position &Position::operator=(const Position &pos) {
	x = pos.x;
	y = pos.y;
	z = pos.z;
	spline = pos.spline ? new Spline(*pos.spline) : NULL;
	expression = pos.expression ? new Expression(*pos.expression) : NULL;
	sector = pos.sector;
	radius = pos.radius;
	num_sectors = pos.num_sectors;
	for(int i = 0; i < num_sectors; i++) sectors[i] = pos.sectors[i];
	return *this;
}

Position &Position::operator=(const vec3 &pos) {
	if(Bsp::num_sectors == 0) return *this;
	x = pos.x;
	y = pos.y;
	z = pos.z;
	num_sectors = 0;
	if(sector == -1) {	// find in all sectors
		for(int i = 0; i < Bsp::num_sectors; i++) {
			if(Bsp::sectors[i].inside(vec3(*this))) {
				sector = i;
				find(sector,radius);
				return *this;
			}
		}
	} else {
		if(Bsp::sectors[sector].inside(*this) == 0) {
			Sector *s = &Bsp::sectors[sector];
			for(int i = 0; i < s->num_portals; i++) {	// find in neighborning sectors
				Portal *p = &Bsp::portals[s->portals[i]];
				for(int j = 0; j < p->num_sectors; j++) {
					if(p->sectors[j] == sector) continue;
					if(Bsp::sectors[p->sectors[j]].inside(*this)) {
						sector = p->sectors[j];
						find(sector,radius);
						return *this;
					}
				}
			}
			for(int i = 0; i < Bsp::num_sectors; i++) {	// find in all sectors
				if(Bsp::sectors[i].inside(*this)) {
					sector = i;
					find(sector,radius);
					return *this;
				}
			}
			sector = -1;
		}
	}
	if(sector != -1) find(sector,radius);
	return *this;
}

void Position::find(int sector,float r) {
	if(num_sectors == NUM_SECTORS) {
		fprintf(stderr,"Position::find(): this object presents in %d sectors\n",num_sectors);
		return;
	}
	sectors[num_sectors++] = sector;
	if(radius < 0.0) return;
	Sector *s = &Bsp::sectors[sector];
	for(int i = 0; i < s->num_portals; i++) {
		Portal *p = &Bsp::portals[s->portals[i]];
		if((*this - p->center).length() > r + p->radius) continue;
		for(int j = 0; j < p->num_sectors; j++) {
			int k = 0;
			for(; k < num_sectors; k++) if(sectors[k] == p->sectors[j]) break;
			if(k != num_sectors) continue;
			if(Bsp::sectors[p->sectors[j]].inside(*this,radius)) find(p->sectors[j],r - (*this - p->center).length());
		}
	}
}

/*
 */
void Position::setSpline(Spline *spline) {
	this->spline = spline;
	expression = NULL;
}

void Position::setExpression(Expression *expression) {
	spline = NULL;
	this->expression = expression;
}

/*
 */
void Position::setRadius(float radius) {
	this->radius = radius;
}

/*
 */
void Position::update(float time,mat4 &transform) {
	if(spline) {
		transform = spline->to_matrix(time);
		operator=(transform * vec3(0,0,0));
	} else if(expression) {
		transform = expression->to_matrix(time);
		operator=(transform * vec3(0,0,0));
	}
}

/*
 */
void Position::update(float time,Object *object) {
	if(spline || expression) {
		if(object) {
			object->is_identity = 0;
			for(int i = 0; i < num_sectors; i++) Bsp::sectors[sectors[i]].removeObject(object);
			radius = object->getRadius();
		}
		mat4 transform;
		if(spline) transform = spline->to_matrix(time);
		else transform = expression->to_matrix(time);
		operator=(transform * vec3(0,0,0));
		if(object) {
			object->transform = transform;
			object->itransform = transform.inverse();
			for(int i = 0; i < num_sectors; i++) Bsp::sectors[sectors[i]].addObject(object);
		}
	}
}

/*
 */
mat4 Position::to_matrix(float time) {
	if(spline) return spline->to_matrix(time);
	if(expression) return expression->to_matrix(time);
	mat4 transform;
	transform.translate(*this);
	return transform;
}

/*****************************************************************************/
/*                                                                           */
/* Spline                                                                    */
/*                                                                           */
/*****************************************************************************/

Spline::Spline(const char *name,float speed,int close,int follow) : num(0), params(NULL), speed(speed), length(0.0), follow(follow) {
	
	FILE *file = fopen(name,"r");
	if(!file) {
		fprintf(stderr,"Spline::Spline(): error open \"%s\" file\n",name);
		return;
	}
	
	vec3 v;
	while(fscanf(file,"%f %f %f",&v.x,&v.y,&v.z) == 3) num++;
	vec3 *val = new vec3[num];
	
	num = 0;
	fseek(file,0,SEEK_SET);
	while(fscanf(file,"%f %f %f",&v.x,&v.y,&v.z) == 3) val[num++] = v;
	fclose(file);
	
	float tension = 0.0;
	float bias = 0.0;
	float continuity = 0.0;
	
	params = new vec3[num * 4];
	for(int i = 0; i < num; i++) {
		vec3 prev,cur,next;
		if(i == 0) {
			if(close) prev = val[num - 1];
			else prev = val[i];
			cur = val[i];
			next = val[i + 1];
		} else if(i == num - 1) {
			prev = val[i - 1];
			cur = val[i];
			if(close) next = val[0];
			else next = val[i];
		} else {
			prev = val[i - 1];
			cur = val[i];
			next = val[i + 1];
		}
		vec3 p0 = (cur - prev) * (1.0f + bias);
		vec3 p1 = (next - cur) * (1.0f - bias);
		vec3 r0 = (p0 + (p1 - p0) * 0.5f * (1.0f + continuity)) * (1.0f - tension);
		vec3 r1 = (p0 + (p1 - p0) * 0.5f * (1.0f - continuity)) * (1.0f - tension);
		params[i * 4 + 0] = cur;
		params[i * 4 + 1] = next;
		params[i * 4 + 2] = r0;
		if(i) params[i * 4 - 1] = r1;
		else params[(num - 1) * 4 + 3] = r1;
		length += (next - cur).length();
	}
	for(int i = 0; i < num; i++) {
		vec3 p0 = params[i * 4 + 0];
		vec3 p1 = params[i * 4 + 1];
		vec3 r0 = params[i * 4 + 2];
		vec3 r1 = params[i * 4 + 3];
		params[i * 4 + 0] = p0;
		params[i * 4 + 1] = r0;
		params[i * 4 + 2] = -p0 * 3.0f + p1 * 3.0f - r0 * 2.0f - r1;
		params[i * 4 + 3] = p0 * 2.0f - p1 * 2.0f + r0 + r1;
	}
	
	delete val;
}

Spline::Spline(const Spline &spline) {
	num = spline.num;
	params = new vec3[num * 4];
	memcpy(params,spline.params,sizeof(vec3) * num * 4);
	speed = spline.speed;
	length = spline.length;
	follow = spline.follow;
}

Spline::~Spline() {
	if(params) delete params;
}

/*
 */
mat4 Spline::to_matrix(float time) {
	if(!params) return mat4();
	time *= speed / length * (float)num;
	int i = (int)time;
	time -= i;
	i = (i % num) * 4;
	float time2 = time * time;
	float time3 = time2 * time;
	vec3 pos = params[i + 0] + params[i + 1] * time + params[i + 2] * time2 + params[i + 3] * time3;
	mat4 transform;
	transform.translate(pos);
	return transform;
}

/*****************************************************************************/
/*                                                                           */
/* Expression                                                                */
/*                                                                           */
/*****************************************************************************/

Expression::Expression(const char *str) {
	int length = strlen(str) + 1;
	for(int i = 0; i < 14; i++) {
		exp[i] = new char[length];
		exp[i][0] = '\0';
	}
	char *s = (char*)str;
	for(int i = 0; i < 14; i++) {
		char *d = NULL;
		d = exp[i];
		if(*s == '\0') {
			if(i == 7) return;
			fprintf(stderr,"Expression::Expression(): missing argument %d \"%s\"\n",i,str);
			return;
		}
		while(1) {
			if(*s && *s != ',') {
				if(strchr(" \t\n\r",*s)) s++;
				else *d++ = *s++;
			} else {
				if(*d == '\0' && *s != ',') {
					if(i == 6) return;
					fprintf(stderr,"Expression::Expression(): missing argument %d \"%s\"\n",i,str);
					return;
				}
				else *d = '\0';
				if(*s) s++;
				break;
			}
		}
	}
}

Expression::Expression(const Expression &expression) {
	for(int i = 0; i < 14; i++) {
		exp[i] = new char[strlen(expression.exp[i]) + 1];
		strcpy(exp[i],expression.exp[i]);
	}
}

Expression::~Expression() {
	for(int i = 0; i < 14; i++) delete exp[i];
}

/*
 */
mat4 Expression::to_matrix(float time) {
	vec3 pos_0 = vec3(Parser::expression(exp[0],"time",time),Parser::expression(exp[1],"time",time),Parser::expression(exp[2],"time",time));
	quat rot_0 = quat(Parser::expression(exp[3],"time",time),Parser::expression(exp[4],"time",time),Parser::expression(exp[5],"time",time),Parser::expression(exp[6],"time",time));
	mat4 translate_0;
	translate_0.translate(pos_0);
	if(exp[7][0] == '\0') return translate_0 * rot_0.to_matrix();
	vec3 pos_1 = vec3(Parser::expression(exp[7],"time",time),Parser::expression(exp[8],"time",time),Parser::expression(exp[9],"time",time));
	quat rot_1 = quat(Parser::expression(exp[10],"time",time),Parser::expression(exp[11],"time",time),Parser::expression(exp[12],"time",time),Parser::expression(exp[13],"time",time));
	mat4 translate_1;
	translate_1.translate(pos_1);
	return (translate_1 * rot_1.to_matrix()) * (translate_0 * rot_0.to_matrix());
}
