#ifndef _JSON_H_
#define _JSON_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { Type_Empty, Type_Bool, Type_Int, Type_Double, Type_String, Type_Object, Type_Array } json_type;

typedef struct _json {
	struct _json *next, *prev;
	struct _json *child;
	json_type type;
	const char *key;
	union {
		int valuebool;
		long valueint;
		double valuedouble;
		const char *valuestring;
	};
	uint32_t internal;		// not used
} *json;

json json_Create(json_type type, ...);
void json_Delete(json obj);

#define json_CreateEmpty()  json_Create(Type_Empty)
#define json_CreateBool(v)  json_Create(Type_Bool, v)
#define json_CreateInt(v)	json_Create(Type_Int, v)
#define json_CreateDouble(v) json_Create(Type_Double, v)
#define json_CreateString(v) json_Create(Type_String, v)

/**
 * parse a JSON string to a JSON object
 * For parsing failed, errptr returns a pointer to the invalid string
 * errptr can be NULL
 */
json json_Parse(const char *str, const char **errptr);
/**
 * translate the json entity to text
 * free the text after using it
 */
char* json_Print(json obj, int formatted);

void json_AddItemToObject(json obj, const char *name, json child);
void json_AddItemToArray(json obj, json item);

json json_MakeIntArray(int *values, int count);
json json_MakeDoubleArray(double *values, int count);
json json_MakeStringArray(const char **strings, int count);

int json_GetArraySize(json obj);
json json_GetItemFromArray(json obj, int index);
json json_GetItemFromObject(json obj, const char *key);

int json_InsertItemToArray(json obj, int index, json item);

/**
 * update object/array item to new one, the old item will be deleted automatically
 */
int json_UpdateItemInArray(json obj, int index, json item);
int json_UpdateItemInObject(json obj, const char *key, json item);

/**
 * detach item from object/array
 * return the detached item, and you should delete it when finished using it.
 */
json json_DetachItemFromArray(json obj, int index);
json json_DetachItemFromObject(json obj, const char *key);
void json_DeleteItemFromArray(json obj, int index);
void json_DeleteItemFromObject(json obj, const char *key);

#ifdef __cplusplus
}
#endif

#endif /* end of include guard: _JSON_H_ */
