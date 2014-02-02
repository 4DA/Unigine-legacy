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

#include "engine.h"
#include "console.h"

Console::Console(const char *name,FILE *file) : Font(name), current_line(0), last_history(0), current_history(0), activity(0), time(1.0), file(file) {
	for(int i = 0; i < NUM_LINES; i++) {
		lines[i].str = new char[1024];
		lines[i].str[0] = '\0';
	}
	for(int i = 0; i < HISTORY; i++) {
		history[i] = new char[1024];
		history[i][0] = '\0';
	}
	cmd[0] = '\0';
	cmd_ptr = cmd;
}

Console::~Console() {
	for(int i = 0; i < NUM_LINES; i++) delete lines[i].str;
	for(int i = 0; i < HISTORY; i++) delete history[i];
}

/*
 */
void Console::printf(float x,float y,const char *str,...) {
	char buf[4096];
	va_list argptr;
	va_start(argptr,str);
	vsprintf(buf,str,argptr);
	va_end(argptr);
	puts(x,y,buf);
}

/*
 */
void Console::printf(const char *str,...) {
	char buf[4096];
	va_list argptr;
	va_start(argptr,str);
	vsprintf(buf,str,argptr);
	va_end(argptr);
	printf(0,1,0,"%s",buf);
}

void Console::printf(float r,float g,float b,const char *str,...) {
	char buf[4096];
	va_list argptr;
	va_start(argptr,str);
	vsprintf(buf,str,argptr);
	va_end(argptr);
	
	Line *line = &lines[current_line];
	line->r = r;
	line->g = g;
	line->b = b;
	char *d = line->str;
	while(*d) d++;	// end of line
	
	for(char *s = buf; *s; s++) {
		if(*s != '\n' && d - line->str < 100) *d++ = *s;
		else {
			if(*s != '\n') {
				char *e = d - 1;
				while(e > line->str && !strchr(" \t",*e)) e--;
				if(e > line->str) {
					s -= d - e;
					d = e;
				}
			}
			*d = '\0';
			if(++current_line == NUM_LINES) current_line = 0;
			line = &lines[current_line];
			line->r = r;
			line->g = g;
			line->b = b;
			d = line->str;
			*d = '\0';
		}
	}
	*d = '\0';
	
	::printf("%s",buf);
	if(file) {
		fprintf(file,"%s",buf);
		fflush(file);
	}
}

/*
 */
void Console::render(float ifps,int num) {
	
	time += ifps;
	
	if(height < step * num + step) num = height / step - 1;	// fluent on/off
	if(activity && time < 0.1) num = (int)((float)num * time * 10.0);
	else if(!activity) {
		if(time > 0.1) return;
		num = (int)((float)num * (0.1 - time) * 10.0);
	}
	
	glDisable(GL_TEXTURE_2D);
	glBlendFunc(GL_ZERO,GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(0,0,0,0.3);
	glBegin(GL_QUADS);
	glVertex2f(0,0);
	glVertex2f(0,step * num + step);
	glVertex2f(width,step * num + step);
	glVertex2f(width,0);
	glEnd();
	glEnable(GL_TEXTURE_2D);
	
	for(int i = 0; i < num; i++) {
		if(current_line - num + i < 0) {
			Line *line = &lines[NUM_LINES + current_line - num + i];
			glColor3f(line->r,line->g,line->b);
			puts(step / 4,step * i,line->str);
		} else {
			Line *line = &lines[current_line - num + i];
			glColor3f(line->r,line->g,line->b);
			puts(step / 4,step * i,line->str);
		}
	}
	glColor3f(1,1,1);
	if(time - (int)time < 0.5) printf(step / 4,step * num,CONSOLE_CMD"%s_",cmd);
	else printf(step / 4,step * num,CONSOLE_CMD"%s",cmd);
	glColor3f(0,1,0);
	if(activity && time > 0.1) printf(width - step * 6,0,"FPS: %0.1f",1.0 / ifps);
	glColor3f(1,1,1);
}

/*
 */
void Console::keyPress(int key) {
	if(key == '`') {
		activity = !activity;
		time = 0;
		return;
	}
	if(!activity) return;
	if(key == '\n') {
		if(cmd_ptr != cmd) {
			printf(1,1,1,CONSOLE_CMD"%s\n",cmd);
			if(!strncmp(cmd,"clear",5)) {
				for(int i = 0; i < NUM_LINES; i++) lines[i].str[0] = '\0';
			} else if(!strncmp(cmd,"ls",2)) {
				printf("variables: ");
				for(int i = 0; i < (int)variables.size(); i++) printf("%s%s",variables[i]->name,i == (int)variables.size() - 1 ? "" : ", ");
				printf("\ncommands: ");
				for(int i = 0; i < (int)commands.size(); i++) printf("%s%s",commands[i]->name,i == (int)commands.size() - 1 ? "" : ", ");
				printf("\n");
			} else if(!strncmp(cmd,"set",3)) {
				cmd_ptr = cmd + 3;
				while(*cmd_ptr == ' ') cmd_ptr++;
				if(!*cmd_ptr) printf("usage: set <variable> <value>\n");
				else {
					char name[1024];
					sscanf(cmd_ptr,"%s",name);
					Variable *v = NULL;
					for(int i = 0; i < (int)variables.size(); i++) {
						if(!strncmp(cmd_ptr,variables[i]->name,strlen(variables[i]->name))) {
							v = variables[i];
							break;
						}
					}
					if(v) {
						if(v->type == BOOL) {
							char val[128];
							if(sscanf(cmd_ptr,"%s %s",name,val) == 2) {
								if(!strcmp(val,"on")) *v->b = 1;
								else if(!strcmp(val,"off")) *v->b = 0;
								else if(!strcmp(val,"1")) *v->b = 1;
								else if(!strcmp(val,"0")) *v->b = 0;
							}
							printf("%s set to %d\n",name,*v->b);
						} else if(v->type == INT) {
							sscanf(cmd_ptr,"%s %d",name,v->i);
							printf("%s set to %d\n",name,*v->i);
						} else if(v->type == FLOAT) {
							sscanf(cmd_ptr,"%s %f",name,v->f);
							printf("%s set to %g\n",name,*v->f);
						}
					} else printf(1,0,0,"unknown variable \"%s\"\n",name);
				}
			} else {
				Command *c = NULL;
				int argc = 1;
				static char *argv[3];
				static char argv_buf[3][64];
				argv[0] = argv_buf[0];
				argv[1] = argv_buf[1];
				argv[2] = argv_buf[2];
				char *s = cmd;
				char *d = argv[0];
				*d = '\0';
				while(1) {
					if(*s != ' ' && *s != '\t') *d++ = *s;
					else if(d != argv[argc - 1]) {
						*d = '\0';
						if(argc < 3) argc++;
						d = argv[argc - 1];
					}
					if(*s == '\0') break;
					s++;
				}
				for(int i = 0; i < (int)commands.size(); i++) {
					if(!strcmp(commands[i]->name,argv[0])) {
						c = commands[i];
						break;
					}
				}
				if(!c) printf(1,0,0,"unknown command \"%s\"\n",argv[0]);
				else c->func(argc,argv,c->data);
			}
			strcpy(history[current_history++],cmd);
			if(current_history == HISTORY) current_history = 0;
			last_history = current_history;
			cmd[0] = '\0';
			cmd_ptr = cmd;
		}
	} else if(key == '\b') {
		if(cmd_ptr > cmd) {
			cmd_ptr--;
			*cmd_ptr = '\0';
		}
	} else if(key == '\1') {
		last_history--;
		if(last_history < 0) last_history = HISTORY - 1;
		if(last_history == current_history || history[last_history][0] == '\0') {
			last_history++;
			if(last_history == HISTORY) last_history = 0;
		}
		strcpy(cmd,history[last_history]);
		cmd_ptr = cmd + strlen(cmd);
	} else if(key == '\2') {
		if(last_history == current_history) {
			cmd[0] = '\0';
			cmd_ptr = cmd;
		} else {
			last_history++;
			if(last_history == HISTORY) last_history = 0;
			strcpy(cmd,history[last_history]);
			cmd_ptr = cmd + strlen(cmd);
		}
	} else {
		*cmd_ptr++ = key;
		*cmd_ptr = '\0';
	}
}

/*
 */
int Console::getActivity() {
	return activity;
}

/*
 */
void Console::addBool(const char *name,int *b) {
	Variable *v = new Variable;
	v->type = BOOL;
	v->b = b;
	strcpy(v->name,name);
	variables.push_back(v);
}

void Console::addInt(const char *name,int *i) {
	Variable *v = new Variable;
	v->type = INT;
	v->i = i;
	strcpy(v->name,name);
	variables.push_back(v);
}

void Console::addFloat(const char *name,float *f) {
	Variable *v = new Variable;
	v->type = FLOAT;
	v->f = f;
	strcpy(v->name,name);
	variables.push_back(v);
}

void Console::addCommand(const char *name,void (*func)(int,char**,void*),void *data) {
	Command *c = new Command;
	c->func = func;
	c->data = data;
	strcpy(c->name,name);
	commands.push_back(c);
}
