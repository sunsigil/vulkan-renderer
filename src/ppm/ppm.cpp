#include "ppm.h"

#include "memory.h"
#include <string.h>
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

static void advance_token(char** text)
{
	*text = seek_whitespace(*text);
	*text = seek_glyph(*text);
}

static int consume_int_token(char** text)
{
	int x = atoi(*text);
	advance_token(text);
	return x;
}

void TOS_PPM_load(TOS_PPM* ppm, const char* path)
{
	ppm->width = 0;
	ppm->height = 0;
	ppm->size = 0;
	ppm->pixels = nullptr;

	size_t file_size;
	char* file_data = (char*) TOS_map_file(path, TOS_FILE_MAP_PRIVATE, &file_size);

	char* ptr = file_data;
	if(strncmp(ptr, "P3", 2) != 0)
	{
		std::cerr << "TOS_PPM_load: magic number failure in file " << path << std::endl;
		return;
	}
	advance_token(&ptr);

	ppm->width = consume_int_token(&ptr);
	ppm->height = consume_int_token(&ptr);
	advance_token(&ptr);

	ppm->size = ppm->width * ppm->height * 4;

	ppm->pixels = (uint8_t*) malloc(ppm->size);
	int channel_idx = 0;
	while(channel_idx < ppm->size)
	{
		ppm->pixels[channel_idx+0] = consume_int_token(&ptr);
		ppm->pixels[channel_idx+1] = consume_int_token(&ptr);
		ppm->pixels[channel_idx+2] = consume_int_token(&ptr);
		ppm->pixels[channel_idx+3] = 255;
		channel_idx += 4;
	}

	TOS_unmap_file(file_data, file_size);
}

void TOS_PPM_destroy(TOS_PPM* ppm)
{
	if(ppm->pixels != nullptr)
		free(ppm->pixels);
	ppm->pixels = nullptr;
}