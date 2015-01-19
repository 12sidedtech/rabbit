#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <amqp_tcp_socket.h>
#include <amqp.h>
#include <amqp_framing.h>

#include "utils.h"
#include <messaging/types.h>
#include <messaging/drop_copy.h>

#define SUMMARY_EVERY_US 1000000

static int construct_dc(char** buffer) {
    struct pope_header hdr = { .msg_type = 0,
                               .msg_length = sizeof(struct pope_header) + sizeof(struct ipope_drop_copy),
                               .originator = 12222,
                               .seq_num = 12345 };

    struct ipope_drop_copy dc = { .symbol_id = 8888,
                                  .passive_party = 54321,
                                  .aggressor_party = 12345,
                                  .exec_size = 100000,
                                  .exec_price = 9999,
                                  .passive_order_id = 888885555544,
                                  .passive_order_id = 444485555544,
                                  .exec_ts = 269945769778649 };

    struct ipope_drop_copy odc = { 0 };

    int sz = dc_to_erl(&dc, NULL);

    char* buf = (char*) malloc(sz);

    assert(sz == dc_to_erl(&dc, buf));

    *buffer = buf;
    return sz;
}

static void send_batch(amqp_connection_state_t conn,
                       char const *queue_name,
                       int rate_limit,
                       int message_count)
{
  uint64_t start_time = now_microseconds();
  int i;
  int sent = 0;
  int previous_sent = 0;
  uint64_t previous_report_time = start_time;
  uint64_t next_summary_time = start_time + SUMMARY_EVERY_US;

  char* message = NULL;
  char sz = construct_dc(&message);

  amqp_bytes_t message_bytes;

  message_bytes.len = sz;
  message_bytes.bytes = message;

  for (i = 0; i < message_count; i++) {
    uint64_t now = now_microseconds();

    die_on_error(amqp_basic_publish(conn,
                                    1,
                                    amqp_cstring_bytes("amq.direct"),
                                    amqp_cstring_bytes(queue_name),
                                    0,
                                    0,
                                    NULL,
                                    message_bytes),
                 "Publishing");
    sent++;
    if (now > next_summary_time) {
      int countOverInterval = sent - previous_sent;
      double intervalRate = countOverInterval / ((now - previous_report_time) / 1000000.0);
      printf("%d ms: Sent %d - %d since last report (%d Hz)\n",
             (int)(now - start_time) / 1000, sent, countOverInterval, (int) intervalRate);

      previous_sent = sent;
      previous_report_time = now;
      next_summary_time += SUMMARY_EVERY_US;
    }

    while (((i * 1000000.0) / (now - start_time)) > rate_limit) {
      microsleep(2000);
      now = now_microseconds();
    }
  }

    free(message);

  {
    uint64_t stop_time = now_microseconds();
    int total_delta = stop_time - start_time;

    printf("PRODUCER - Message count: %d\n", message_count);
    printf("Total time, milliseconds: %d\n", total_delta / 1000);
    printf("Overall messages-per-second: %g\n", (message_count / (total_delta / 1000000.0)));
  }
}

int main(int argc, char const *const *argv)
{
  char const *hostname;
  int port, status;
  int rate_limit;
  int message_count;
  amqp_socket_t *socket = NULL;
  amqp_connection_state_t conn;

  if (argc < 5) {
    fprintf(stderr, "Usage: amqp_producer host port rate_limit message_count\n");
    return 1;
  }

  hostname = argv[1];
  port = atoi(argv[2]);
  rate_limit = atoi(argv[3]);
  message_count = atoi(argv[4]);

  conn = amqp_new_connection();

  socket = amqp_tcp_socket_new(conn);
  if (!socket) {
    die("creating TCP socket");
  }

  status = amqp_socket_open(socket, hostname, port);
  if (status) {
    die("opening TCP socket");
  }

  die_on_amqp_error(amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, "guest", "guest"),
                    "Logging in");
  amqp_channel_open(conn, 1);
  die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel");

  send_batch(conn, "test queue", rate_limit, message_count);

  die_on_amqp_error(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS), "Closing channel");
  die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS), "Closing connection");
  die_on_error(amqp_destroy_connection(conn), "Ending connection");
  return 0;
}

