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

#ifndef JSON__h
#define JSON__h

#ifdef __cplusplus
extern "C"
{
#endif

#if !defined(__WINDOWS__) && (defined(WIN32) || defined(WIN64) || defined(_MSC_VER) || defined(_WIN32))
#define __WINDOWS__
#endif

#ifdef __WINDOWS__

/* When compiling for windows, we specify a specific calling convention to avoid issues where we are being called from a project with a different default calling convention.  For windows you have 3 define options:

JSON_HIDE_SYMBOLS - Define this in the case where you don't want to ever dllexport symbols
JSON_EXPORT_SYMBOLS - Define this on library build when you want to dllexport symbols (default)
JSON_IMPORT_SYMBOLS - Define this if you want to dllimport symbol

For *nix builds that support visibility attribute, you can define similar behavior by

setting default visibility to hidden by adding
-fvisibility=hidden (for gcc)
or
-xldscope=hidden (for sun cc)
to CFLAGS

then using the JSON_API_VISIBILITY flag to "export" the same symbols the way JSON_EXPORT_SYMBOLS does

*/

#define JSON_CDECL __cdecl
#define JSON_STDCALL __stdcall

/* export symbols by default, this is necessary for copy pasting the C and header file */
#if !defined(JSON_HIDE_SYMBOLS) && !defined(JSON_IMPORT_SYMBOLS) && !defined(JSON_EXPORT_SYMBOLS)
#define JSON_EXPORT_SYMBOLS
#endif

#if defined(JSON_HIDE_SYMBOLS)
#define JSON_PUBLIC(type)   type JSON_STDCALL
#elif defined(JSON_EXPORT_SYMBOLS)
#define JSON_PUBLIC(type)   __declspec(dllexport) type JSON_STDCALL
#elif defined(JSON_IMPORT_SYMBOLS)
#define JSON_PUBLIC(type)   __declspec(dllimport) type JSON_STDCALL
#endif
#else /* !__WINDOWS__ */
#define JSON_CDECL
#define JSON_STDCALL

#if (defined(__GNUC__) || defined(__SUNPRO_CC) || defined (__SUNPRO_C)) && defined(JSON_API_VISIBILITY)
#define JSON_PUBLIC(type)   __attribute__((visibility("default"))) type
#else
#define JSON_PUBLIC(type) type
#endif
#endif

/* project version */
#define JSON_VERSION_MAJOR 1
#define JSON_VERSION_MINOR 7
#define JSON_VERSION_PATCH 18

/* cJSON Types: */
#define JSON_Invalid (0)
#define JSON_False  (1 << 0)
#define JSON_True   (1 << 1)
#define JSON_NULL   (1 << 2)
#define JSON_Number (1 << 3)
#define JSON_String (1 << 4)
#define JSON_Array  (1 << 5)
#define JSON_Object (1 << 6)
#define JSON_Raw    (1 << 7) /* raw json */

#define JSON_IsReference 256
#define JSON_StringIsConst 512

/* The cJSON structure: */
typedef struct JSON_Node
{
    /* next/prev allow you to walk array/object chains. Alternatively, use GetArraySize/GetArrayItem/GetObjectItem */
    struct JSON_Node *next;
    struct JSON_Node *prev;
    /* An array or object item will have a child pointer pointing to a chain of the items in the array/object. */
    struct JSON_Node *child;

    /* The type of the item, as above. */
    int32_t type;

    /* The item's string, if type==JSON_String  and type == JSON_Raw */
    char *valuestring;
    /* writing to valueint is DEPRECATED, use JSON_SetNumberValue instead */
    int32_t valueint;
    /* The item's number, if type==JSON_Number */
    double valuedouble;

    /* The item's name string, if this item is the child of, or is in the list of subitems of an object. */
    char *string;
} JSON_Node;

/* Limits how deeply nested arrays/objects can be before cJSON rejects to parse them.
 * This is to prevent stack overflows. */
#ifndef JSON_NESTING_LIMIT
#define JSON_NESTING_LIMIT 1000
#endif

/* Limits the length of circular references can be before cJSON rejects to parse them.
 * This is to prevent stack overflows. */
#ifndef JSON_CIRCULAR_LIMIT
#define JSON_CIRCULAR_LIMIT 10000
#endif

/* returns the version of cJSON as a string */
JSON_PUBLIC(const char*) JSON_Version(void);

/* Memory Management: the caller is always responsible to free the results from all variants of JSON_Parse (with JSON_Delete) and JSON_Print (with stdlib free, JSON_Hooks.free_fn, or JSON_free as appropriate). The exception is JSON_PrintPreallocated, where the caller has full responsibility of the buffer. */
/* Supply a block of JSON, and this returns a cJSON object you can interrogate. */
JSON_PUBLIC(JSON_Node *) JSON_Parse(const char *value);
JSON_PUBLIC(JSON_Node *) JSON_ParseWithLength(const char *value, size_t buffer_length);
/* ParseWithOpts allows you to require (and check) that the JSON is null terminated, and to retrieve the pointer to the final byte parsed. */
/* If you supply a ptr in return_parse_end and parsing fails, then return_parse_end will contain a pointer to the error so will match JSON_GetErrorPtr(). */
JSON_PUBLIC(JSON_Node *) JSON_ParseWithOpts(const char *value, const char **return_parse_end, bool require_null_terminated);
JSON_PUBLIC(JSON_Node *) JSON_ParseWithLengthOpts(const char *value, size_t buffer_length, const char **return_parse_end, bool require_null_terminated);

/* Render a cJSON entity to text for transfer/storage. */
JSON_PUBLIC(char *) JSON_Print(const JSON_Node *item, bool format);
/* Render a cJSON entity to text using a buffered strategy. prebuffer is a guess at the final size. guessing well reduces reallocation. fmt=0 gives unformatted, =1 gives formatted */
JSON_PUBLIC(char *) JSON_PrintBuffered(const JSON_Node *item, int32_t prebuffer, bool fmt);
/* Render a cJSON entity to text using a buffer already allocated in memory with given length. Returns 1 on success and 0 on failure. */
/* NOTE: cJSON is not always 100% accurate in estimating how much memory it will use, so to be safe allocate 5 bytes more than you actually need */
JSON_PUBLIC(bool) JSON_PrintPreallocated(JSON_Node *item, char *buffer, const int32_t length, const bool format);
/* Delete a cJSON entity and all subentities. */
JSON_PUBLIC(void) JSON_Delete(JSON_Node *item);

/* Returns the number of items in an array (or object). */
JSON_PUBLIC(int32_t) JSON_GetArraySize(const JSON_Node *array);
/* Retrieve item number "index" from array "array". Returns NULL if unsuccessful. */
JSON_PUBLIC(JSON_Node *) JSON_GetArrayItem(const JSON_Node *array, ssize_t index);
/* Get item "string" from object. */
JSON_PUBLIC(JSON_Node *) JSON_GetObjectItem(const JSON_Node * const object, const char * const string, bool case_sensitive);
JSON_PUBLIC(bool) JSON_HasObjectItem(const JSON_Node *object, const char *string);
/* For analysing failed parses. This returns a pointer to the parse error. You'll probably need to look a few chars back to make sense of it. Defined when JSON_Parse() returns 0. 0 when JSON_Parse() succeeds. */
JSON_PUBLIC(const char *) JSON_GetErrorPtr(void);

/* Check item type and return its value */
JSON_PUBLIC(char *) JSON_GetStringValue(const JSON_Node * const item);
JSON_PUBLIC(double) JSON_GetNumberValue(const JSON_Node * const item);

/* These functions check the type of an item */
JSON_PUBLIC(bool) JSON_IsInvalid(const JSON_Node * const item);
JSON_PUBLIC(bool) JSON_IsFalse(const JSON_Node * const item);
JSON_PUBLIC(bool) JSON_IsTrue(const JSON_Node * const item);
JSON_PUBLIC(bool) JSON_IsBool(const JSON_Node * const item);
JSON_PUBLIC(bool) JSON_IsNull(const JSON_Node * const item);
JSON_PUBLIC(bool) JSON_IsNumber(const JSON_Node * const item);
JSON_PUBLIC(bool) JSON_IsString(const JSON_Node * const item);
JSON_PUBLIC(bool) JSON_IsArray(const JSON_Node * const item);
JSON_PUBLIC(bool) JSON_IsObject(const JSON_Node * const item);
JSON_PUBLIC(bool) JSON_IsRaw(const JSON_Node * const item);

/* These calls create a cJSON item of the appropriate type. */
JSON_PUBLIC(JSON_Node *) JSON_CreateNull(void);
JSON_PUBLIC(JSON_Node *) JSON_CreateTrue(void);
JSON_PUBLIC(JSON_Node *) JSON_CreateFalse(void);
JSON_PUBLIC(JSON_Node *) JSON_CreateBool(bool boolean);
JSON_PUBLIC(JSON_Node *) JSON_CreateNumber(double num);
JSON_PUBLIC(JSON_Node *) JSON_CreateString(const char *string);
/* raw json */
JSON_PUBLIC(JSON_Node *) JSON_CreateRaw(const char *raw);
JSON_PUBLIC(JSON_Node *) JSON_CreateArray(void);
JSON_PUBLIC(JSON_Node *) JSON_CreateObject(void);

/* Create a string where valuestring references a string so
 * it will not be freed by JSON_Delete */
JSON_PUBLIC(JSON_Node *) JSON_CreateStringReference(const char *string);
/* Create an object/array that only references it's elements so
 * they will not be freed by JSON_Delete */
JSON_PUBLIC(JSON_Node *) JSON_CreateObjectReference(const JSON_Node *child);
JSON_PUBLIC(JSON_Node *) JSON_CreateArrayReference(const JSON_Node *child);

/* These utilities create an Array of count items.
 * The parameter count cannot be greater than the number of elements in the number array, otherwise array access will be out of bounds.*/
JSON_PUBLIC(JSON_Node *) JSON_CreateIntArray(const int32_t *numbers, int32_t count);
JSON_PUBLIC(JSON_Node *) JSON_CreateFloatArray(const float *numbers, int32_t count);
JSON_PUBLIC(JSON_Node *) JSON_CreateDoubleArray(const double *numbers, int32_t count);
JSON_PUBLIC(JSON_Node *) JSON_CreateStringArray(const char *const *strings, int32_t count);

/* Append item to the specified array/object. */
JSON_PUBLIC(bool) JSON_AddItemToArray(JSON_Node *array, JSON_Node *item);
JSON_PUBLIC(bool) JSON_AddItemToObject(JSON_Node *object, const char *string, JSON_Node *item, const bool constant_key);
/* Use this when string is definitely const (i.e. a literal, or as good as), and will definitely survive the cJSON object.
 * WARNING: When this function was used, make sure to always check that (item->type & JSON_StringIsConst) is zero before
 * writing to `item->string` */
JSON_PUBLIC(bool) JSON_AddItemToObjectCS(JSON_Node *object, const char *string, JSON_Node *item);
/* Append reference to item to the specified array/object. Use this when you want to add an existing cJSON to a new cJSON, but don't want to corrupt your existing cJSON. */
JSON_PUBLIC(bool) JSON_AddItemReferenceToArray(JSON_Node *array, JSON_Node *item);
JSON_PUBLIC(bool) JSON_AddItemReferenceToObject(JSON_Node *object, const char *string, JSON_Node *item);

/* Remove/Detach items from Arrays/Objects. */
JSON_PUBLIC(JSON_Node *) JSON_DetachItemViaPointer(JSON_Node *parent, JSON_Node * const item);
JSON_PUBLIC(JSON_Node *) JSON_DetachItemFromArray(JSON_Node *array, int32_t which);
JSON_PUBLIC(void) JSON_DeleteItemFromArray(JSON_Node *array, int32_t which);
JSON_PUBLIC(JSON_Node *) JSON_DetachItemFromObject(JSON_Node *object, const char *string);
JSON_PUBLIC(JSON_Node *) JSON_DetachItemFromObjectCaseSensitive(JSON_Node *object, const char *string);
JSON_PUBLIC(void) JSON_DeleteItemFromObject(JSON_Node *object, const char *string);
JSON_PUBLIC(void) JSON_DeleteItemFromObjectCaseSensitive(JSON_Node *object, const char *string);

/* Update array items. */
JSON_PUBLIC(bool) JSON_InsertItemInArray(JSON_Node *array, int32_t which, JSON_Node *newitem); /* Shifts pre-existing items to the right. */
JSON_PUBLIC(bool) JSON_ReplaceItemViaPointer(JSON_Node * const parent, JSON_Node * const item, JSON_Node * replacement);
JSON_PUBLIC(bool) JSON_ReplaceItemInArray(JSON_Node *array, int32_t which, JSON_Node *newitem);
JSON_PUBLIC(bool) JSON_ReplaceItemInObject(JSON_Node *object,const char *string,JSON_Node *replacement, bool case_sensitive);

/* Duplicate a cJSON item */
JSON_PUBLIC(JSON_Node *) JSON_Duplicate(const JSON_Node *item, bool recurse);
/* Duplicate will create a new, identical cJSON item to the one you pass, in new memory that will
 * need to be released. With recurse!=0, it will duplicate any children connected to the item.
 * The item->next and ->prev pointers are always zero on return from Duplicate. */
/* Recursively compare two cJSON items for equality. If either a or b is NULL or invalid, they will be considered unequal.
 * case_sensitive determines if object keys are treated case sensitive (1) or case insensitive (0) */
JSON_PUBLIC(bool) JSON_Compare(const JSON_Node * const a, const JSON_Node * const b, const bool case_sensitive);

/* Minify a strings, remove blank characters(such as ' ', '\t', '\r', '\n') from strings.
 * The input pointer json cannot point to a read-only address area, such as a string constant, 
 * but should point to a readable and writable address area. */
JSON_PUBLIC(void) JSON_Minify(char *json);

/* Helper functions for creating and adding items to an object at the same time.
 * They return the added item or NULL on failure. */
JSON_PUBLIC(JSON_Node*) JSON_AddNullToObject(JSON_Node * const object, const char * const name);
JSON_PUBLIC(JSON_Node*) JSON_AddTrueToObject(JSON_Node * const object, const char * const name);
JSON_PUBLIC(JSON_Node*) JSON_AddFalseToObject(JSON_Node * const object, const char * const name);
JSON_PUBLIC(JSON_Node*) JSON_AddBoolToObject(JSON_Node * const object, const char * const name, const bool boolean);
JSON_PUBLIC(JSON_Node*) JSON_AddNumberToObject(JSON_Node * const object, const char * const name, const double number);
JSON_PUBLIC(JSON_Node*) JSON_AddStringToObject(JSON_Node * const object, const char * const name, const char * const string);
JSON_PUBLIC(JSON_Node*) JSON_AddRawToObject(JSON_Node * const object, const char * const name, const char * const raw);
JSON_PUBLIC(JSON_Node*) JSON_AddObjectToObject(JSON_Node * const object, const char * const name);
JSON_PUBLIC(JSON_Node*) JSON_AddArrayToObject(JSON_Node * const object, const char * const name);

/* When assigning an integer value, it needs to be propagated to valuedouble too. */
#define JSON_SetIntValue(object, number) ((object) ? (object)->valueint = (object)->valuedouble = (number) : (number))
/* helper for the JSON_SetNumberValue macro */
JSON_PUBLIC(double) JSON_SetNumberHelper(JSON_Node *object, double number);
#define JSON_SetNumberValue(object, number) ((object != NULL) ? JSON_SetNumberHelper(object, (double)number) : (number))
/* Change the valuestring of a JSON_String object, only takes effect when type of object is JSON_String */
JSON_PUBLIC(char*) JSON_SetValuestring(JSON_Node *object, const char *valuestring);

/* If the object is not a boolean type this does nothing and returns JSON_Invalid else it returns the new type*/
#define JSON_SetBoolValue(object, boolValue) ( \
    (object != NULL && ((object)->type & (JSON_False|JSON_True))) ? \
    (object)->type=((object)->type &(~(JSON_False|JSON_True)))|((boolValue)?JSON_True:JSON_False) : \
    JSON_Invalid\
)

/* Macro for iterating over an array or object */
#define JSON_ArrayForEach(element, array) for(element = (array != NULL) ? (array)->child : NULL; element != NULL; element = element->next)

#ifdef __cplusplus
}
#endif

#endif
