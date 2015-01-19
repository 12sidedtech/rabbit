#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdint.h>
#include <amqp_tcp_socket.h>
#include <amqp.h>
#include <amqp_framing.h>

#include <assert.h>

#include "utils.h"
#include "erl_msgs.h"

#include <messaging/types.h>
#include <messaging/drop_copy.h>

int main(int argc, char const *const *argv)
{
    char const *hostname;
    int port, status, iters;
    char const *exchange;
    char const *bindingkey;
    amqp_socket_t *socket = NULL;
    amqp_connection_state_t conn;

    amqp_bytes_t queuename;

    if (argc < 4) {
        fprintf(stderr, "Usage: amqp_listen host port iters\n");
        return 1;
    }

    hostname = argv[1];
    port = atoi(argv[2]);
    iters = atoi(argv[3]);

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

    {
        amqp_queue_declare_ok_t *r = amqp_queue_declare(conn, 1, amqp_empty_bytes, 0, 0, 0, 1,
                                                        amqp_empty_table);
        die_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring queue");
        queuename = amqp_bytes_malloc_dup(r->queue);
        if (queuename.bytes == NULL) {
            fprintf(stderr, "Out of memory while copying queue name");
            return 1;
        }
    }

    amqp_queue_bind(conn, 1, queuename, amqp_cstring_bytes("amq.direct"), amqp_cstring_bytes("test queue"), amqp_empty_table);
    die_on_amqp_error(amqp_get_rpc_reply(conn), "Binding queue");

    amqp_basic_consume(conn, 1, queuename, amqp_empty_bytes, 0, 1, 0, amqp_empty_table);
    die_on_amqp_error(amqp_get_rpc_reply(conn), "Consuming");

    int count = 0;
    while (count++ < iters) {
        amqp_rpc_reply_t res;
        amqp_envelope_t envelope;

        amqp_maybe_release_buffers(conn);

        res = amqp_consume_message(conn, &envelope, NULL, 0);

        if (AMQP_RESPONSE_NORMAL != res.reply_type) {
            break;
        }

        printf("--%d-- Delivery %u, exchange %.*s routingkey %.*s\n",
               count,
               (unsigned) envelope.delivery_tag,
               (int) envelope.exchange.len, (char *) envelope.exchange.bytes,
               (int) envelope.routing_key.len, (char *) envelope.routing_key.bytes);

        if (envelope.message.properties._flags & AMQP_BASIC_CONTENT_TYPE_FLAG) {
            printf("Content-type: %.*s\n",
                   (int) envelope.message.properties.content_type.len,
                   (char *) envelope.message.properties.content_type.bytes);
        }
        printf("----\n");

        amqp_dump(envelope.message.body.bytes, envelope.message.body.len);
        struct ipope_drop_copy dc;
        int sz = erl_to_dc(envelope.message.body.bytes, &dc);
        printf("Received drop copy message of %d bytes in a payload of %lu bytes\n", sz, envelope.message.body.len);
        amqp_destroy_envelope(&envelope);
    }
    printf("Done receiving messages\n");


    die_on_amqp_error(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS), "Closing channel");
    die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS), "Closing connection");
    die_on_error(amqp_destroy_connection(conn), "Ending connection");

    return 0;
}

