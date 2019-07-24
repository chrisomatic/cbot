#pragma once

static void C_strncpy(char* dest, char* src, s32 count)
{
	if (count < 0) return;

	while (*src && count) {
		*dest++ = *src++;
		--count;
	}

	while (count) {
		*dest++ = 0;
		--count;
	}
}

static char* C_concat(int num, ...)
{
	va_list valist;

	char* concatenation = (char*)calloc(1000,sizeof(char));

	va_start(valist, num);

	strcpy(concatenation, va_arg(valist, char*));

	for (int i = 1; i < num; ++i)
		strcat(concatenation, va_arg(valist, char*));

	va_end(valist);

	return concatenation;
}

static BOOL C_atoi(const char* src, int* dst)
{
    // Skip any leading whitespace
	while (*src == ' ' || *src == '\t')
		++src;

	// Determine base and direction
	BOOL is_hex = FALSE;
	BOOL is_negative = FALSE;
	if (*src == '0' && (*(src + 1) == 'x' || *(src + 1) == 'X'))
	{
		is_hex = TRUE;
		src += 2;	// move pointer past '0x' part of hex number
	}
	else if (*src == '-')
	{
		is_negative = TRUE;
		++src;		// move pointer past '-' part of decimal number
	}

	int result = 0;

	if (is_hex)
	{
		// base 16: e.g. 0xAb09
		while (*src)
		{
			if ((*src >= '0' && *src <= '9'))
				result += (*src++ - '0');
			else if (*src >= 'A' && *src <= 'F')
				result += (*src++ - 'A' + 10);
			else if (*src >= 'a' && *src <= 'f')
				result += (*src++ - 'a' + 10);
			else
				return FALSE;
			
			if (*src) result *= 16;
		}
	}
	else
	{
		// base 10: e.g. 1037, -24
		while (*src >= '0' && *src <= '9')
		{
			result += (*src++ - '0');
			if(*src) result *= 10;
		}

		if (is_negative)
			result *= -1;
	}

	*dst = result;

	return TRUE;
}

static BOOL str_contains(char* base, char* search_pattern)
{
    char* b_p = base;
    char* p_p = search_pattern;

    while(*b_p)
    {
        if(*b_p == *p_p)
        {
            char* bb_p = b_p;
            BOOL match = TRUE;
            int pattern_length = 0;

            while(*p_p)
            {
                if(*bb_p != *p_p)
                    match = FALSE;
                
                ++p_p;
                ++bb_p;
                ++pattern_length;
            }

            p_p = search_pattern;

            if(match)
            {
                return TRUE;
            }
        }

        ++b_p;
    }

    return FALSE;
}

static void str_replace(char* base,int base_length, char* search_pattern, char* replacement, char* ret_string)
{
    int  ret_string_i = 0;

    char* b_p = base;
    char* p_p = search_pattern;
    char* r_p = replacement;

    while(*b_p)
    {
        if(*b_p == *p_p)
        {
            char* bb_p = b_p;
            BOOL match = TRUE;
            int pattern_length = 0;

            while(*p_p)
            {
                if(*bb_p != *p_p)
                    match = FALSE;
                
                ++p_p;
                ++bb_p;
                ++pattern_length;
            }

            if(match)
            {
                // replace search_pattern with replacement in base
                while(*r_p)
                    ret_string[ret_string_i++] = *r_p++;

                //reset pointers
                b_p += pattern_length;
                p_p = search_pattern;
                r_p = replacement;
            }
        }

        ret_string[ret_string_i++] = *b_p++;
    }
}

