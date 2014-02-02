/* Parser
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
#include "parser.h"

float Parser::variables[26];

/*
 */
Parser::Parser(const char *name) : data(NULL) {
	
	FILE *file = fopen(name,"r");
	if(!file) {
		fprintf(stderr,"Parse::Parse(): error open \"%s\" file\n",name);
		return;
	}
	fseek(file,0,SEEK_END);
	int size = ftell(file);
	fseek(file,0,SEEK_SET);
	data = new char[size + 1];
	memset(data,0,sizeof(char) * (size + 1));
	fread(data,sizeof(char),size,file);
	fclose(file);
	
	char *s = data;
	char *d = data;
	int define_ok[32];
	int depth = 0;
	
	while(*s) {
		
		// #ifdef, #ifndef, #else, #endif preprocessor
		if(*s == '#' && !strncmp(s + 1,"ifdef",5)) {
			char define[1024];
			sscanf(s + 6,"%s",define);
			while(*s != '\0' && *s != '\n') s++;
			if(Engine::isDefine(define) == 1) define_ok[depth++] = 1;
			else define_ok[depth++] = 0;
		} else if(*s == '#' && !strncmp(s + 1,"ifndef",6)) {
			char define[1024];
			sscanf(s + 7,"%s",define);
			while(*s != '\0' && *s != '\n') s++;
			if(Engine::isDefine(define) == 0) define_ok[depth++] = 1;
			else define_ok[depth++] = 0;
		} else if(*s == '#' && !strncmp(s + 1,"else",4)) {
			while(*s != '\0' && *s != '\n') s++;
			define_ok[depth - 1] = !define_ok[depth - 1];
		} else if(*s == '#' && !strncmp(s + 1,"endif",5)) {
			while(*s != '\0' && *s != '\n') s++;
			depth--;
		}
		
		// C-like comments // and /* */
		else if(*s == '/' && *(s + 1) == '/') {
			while(*s && *s != '\n') s++;
			while(*s && *s == '\n') s++;
			*d++ = '\n';
		} else if(*s == '/' && *(s + 1) == '*') {
			while(*s && (*s != '*' || *(s + 1) != '/')) s++;
			s += 2;
			while(*s && *s == '\n') s++;
			*d++ = '\n';
		}
		
		// blocks
		else if(*s == '<' && isalpha(*(s + 1))) {
			int i = 0;
			for(; i < depth; i++) if(define_ok[i] == 0) break;
			if(i == depth) {
				while(d > data && *(d - 1) == '\n') d--;
				*d++ = *s = '\0';
				char *name = ++s;
				while(*s && *s != '>') s++;
				*s++ = '\0';
				while(*s && strchr(" \t\n\r",*s)) s++;
				Block b;
				b.pointer = d;
				b.use = 0;
				blocks[name] = b;
			} else {
				s++;
			}
		}
		
		else {
			if(depth == 0) *d++ = *s++;
			else {
				int i = 0;
				for(; i < depth; i++) if(define_ok[i] == 0) break;
				if(i == depth) *d++ = *s++;
				else s++;
			}
		}
	}
	while(d > data && *(d - 1) == '\n') d--;
	*d = '\0';
}

Parser::~Parser() {
	for(std::map<std::string,Block>::iterator it = blocks.begin(); it != blocks.end(); it++) {
		if(!it->second.use) fprintf(stderr,"Parser::~Parser(): warning unused block \"%s\"\n",it->first.c_str());
	}
	blocks.clear();
	delete data;
}

/*
 */
char *Parser::get(const char *name) {
	std::map<std::string,Block>::iterator it = blocks.find(name);
	if(it == blocks.end()) return NULL;
	it->second.use = 1;
	char *ret = it->second.pointer;
	while(*ret && strchr(" \t\n\r",*ret)) ret++;
	char *s = ret + strlen(ret) - 1;
	while(s > ret && strchr(" \t\n\r",*s)) s--;
	s++;
	*s = '\0';
	return ret;
}

/*****************************************************************************/
/*                                                                           */
/* read string                                                               */
/*                                                                           */
/*****************************************************************************/

int Parser::read_string(const char *str,char *dest) {
	const char *s = str;
	char *d = dest;
	while(*s && strchr(" \t",*s)) s++;
	if(*s == '"') {
		s++;
		while(*s && *s != '"') *d++ = *s++;
		s++;
		*d = '\0';
	} else {
		while(*s && !strchr(" \t",*s)) *d++ = *s++;
		*d = '\0';
	}
	return s - str;
}

/*****************************************************************************/
/*                                                                           */
/* expressions                                                               */
/*                                                                           */
/*****************************************************************************/

/*
 */
int Parser::priority(char op) {
	if(strchr("sScCtTleqfr",op)) return 4;
	if(strchr("*/%",op)) return 3;
	if(strchr("+-",op)) return 2;
	if(strchr("<>!=&|",op)) return 1;
	return 0;
}

/*
 */
float Parser::expression(const char *str,const char *variable,float value) {
	static struct {
		char op;
		float num;
	} stack[1024];
	int stack_depth = 0;
	static char stack_op[1024];
	int stack_op_depth = 0;
	const char *s = str;
	int brackets = 0;
	while(*s) {
		if(*s == '(') brackets++;
		else if(*s == ')') brackets--;
		s++;
	}
	if(brackets != 0) {
		fprintf(stderr,"Paser::expression(): parse error before '%c'\n",brackets > 0 ? '(' : ')');
		return 0.0;
	}
	s = str;
	while(*s) {
		if(*s == '(') {
			stack_op[stack_op_depth++] = *s++;
		}
		else if(*s == ')') {
			while(stack_op_depth > 0 && stack_op[--stack_op_depth] != '(') stack[stack_depth++].op = stack_op[stack_op_depth];
			s++;
		}
		else if(strchr("<>+-*/%",*s)) {
			while(stack_op_depth > 0 && priority(*s) <= priority(stack_op[stack_op_depth - 1])) {
				stack[stack_depth++].op = stack_op[--stack_op_depth];
			}
			stack_op[stack_op_depth++] = *s++;
		}
		else if(!strncmp("==",s,2)) {
			stack_op[stack_op_depth++] = '=';
			s += 2;
		}
		else if(!strncmp("!=",s,2)) {
			stack_op[stack_op_depth++] = '!';
			s += 2;
		}
		else if(!strncmp("&&",s,2)) {
			stack_op[stack_op_depth++] = '&';
			s += 2;
		}
		else if(!strncmp("||",s,2)) {
			stack_op[stack_op_depth++] = '|';
			s += 2;
		}
		else if(!strncmp("sin",s,3)) {
			stack_op[stack_op_depth++] = 's';
			s += 3;
		}
		else if(!strncmp("asin",s,4)) {
			stack_op[stack_op_depth++] = 'S';
			s += 4;
		}
		else if(!strncmp("cos",s,3)) {
			stack_op[stack_op_depth++] = 'c';
			s += 3;
		}
		else if(!strncmp("acos",s,4)) {
			stack_op[stack_op_depth++] = 'C';
			s += 4;
		}
		else if(!strncmp("tan",s,3)) {
			stack_op[stack_op_depth++] = 't';
			s += 3;
		}
		else if(!strncmp("atan",s,4)) {
			stack_op[stack_op_depth++] = 'T';
			s += 4;
		}
		else if(!strncmp("log",s,3)) {
			stack_op[stack_op_depth++] = 'l';
			s += 3;
		}
		else if(!strncmp("exp",s,3)) {
			stack_op[stack_op_depth++] = 'e';
			s += 3;
		}
		else if(!strncmp("sqrt",s,4)) {
			stack_op[stack_op_depth++] = 'q';
			s += 4;
		}
		else if(!strncmp("fabs",s,4)) {
			stack_op[stack_op_depth++] = 'f';
			s += 4;
		}
		else if(!strncmp("rand",s,4)) {
			stack_op[stack_op_depth++] = 'r';
			s += 4;
		}
		else if(strchr("0123456789.",*s)) {
			char buf[1024];
			char *b = buf;
			*b++ = *s++;
			while(*s && strchr("0123456789.",*s)) *b++ = *s++;
			*b = '\0';
			stack[stack_depth].op = 'n';
			stack[stack_depth++].num = atof(buf);
		}
		else if(variable && !strncmp(variable,s,strlen(variable))) {
			stack[stack_depth].op = 'n';
			stack[stack_depth++].num = value;
			s += 4;
		}
		else if(!variable && *s == '$') {
			s++;
			if(!isalpha(*s)) {
				fprintf(stderr,"Paser::expression(): unknown variable \"%c\"\n",*s);
				return 0.0;
			}
			stack[stack_depth].op = 'n';
			stack[stack_depth++].num = variables[tolower(*s++) - 'a'];
		}
		else if(strchr(" \t\n\r",*s)) s++;
		else {
			fprintf(stderr,"Paser::expression(): unknown token \"%s\"\n",s);
			return 0.0;
		}
	}
	while(stack_op_depth--) stack[stack_depth++].op = stack_op[stack_op_depth];
	for(int tries = 0; tries < 1024; tries++) {
		int end = 0;
		for(int i = 0, a = -1, b = -1, num = 0; i < stack_depth; i++) {
			if(!stack[i].op) continue;
			end++;
			if(num >= 1 && strchr("sScCtTleqfr",stack[i].op)) {
				int c = a;
				if(b != -1) c = b;
				switch(stack[i].op) {
					case 's': stack[i].num = sin(stack[c].num); break;
					case 'S': stack[i].num = asin(stack[c].num); break;
					case 'c': stack[i].num = cos(stack[c].num); break;
					case 'C': stack[i].num = acos(stack[c].num); break;
					case 't': stack[i].num = tan(stack[c].num); break;
					case 'T': stack[i].num = atan(stack[c].num); break;
					case 'l': stack[i].num = log(stack[c].num); break;
					case 'e': stack[i].num = exp(stack[c].num); break;
					case 'q': stack[i].num = sqrt(stack[c].num); break;
					case 'f': stack[i].num = fabs(stack[c].num); break;
					case 'r': stack[i].num = rand() / (float)RAND_MAX * stack[c].num; break;
				}
				stack[i].op = 'n';
				stack[c].op = 0;
				end += 10;
				break;
			}
			else if(num >= 2 && strchr("<>!=&|+-*/%",stack[i].op)) {
				switch(stack[i].op) {
					case '<': stack[i].num = stack[a].num < stack[b].num; break;
					case '>': stack[i].num = stack[a].num > stack[b].num; break;
					case '=': stack[i].num = stack[a].num == stack[b].num; break;
					case '!': stack[i].num = stack[a].num != stack[b].num; break;
					case '&': stack[i].num = stack[a].num && stack[b].num; break;
					case '|': stack[i].num = stack[a].num || stack[b].num; break;
					case '+': stack[i].num = stack[a].num + stack[b].num; break;
					case '-': stack[i].num = stack[a].num - stack[b].num; break;
					case '*': stack[i].num = stack[a].num * stack[b].num; break;
					case '/': stack[i].num = stack[a].num / stack[b].num; break;
					case '%': stack[i].num = (int)stack[a].num % (int)stack[b].num; break;
				}
				stack[i].op = 'n';
				stack[a].op = stack[b].op = 0;
				break;
			}
			else if(num == 1 && strchr("!+-",stack[i].op)) {
				if(stack[i].op == '!') stack[a].num = !stack[a].num;
				if(stack[i].op == '-') stack[a].num = -stack[a].num;
				stack[i].op = 0;
				end += 10;
				break;
			}
			else if(stack[i].op == 'n') {
				switch(num++) {
					case 0: a = i; break;
					case 1: b = i; break;
					default: a = b; b = i; break;
				}
			}
			else num = 0;
		}
		if(end < 3) {
			for(int i = 0; i < stack_depth; i++) {
				if(stack[i].op == 'n') return stack[i].num;
			}
		}
	}
	return 0.0;
}

/*****************************************************************************/
/*                                                                           */
/* simple interpreter                                                        */
/*                                                                           */
/*****************************************************************************/

/*
 */
int Parser::read_token(const char *src,char *dest) {
	const char *s = src;
	int brackets_0 = 0;
	int brackets_1 = 0;
	while(*s) {
		if(*s == '(') brackets_0++;
		else if(*s == ')' && --brackets_0 < 0) break;
		else if(*s == '{') brackets_1++;
		else if(*s == '}' && --brackets_1 < 0) break;
		else if(*s == ';') break;
		if(dest) *dest++ = *s++;
		else s++;
	}
	if(dest) *dest = '\0';
	return s - src;
}

/*
 */
int Parser::interpret_eq(const char *src) {
	const char *s = src;
	
	while(*s && strchr(" \t",*s)) s++;
	if(*s++ != '$') throw("Parser::interpret_eq(): missing '$'");
	
	char var = *s++;
	if(!isalpha(var)) throw("Parser::interpret_eq(): unknown variable");
	var = tolower(var);
	
	int op = 0;
	while(*s && strchr(" \t",*s)) s++;
	if(!strncmp("=",s,1)) { s++; op = 0; }
	else if(!strncmp("+=",s,2)) { s += 2; op = 1; }
	else if(!strncmp("-=",s,2)) { s += 2; op = 2; }
	else if(!strncmp("++",s,2)) { s += 2; op = 3; }
	else if(!strncmp("--",s,2)) { s += 2; op = 4; }
	else throw("Parser::interpret_eq(): missing '='");
	
	char exp[1024];
	s += read_token(s,exp);
	if(*s) *s++;
	
	if(op == 0) variables[var - 'a'] = expression(exp);
	else if(op == 1) variables[var - 'a'] += expression(exp);
	else if(op == 2) variables[var - 'a'] -= expression(exp);
	else if(op == 3) variables[var - 'a'] += 1.0f;
	else if(op == 3) variables[var - 'a'] -= 1.0f;
	
	return s - src;
}

/*
 */
int Parser::interpret_if(const char *src,char **dest) {
	const char *s = src + 2;
	char condition[1024];
	
	while(*s && strchr(" \t",*s)) s++;
	if(*s++ != '(') throw("Parser::interpret_if(): can`t find '('");
	
	s += read_token(s,condition);
	if(*s++ != ')') throw("Parser::interpret_if(): can`t find ')'");
	
	while(*s && strchr(" \t",*s)) s++;
	if(*s++ != '{') throw("Parser::interpret_if(): can`t find '{'");
	
	if(expression(condition)) {
		s += interpret_main(s,dest);
		while(*s && strchr(" \t",*s)) s++;
		if(!strncmp("else",s,4)) {
			s += 4;
			while(*s && strchr(" \t",*s)) s++;
			if(*s++ != '{') throw("Parser::interpret_if(): can`t find '{'");
			s += read_token(s,NULL);
			if(*s++ != '}') throw("Parser::interpret_if(): can`t find '}'");
		}
	} else {
		s += read_token(s,NULL);
		if(*s++ != '}') throw("Parser::interpret_if(): can`t find '}'");
		while(*s && strchr(" \t",*s)) s++;
		if(!strncmp("else",s,4)) {
			s += 4;
			while(*s && strchr(" \t",*s)) s++;
			if(*s++ != '{') throw("Parser::interpret_if(): can`t find '{'");
			s += interpret_main(s,dest);
		}
	}
	return s - src;
}

/*
 */
int Parser::interpret_for(const char *src,char **dest) {
	const char *s = src + 3;
	char condition[1024];
	char addition[1024];
	
	while(*s && strchr(" \t",*s)) s++;
	if(*s++ != '(') throw("Parser::interpret_for(): can`t find '('");
	s += interpret_eq(s);
	
	s += read_token(s,condition);
	if(*s++ != ';') throw("Parser::interpret_for(): can`t find ';'");
	
	s += read_token(s,addition);
	if(*s++ != ')') throw("Parser::interpret_for(): can`t find ')'");
	
	while(*s && strchr(" \t",*s)) s++;
	if(*s++ != '{') throw("Parser::interpret_for(): can`t find '{'");
	
	const char *begin = NULL;
	do {
		if(!begin) {
			begin = s;
			s += interpret_main(begin,dest);
		} else interpret_main(begin,dest);
		interpret_eq(addition);
	} while(expression(condition));
	return s - src;
}

/*
 */
int Parser::interpret_main(const char *src,char **dest) {
	const char *s = src;
	int brackets = 0;
	int new_word = 1;
	while(*s) {
		if(new_word && !strncmp("if",s,2) && strchr("( \t",*(s + 2))) {
			s += interpret_if(s,dest);
		}
		else if(new_word && !strncmp("for",s,3) && strchr("( \t",*(s + 3))) {
			s += interpret_for(s,dest);
		}
		else if(*s == '$' && (*(s + 1) == '(' || isalpha(*(s + 1)))) {
			if(*(s + 1) == '(') {
				s += 2;
				char exp[1024];
				s += read_token(s,exp);
				if(*s) *s++;
				(*dest) += sprintf(*dest,"%g",expression(exp));
			} else {
				const char *e = ++s;
				while(*e && !strchr("=(){};\n\r",*e)) e++;
				if(*e == '=') s += interpret_eq(s - 1);
				else (*dest) += sprintf(*dest,"%g",variables[tolower(*s++ - 'a')]);
			}
		} else {
			if(*s == '{') brackets++;
			else if(*s == '}') {
				brackets--;
				if(brackets < 0) {
					s++;
					break;
				}
			}
			if(strchr(" \t\n\r",*s)) new_word = 1;
			else new_word = 0;
			*(*dest)++ = *s++;
		}
	}
	return s - src;
}

/*
 */
const char *Parser::interpret(const char *src) {
	if(!src) return NULL;
	
	char *dest = new char[2 * 1024 * 1024];	// 2 Mb
	
	try {
		char *d = dest;
		interpret_main(src,&d);
		*d = '\0';
	}
	catch(const char *msg) {
		fprintf(stderr,"%s\n",msg);
		delete dest;
		return NULL;
	}
	
	return dest;
}
