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

#ifndef __PARSER_H__
#define __PARSER_H__

#include <string>
#include <map>

class Parser {
public:
	
	Parser(const char *name);
	~Parser();
	
	char *get(const char *name);
	
	static int read_string(const char *str,char *dest);
	static float expression(const char *str,const char *variable = NULL,float value = 0.0);
	static const char *interpret(const char *src);
	
protected:
	
	static int  priority(char op);
	
	static int read_token(const char *src,char *dest);
	static int interpret_eq(const char *src);
	static int interpret_if(const char *src,char **dest);
	static int interpret_for(const char *src,char **dest);
	static int interpret_main(const char *src,char **dest);
	
	char *data;
	
	struct Block {
		char *pointer;
		int use;
	};
	
	std::map<std::string,Block> blocks;
	
	static float variables[26];
};

#endif /* __PARSER_H__ */
