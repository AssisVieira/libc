#include "json.h"

#include "str/str.h"

typedef struct Json {
  str_t *buff;
  int lastcomma;
} Json;

Json *json_new(size_t initialSize) {
  Json *json = malloc(sizeof(Json));

  if (json == NULL) {
    return NULL;
  }

  json->buff = str_new(initialSize);

  if (json->buff == NULL) {
    free(json);
    return NULL;
  }

  json->lastcomma = -1;

  return json;
}

void json_free(Json **json) {
  str_free(&(*json)->buff);
  free(*json);
  *json = NULL;
}

const char *json_cstr(const Json *json) {
  return str_cstr(json->buff);
}

size_t json_len(const Json *json) {
  return str_len(json->buff);
}

void json_add(Json *json, const char *name, const char *value) {
  json->buff = str_fmt(&json->buff, "\"%s\": \"%s\",", name, value);
  json->lastcomma = str_len(json->buff) - 1;
}

void json_addInt(Json *json, const char *name, int value) {
  json->buff = str_fmt(&json->buff, "\"%s\": %d,", name, value);
  json->lastcomma = str_len(json->buff) - 1;
}

void json_addBool(Json *json, const char *name, bool value) {
  json->buff = str_fmt(&json->buff, "\"%s\": %s,", name, (value) ? "true" : "false");
  json->lastcomma = str_len(json->buff) - 1;
}

void json_beginObject(Json *json, const char *name) {
  str_addc(&json->buff, '{');
}

void json_endObject(Json *json) {
  if (json->lastcomma >= 0) {
    str_setc(&json->buff, json->lastcomma, ' ');
    json->lastcomma = -1;
  }
  str_addc(&json->buff, '}');
}

void json_beginArray(Json *json, const char *name) {
  str_addc(&json->buff, '[');
}

void json_endArray(Json *json) {
  if (json->lastcomma >= 0) {
    str_setc(&json->buff, json->lastcomma, ' ');
    json->lastcomma = -1;
  }
  str_addc(&json->buff, ']');
}