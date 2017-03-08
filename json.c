#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <float.h>
#include "json.h"

json json_Create(json_type type, ...)
{
	va_list ap;
	json obj = (json)calloc(1, sizeof(*obj));
	if (!obj) {
		return NULL;
	}
	obj->type = type;
	va_start(ap, type);
	switch (type) {
	case Type_Bool:
	case Type_Int:
		obj->valueint = va_arg(ap, long);
		break;
	case Type_Double:
		obj->valuedouble = va_arg(ap, double);
		break;
	case Type_String:
		obj->valuestring = _strdup(va_arg(ap, const char*));
		break;
	default:
		break;
	}
	va_end(ap);
	return obj;
}

void json_Delete(json obj)
{
	json next;
	while (obj) {
		next = obj->next;
		if (obj->child) {
			json_Delete(obj->child);
		}
		if (obj->key) {
			free((void*)obj->key);
		}
		if (obj->type == Type_String && obj->valuestring) {
			free((void*)obj->valuestring);
		}
		free(obj);
		obj = next;
	}
}

/**
 * you should remove comment before parsing it
 */
/*
static const char* json_SkipComment(const char *str)
{
	if (str[0] == '/') {
		if (str[1] == '/') {
			while (*str != '\n') str++;
			str++;
		} else if (str[1] == '*') {
			str += 2;
			while (!(str[0] == '*' && str[1] == '/')) str++;
			str += 2;
		}
	}
	return str;
}
*/

static const char* json_Skip(const char *str)
{
	if (!str) {
		return NULL;
	}
	while (*str && (unsigned char)*str <= ' ') {
		str++;
	}
	return str;
}

static const char* json_ParseValue(json obj, const char *str);
static const char* json_ParseNumber(json obj, const char *str)
{
	double num = 0.0;
	int sign = 1, expsign = 1;
	int scale = 0, expscale = 0;

	if (*str == '-') {
		str++;
		sign = -1;
	}
	while (*str == '0') {
		str++;
	}
	while (*str > '0' && *str <= '9') {
		num = num * 10 + *str++ - '0';
	}
	if (*str == '.' && str[1] >= '0' && str[1] <= '9') {
		str++;
		while (*str >= '0' && *str <= '9') {
			num = num * 10 + *str++ - '0';
			scale--;
		}
	}
	if (*str == 'e' || *str == 'E') {
		str++;
		if (*str == '+') {
			expsign = 1;
			str++;
		} else if (*str == '-') {
			expsign = -1;
			str++;
		}
		while (*str >= '0' && *str <= '9') {
			expscale = expscale * 10 + *str++ - '0';
		}
	}

	num = sign * num * pow(10, (scale + expsign * expscale));
	if (fabs(num - (long)num) < DBL_EPSILON && (long)num <= LONG_MAX && (long)num >= LONG_MIN) {
		obj->type = Type_Int;
		obj->valueint = (long)num;
	} else {
		obj->type = Type_Double;
		obj->valuedouble = num;
	}

	return str;
}

static const char* json_ParseString(json obj, const char *str)
{
	int len;
	char *out;
	const char *ptr;

	if (*str != '\"') {
		return str;
	}
	len = 0;
	ptr = str + 1;
	while (*ptr != '\"' &&  ++len) {
		if (*ptr++ == '\\') {
			ptr++;
		}
		if (*ptr == '\0') {
			return str;
		}
	}
	obj->type = Type_String;
	obj->valuestring = out = (char*)calloc(1, len + 1);
	ptr = str + 1;
	while (*ptr != '\"' && *ptr != '\0') {
		if (*ptr != '\\') {
			*out++ = *ptr++;
		} else {
			ptr++;
			switch (*ptr) {
			case 'b':
				*out++ = '\b';
				break;
			case 'f':
				*out++ = '\f';
				break;
			case 'n':
				*out++ = '\n';
				break;
			case 'r':
				*out++ = '\r';
				break;
			case 't':
				*out++ = '\t';
				break;
			default:
				*out++ = *ptr;
				break;
			}
			ptr++;
		}
	}
	if (*ptr == '\"') {
		ptr++;
	}

	return ptr;
}

static const char* json_ParseArray(json obj, const char *str)
{
	json child, next;
	if (*str != '[') {
		return str;
	}
	obj->type = Type_Array;
	str = json_Skip(str + 1);
	if (*str == ']') {
		return str + 1;
	}

	child = next = NULL;
	do {
		next = json_Create(Type_Empty);
		if (child) {
			child->next = next;
			next->prev = child;
			child = next;
		} else {
			obj->child = child = next;
		}
		str = json_Skip(json_ParseValue(child, json_Skip(str)));
	} while (*str == ',' && str++);

	if (*str == ']') {
		return str + 1;
	}
	else {
		return str;
	}
}

static const char* json_ParseObject(json obj, const char *str)
{
	json child, next;
	if (*str != '{') {
		return str;
	}
	obj->type = Type_Object;
	str = json_Skip(str + 1);
	if (*str == '}') {
		return str + 1;
	}

	child = next = NULL;
	do {
		next = json_Create(Type_Empty);
		if (child) {
			child->next = next;
			next->prev = child;
			child = next;
		} else {
			obj->child = child = next;
		}
		str = json_ParseString(child, json_Skip(str));
		child->key = child->valuestring;
		child->valuestring = NULL;
		str = json_Skip(str);
		if (*str != ':') {
			return str;
		}
		str = json_Skip(json_ParseValue(child, str + 1));
	} while (*str == ',' && str++);
	
	if (*str == '}') {
		return str + 1;
	} else {
		return str;
	}
}

static const char* json_ParseValue(json obj, const char *str)
{
	if (!obj) {
		return "JSON Object is NULL";
	}
	str = json_Skip(str);
	if (*str == '{') {
		return json_ParseObject(obj, str);
	} else if (*str == '[') {
		return json_ParseArray(obj, str);
	} else if (*str == '\"') {
		return json_ParseString(obj, str);
	} else if (*str == '-' || *str == '.' || (*str >= '0' && *str <= '9')) {
		return json_ParseNumber(obj, str);
	} else if (strncmp(str, "null", 4) == 0) {
		obj->type = Type_Empty;
		return str + 4;
	} else if (strncmp(str, "true", 4) == 0) {
		obj->type = Type_Bool;
		obj->valuebool = 1;
		return str + 4;
	} else if (strncmp(str, "false", 5) == 0) {
		obj->type = Type_Bool;
		obj->valuebool = 0;
		return str + 5;
	} else {
		return str;
	}
}

json json_Parse(const char *str, const char **errptr)
{
	json obj;
	if (!str) {
		return NULL;
	}
	obj = json_Create(Type_Empty);
	str = json_ParseValue(obj, str);
	str = json_Skip(str);
	if (*str != '\0') {
		if (errptr) {
			*errptr = str;
		}
		json_Delete(obj);
		return NULL;
	}
	if (errptr) {
		*errptr = NULL;
	}
	return obj;
}

static int pow2gt(int n)
{
	n--;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	return n + 1;
}

typedef struct {
	char *buf;
	int size;
	int offset;
} *json_buf;

static void json_Printf(json_buf buf, int size, const char *format, ...)
{
	va_list ap;

	while (buf->offset + size + 1> buf->size) {
		buf->size *= 2;
		buf->buf = (char*)realloc(buf->buf, buf->size);
	}
	va_start(ap, format);
	buf->offset += _vsnprintf(buf->buf + buf->offset, size + 1, format, ap); 
	va_end(ap);
}

static void json_PrintIndent(json_buf buf, int indent)
{
	int i;
	for (i = indent; i > 0; i--) {
		json_Printf(buf, 1, " ");
	}
}

static void json_PrintKeyName(json_buf buf, json obj, int formatted, int indent)
{
	if (obj->key) {
		if (formatted) {
			json_PrintIndent(buf, indent);
		}
		json_Printf(buf, strlen(obj->key) + 8, "\"%s\" : ", obj->key);
	}
}

static void json_PrintValue(json_buf buf, json obj, int formatted, int indent);

static void json_PrintObject(json_buf buf, json obj, int formatted, int indent)
{
	if (formatted) {
		if (!obj->child) {
			json_Printf(buf, 8, "{ }");
		}
		else {
			json item = obj->child;
			json_Printf(buf, 4, "{\n");
			while (item) {
				json_PrintValue(buf, item, 1, indent + 4);
				item = item->next;
				if (item) {
					json_Printf(buf, 8, ",\n");
				}
			}
			json_Printf(buf, 8, "\n");
			json_PrintIndent(buf, indent);
			json_Printf(buf, 1, "}");
		}
	}
	else {
		json item = obj->child;
		json_Printf(buf, 2, "{ ");
		while (item) {
			json_PrintValue(buf, item, 0, 0);
			item = item->next;
			if (item) {
				json_Printf(buf, 2, ", ");
			}
		}
		json_Printf(buf, 2, " }");
	}
}

static void json_PrintArray(json_buf buf, json obj, int formatted, int indent)
{
	if (formatted) {
		if (!obj->child) {
			json_Printf(buf, 8, "[ ]");
		}
		else {
			json item = obj->child;
			json_Printf(buf, 4, "[\n");
			while (item) {
				json_PrintIndent(buf, indent + 4);
				json_PrintValue(buf, item, 1, indent + 4);
				item = item->next;
				if (item) {
					json_Printf(buf, 8, ",\n");
				}
			}
			json_Printf(buf, 8, "\n");
			json_PrintIndent(buf, indent);
			json_Printf(buf, 1, "]");
		}
	}
	else {
		json item = obj->child;
		json_Printf(buf, 2, "[ ");
		while (item) {
			json_PrintValue(buf, item, 0, 0);
			item = item->next;
			if (item) {
				json_Printf(buf, 2, ", ");
			}
		}
		json_Printf(buf, 2, " ]");
	}
}

static void json_PrintValue(json_buf buf, json obj, int formatted, int indent)
{
	json_PrintKeyName(buf, obj, formatted, indent);
	switch (obj->type) {
	case Type_Empty:
		json_Printf(buf, 4, "null");
		break;
	case Type_Bool:
		json_Printf(buf, 5, "%s", obj->valuebool ? "true" : "false");
		break;
	case Type_Int:
		json_Printf(buf, 21, "%ld", obj->valueint);
		break;
	case Type_Double: {
		double d = obj->valuedouble;
		if (fabs(floor(d) - d) <= DBL_EPSILON) {
			json_Printf(buf, 64, "%.0f", d);
		}
		else if (fabs(d) < 1e-6 || fabs(d) > 1e10) {
			json_Printf(buf, 64, "%e", d);
		} else {
			json_Printf(buf, 64, "%f", d);
		}
		break;
	}
	case Type_String:
		if (obj->valuestring) {
			json_Printf(buf, strlen(obj->valuestring) + 16, "\"%s\"", obj->valuestring);
		}
		break;
	case Type_Array:
		json_PrintArray(buf, obj, formatted, indent);
		break;
	case Type_Object: 
		json_PrintObject(buf, obj, formatted, indent);
		break;
	default:
		break;
	}
}

char* json_Print(json obj, int formatted)
{
	char *ret;
	json_buf buf = (json_buf)malloc(sizeof(*buf));
	buf->size = 512;
	buf->offset = 0;
	buf->buf = (char*)malloc(buf->size);
	json_PrintValue(buf, obj, formatted, 0);
    json_Printf(buf, 2, "\n");
	ret = buf->buf;
	free(buf);
	return ret;
}

void json_AddItemToObject(json obj, const char *name, json child)
{
	if (!name || !child) {
		return;
	}
	if (child->key) {
		free((void*)child->key);
	}
	child->key = _strdup(name);
	json_AddItemToArray(obj, child);
}

void json_AddItemToArray(json obj, json item)
{
	if (!obj || !item) {
		return;
	}
	if (obj->child) {
		json p = obj->child;
		while (p && p->next) {
			p = p->next;
		}
		p->next = item;
		item->prev = p;
	} else {
		obj->child = item;
	}
}

json json_MakeIntArray(int *values, int count)
{
	int i;
	json obj, next, last;
	if (!values || count < 1) {
		return NULL;
	}
	obj = json_Create(Type_Array);
	last = obj->child = json_CreateInt(values[0]);
	for (i = 1; i < count; i++) {
		next = json_CreateInt(values[i]);
		next->prev = last;
		last->next = next;
		last = next;
	}
	return obj;
}

json json_MakeDoubleArray(double *values, int count)
{
	int i;
	json obj, next, last;
	if (!values || count < 1) {
		return NULL;
	}
	obj = json_Create(Type_Array);
	last = obj->child = json_CreateDouble(values[0]);
	for (i = 1; i < count; i++) {
		next = json_CreateDouble(values[i]);
		next->prev = last;
		last->next = next;
		last = next;
	}
	return obj;
}

json json_MakeStringArray(const char **strings, int count)
{
	int i;
	json obj, next, last;
	if (!strings || count < 1) {
		return NULL;
	}
	obj = json_Create(Type_Array);
	last = obj->child = json_CreateString(strings[0]);
	for (i = 1; i < count; i++) {
		next = json_CreateString(strings[i]);
		next->prev = last;
		last->next = next;
		last = next;
	}
	return obj;
}

int json_GetArraySize(json obj)
{
	int count = 0;
	json next;
	if (!obj || obj->type != Type_Array || !obj->child) {
		return 0;
	}
	next = obj->child;
	while (next) {
		next = next->next;
		count++;
	}
	return count;
}

json json_GetItemFromArray(json obj, int index)
{
	int i;
	json next;
	if (!obj || !obj->child) {
		return NULL;
	}
	next = obj->child;
	for (i = 0; i < index && next; i++) {
		next = next->next;
	}
	return next;
}

json json_GetItemFromObject(json obj, const char *key)
{
	json next;
	if (!obj || obj->type != Type_Object || !obj->child) {
		return NULL;
	}
	next = obj->child;
	while (next) {
		if (strcmp(key, next->key) == 0) {
			return next;
		}
		next = next->next;
	}
	return next;
}

int json_InsertItemToArray(json obj, int index, json item)
{
	json target;
	if (!obj || !item) {
		return 0;
	}
	if (!obj->child) {	// no item in this array
		obj->child = item;
		return 1;
	}
	target = json_GetItemFromArray(obj, index);
	if (!target) {
		json_AddItemToArray(obj, item);
		return 1;
	}
	if (target->prev) {
		target->prev->next = item;
	}
	item->prev = target->prev;
	item->next = target;
	target->prev = item;
	if (index == 0) {
		obj->child = item;
	}
	return 1;
}

int json_UpdateItemInArray(json obj, int index, json item)
{
	json target;
	if (!obj || !item) {
		return 0;
	}
	target = json_GetItemFromArray(obj, index);
	if (!target) {
		return 0;
	}
	if (obj->child == target) {
		obj->child = item;		// update the header node to new item
	}
	if (target->prev) {
		target->prev->next = item;
	}
	item->prev = target->prev;
	item->next = target->next;
	if (target->next) {
		target->next->prev = item;
	}
	target->prev = target->next = NULL;
	json_Delete(target);
	return 0;
}

int json_UpdateItemInObject(json obj, const char *key, json item)
{
	json target;
	if (!obj || !item) {
		return 0;
	}
	target = json_GetItemFromObject(obj, key);
	if (!target) {
		return 0;
	}
	if (obj->child == target) {
		obj->child = item;		// update the header node to new item
	}
	if (target->prev) {
		target->prev->next = item;
	}
	item->prev = target->prev;
	item->next = target->next;
	if (target->next) {
		target->next->prev = item;
	}
	target->prev = target->next = NULL;
	json_Delete(target);
	return 0;
}

json json_DetachItemFromArray(json obj, int index)
{
	json target;
	if (!obj) {
		return 0;
	}
	target = json_GetItemFromArray(obj, index);
	if (!target) {
		return 0;
	}
	if (obj->child == target) {
		obj->child = target->next;		// the header node will be removed, and the next node will be a new header
	}

	if (target->prev) {
		target->prev->next = target->next;
	}
	if (target->next) {
		target->next->prev = target->prev;
	}
	target->prev = target->next = NULL;
	return target;
}

json json_DetachItemFromObject(json obj, const char *key)
{
	json target;
	if (!obj) {
		return 0;
	}
	target = json_GetItemFromObject(obj, key);
	if (!target) {
		return 0;
	}
	if (obj->child == target) {
		obj->child = target->next;		// the header node will be removed, and the next node will be a new header
	}

	if (target->prev) {
		target->prev->next = target->next;
	}
	if (target->next) {
		target->next->prev = target->prev;
	}
	target->prev = target->next = NULL;
	return target;
}

void json_DeleteItemFromArray(json obj, int index)
{
	json target;
	if (!obj) {
		return;
	}
	target = json_GetItemFromArray(obj, index);
	if (!target) {
		return;
	}
	if (obj->child == target) {
		obj->child = target->next;		// the header node will be removed, and the next node will be a new header
	}

	if (target->prev) {
		target->prev->next = target->next;
	}
	if (target->next) {
		target->next->prev = target->prev;
	}
	target->prev = target->next = NULL;
	json_Delete(target);
}

void json_DeleteItemFromObject(json obj, const char *key)
{
	json target;
	if (!obj) {
		return;
	}
	target = json_GetItemFromObject(obj, key);
	if (!target) {
		return;
	}
	if (obj->child == target) {
		obj->child = target->next;		// the header node will be removed, and the next node will be a new header
	}

	if (target->prev) {
		target->prev->next = target->next;
	}
	if (target->next) {
		target->next->prev = target->prev;
	}
	target->prev = target->next = NULL;
	json_Delete(target);
}
