// TIM parser public API

#ifndef TIM_PARSER_H
#define TIM_PARSER_H

#include "tim_types.h"
#include <stdio.h>

// Parse TIM file header from file
bool TIM_ParseHeader(FILE* file, TIMFileHeader* header);

// Load CLUT from file at specified offset
bool TIM_LoadCLUT(FILE* file, uint32_t offset, uint8_t palette_format, uint8_t clut_entries, PS1CLUT** out_clut);

// Load complete TIM file from filepath
TIMFile* TIM_LoadFile(const char* filepath);

// Free TIM file resources
void TIM_FreeFile(TIMFile* tim);

#endif // TIM_PARSER_H