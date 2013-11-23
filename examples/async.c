#define MAX_RETRIES 3

int main (void)
{
  cometd* h = cometd_new();
  cometd_configure(h, COMETDOPT_URL, "http://localhost:8089");

  // meta subscriptions don't need to make calls to the server
  cometd_subscribe(h, "/meta/subscribe", handle_meta);

  // blocks until connected
  // connection is spawned in a separate thread or via
  // some event loop that I don't know how to use yet.
  if (ECOMETD_FAILED == cometd_connect(h))
    exit(1);

  // blocking subscribe
  int retries;
  for (retries = 0; retries < MAX_RETRIES; retries++) {
    if (ECOMTED_FAILED == cometd_subscribe(h, "/foo/bar", handle_data)
      break;
  }
  
  cometd_subscribe_async(h, "/baz/bar", handle_data, handle_subscription);

  cometd_listen();
}
