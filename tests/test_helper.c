#include <check.h>
#include <stdio.h>
#include <unistd.h>
#include <glib.h>
#include "cometd_json.h"
#include "../tests/test_helper.h"

static GList* log = NULL;

int
log_handler(const cometd* h, JsonNode* message)
{
  // FIXME: This holds invalid memory addresses
  // because message is free'd after the handler
  // is called.
  log = g_list_prepend(log, json_node_copy(message));

  gchar* str = cometd_json_node2str(message);
  printf("== added message to log\n%s\n\n", str);
  g_free(str);
  return 0;
}

int
log_has_message(JsonNode* message)
{
  return 0;
}

guint
log_size(void)
{
  return g_list_length(log);
}

guint
wait_for_log_size(guint size)
{
  guint actual;
  for (actual = 0; actual == 0; actual = log_size()){
    printf("log_size() == %d\n", actual);
    sleep(1);
  }
  ck_assert_int_eq(size, actual);
}
  
static void
free_node(gpointer data) { json_node_free((JsonNode*) data); }

void
log_clear(void)
{
  g_list_free_full(log, free_node);
  log = NULL;
}

static gboolean
json_value_equal(JsonNode* a, JsonNode* b, GList* exclude_props)
{
  g_return_val_if_fail(json_node_get_value_type(a) == json_node_get_value_type(b),
    FALSE);

  switch (json_node_get_value_type(a))
  {
    case G_TYPE_INT64:
      return json_node_get_int(a) == json_node_get_int(b);

    case G_TYPE_DOUBLE:
      return json_node_get_double(a) == json_node_get_double(b);

    case G_TYPE_BOOLEAN:
      return json_node_get_boolean(a) == json_node_get_boolean(b);

    case G_TYPE_STRING:
      return strcmp(json_node_get_string(a), json_node_get_string(b)) == 0;
  }

  return FALSE;
}

static gboolean
json_array_equal(JsonNode* a, JsonNode* b, GList* exclude_props)
{
  g_return_val_if_fail(JSON_NODE_TYPE(a) == JSON_NODE_ARRAY, FALSE);
  g_return_val_if_fail(JSON_NODE_TYPE(b) == JSON_NODE_ARRAY, FALSE);
  
  JsonArray* array_a = json_node_get_array(a);
  JsonArray* array_b = json_node_get_array(b);

  if (json_array_get_length(array_a) != json_array_get_length(array_b))
    return FALSE;

  gint i;
  gint len = json_array_get_length(array_a);

  for (i = 0; i < len; i++)
  {
    JsonNode* element_a = json_array_get_element(array_a, i);
    JsonNode* element_b = json_array_get_element(array_b, i);

    if (!json_node_equal(element_a, element_b, NULL))
      return FALSE;
  }
  return TRUE;
}

static gboolean
json_object_equal(JsonNode* a, JsonNode* b, GList* exclude_props)
{
  g_return_val_if_fail(JSON_NODE_TYPE(a) == JSON_NODE_OBJECT, FALSE);
  g_return_val_if_fail(JSON_NODE_TYPE(b) == JSON_NODE_OBJECT, FALSE);
  
  JsonObject* object_a = json_node_get_object(a);
  JsonObject* object_b = json_node_get_object(b);

  if (json_object_get_size(object_a) != json_object_get_size(object_b))
    return FALSE;

  GList* members = json_object_get_members(object_a);

  GList* member;
  for (member = members; member; member = g_list_next(member))
  {
    if (g_list_find_custom(exclude_props, member->data, (GCompareFunc)strcmp))
      continue;

    JsonNode* value_a = json_object_get_member(object_a, member->data);
    JsonNode* value_b = json_object_get_member(object_b, member->data);
    
    if (!json_node_equal(value_a, value_b, exclude_props))
      return FALSE;
  }

  return TRUE;
}


gboolean
json_node_equal(JsonNode* a, JsonNode* b, GList* exclude_props)
{
  g_return_val_if_fail(a != NULL && b != NULL, FALSE);
  g_return_val_if_fail(JSON_NODE_TYPE(a) == JSON_NODE_TYPE(b), FALSE);

  JsonNodeType t = JSON_NODE_TYPE(a);

  switch (t)
  {
    case JSON_NODE_OBJECT:
      return json_object_equal(a, b, exclude_props);
      
    case JSON_NODE_ARRAY:
      return json_array_equal(a, b, exclude_props);

    case JSON_NODE_NULL:
      return TRUE;

    case JSON_NODE_VALUE:
      return json_value_equal(a, b, exclude_props);
  }

  return FALSE;
}
