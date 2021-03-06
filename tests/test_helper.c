#include "test_helper.h"
#include <check.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

static GQueue* log = NULL;
static GMutex log_mutex;
static GCond  log_cond;

int
test_empty_handler(const cometd* h, JsonNode* message)
{
  return COMETD_SUCCESS;
}

int
log_handler(const cometd* h, JsonNode* message)
{
  g_mutex_lock(&log_mutex);
  g_queue_push_tail(log, json_node_copy(message));

  gchar* str = cometd_json_node2str(message);
  printf("== added message to log\n%s\n\n", str);
  g_free(str);

  g_cond_signal(&log_cond);
  g_mutex_unlock(&log_mutex);

  return 0;
}

guint
log_size(void)
{
  guint size;
  g_mutex_lock(&log_mutex);
  size = g_queue_get_length(log);
  g_mutex_unlock(&log_mutex);
  return size;
}

void
wait_for_message(glong timeout_secs, GList* excludes, char* json)
{
  JsonNode* node;
  JsonNode* message = cometd_json_str2node(json);

  GTimeVal now;
  g_get_current_time(&now);

  while (1)
  {
    g_mutex_lock(&log_mutex);

    GTimeVal remaining_wait;
    remaining_wait.tv_sec  = now.tv_sec + timeout_secs;
    remaining_wait.tv_usec  = now.tv_usec;

    while (g_queue_is_empty(log) == TRUE)
      // FIXME: breaking the wait loop innaccurately if a timeout occurs
      if (g_cond_timed_wait(&log_cond, &log_mutex, &remaining_wait) != TRUE)
        break;

    if (g_queue_is_empty(log) == TRUE) {
      g_mutex_unlock(&log_mutex);
      break;
    }

    node = (JsonNode*) g_queue_pop_head(log);

    gchar* str = cometd_json_node2str(node);
    printf("== processing message from log\n%s\n\n", str);
    g_free(str);

    g_mutex_unlock(&log_mutex);

    gboolean is_equal = json_node_equal(node, message, excludes);
  
    // free the node that was allocated by the log_handler
    json_node_free(node);

    if (is_equal)
    {
      printf("== found match in log\n\n");
      break;
    }
  }
  json_node_free(message);
}
  
static void
free_node(gpointer data) { json_node_free((JsonNode*) data); }

void
log_clear(void)
{
  if (log)
    g_queue_free_full(log, free_node);

  log = g_queue_new();
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

    if (!json_node_equal(element_a, element_b, exclude_props))
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

  gboolean is_equal = TRUE;

  GList* members = json_object_get_members(object_a);

  GList* member;
  for (member = members; member; member = g_list_next(member))
  {
    if (g_list_find_custom(exclude_props, member->data, (GCompareFunc)strcmp))
      continue;

    JsonNode* value_a = json_object_get_member(object_a, member->data);
    JsonNode* value_b = json_object_get_member(object_b, member->data);
    
    if (!json_node_equal(value_a, value_b, exclude_props))
    {
      is_equal = FALSE;
      break;
    }
  }

  g_list_free(members);

  return is_equal;
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

char* read_file(char* path)
{
  char* buffer = 0;
  long length;
  FILE* f = fopen (path, "rb");

  if (f)
  {
    fseek (f, 0, SEEK_END);
    length = ftell (f);
    fseek (f, 0, SEEK_SET);
    buffer = malloc (length+1);
    if (buffer)
    {
      fread (buffer, 1, length, f);
      buffer[length] = '\0';
    }
    fclose (f);
  }

  return buffer;
}

JsonNode*
json_from_fixture(char* fixture_name)
{
  JsonNode* n;
  char* contents;
  char path[255];

  sprintf(path, "../../tests/fixtures/%s.json", fixture_name);
  contents = read_file(path);

  if (contents == NULL)
    printf("Can't read file at path: %s\n", path);

  n = cometd_json_str2node(contents);
  g_assert(n != NULL);

  free(contents);
  return n;
}
