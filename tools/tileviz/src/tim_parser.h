// TIM parser public API

#ifndef TIM_PARSER_H
#define TIM_PARSER_H

#include "tim_types.h"
#include <stdio.h>
#include <stdbool.h>

// Parse TIM file header from file (validates magic number)
bool TIM_ParseHeader(FILE* file, TIMFileHeader* header);

// Load complete TIM file from filepath
// Returns NULL on failure, pointer to TIMFile on success
// The TIMFile structure contains parsed header, palette (if present), and image data
TIMFile* TIM_LoadFile(const char* filepath);

// Free TIM file resources
void TIM_FreeFile(TIMFile* tim);

#endif // TIM_PARSER_H