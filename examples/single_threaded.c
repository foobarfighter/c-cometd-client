int main (void)
{
  cometd* h = cometd_new();
  cometd_configure(h, COMETDOPT_URL, "http://localhost:8089");
  cometd_connect(h);

  cometd_subscribe(h, "/foo/bar", handle_foo_bar);
  cometd_subscribe(h, "/baz/bar", handle_baz_bar);

  cometd_listen();
}
