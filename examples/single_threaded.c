ui_object ui = ui_create();

cometd_message* msg;

cometd* h = cometd_connect(config);

thread = pthread_create();

cometd_subscribe(h, "/my/topic");
cometd_subscribe(h, "/my/other/topic");

while (msg = cometd_recv(h)){
  if (strcmp(msg->channel, "my/topic") == 0){
    on_my_topic(msg);
  } else if (strcmp(msg->channel, "/my/other/topic") == 0){
    on_my_other_topic(msg);
  }
}

