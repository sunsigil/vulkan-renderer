#include "obj.h"

#include "memory.h"
#include <stdlib.h>
#include <iostream>

static bool is_whitespace(char c)
{
	return
	c == ' ' ||
	c == '\n' ||
	c == '\t';
}

static char* seek_whitespace(char* text)
{
	while(*text != '\0' && !is_whitespace(*text))
		text++;
	return text;
}

static char* seek_glyph(char* text)
{
	while(*text != '\0' && is_whitespace(*text))
		text++;
	return text;
}

static char* advance_token(char** text)
{
	*text = seek_whitespace(*text);
	*text = seek_glyph(*text);
	return *text;
}

struct token
{
	char* start;
	size_t length;
};

static token consume_token(char** text)
{
	char* start = seek_glyph(*text);
	char* end = seek_whitespace(start);
	advance_token(text);
	return
	{
		.start = start,
		.length = (size_t) (end-start)
	};
}

static bool token_says(token* tok, const char* text)
{
	if(strlen(text) != tok->length)
		return false;
	return strncmp(tok->start, text, tok->length) == 0;
}

static float consume_float(char** text)
{
	return atof(consume_token(text).start);
}

enum face_component
{
	FACE_V = (1 << 0),
	FACE_VT = (1 << 1),
	FACE_VN = (1 << 2)
};

static int classify_face(token* tok)
{
	char* ptr = tok->start;
	char* end = tok->start + tok->length;
	int delimiters = 0;
	int component_flags = FACE_V;
	while(ptr < end)
	{	
		if(*ptr == '/')
		{	
			if(delimiters == 0)
				component_flags |= FACE_VT;
			else if(delimiters == 1)
				component_flags |= FACE_VN;
			delimiters += 1;
			
		}
		ptr++;
	}
	return component_flags;
}

static char* seek_face_component(char* text)
{
	while(*text != '\0' && *text != '/')
		text++;
	return text+1;
}

static int read_face_component(token* tok, int component)
{
	char* text = tok->start;
	int component_pos =
	component == FACE_V ? 0 :
	component == FACE_VT ? 1 :
	2;
	for(int i = 0; i < component_pos; i++)
		text = seek_face_component(text);
	return atoi(text);
}

void TOS_OBJ_load(TOS_OBJ* obj, const char* path)
{
	*obj =
	{
		.v = std::vector<float>(),
		.vt = std::vector<float>(),
		.vn = std::vector<float>(),
		.f = std::vector<int>(),
	};

	size_t file_size;
	char* file_data = (char*) TOS_map_file(path, TOS_FILE_MAP_PRIVATE, &file_size);

	char* ptr = seek_glyph(file_data);

	token tok;
	do
	{
		tok = consume_token(&ptr);
		if(token_says(&tok, "v"))
		{
			obj->v.push_back(consume_float(&ptr));
			obj->v.push_back(consume_float(&ptr));
			obj->v.push_back(consume_float(&ptr));
		}
		else if(token_says(&tok, "vt"))
		{
			obj->vt.push_back(consume_float(&ptr));
			obj->vt.push_back(consume_float(&ptr));
		}
		else if(token_says(&tok, "vn"))
		{
			obj->vn.push_back(consume_float(&ptr));
			obj->vn.push_back(consume_float(&ptr));
			obj->vn.push_back(consume_float(&ptr));
		}
		else if(token_says(&tok, "f"))
		{
			for(int pt_idx = 0; pt_idx < 3; pt_idx ++)
			{
				token ftok = consume_token(&ptr);
				int component_flags = classify_face(&ftok);
				for(int component_idx = 0; component_idx < 3; component_idx++)
				{
					int component_flag = 1 << component_idx;
					int component = component_flags & component_flag ? read_face_component(&ftok, component_flag) : 0;
					obj->f.push_back(component);
				}
			}
		}
	}
	while(tok.length > 0);

	TOS_unmap_file(file_data, file_size);
}