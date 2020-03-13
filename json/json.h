#ifndef JSON_H
#define JSON_H

#include <stddef.h>
#include <stdbool.h>

typedef struct Json Json;

Json *json_new();
void json_free(Json **json);

const char *json_cstr(const Json *json);
size_t json_len(const Json *json);

void json_add(Json *json, const char *name, const char *value);
void json_addInt(Json *json, const char *name, int value);
void json_addBool(Json *json, const char *name, bool value);

void json_beginObject(Json *json, const char *name);
void json_endObject(Json *json);

void json_beginArray(Json *json, const char *name);
void json_endArray(Json *json);

#endif