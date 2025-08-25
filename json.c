/*
  Copyright (c) 2009-2017 Dave Gamble and cJSON contributors

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

/* cJSON */
/* JSON parser in C. */

/* disable warnings about old C89 functions in MSVC */
#if !defined(_CRT_SECURE_NO_DEPRECATE) && defined(_MSC_VER)
#define _CRT_SECURE_NO_DEPRECATE
#endif

#ifdef __GNUC__
#pragma GCC visibility push(default)
#endif
#if defined(_MSC_VER)
#pragma warning (push)
/* disable warning about single line comments in system headers */
#pragma warning (disable : 4001)
#endif

#ifdef ENABLE_LOCALES
#include <locale.h>
#endif

#if defined(_MSC_VER)
#pragma warning (pop)
#endif
#ifdef __GNUC__
#pragma GCC visibility pop
#endif

#include "json.h"

#define NAN SDL_sqrt(-1.0)

typedef struct {
    const uint8_t *json;
    size_t position;
} JSON_Error;
static JSON_Error global_error = { NULL, 0 };

JSON_PUBLIC(const char *) JSON_GetErrorPtr(void)
{
    return (const char*) (global_error.json + global_error.position);
}

JSON_PUBLIC(char *) JSON_GetStringValue(const JSON_Node * const item)
{
    if (!JSON_IsString(item))
    {
        return NULL;
    }

    return item->valuestring;
}

JSON_PUBLIC(double) JSON_GetDoubleValue(const JSON_Node * const item)
{
    if (!JSON_IsNumber(item))
    {
        return (double) NAN;
    }

    return item->valuedouble;
}

JSON_PUBLIC(int32_t) JSON_GetIntValue(const JSON_Node * const item)
{
    if (!JSON_IsNumber(item))
    {
        return 0;
    }

    return item->valueint;
}

/* This is a safeguard to prevent copy-pasters from using incompatible C and header files */
#if (JSON_VERSION_MAJOR != 1) || (JSON_VERSION_MINOR != 7) || (JSON_VERSION_PATCH != 18)
    #error json.h and json.c have different versions. Make sure that both have the same.
#endif

JSON_PUBLIC(const char*) JSON_Version(void)
{
    static char version[15];
    SDL_snprintf(version, SDL_arraysize(version), "%i.%i.%i", JSON_VERSION_MAJOR, JSON_VERSION_MINOR, JSON_VERSION_PATCH);

    return version;
}

/* Case insensitive string comparison, doesn't consider two NULL pointers equal though */
static int32_t JSON_case_insensitive_strcmp(const uint8_t *string1, const uint8_t *string2)
{
    if ((string1 == NULL) || (string2 == NULL))
    {
        return 1;
    }

    if (string1 == string2)
    {
        return 0;
    }

    for(; SDL_tolower(*string1) == SDL_tolower(*string2); (void)string1++, string2++)
    {
        if (*string1 == '\0')
        {
            return 0;
        }
    }

    return SDL_tolower(*string1) - SDL_tolower(*string2);
}

/* strlen of character literals resolved at compile time */
#define JSON_static_strlen(string_literal) (sizeof(string_literal) - sizeof(""))

/* Delete a cJSON structure. */
JSON_PUBLIC(void) JSON_Delete(JSON_Node *item)
{
    JSON_Node *next = NULL;
    while (item != NULL)
    {
        next = item->next;
        if (!(item->type & JSON_IsReference) && (item->child != NULL))
        {
            JSON_Delete(item->child);
        }
        if (!(item->type & JSON_IsReference) && (item->valuestring != NULL))
        {
            SDL_free(item->valuestring);
            item->valuestring = NULL;
        }
        if (!(item->type & JSON_StringIsConst) && (item->string != NULL))
        {
            SDL_free(item->string);
            item->string = NULL;
        }
        SDL_free(item);
        item = next;
    }
}

/* get the decimal point character of the current locale */
static uint8_t JSON_GetDecimalPoint(void)
{
#ifdef ENABLE_LOCALES
    struct lconv *lconv = localeconv();
    return (uint8_t) lconv->decimal_point[0];
#else
    return '.';
#endif
}

typedef struct
{
    const uint8_t *content;
    size_t length;
    size_t offset;
    size_t depth; /* How deeply nested (in arrays/objects) is the input at the current offset. */
} JSON_ParseBuffer;

/* check if the given size is left to read in a given parse buffer (starting with 1) */
#define JSON_CanRead(buffer, size) ((buffer != NULL) && (((buffer)->offset + size) <= (buffer)->length))
/* check if the buffer can be accessed at the given index (starting with 0) */
#define JSON_CanAccessAtIndex(buffer, index) ((buffer != NULL) && (((buffer)->offset + index) < (buffer)->length))
#define JSON_CannotAccessAtIndex(buffer, index) (!JSON_CanAccessAtIndex(buffer, index))
/* get a pointer to the buffer at the position */
#define JSON_BufferAtOffset(buffer) ((buffer)->content + (buffer)->offset)

/* Parse the input text to generate a number, and populate the result into item. */
static bool JSON_ParseNumber(JSON_Node * const item, JSON_ParseBuffer * const input_buffer)
{
    double number = 0;
    uint8_t *after_end = NULL;
    uint8_t *number_c_string;
    uint8_t decimal_point = JSON_GetDecimalPoint();
    size_t i = 0;
    size_t number_string_length = 0;
    bool has_decimal_point = false;

    if ((input_buffer == NULL) || (input_buffer->content == NULL))
    {
        return false;
    }

    /* copy the number into a temporary buffer and replace '.' with the decimal point
     * of the current locale (for strtod)
     * This also takes care of '\0' not necessarily being available for marking the end of the input */
    for (i = 0; JSON_CanAccessAtIndex(input_buffer, i); i++)
    {
        switch (JSON_BufferAtOffset(input_buffer)[i])
        {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case '+':
            case '-':
            case 'e':
            case 'E':
                number_string_length++;
                break;

            case '.':
                number_string_length++;
                has_decimal_point = true;
                break;

            default:
                goto loop_end;
        }
    }
loop_end:
    /* malloc for temporary buffer, add 1 for '\0' */
    number_c_string = (uint8_t *) SDL_malloc(number_string_length + 1);
    if (number_c_string == NULL)
    {
        return false; /* allocation failure */
    }

    SDL_memcpy(number_c_string, JSON_BufferAtOffset(input_buffer), number_string_length);
    number_c_string[number_string_length] = '\0';

    if (has_decimal_point)
    {
        for (i = 0; i < number_string_length; i++)
        {
            if (number_c_string[i] == '.')
            {
                /* replace '.' with the decimal point of the current locale (for strtod) */
                number_c_string[i] = decimal_point;
            }
        }
    }

    number = SDL_strtod((const char*)number_c_string, (char**)&after_end);
    if (number_c_string == after_end)
    {
        /* free the temporary buffer */
        SDL_free(number_c_string);
        return false; /* parse_error */
    }

    item->valuedouble = number;

    /* use saturation in case of overflow */
    if (number >= SDL_MAX_SINT32)
    {
        item->valueint = SDL_MAX_SINT32;
    }
    else if (number <= (double)SDL_MIN_SINT32)
    {
        item->valueint = SDL_MIN_SINT32;
    }
    else
    {
        item->valueint = (int)number;
    }

    item->type = JSON_Number;

    input_buffer->offset += (size_t)(after_end - number_c_string);
    /* free the temporary buffer */
    SDL_free(number_c_string);
    return true;
}

/* don't ask me, but the original JSON_SetNumberValue returns an integer or double */
JSON_PUBLIC(double) JSON_SetNumberHelper(JSON_Node *object, double number)
{
    if (number >= SDL_MAX_SINT32)
    {
        object->valueint = SDL_MAX_SINT32;
    }
    else if (number <= (double)SDL_MIN_SINT32)
    {
        object->valueint = SDL_MIN_SINT32;
    }
    else
    {
        object->valueint = (int)number;
    }

    return object->valuedouble = number;
}

/* Note: when passing a NULL valuestring, JSON_SetValuestring treats this as an error and return NULL */
JSON_PUBLIC(char*) JSON_SetValuestring(JSON_Node *object, const char *valuestring)
{
    char *copy = NULL;
    size_t v1_len;
    size_t v2_len;
    /* if object's type is not JSON_String or is JSON_IsReference, it should not set valuestring */
    if ((object == NULL) || !(object->type & JSON_String) || (object->type & JSON_IsReference))
    {
        return NULL;
    }
    /* return NULL if the object is corrupted or valuestring is NULL */
    if (object->valuestring == NULL || valuestring == NULL)
    {
        return NULL;
    }

    v1_len = SDL_strlen(valuestring);
    v2_len = SDL_strlen(object->valuestring);

    if (v1_len <= v2_len)
    {
        /* strcpy does not handle overlapping string: [X1, X2] [Y1, Y2] => X2 < Y1 or Y2 < X1 */
        if (!( valuestring + v1_len < object->valuestring || object->valuestring + v2_len < valuestring ))
        {
            return NULL;
        }
        SDL_strlcpy(object->valuestring, valuestring, v2_len);
        return object->valuestring;
    }
    copy = (char*) SDL_strdup((const uint8_t*)valuestring);
    if (copy == NULL)
    {
        return NULL;
    }
    if (object->valuestring != NULL)
    {
        SDL_free(object->valuestring);
    }
    object->valuestring = copy;

    return copy;
}

typedef struct
{
    uint8_t *buffer;
    size_t length;
    size_t offset;
    size_t depth; /* current nesting depth (for formatted printing) */
    bool noalloc;
    bool format; /* is this print a formatted print */
} JSON_PrintBuffer;

/* realloc JSON_PrintBuffer if necessary to have at least "needed" bytes more */
static uint8_t* JSON_Ensure(JSON_PrintBuffer * const p, size_t needed)
{
    uint8_t *newbuffer = NULL;
    size_t newsize = 0;

    if ((p == NULL) || (p->buffer == NULL))
    {
        return NULL;
    }

    if ((p->length > 0) && (p->offset >= p->length))
    {
        /* make sure that offset is valid */
        return NULL;
    }

    if (needed > SDL_MAX_SINT32)
    {
        /* sizes bigger than SDL_MAX_SINT32 are currently not supported */
        return NULL;
    }

    needed += p->offset + 1;
    if (needed <= p->length)
    {
        return p->buffer + p->offset;
    }

    if (p->noalloc) {
        return NULL;
    }

    /* calculate new buffer size */
    if (needed > (SDL_MAX_SINT32 / 2))
    {
        /* overflow of int, use SDL_MAX_SINT32 if possible */
        if (needed <= SDL_MAX_SINT32)
        {
            newsize = SDL_MAX_SINT32;
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        newsize = needed * 2;
    }

    newbuffer = (uint8_t*)SDL_realloc(p->buffer, newsize);
    if (newbuffer == NULL)
    {
        SDL_free(p->buffer);
        p->length = 0;
        p->buffer = NULL;

        return NULL;
    }
    p->length = newsize;
    p->buffer = newbuffer;

    return newbuffer + p->offset;
}

/* calculate the new length of the string in a JSON_PrintBuffer and update the offset */
static void JSON_UpdateOffset(JSON_PrintBuffer * const buffer)
{
    const uint8_t *buffer_pointer = NULL;
    if ((buffer == NULL) || (buffer->buffer == NULL))
    {
        return;
    }
    buffer_pointer = buffer->buffer + buffer->offset;

    buffer->offset += SDL_strlen((const char*)buffer_pointer);
}

/* securely comparison of floating-point variables */
static bool JSON_CompareDouble(double a, double b)
{
    double maxVal = SDL_fabs(a) > SDL_fabs(b) ? SDL_fabs(a) : SDL_fabs(b);
    return (SDL_fabs(a - b) <= maxVal * DBL_EPSILON);
}

/* Render the number nicely from the given item into a string. */
static bool JSON_PrintNumber(const JSON_Node * const item, JSON_PrintBuffer * const output_buffer)
{
    uint8_t *output_pointer = NULL;
    double d = item->valuedouble;
    int length = 0;
    size_t i = 0;
    uint8_t number_buffer[26] = {0}; /* temporary buffer to print the number into */
    uint8_t decimal_point = JSON_GetDecimalPoint();
    double test = 0.0;

    if (output_buffer == NULL)
    {
        return false;
    }

    /* This checks for NaN and Infinity */
    if (SDL_isnan(d) || SDL_isinf(d))
    {
        length = SDL_snprintf((char*)number_buffer, SDL_arraysize(number_buffer), "null");
    }
    else if(d == (double)item->valueint)
    {
        length = SDL_snprintf((char*)number_buffer, SDL_arraysize(number_buffer), "%d", item->valueint);
    }
    else
    {
        /* Try 15 decimal places of precision to avoid nonsignificant nonzero digits */
        length = SDL_snprintf((char*)number_buffer, SDL_arraysize(number_buffer), "%1.15g", d);

        /* Check whether the original double can be recovered */
        if ((SDL_sscanf((char*)number_buffer, "%lg", &test) != 1) || !JSON_CompareDouble((double)test, d))
        {
            /* If not, print with 17 decimal places of precision */
            length = SDL_snprintf((char*)number_buffer, SDL_arraysize(number_buffer), "%1.17g", d);
        }
    }

    /* sprintf failed or buffer overrun occurred */
    if ((length < 0) || (length > (int32_t)(sizeof(number_buffer) - 1)))
    {
        return false;
    }

    /* reserve appropriate space in the output */
    output_pointer = JSON_Ensure(output_buffer, (size_t)length + sizeof(""));
    if (output_pointer == NULL)
    {
        return false;
    }

    /* copy the printed number to the output and replace locale
     * dependent decimal point with '.' */
    for (i = 0; i < ((size_t)length); i++)
    {
        if (number_buffer[i] == decimal_point)
        {
            output_pointer[i] = '.';
            continue;
        }

        output_pointer[i] = number_buffer[i];
    }
    output_pointer[i] = '\0';

    output_buffer->offset += (size_t)length;

    return true;
}

/* parse 4 digit hexadecimal number */
static uint32_t JSON_ParseHex4(const uint8_t * const input)
{
    uint32_t h = 0;
    size_t i = 0;

    for (i = 0; i < 4; i++)
    {
        /* parse digit */
        if ((input[i] >= '0') && (input[i] <= '9'))
        {
            h += (uint32_t) input[i] - '0';
        }
        else if ((input[i] >= 'A') && (input[i] <= 'F'))
        {
            h += (uint32_t) 10 + input[i] - 'A';
        }
        else if ((input[i] >= 'a') && (input[i] <= 'f'))
        {
            h += (uint32_t) 10 + input[i] - 'a';
        }
        else /* invalid */
        {
            return 0;
        }

        if (i < 3)
        {
            /* shift left to make place for the next nibble */
            h = h << 4;
        }
    }

    return h;
}

/* converts a UTF-16 literal to UTF-8
 * A literal can be one or two sequences of the form \uXXXX */
static uint8_t JSON_UTF16LiteralToUTF8(const uint8_t * const input_pointer, const uint8_t * const input_end, uint8_t **output_pointer)
{
    uint32_t codepoint = 0;
    uint32_t first_code = 0;
    const uint8_t *first_sequence = input_pointer;
    uint8_t utf8_length = 0;
    uint8_t utf8_position = 0;
    uint8_t sequence_length = 0;
    uint8_t first_byte_mark = 0;

    if ((input_end - first_sequence) < 6)
    {
        /* input ends unexpectedly */
        goto fail;
    }

    /* get the first utf16 sequence */
    first_code = JSON_ParseHex4(first_sequence + 2);

    /* check that the code is valid */
    if (((first_code >= 0xDC00) && (first_code <= 0xDFFF)))
    {
        goto fail;
    }

    /* UTF16 surrogate pair */
    if ((first_code >= 0xD800) && (first_code <= 0xDBFF))
    {
        const uint8_t *second_sequence = first_sequence + 6;
        uint32_t second_code = 0;
        sequence_length = 12; /* \uXXXX\uXXXX */

        if ((input_end - second_sequence) < 6)
        {
            /* input ends unexpectedly */
            goto fail;
        }

        if ((second_sequence[0] != '\\') || (second_sequence[1] != 'u'))
        {
            /* missing second half of the surrogate pair */
            goto fail;
        }

        /* get the second utf16 sequence */
        second_code = JSON_ParseHex4(second_sequence + 2);
        /* check that the code is valid */
        if ((second_code < 0xDC00) || (second_code > 0xDFFF))
        {
            /* invalid second half of the surrogate pair */
            goto fail;
        }


        /* calculate the unicode codepoint from the surrogate pair */
        codepoint = 0x10000 + (((first_code & 0x3FF) << 10) | (second_code & 0x3FF));
    }
    else
    {
        sequence_length = 6; /* \uXXXX */
        codepoint = first_code;
    }

    /* encode as UTF-8
     * takes at maximum 4 bytes to encode:
     * 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
    if (codepoint < 0x80)
    {
        /* normal ascii, encoding 0xxxxxxx */
        utf8_length = 1;
    }
    else if (codepoint < 0x800)
    {
        /* two bytes, encoding 110xxxxx 10xxxxxx */
        utf8_length = 2;
        first_byte_mark = 0xC0; /* 11000000 */
    }
    else if (codepoint < 0x10000)
    {
        /* three bytes, encoding 1110xxxx 10xxxxxx 10xxxxxx */
        utf8_length = 3;
        first_byte_mark = 0xE0; /* 11100000 */
    }
    else if (codepoint <= 0x10FFFF)
    {
        /* four bytes, encoding 1110xxxx 10xxxxxx 10xxxxxx 10xxxxxx */
        utf8_length = 4;
        first_byte_mark = 0xF0; /* 11110000 */
    }
    else
    {
        /* invalid unicode codepoint */
        goto fail;
    }

    /* encode as utf8 */
    for (utf8_position = (uint8_t)(utf8_length - 1); utf8_position > 0; utf8_position--)
    {
        /* 10xxxxxx */
        (*output_pointer)[utf8_position] = (uint8_t)((codepoint | 0x80) & 0xBF);
        codepoint >>= 6;
    }
    /* encode first byte */
    if (utf8_length > 1)
    {
        (*output_pointer)[0] = (uint8_t)((codepoint | first_byte_mark) & 0xFF);
    }
    else
    {
        (*output_pointer)[0] = (uint8_t)(codepoint & 0x7F);
    }

    *output_pointer += utf8_length;

    return sequence_length;

fail:
    return 0;
}

/* Parse the input text into an unescaped cinput, and populate item. */
static bool JSON_ParseString(JSON_Node * const item, JSON_ParseBuffer * const input_buffer)
{
    const uint8_t *input_pointer = JSON_BufferAtOffset(input_buffer) + 1;
    const uint8_t *input_end = JSON_BufferAtOffset(input_buffer) + 1;
    uint8_t *output_pointer = NULL;
    uint8_t *output = NULL;

    /* not a string */
    if (JSON_BufferAtOffset(input_buffer)[0] != '\"')
    {
        goto fail;
    }

    {
        /* calculate approximate size of the output (overestimate) */
        size_t allocation_length = 0;
        size_t skipped_bytes = 0;
        while (((size_t)(input_end - input_buffer->content) < input_buffer->length) && (*input_end != '\"'))
        {
            /* is escape sequence */
            if (input_end[0] == '\\')
            {
                if ((size_t)(input_end + 1 - input_buffer->content) >= input_buffer->length)
                {
                    /* prevent buffer overflow when last input character is a backslash */
                    goto fail;
                }
                skipped_bytes++;
                input_end++;
            }
            input_end++;
        }
        if (((size_t)(input_end - input_buffer->content) >= input_buffer->length) || (*input_end != '\"'))
        {
            goto fail; /* string ended unexpectedly */
        }

        /* This is at most how much we need for the output */
        allocation_length = (size_t) (input_end - JSON_BufferAtOffset(input_buffer)) - skipped_bytes;
        output = (uint8_t*)SDL_malloc(allocation_length + sizeof(""));
        if (output == NULL)
        {
            goto fail; /* allocation failure */
        }
    }

    output_pointer = output;
    /* loop through the string literal */
    while (input_pointer < input_end)
    {
        if (*input_pointer != '\\')
        {
            *output_pointer++ = *input_pointer++;
        }
        /* escape sequence */
        else
        {
            uint8_t sequence_length = 2;
            if ((input_end - input_pointer) < 1)
            {
                goto fail;
            }

            switch (input_pointer[1])
            {
                case 'b':
                    *output_pointer++ = '\b';
                    break;
                case 'f':
                    *output_pointer++ = '\f';
                    break;
                case 'n':
                    *output_pointer++ = '\n';
                    break;
                case 'r':
                    *output_pointer++ = '\r';
                    break;
                case 't':
                    *output_pointer++ = '\t';
                    break;
                case '\"':
                case '\\':
                case '/':
                    *output_pointer++ = input_pointer[1];
                    break;

                /* UTF-16 literal */
                case 'u':
                    sequence_length = JSON_UTF16LiteralToUTF8(input_pointer, input_end, &output_pointer);
                    if (sequence_length == 0)
                    {
                        /* failed to convert UTF16-literal to UTF-8 */
                        goto fail;
                    }
                    break;

                default:
                    goto fail;
            }
            input_pointer += sequence_length;
        }
    }

    /* zero terminate the output */
    *output_pointer = '\0';

    item->type = JSON_String;
    item->valuestring = (char*)output;

    input_buffer->offset = (size_t) (input_end - input_buffer->content);
    input_buffer->offset++;

    return true;

fail:
    if (output != NULL)
    {
        SDL_free(output);
        output = NULL;
    }

    if (input_pointer != NULL)
    {
        input_buffer->offset = (size_t)(input_pointer - input_buffer->content);
    }

    return false;
}

/* Render the cstring provided to an escaped version that can be printed. */
static bool JSON_PrintStringPtr(const uint8_t * const input, JSON_PrintBuffer * const output_buffer)
{
    const uint8_t *input_pointer = NULL;
    uint8_t *output = NULL;
    uint8_t *output_pointer = NULL;
    size_t output_length = 0;
    /* numbers of additional characters needed for escaping */
    size_t escape_characters = 0;

    if (output_buffer == NULL)
    {
        return false;
    }

    /* empty string */
    if (input == NULL)
    {
        output = JSON_Ensure(output_buffer, sizeof("\"\""));
        if (output == NULL)
        {
            return false;
        }
        SDL_strlcpy((char*)output, "\"\"", sizeof("\"\""));

        return true;
    }

    /* set "flag" to 1 if something needs to be escaped */
    for (input_pointer = input; *input_pointer; input_pointer++)
    {
        switch (*input_pointer)
        {
            case '\"':
            case '\\':
            case '\b':
            case '\f':
            case '\n':
            case '\r':
            case '\t':
                /* one character escape sequence */
                escape_characters++;
                break;
            default:
                if (*input_pointer < 32)
                {
                    /* UTF-16 escape sequence uXXXX */
                    escape_characters += 5;
                }
                break;
        }
    }
    output_length = (size_t)(input_pointer - input) + escape_characters;

    output = JSON_Ensure(output_buffer, output_length + sizeof("\"\""));
    if (output == NULL)
    {
        return false;
    }

    /* no characters have to be escaped */
    if (escape_characters == 0)
    {
        output[0] = '\"';
        SDL_memcpy(output + 1, input, output_length);
        output[output_length + 1] = '\"';
        output[output_length + 2] = '\0';

        return true;
    }

    output[0] = '\"';
    output_pointer = output + 1;
    /* copy the string */
    for (input_pointer = input; *input_pointer != '\0'; (void)input_pointer++, output_pointer++)
    {
        if ((*input_pointer > 31) && (*input_pointer != '\"') && (*input_pointer != '\\'))
        {
            /* normal character, copy */
            *output_pointer = *input_pointer;
        }
        else
        {
            /* character needs to be escaped */
            *output_pointer++ = '\\';
            switch (*input_pointer)
            {
                case '\\':
                    *output_pointer = '\\';
                    break;
                case '\"':
                    *output_pointer = '\"';
                    break;
                case '\b':
                    *output_pointer = 'b';
                    break;
                case '\f':
                    *output_pointer = 'f';
                    break;
                case '\n':
                    *output_pointer = 'n';
                    break;
                case '\r':
                    *output_pointer = 'r';
                    break;
                case '\t':
                    *output_pointer = 't';
                    break;
                default:
                    /* escape and print as unicode codepoint */
                    SDL_snprintf((char*)output_pointer, output_length, "u%04x", *input_pointer);
                    output_pointer += 4;
                    break;
            }
        }
    }
    output[output_length + 1] = '\"';
    output[output_length + 2] = '\0';

    return true;
}

/* Invoke JSON_PrintStringPtr (which is useful) on an item. */
static bool JSON_PrintString(const JSON_Node * const item, JSON_PrintBuffer * const p)
{
    return JSON_PrintStringPtr((uint8_t*)item->valuestring, p);
}

/* Predeclare these prototypes. */
static bool JSON_ParseValue(JSON_Node * const item, JSON_ParseBuffer * const input_buffer);
static bool JSON_PrintValue(const JSON_Node * const item, JSON_PrintBuffer * const output_buffer);
static bool JSON_ParseArray(JSON_Node * const item, JSON_ParseBuffer * const input_buffer);
static bool JSON_PrintArray(const JSON_Node * const item, JSON_PrintBuffer * const output_buffer);
static bool JSON_ParseObject(JSON_Node * const item, JSON_ParseBuffer * const input_buffer);
static bool JSON_PrintObject(const JSON_Node * const item, JSON_PrintBuffer * const output_buffer);

/* Utility to jump whitespace and cr/lf */
static JSON_ParseBuffer *JSON_BufferSkipWhitespace(JSON_ParseBuffer * const buffer)
{
    if ((buffer == NULL) || (buffer->content == NULL))
    {
        return NULL;
    }

    if (JSON_CannotAccessAtIndex(buffer, 0))
    {
        return buffer;
    }

    while (JSON_CanAccessAtIndex(buffer, 0) && (JSON_BufferAtOffset(buffer)[0] <= 32))
    {
       buffer->offset++;
    }

    if (buffer->offset == buffer->length)
    {
        buffer->offset--;
    }

    return buffer;
}

/* skip the UTF-8 BOM (byte order mark) if it is at the beginning of a buffer */
static JSON_ParseBuffer *JSON_SkipUTF8Bom(JSON_ParseBuffer * const buffer)
{
    if ((buffer == NULL) || (buffer->content == NULL) || (buffer->offset != 0))
    {
        return NULL;
    }

    if (JSON_CanAccessAtIndex(buffer, 4) && (SDL_strncmp((const char*)JSON_BufferAtOffset(buffer), "\xEF\xBB\xBF", 3) == 0))
    {
        buffer->offset += 3;
    }

    return buffer;
}

JSON_PUBLIC(JSON_Node *) JSON_ParseWithOpts(const char *value, const char **return_parse_end, bool require_null_terminated)
{
    size_t buffer_length;

    if (NULL == value)
    {
        return NULL;
    }

    /* Adding null character size due to require_null_terminated. */
    buffer_length = SDL_strlen(value) + sizeof("");

    return JSON_ParseWithLengthOpts(value, buffer_length, return_parse_end, require_null_terminated);
}

/* Parse an object - create a new root, and populate. */
JSON_PUBLIC(JSON_Node *) JSON_ParseWithLengthOpts(const char *value, size_t buffer_length, const char **return_parse_end, bool require_null_terminated)
{
    JSON_ParseBuffer buffer = { 0 };
    JSON_Node *item = NULL;

    /* reset error position */
    global_error.json = NULL;
    global_error.position = 0;

    if (value == NULL || 0 == buffer_length)
    {
        goto fail;
    }

    buffer.content = (const uint8_t*)value;
    buffer.length = buffer_length;
    buffer.offset = 0;

    item = (JSON_Node*)SDL_calloc(1, sizeof(JSON_Node));
    if (item == NULL) /* memory fail */
    {
        goto fail;
    }

    if (!JSON_ParseValue(item, JSON_BufferSkipWhitespace(JSON_SkipUTF8Bom(&buffer))))
    {
        /* parse failure. ep is set. */
        goto fail;
    }

    /* if we require null-terminated JSON without appended garbage, skip and then check for a null terminator */
    if (require_null_terminated)
    {
        JSON_BufferSkipWhitespace(&buffer);
        if ((buffer.offset >= buffer.length) || JSON_BufferAtOffset(&buffer)[0] != '\0')
        {
            goto fail;
        }
    }
    if (return_parse_end)
    {
        *return_parse_end = (const char*)JSON_BufferAtOffset(&buffer);
    }

    return item;

fail:
    if (item != NULL)
    {
        JSON_Delete(item);
    }

    if (value != NULL)
    {
        JSON_Error local_error;
        local_error.json = (const uint8_t*)value;
        local_error.position = 0;

        if (buffer.offset < buffer.length)
        {
            local_error.position = buffer.offset;
        }
        else if (buffer.length > 0)
        {
            local_error.position = buffer.length - 1;
        }

        if (return_parse_end != NULL)
        {
            *return_parse_end = (const char*)local_error.json + local_error.position;
        }

        global_error = local_error;
    }

    return NULL;
}

/* Default options for JSON_Parse */
JSON_PUBLIC(JSON_Node *) JSON_Parse(const char *value)
{
    return JSON_ParseWithOpts(value, 0, 0);
}

JSON_PUBLIC(JSON_Node *) JSON_ParseWithLength(const char *value, size_t buffer_length)
{
    return JSON_ParseWithLengthOpts(value, buffer_length, 0, 0);
}

#define cjson_min(a, b) (((a) < (b)) ? (a) : (b))

static uint8_t *JSON_Print(const JSON_Node * const item, bool format)
{
    static const size_t default_buffer_size = 256;
    JSON_PrintBuffer buffer[1];
    uint8_t *printed = NULL;

    memset(buffer, 0, sizeof(buffer));

    /* create buffer */
    buffer->buffer = (uint8_t*) SDL_malloc(default_buffer_size);
    buffer->length = default_buffer_size;
    buffer->format = format;
    if (buffer->buffer == NULL)
    {
        goto fail;
    }

    /* print the value */
    if (!JSON_PrintValue(item, buffer))
    {
        goto fail;
    }
    JSON_UpdateOffset(buffer);

    printed = (uint8_t*) SDL_realloc(buffer->buffer, buffer->offset + 1);
    if (printed == NULL) {
        goto fail;
    }
    buffer->buffer = NULL;

    return printed;

fail:
    if (buffer->buffer != NULL)
    {
        SDL_free(buffer->buffer);
        buffer->buffer = NULL;
    }

    if (printed != NULL)
    {
        SDL_free(printed);
        printed = NULL;
    }

    return NULL;
}

JSON_PUBLIC(char *) JSON_PrintBuffered(const JSON_Node *item, int prebuffer, bool fmt)
{
    JSON_PrintBuffer p = { 0 };

    if (prebuffer < 0)
    {
        return NULL;
    }

    p.buffer = (uint8_t*)SDL_malloc((size_t)prebuffer);
    if (!p.buffer)
    {
        return NULL;
    }

    p.length = (size_t)prebuffer;
    p.offset = 0;
    p.noalloc = false;
    p.format = fmt;

    if (!JSON_PrintValue(item, &p))
    {
        SDL_free(p.buffer);
        p.buffer = NULL;
        return NULL;
    }

    return (char*)p.buffer;
}

JSON_PUBLIC(bool) JSON_PrintPreallocated(JSON_Node *item, char *buffer, const int32_t length, const bool format)
{
    JSON_PrintBuffer p = { 0 };

    if ((length < 0) || (buffer == NULL))
    {
        return false;
    }

    p.buffer = (uint8_t*)buffer;
    p.length = (size_t)length;
    p.offset = 0;
    p.noalloc = true;
    p.format = format;

    return JSON_PrintValue(item, &p);
}

/* Parser core - when encountering text, process appropriately. */
static bool JSON_ParseValue(JSON_Node * const item, JSON_ParseBuffer * const input_buffer)
{
    if ((input_buffer == NULL) || (input_buffer->content == NULL))
    {
        return false; /* no input */
    }

    /* parse the different types of values */
    /* null */
    if (JSON_CanRead(input_buffer, 4) && (SDL_strncmp((const char*)JSON_BufferAtOffset(input_buffer), "null", 4) == 0))
    {
        item->type = JSON_NULL;
        input_buffer->offset += 4;
        return true;
    }
    /* false */
    if (JSON_CanRead(input_buffer, 5) && (SDL_strncmp((const char*)JSON_BufferAtOffset(input_buffer), "false", 5) == 0))
    {
        item->type = JSON_False;
        input_buffer->offset += 5;
        return true;
    }
    /* true */
    if (JSON_CanRead(input_buffer, 4) && (SDL_strncmp((const char*)JSON_BufferAtOffset(input_buffer), "true", 4) == 0))
    {
        item->type = JSON_True;
        item->valueint = 1;
        input_buffer->offset += 4;
        return true;
    }
    /* string */
    if (JSON_CanAccessAtIndex(input_buffer, 0) && (JSON_BufferAtOffset(input_buffer)[0] == '\"'))
    {
        return JSON_ParseString(item, input_buffer);
    }
    /* number */
    if (JSON_CanAccessAtIndex(input_buffer, 0) && ((JSON_BufferAtOffset(input_buffer)[0] == '-') || ((JSON_BufferAtOffset(input_buffer)[0] >= '0') && (JSON_BufferAtOffset(input_buffer)[0] <= '9'))))
    {
        return JSON_ParseNumber(item, input_buffer);
    }
    /* array */
    if (JSON_CanAccessAtIndex(input_buffer, 0) && (JSON_BufferAtOffset(input_buffer)[0] == '['))
    {
        return JSON_ParseArray(item, input_buffer);
    }
    /* object */
    if (JSON_CanAccessAtIndex(input_buffer, 0) && (JSON_BufferAtOffset(input_buffer)[0] == '{'))
    {
        return JSON_ParseObject(item, input_buffer);
    }

    return false;
}

/* Render a value to text. */
static bool JSON_PrintValue(const JSON_Node * const item, JSON_PrintBuffer * const output_buffer)
{
    uint8_t *output = NULL;

    if ((item == NULL) || (output_buffer == NULL))
    {
        return false;
    }

    switch ((item->type) & 0xFF)
    {
        case JSON_NULL:
            output = JSON_Ensure(output_buffer, 5);
            if (output == NULL)
            {
                return false;
            }
            SDL_strlcpy((char*)output, "null", 5);
            return true;

        case JSON_False:
            output = JSON_Ensure(output_buffer, 6);
            if (output == NULL)
            {
                return false;
            }
            SDL_strlcpy((char*)output, "false", 6);
            return true;

        case JSON_True:
            output = JSON_Ensure(output_buffer, 5);
            if (output == NULL)
            {
                return false;
            }
            SDL_strlcpy((char*)output, "true", 5);
            return true;

        case JSON_Number:
            return JSON_PrintNumber(item, output_buffer);

        case JSON_Raw:
        {
            size_t raw_length = 0;
            if (item->valuestring == NULL)
            {
                return false;
            }

            raw_length = SDL_strlen(item->valuestring) + sizeof("");
            output = JSON_Ensure(output_buffer, raw_length);
            if (output == NULL)
            {
                return false;
            }
            SDL_memcpy(output, item->valuestring, raw_length);
            return true;
        }

        case JSON_String:
            return JSON_PrintString(item, output_buffer);

        case JSON_Array:
            return JSON_PrintArray(item, output_buffer);

        case JSON_Object:
            return JSON_PrintObject(item, output_buffer);

        default:
            return false;
    }
}

/* Build an array from input text. */
static bool JSON_ParseArray(JSON_Node * const item, JSON_ParseBuffer * const input_buffer)
{
    JSON_Node *head = NULL; /* head of the linked list */
    JSON_Node *current_item = NULL;

    if (input_buffer->depth >= JSON_NESTING_LIMIT)
    {
        return false; /* to deeply nested */
    }
    input_buffer->depth++;

    if (JSON_BufferAtOffset(input_buffer)[0] != '[')
    {
        /* not an array */
        goto fail;
    }

    input_buffer->offset++;
    JSON_BufferSkipWhitespace(input_buffer);
    if (JSON_CanAccessAtIndex(input_buffer, 0) && (JSON_BufferAtOffset(input_buffer)[0] == ']'))
    {
        /* empty array */
        goto success;
    }

    /* check if we skipped to the end of the buffer */
    if (JSON_CannotAccessAtIndex(input_buffer, 0))
    {
        input_buffer->offset--;
        goto fail;
    }

    /* step back to character in front of the first element */
    input_buffer->offset--;
    /* loop through the comma separated array elements */
    do
    {
        /* allocate next item */
        JSON_Node *new_item = (JSON_Node*)SDL_calloc(1, sizeof(JSON_Node));
        if (new_item == NULL)
        {
            goto fail; /* allocation failure */
        }

        /* attach next item to list */
        if (head == NULL)
        {
            /* start the linked list */
            current_item = head = new_item;
        }
        else
        {
            /* add to the end and advance */
            current_item->next = new_item;
            new_item->prev = current_item;
            current_item = new_item;
        }

        /* parse next value */
        input_buffer->offset++;
        JSON_BufferSkipWhitespace(input_buffer);
        if (!JSON_ParseValue(current_item, input_buffer))
        {
            goto fail; /* failed to parse value */
        }
        JSON_BufferSkipWhitespace(input_buffer);
    }
    while (JSON_CanAccessAtIndex(input_buffer, 0) && (JSON_BufferAtOffset(input_buffer)[0] == ','));

    if (JSON_CannotAccessAtIndex(input_buffer, 0) || JSON_BufferAtOffset(input_buffer)[0] != ']')
    {
        goto fail; /* expected end of array */
    }

success:
    input_buffer->depth--;

    if (head != NULL) {
        head->prev = current_item;
    }

    item->type = JSON_Array;
    item->child = head;

    input_buffer->offset++;

    return true;

fail:
    if (head != NULL)
    {
        JSON_Delete(head);
    }

    return false;
}

/* Render an array to text */
static bool JSON_PrintArray(const JSON_Node * const item, JSON_PrintBuffer * const output_buffer)
{
    uint8_t *output_pointer = NULL;
    size_t length = 0;
    JSON_Node *current_element = item->child;

    if (output_buffer == NULL)
    {
        return false;
    }

    /* Compose the output array. */
    /* opening square bracket */
    output_pointer = JSON_Ensure(output_buffer, 1);
    if (output_pointer == NULL)
    {
        return false;
    }

    *output_pointer = '[';
    output_buffer->offset++;
    output_buffer->depth++;

    while (current_element != NULL)
    {
        if (!JSON_PrintValue(current_element, output_buffer))
        {
            return false;
        }
        JSON_UpdateOffset(output_buffer);
        if (current_element->next)
        {
            length = (size_t) (output_buffer->format ? 2 : 1);
            output_pointer = JSON_Ensure(output_buffer, length + 1);
            if (output_pointer == NULL)
            {
                return false;
            }
            *output_pointer++ = ',';
            if(output_buffer->format)
            {
                *output_pointer++ = ' ';
            }
            *output_pointer = '\0';
            output_buffer->offset += length;
        }
        current_element = current_element->next;
    }

    output_pointer = JSON_Ensure(output_buffer, 2);
    if (output_pointer == NULL)
    {
        return false;
    }
    *output_pointer++ = ']';
    *output_pointer = '\0';
    output_buffer->depth--;

    return true;
}

/* Build an object from the text. */
static bool JSON_ParseObject(JSON_Node * const item, JSON_ParseBuffer * const input_buffer)
{
    JSON_Node *head = NULL; /* linked list head */
    JSON_Node *current_item = NULL;

    if (input_buffer->depth >= JSON_NESTING_LIMIT)
    {
        return false; /* to deeply nested */
    }
    input_buffer->depth++;

    if (JSON_CannotAccessAtIndex(input_buffer, 0) || (JSON_BufferAtOffset(input_buffer)[0] != '{'))
    {
        goto fail; /* not an object */
    }

    input_buffer->offset++;
    JSON_BufferSkipWhitespace(input_buffer);
    if (JSON_CanAccessAtIndex(input_buffer, 0) && (JSON_BufferAtOffset(input_buffer)[0] == '}'))
    {
        goto success; /* empty object */
    }

    /* check if we skipped to the end of the buffer */
    if (JSON_CannotAccessAtIndex(input_buffer, 0))
    {
        input_buffer->offset--;
        goto fail;
    }

    /* step back to character in front of the first element */
    input_buffer->offset--;
    /* loop through the comma separated array elements */
    do
    {
        /* allocate next item */
        JSON_Node *new_item = (JSON_Node*)SDL_calloc(1, sizeof(JSON_Node));
        if (new_item == NULL)
        {
            goto fail; /* allocation failure */
        }

        /* attach next item to list */
        if (head == NULL)
        {
            /* start the linked list */
            current_item = head = new_item;
        }
        else
        {
            /* add to the end and advance */
            current_item->next = new_item;
            new_item->prev = current_item;
            current_item = new_item;
        }

        if (JSON_CannotAccessAtIndex(input_buffer, 1))
        {
            goto fail; /* nothing comes after the comma */
        }

        /* parse the name of the child */
        input_buffer->offset++;
        JSON_BufferSkipWhitespace(input_buffer);
        if (!JSON_ParseString(current_item, input_buffer))
        {
            goto fail; /* failed to parse name */
        }
        JSON_BufferSkipWhitespace(input_buffer);

        /* swap valuestring and string, because we parsed the name */
        current_item->string = current_item->valuestring;
        current_item->valuestring = NULL;

        if (JSON_CannotAccessAtIndex(input_buffer, 0) || (JSON_BufferAtOffset(input_buffer)[0] != ':'))
        {
            goto fail; /* invalid object */
        }

        /* parse the value */
        input_buffer->offset++;
        JSON_BufferSkipWhitespace(input_buffer);
        if (!JSON_ParseValue(current_item, input_buffer))
        {
            goto fail; /* failed to parse value */
        }
        JSON_BufferSkipWhitespace(input_buffer);
    }
    while (JSON_CanAccessAtIndex(input_buffer, 0) && (JSON_BufferAtOffset(input_buffer)[0] == ','));

    if (JSON_CannotAccessAtIndex(input_buffer, 0) || (JSON_BufferAtOffset(input_buffer)[0] != '}'))
    {
        goto fail; /* expected end of object */
    }

success:
    input_buffer->depth--;

    if (head != NULL) {
        head->prev = current_item;
    }

    item->type = JSON_Object;
    item->child = head;

    input_buffer->offset++;
    return true;

fail:
    if (head != NULL)
    {
        JSON_Delete(head);
    }

    return false;
}

/* Render an object to text. */
static bool JSON_PrintObject(const JSON_Node * const item, JSON_PrintBuffer * const output_buffer)
{
    uint8_t *output_pointer = NULL;
    size_t length = 0;
    JSON_Node *current_item = item->child;

    if (output_buffer == NULL)
    {
        return false;
    }

    /* Compose the output: */
    length = (size_t) (output_buffer->format ? 2 : 1); /* fmt: {\n */
    output_pointer = JSON_Ensure(output_buffer, length + 1);
    if (output_pointer == NULL)
    {
        return false;
    }

    *output_pointer++ = '{';
    output_buffer->depth++;
    if (output_buffer->format)
    {
        *output_pointer++ = '\n';
    }
    output_buffer->offset += length;

    while (current_item)
    {
        if (output_buffer->format)
        {
            size_t i;
            output_pointer = JSON_Ensure(output_buffer, output_buffer->depth);
            if (output_pointer == NULL)
            {
                return false;
            }
            for (i = 0; i < output_buffer->depth; i++)
            {
                *output_pointer++ = '\t';
            }
            output_buffer->offset += output_buffer->depth;
        }

        /* print key */
        if (!JSON_PrintStringPtr((uint8_t*)current_item->string, output_buffer))
        {
            return false;
        }
        JSON_UpdateOffset(output_buffer);

        length = (size_t) (output_buffer->format ? 2 : 1);
        output_pointer = JSON_Ensure(output_buffer, length);
        if (output_pointer == NULL)
        {
            return false;
        }
        *output_pointer++ = ':';
        if (output_buffer->format)
        {
            *output_pointer++ = '\t';
        }
        output_buffer->offset += length;

        /* print value */
        if (!JSON_PrintValue(current_item, output_buffer))
        {
            return false;
        }
        JSON_UpdateOffset(output_buffer);

        /* print comma if not last */
        length = ((size_t)(output_buffer->format ? 1 : 0) + (size_t)(current_item->next ? 1 : 0));
        output_pointer = JSON_Ensure(output_buffer, length + 1);
        if (output_pointer == NULL)
        {
            return false;
        }
        if (current_item->next)
        {
            *output_pointer++ = ',';
        }

        if (output_buffer->format)
        {
            *output_pointer++ = '\n';
        }
        *output_pointer = '\0';
        output_buffer->offset += length;

        current_item = current_item->next;
    }

    output_pointer = JSON_Ensure(output_buffer, output_buffer->format ? (output_buffer->depth + 1) : 2);
    if (output_pointer == NULL)
    {
        return false;
    }
    if (output_buffer->format)
    {
        size_t i;
        for (i = 0; i < (output_buffer->depth - 1); i++)
        {
            *output_pointer++ = '\t';
        }
    }
    *output_pointer++ = '}';
    *output_pointer = '\0';
    output_buffer->depth--;

    return true;
}

/* Get Array size/item / object item. */
JSON_PUBLIC(int) JSON_GetArraySize(const JSON_Node *array)
{
    JSON_Node *child = NULL;
    size_t size = 0;

    if (array == NULL)
    {
        return 0;
    }

    child = array->child;

    while(child != NULL)
    {
        size++;
        child = child->next;
    }

    /* FIXME: Can overflow here. Cannot be fixed without breaking the API */

    return (int)size;
}

JSON_PUBLIC(JSON_Node*) JSON_GetArrayItem(const JSON_Node *array, ssize_t index)
{
    if (index < 0) return NULL;

    JSON_Node *current_child = NULL;

    if (array == NULL)
    {
        return NULL;
    }

    current_child = array->child;
    while ((current_child != NULL) && (index > 0))
    {
        index--;
        current_child = current_child->next;
    }

    return current_child;
}

static JSON_Node *JSON_GetObjectItem(const JSON_Node * const object, const char * const name, const bool case_sensitive)
{
    JSON_Node *current_element = NULL;

    if ((object == NULL) || (name == NULL))
    {
        return NULL;
    }

    current_element = object->child;
    if (case_sensitive)
    {
        while ((current_element != NULL) && (current_element->string != NULL) && (SDL_strcmp(name, current_element->string) != 0))
        {
            current_element = current_element->next;
        }
    }
    else
    {
        while ((current_element != NULL) && (JSON_case_insensitive_strcmp((const uint8_t*)name, (const uint8_t*)(current_element->string)) != 0))
        {
            current_element = current_element->next;
        }
    }

    if ((current_element == NULL) || (current_element->string == NULL)) {
        return NULL;
    }

    return current_element;
}

JSON_PUBLIC(bool) JSON_HasObjectItem(const JSON_Node *object, const char *string)
{
    return JSON_GetObjectItem(object, string, false) ? 1 : 0;
}

/* Utility for array list handling. */
static void JSON_SuffixObject(JSON_Node *prev, JSON_Node *item)
{
    prev->next = item;
    item->prev = prev;
}

/* Utility for handling references. */
static JSON_Node *JSON_CreateReference(const JSON_Node *item)
{
    JSON_Node *reference = NULL;
    if (item == NULL)
    {
        return NULL;
    }

    reference = (JSON_Node*)SDL_calloc(1, sizeof(JSON_Node));
    if (reference == NULL)
    {
        return NULL;
    }

    SDL_memcpy(reference, item, sizeof(JSON_Node));
    reference->string = NULL;
    reference->type |= JSON_IsReference;
    reference->next = reference->prev = NULL;
    return reference;
}

JSON_PUBLIC(bool) JSON_AddItemToArray(JSON_Node *array, JSON_Node *item)
{
    JSON_Node *child = NULL;

    if ((item == NULL) || (array == NULL) || (array == item))
    {
        return false;
    }

    child = array->child;
    /*
     * To find the last item in array quickly, we use prev in array
     */
    if (child == NULL)
    {
        /* list is empty, start new one */
        array->child = item;
        item->prev = item;
        item->next = NULL;
    }
    else
    {
        /* append to the end */
        if (child->prev)
        {
            JSON_SuffixObject(child->prev, item);
            array->child->prev = item;
        }
    }

    return true;
}

#if defined(__clang__) || (defined(__GNUC__)  && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
    #pragma GCC diagnostic push
#endif
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wcast-qual"
#endif
/* helper function to cast away const */
static void* JSON_CastAwayConst(const void* string)
{
    return (void*)string;
}
#if defined(__clang__) || (defined(__GNUC__)  && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
    #pragma GCC diagnostic pop
#endif


JSON_PUBLIC(bool) JSON_AddItemToObject(JSON_Node * const object, const char * const string, JSON_Node * const item, const bool constant_key)
{
    char *new_key = NULL;
    int32_t new_type = JSON_Invalid;

    if ((object == NULL) || (string == NULL) || (item == NULL) || (object == item))
    {
        return false;
    }

    if (constant_key)
    {
        new_key = (char*)JSON_CastAwayConst(string);
        new_type = item->type | JSON_StringIsConst;
    }
    else
    {
        new_key = (char*)SDL_strdup((const uint8_t*)string);
        if (new_key == NULL)
        {
            return false;
        }

        new_type = item->type & ~JSON_StringIsConst;
    }

    if (!(item->type & JSON_StringIsConst) && (item->string != NULL))
    {
        SDL_free(item->string);
    }

    item->string = new_key;
    item->type = new_type;

    return JSON_AddItemToArray(object, item);
}

JSON_PUBLIC(bool) JSON_AddItemReferenceToArray(JSON_Node *array, JSON_Node *item)
{
    if (array == NULL)
    {
        return false;
    }

    return JSON_AddItemToArray(array, JSON_CreateReference(item));
}

JSON_PUBLIC(bool) JSON_AddItemReferenceToObject(JSON_Node *object, const char *string, JSON_Node *item)
{
    if ((object == NULL) || (string == NULL))
    {
        return false;
    }

    return JSON_AddItemToObject(object, string, JSON_CreateReference(item), false);
}

JSON_PUBLIC(JSON_Node*) JSON_AddNullToObject(JSON_Node * const object, const char * const name)
{
    JSON_Node *null = JSON_CreateNull();
    if (JSON_AddItemToObject(object, name, null, false))
    {
        return null;
    }

    JSON_Delete(null);
    return NULL;
}

JSON_PUBLIC(JSON_Node*) JSON_AddTrueToObject(JSON_Node * const object, const char * const name)
{
    JSON_Node *true_item = JSON_CreateTrue();
    if (JSON_AddItemToObject(object, name, true_item, false))
    {
        return true_item;
    }

    JSON_Delete(true_item);
    return NULL;
}

JSON_PUBLIC(JSON_Node*) JSON_AddFalseToObject(JSON_Node * const object, const char * const name)
{
    JSON_Node *false_item = JSON_CreateFalse();
    if (JSON_AddItemToObject(object, name, false_item, false))
    {
        return false_item;
    }

    JSON_Delete(false_item);
    return NULL;
}

JSON_PUBLIC(JSON_Node*) JSON_AddBoolToObject(JSON_Node * const object, const char * const name, const bool boolean)
{
    JSON_Node *bool_item = JSON_CreateBool(boolean);
    if (JSON_AddItemToObject(object, name, bool_item, false))
    {
        return bool_item;
    }

    JSON_Delete(bool_item);
    return NULL;
}

JSON_PUBLIC(JSON_Node*) JSON_AddNumberToObject(JSON_Node * const object, const char * const name, const double number)
{
    JSON_Node *number_item = JSON_CreateNumber(number);
    if (JSON_AddItemToObject(object, name, number_item, false))
    {
        return number_item;
    }

    JSON_Delete(number_item);
    return NULL;
}

JSON_PUBLIC(JSON_Node*) JSON_AddStringToObject(JSON_Node * const object, const char * const name, const char * const string)
{
    JSON_Node *string_item = JSON_CreateString(string);
    if (JSON_AddItemToObject(object, name, string_item, false))
    {
        return string_item;
    }

    JSON_Delete(string_item);
    return NULL;
}

JSON_PUBLIC(JSON_Node*) JSON_AddRawToObject(JSON_Node * const object, const char * const name, const char * const raw)
{
    JSON_Node *raw_item = JSON_CreateRaw(raw);
    if (JSON_AddItemToObject(object, name, raw_item, false))
    {
        return raw_item;
    }

    JSON_Delete(raw_item);
    return NULL;
}

JSON_PUBLIC(JSON_Node*) JSON_AddObjectToObject(JSON_Node * const object, const char * const name)
{
    JSON_Node *object_item = JSON_CreateObject();
    if (JSON_AddItemToObject(object, name, object_item, false))
    {
        return object_item;
    }

    JSON_Delete(object_item);
    return NULL;
}

JSON_PUBLIC(JSON_Node*) JSON_AddArrayToObject(JSON_Node * const object, const char * const name)
{
    JSON_Node *array = JSON_CreateArray();
    if (JSON_AddItemToObject(object, name, array, false))
    {
        return array;
    }

    JSON_Delete(array);
    return NULL;
}

JSON_PUBLIC(JSON_Node *) JSON_DetachItemViaPointer(JSON_Node *parent, JSON_Node * const item)
{
    if ((parent == NULL) || (item == NULL) || (item != parent->child && item->prev == NULL))
    {
        return NULL;
    }

    if (item != parent->child)
    {
        /* not the first element */
        item->prev->next = item->next;
    }
    if (item->next != NULL)
    {
        /* not the last element */
        item->next->prev = item->prev;
    }

    if (item == parent->child)
    {
        /* first element */
        parent->child = item->next;
    }
    else if (item->next == NULL)
    {
        /* last element */
        parent->child->prev = item->prev;
    }

    /* make sure the detached item doesn't point anywhere anymore */
    item->prev = NULL;
    item->next = NULL;

    return item;
}

JSON_PUBLIC(JSON_Node *) JSON_DetachItemFromArray(JSON_Node *array, int32_t which)
{
    if (which < 0)
    {
        return NULL;
    }

    return JSON_DetachItemViaPointer(array, JSON_GetArrayItem(array, (size_t)which));
}

JSON_PUBLIC(void) JSON_DeleteItemFromArray(JSON_Node *array, int32_t which)
{
    JSON_Delete(JSON_DetachItemFromArray(array, which));
}

JSON_PUBLIC(JSON_Node *) JSON_DetachItemFromObject(JSON_Node *object, const char *string)
{
    JSON_Node *to_detach = JSON_GetObjectItem(object, string, false);

    return JSON_DetachItemViaPointer(object, to_detach);
}

JSON_PUBLIC(JSON_Node *) JSON_DetachItemFromObjectCaseSensitive(JSON_Node *object, const char *string)
{
    JSON_Node *to_detach = JSON_GetObjectItem(object, string, true);

    return JSON_DetachItemViaPointer(object, to_detach);
}

JSON_PUBLIC(void) JSON_DeleteItemFromObject(JSON_Node *object, const char *string)
{
    JSON_Delete(JSON_DetachItemFromObject(object, string));
}

JSON_PUBLIC(void) JSON_DeleteItemFromObjectCaseSensitive(JSON_Node *object, const char *string)
{
    JSON_Delete(JSON_DetachItemFromObjectCaseSensitive(object, string));
}

/* Replace array/object items with new ones. */
JSON_PUBLIC(bool) JSON_InsertItemInArray(JSON_Node *array, int32_t which, JSON_Node *newitem)
{
    JSON_Node *after_inserted = NULL;

    if (which < 0 || newitem == NULL)
    {
        return false;
    }

    after_inserted = JSON_GetArrayItem(array, (size_t)which);
    if (after_inserted == NULL)
    {
        return JSON_AddItemToArray(array, newitem);
    }

    if (after_inserted != array->child && after_inserted->prev == NULL) {
        /* return false if after_inserted is a corrupted array item */
        return false;
    }

    newitem->next = after_inserted;
    newitem->prev = after_inserted->prev;
    after_inserted->prev = newitem;
    if (after_inserted == array->child)
    {
        array->child = newitem;
    }
    else
    {
        newitem->prev->next = newitem;
    }
    return true;
}

JSON_PUBLIC(bool) JSON_ReplaceItemViaPointer(JSON_Node * const parent, JSON_Node * const item, JSON_Node * replacement)
{
    if ((parent == NULL) || (parent->child == NULL) || (replacement == NULL) || (item == NULL))
    {
        return false;
    }

    if (replacement == item)
    {
        return true;
    }

    replacement->next = item->next;
    replacement->prev = item->prev;

    if (replacement->next != NULL)
    {
        replacement->next->prev = replacement;
    }
    if (parent->child == item)
    {
        if (parent->child->prev == parent->child)
        {
            replacement->prev = replacement;
        }
        parent->child = replacement;
    }
    else
    {   /*
         * To find the last item in array quickly, we use prev in array.
         * We can't modify the last item's next pointer where this item was the parent's child
         */
        if (replacement->prev != NULL)
        {
            replacement->prev->next = replacement;
        }
        if (replacement->next == NULL)
        {
            parent->child->prev = replacement;
        }
    }

    item->next = NULL;
    item->prev = NULL;
    JSON_Delete(item);

    return true;
}

JSON_PUBLIC(bool) JSON_ReplaceItemInArray(JSON_Node *array, int32_t which, JSON_Node *newitem)
{
    if (which < 0)
    {
        return false;
    }

    return JSON_ReplaceItemViaPointer(array, JSON_GetArrayItem(array, (size_t)which), newitem);
}

JSON_PUBLIC(bool) JSON_ReplaceItemInObject(JSON_Node *object, const char *string, JSON_Node *replacement, bool case_sensitive)
{
    if ((replacement == NULL) || (string == NULL))
    {
        return false;
    }

    /* replace the name in the replacement */
    if (!(replacement->type & JSON_StringIsConst) && (replacement->string != NULL))
    {
        SDL_free(replacement->string);
    }
    replacement->string = (char*)SDL_strdup((const uint8_t*)string);
    if (replacement->string == NULL)
    {
        return false;
    }

    replacement->type &= ~JSON_StringIsConst;

    return JSON_ReplaceItemViaPointer(object, JSON_GetObjectItem(object, string, case_sensitive), replacement);
}

/* Create basic types: */
JSON_PUBLIC(JSON_Node *) JSON_CreateNull(void)
{
    JSON_Node *item = (JSON_Node*)SDL_calloc(1, sizeof(JSON_Node));
    if(item)
    {
        item->type = JSON_NULL;
    }

    return item;
}

JSON_PUBLIC(JSON_Node *) JSON_CreateTrue(void)
{
    JSON_Node *item = (JSON_Node*)SDL_calloc(1, sizeof(JSON_Node));
    if(item)
    {
        item->type = JSON_True;
    }

    return item;
}

JSON_PUBLIC(JSON_Node *) JSON_CreateFalse(void)
{
    JSON_Node *item = (JSON_Node*)SDL_calloc(1, sizeof(JSON_Node));
    if(item)
    {
        item->type = JSON_False;
    }

    return item;
}

JSON_PUBLIC(JSON_Node *) JSON_CreateBool(bool boolean)
{
    JSON_Node *item = (JSON_Node*)SDL_calloc(1, sizeof(JSON_Node));
    if(item)
    {
        item->type = boolean ? JSON_True : JSON_False;
    }

    return item;
}

JSON_PUBLIC(JSON_Node *) JSON_CreateNumber(double num)
{
    JSON_Node *item = (JSON_Node*)SDL_calloc(1, sizeof(JSON_Node));
    if(item)
    {
        item->type = JSON_Number;
        item->valuedouble = num;

        /* use saturation in case of overflow */
        if (num >= SDL_MAX_SINT32)
        {
            item->valueint = SDL_MAX_SINT32;
        }
        else if (num <= (double)SDL_MIN_SINT32)
        {
            item->valueint = SDL_MIN_SINT32;
        }
        else
        {
            item->valueint = (int32_t)num;
        }
    }

    return item;
}

JSON_PUBLIC(JSON_Node *) JSON_CreateString(const char *string)
{
    JSON_Node *item = (JSON_Node*)SDL_calloc(1, sizeof(JSON_Node));
    if(item)
    {
        item->type = JSON_String;
        item->valuestring = (char*)SDL_strdup((const uint8_t*)string);
        if(!item->valuestring)
        {
            JSON_Delete(item);
            return NULL;
        }
    }

    return item;
}

JSON_PUBLIC(JSON_Node *) JSON_CreateStringReference(const char *string)
{
    JSON_Node *item = (JSON_Node*)SDL_calloc(1, sizeof(JSON_Node));
    if (item != NULL)
    {
        item->type = JSON_String | JSON_IsReference;
        item->valuestring = (char*)JSON_CastAwayConst(string);
    }

    return item;
}

JSON_PUBLIC(JSON_Node *) JSON_CreateObjectReference(const JSON_Node *child)
{
    JSON_Node *item = (JSON_Node*)SDL_calloc(1, sizeof(JSON_Node));
    if (item != NULL) {
        item->type = JSON_Object | JSON_IsReference;
        item->child = (JSON_Node*)JSON_CastAwayConst(child);
    }

    return item;
}

JSON_PUBLIC(JSON_Node *) JSON_CreateArrayReference(const JSON_Node *child) {
    JSON_Node *item = (JSON_Node*)SDL_calloc(1, sizeof(JSON_Node));
    if (item != NULL) {
        item->type = JSON_Array | JSON_IsReference;
        item->child = (JSON_Node*)JSON_CastAwayConst(child);
    }

    return item;
}

JSON_PUBLIC(JSON_Node *) JSON_CreateRaw(const char *raw)
{
    JSON_Node *item = (JSON_Node*)SDL_calloc(1, sizeof(JSON_Node));
    if(item)
    {
        item->type = JSON_Raw;
        item->valuestring = (char*)SDL_strdup((const uint8_t*)raw);
        if(!item->valuestring)
        {
            JSON_Delete(item);
            return NULL;
        }
    }

    return item;
}

JSON_PUBLIC(JSON_Node *) JSON_CreateArray(void)
{
    JSON_Node *item = (JSON_Node*)SDL_calloc(1, sizeof(JSON_Node));
    if(item)
    {
        item->type=JSON_Array;
    }

    return item;
}

JSON_PUBLIC(JSON_Node *) JSON_CreateObject(void)
{
    JSON_Node *item = (JSON_Node*)SDL_calloc(1, sizeof(JSON_Node));
    if (item)
    {
        item->type = JSON_Object;
    }

    return item;
}

/* Create Arrays: */
JSON_PUBLIC(JSON_Node *) JSON_CreateIntArray(const int32_t *numbers, int32_t count)
{
    size_t i = 0;
    JSON_Node *n = NULL;
    JSON_Node *p = NULL;
    JSON_Node *a = NULL;

    if ((count < 0) || (numbers == NULL))
    {
        return NULL;
    }

    a = JSON_CreateArray();

    for(i = 0; a && (i < (size_t)count); i++)
    {
        n = JSON_CreateNumber(numbers[i]);
        if (!n)
        {
            JSON_Delete(a);
            return NULL;
        }
        if(!i)
        {
            a->child = n;
        }
        else
        {
            JSON_SuffixObject(p, n);
        }
        p = n;
    }

    if (a && a->child) {
        a->child->prev = n;
    }

    return a;
}

JSON_PUBLIC(JSON_Node *) JSON_CreateFloatArray(const float *numbers, int32_t count)
{
    size_t i = 0;
    JSON_Node *n = NULL;
    JSON_Node *p = NULL;
    JSON_Node *a = NULL;

    if ((count < 0) || (numbers == NULL))
    {
        return NULL;
    }

    a = JSON_CreateArray();

    for(i = 0; a && (i < (size_t)count); i++)
    {
        n = JSON_CreateNumber((double)numbers[i]);
        if(!n)
        {
            JSON_Delete(a);
            return NULL;
        }
        if(!i)
        {
            a->child = n;
        }
        else
        {
            JSON_SuffixObject(p, n);
        }
        p = n;
    }

    if (a && a->child) {
        a->child->prev = n;
    }

    return a;
}

JSON_PUBLIC(JSON_Node *) JSON_CreateDoubleArray(const double *numbers, int32_t count)
{
    size_t i = 0;
    JSON_Node *n = NULL;
    JSON_Node *p = NULL;
    JSON_Node *a = NULL;

    if ((count < 0) || (numbers == NULL))
    {
        return NULL;
    }

    a = JSON_CreateArray();

    for(i = 0; a && (i < (size_t)count); i++)
    {
        n = JSON_CreateNumber(numbers[i]);
        if(!n)
        {
            JSON_Delete(a);
            return NULL;
        }
        if(!i)
        {
            a->child = n;
        }
        else
        {
            JSON_SuffixObject(p, n);
        }
        p = n;
    }

    if (a && a->child) {
        a->child->prev = n;
    }

    return a;
}

JSON_PUBLIC(JSON_Node *) JSON_CreateStringArray(const char *const *strings, int32_t count)
{
    size_t i = 0;
    JSON_Node *n = NULL;
    JSON_Node *p = NULL;
    JSON_Node *a = NULL;

    if ((count < 0) || (strings == NULL))
    {
        return NULL;
    }

    a = JSON_CreateArray();

    for (i = 0; a && (i < (size_t)count); i++)
    {
        n = JSON_CreateString(strings[i]);
        if(!n)
        {
            JSON_Delete(a);
            return NULL;
        }
        if(!i)
        {
            a->child = n;
        }
        else
        {
            JSON_SuffixObject(p,n);
        }
        p = n;
    }

    if (a && a->child) {
        a->child->prev = n;
    }

    return a;
}

/* Duplication */
JSON_Node * JSON_DuplicateRec(const JSON_Node *item, size_t depth, bool recurse);

JSON_PUBLIC(JSON_Node *) JSON_Duplicate(const JSON_Node *item, bool recurse)
{
    return JSON_DuplicateRec(item, 0, recurse );
}

JSON_Node * JSON_DuplicateRec(const JSON_Node *item, size_t depth, bool recurse)
{
    JSON_Node *newitem = NULL;
    JSON_Node *child = NULL;
    JSON_Node *next = NULL;
    JSON_Node *newchild = NULL;

    /* Bail on bad ptr */
    if (!item)
    {
        goto fail;
    }
    /* Create new item */
    newitem = (JSON_Node*)SDL_calloc(1, sizeof(JSON_Node));
    if (!newitem)
    {
        goto fail;
    }
    /* Copy over all vars */
    newitem->type = item->type & (~JSON_IsReference);
    newitem->valueint = item->valueint;
    newitem->valuedouble = item->valuedouble;
    if (item->valuestring)
    {
        newitem->valuestring = (char*)SDL_strdup((uint8_t*)item->valuestring);
        if (!newitem->valuestring)
        {
            goto fail;
        }
    }
    if (item->string)
    {
        newitem->string = (item->type&JSON_StringIsConst) ? item->string : (char*)SDL_strdup((uint8_t*)item->string);
        if (!newitem->string)
        {
            goto fail;
        }
    }
    /* If non-recursive, then we're done! */
    if (!recurse)
    {
        return newitem;
    }
    /* Walk the ->next chain for the child. */
    child = item->child;
    while (child != NULL)
    {
        if(depth >= JSON_CIRCULAR_LIMIT) {
            goto fail;
        }
        newchild = JSON_DuplicateRec(child, depth + 1, true); /* Duplicate (with recurse) each item in the ->next chain */
        if (!newchild)
        {
            goto fail;
        }
        if (next != NULL)
        {
            /* If newitem->child already set, then crosswire ->prev and ->next and move on */
            next->next = newchild;
            newchild->prev = next;
            next = newchild;
        }
        else
        {
            /* Set newitem->child and move to it */
            newitem->child = newchild;
            next = newchild;
        }
        child = child->next;
    }
    if (newitem && newitem->child)
    {
        newitem->child->prev = newchild;
    }

    return newitem;

fail:
    if (newitem != NULL)
    {
        JSON_Delete(newitem);
    }

    return NULL;
}

static void JSON_SkipOnelineComment(char **input)
{
    *input += JSON_static_strlen("//");

    for (; (*input)[0] != '\0'; ++(*input))
    {
        if ((*input)[0] == '\n') {
            *input += JSON_static_strlen("\n");
            return;
        }
    }
}

static void JSON_SkipMultilineComment(char **input)
{
    *input += JSON_static_strlen("/*");

    for (; (*input)[0] != '\0'; ++(*input))
    {
        if (((*input)[0] == '*') && ((*input)[1] == '/'))
        {
            *input += JSON_static_strlen("*/");
            return;
        }
    }
}

static void JSON_MinifyString(char **input, char **output) {
    (*output)[0] = (*input)[0];
    *input += JSON_static_strlen("\"");
    *output += JSON_static_strlen("\"");


    for (; (*input)[0] != '\0'; (void)++(*input), ++(*output)) {
        (*output)[0] = (*input)[0];

        if ((*input)[0] == '\"') {
            (*output)[0] = '\"';
            *input += JSON_static_strlen("\"");
            *output += JSON_static_strlen("\"");
            return;
        } else if (((*input)[0] == '\\') && ((*input)[1] == '\"')) {
            (*output)[1] = (*input)[1];
            *input += JSON_static_strlen("\"");
            *output += JSON_static_strlen("\"");
        }
    }
}

JSON_PUBLIC(void) JSON_Minify(char *json)
{
    char *into = json;

    if (json == NULL)
    {
        return;
    }

    while (json[0] != '\0')
    {
        switch (json[0])
        {
            case ' ':
            case '\t':
            case '\r':
            case '\n':
                json++;
                break;

            case '/':
                if (json[1] == '/')
                {
                    JSON_SkipOnelineComment(&json);
                }
                else if (json[1] == '*')
                {
                    JSON_SkipMultilineComment(&json);
                } else {
                    json++;
                }
                break;

            case '\"':
                JSON_MinifyString(&json, (char**)&into);
                break;

            default:
                into[0] = json[0];
                json++;
                into++;
        }
    }

    /* and null-terminate. */
    *into = '\0';
}

JSON_PUBLIC(bool) JSON_IsInvalid(const JSON_Node * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & 0xFF) == JSON_Invalid;
}

JSON_PUBLIC(bool) JSON_IsFalse(const JSON_Node * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & 0xFF) == JSON_False;
}

JSON_PUBLIC(bool) JSON_IsTrue(const JSON_Node * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & 0xff) == JSON_True;
}


JSON_PUBLIC(bool) JSON_IsBool(const JSON_Node * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & (JSON_True | JSON_False)) != 0;
}
JSON_PUBLIC(bool) JSON_IsNull(const JSON_Node * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & 0xFF) == JSON_NULL;
}

JSON_PUBLIC(bool) JSON_IsNumber(const JSON_Node * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & 0xFF) == JSON_Number;
}

JSON_PUBLIC(bool) JSON_IsString(const JSON_Node * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & 0xFF) == JSON_String;
}

JSON_PUBLIC(bool) JSON_IsArray(const JSON_Node * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & 0xFF) == JSON_Array;
}

JSON_PUBLIC(bool) JSON_IsObject(const JSON_Node * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & 0xFF) == JSON_Object;
}

JSON_PUBLIC(bool) JSON_IsRaw(const JSON_Node * const item)
{
    if (item == NULL)
    {
        return false;
    }

    return (item->type & 0xFF) == JSON_Raw;
}

JSON_PUBLIC(bool) JSON_Compare(const JSON_Node * const a, const JSON_Node * const b, const bool case_sensitive)
{
    if ((a == NULL) || (b == NULL) || ((a->type & 0xFF) != (b->type & 0xFF)))
    {
        return false;
    }

    /* check if type is valid */
    switch (a->type & 0xFF)
    {
        case JSON_False:
        case JSON_True:
        case JSON_NULL:
        case JSON_Number:
        case JSON_String:
        case JSON_Raw:
        case JSON_Array:
        case JSON_Object:
            break;

        default:
            return false;
    }

    /* identical objects are equal */
    if (a == b)
    {
        return true;
    }

    switch (a->type & 0xFF)
    {
        /* in these cases and equal type is enough */
        case JSON_False:
        case JSON_True:
        case JSON_NULL:
            return true;

        case JSON_Number:
            if (JSON_CompareDouble(a->valuedouble, b->valuedouble))
            {
                return true;
            }
            return false;

        case JSON_String:
        case JSON_Raw:
            if ((a->valuestring == NULL) || (b->valuestring == NULL))
            {
                return false;
            }
            if (SDL_strcmp(a->valuestring, b->valuestring) == 0)
            {
                return true;
            }

            return false;

        case JSON_Array:
        {
            JSON_Node *a_element = a->child;
            JSON_Node *b_element = b->child;

            for (; (a_element != NULL) && (b_element != NULL);)
            {
                if (!JSON_Compare(a_element, b_element, case_sensitive))
                {
                    return false;
                }

                a_element = a_element->next;
                b_element = b_element->next;
            }

            /* one of the arrays is longer than the other */
            if (a_element != b_element) {
                return false;
            }

            return true;
        }

        case JSON_Object:
        {
            JSON_Node *a_element = NULL;
            JSON_Node *b_element = NULL;
            JSON_ArrayForEach(a_element, a)
            {
                /* TODO This has O(n^2) runtime, which is horrible! */
                b_element = JSON_GetObjectItem(b, a_element->string, case_sensitive);
                if (b_element == NULL)
                {
                    return false;
                }

                if (!JSON_Compare(a_element, b_element, case_sensitive))
                {
                    return false;
                }
            }

            /* doing this twice, once on a and b to prevent true comparison if a subset of b
             * TODO: Do this the proper way, this is just a fix for now */
            JSON_ArrayForEach(b_element, b)
            {
                a_element = JSON_GetObjectItem(a, b_element->string, case_sensitive);
                if (a_element == NULL)
                {
                    return false;
                }

                if (!JSON_Compare(b_element, a_element, case_sensitive))
                {
                    return false;
                }
            }

            return true;
        }

        default:
            return false;
    }
}