/* Console
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

#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include <vector>
#include "font.h"
#include <stdio.h>

#define CONSOLE_CMD	"~# "

class Console : public Font {
public:
	
	Console(const char *name,FILE *file = NULL);
	~Console();
	
	void printf(float x,float y,const char *str,...);
	void printf(const char *str,...);
	void printf(float r,float g,float b,const char *str,...);
	
	void render(float ifps,int num = 20);
	
	void keyPress(int key);
	
	int getActivity();
	
	void addBool(const char *name,int *b);
	void addInt(const char *name,int *i);
	void addFloat(const char *name,float *f);
	void addCommand(const char *name,void (*func)(int,char**,void*),void *data = NULL);
	
protected:
	
	enum {
		NUM_LINES = 40,
		HISTORY = 20
	};
	
	enum {
		BOOL = 0,
		INT,
		FLOAT,
	};
	
	struct Variable {
		int type;
		union {
			int *b;
			int *i;
			float *f;
		};
		char name[256];
	};
	
	struct Command {
		void (*func)(int,char**,void*);
		void *data;
		char name[256];
	};
	
	std::vector<Variable*> variables;
	std::vector<Command*> commands;
	
	struct Line {
		char *str;
		float r,g,b;
	};
	
	int current_line;
	Line lines[NUM_LINES];
	
	int last_history;
	int current_history;
	char *history[HISTORY];
	
	char cmd[1024];
	char *cmd_ptr;
	
	int activity;
	float time;

	FILE *file;
};

#endif /* __CONSOLE_H__ */
