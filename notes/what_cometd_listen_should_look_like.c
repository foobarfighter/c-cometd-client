void
cometd_listen(const cometd* h)
{
  cometd_inbox* inbox = h->inbox;

  while (cometd_conn_should_listen(h->conn))
  {
    JsonNode* node;

    while (node = cometd_inbox_take(inbox))
    {
      JsonNode* node = cometd_inbox_take(inbox);
      cometd_listener_fire_all(h, channel, node);
      json_node_free(node);
    }
  }
}
