#ifndef CHECK_COMETD_H
#define CHECK_COMETD_H

#include <check.h>
#include "cometd.h"
#include "test_helper.h"

Suite* make_cometd_unit_suite (void);
Suite* make_cometd_integration_suite (void);
Suite* make_http_integration_suite (void);
Suite* make_msg_suite (void);
Suite* make_conn_suite (void);
Suite* make_transport_suite (void);
Suite* make_channel_suite (void);
Suite* make_cometd_inbox_suite (void);
Suite* make_cometd_event_suite (void);
Suite* make_test_helper_suite (void);

#endif /* CHECK_COMETD_H */

