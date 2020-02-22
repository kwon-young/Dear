#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <amqp.h>
#include <amqp_tcp_socket.h>

#include "utils.h"


int main(int argc, char const *const *argv) {
  char const *hostname;
  int port, status;
  amqp_socket_t *socket = NULL;
  amqp_connection_state_t conn;

  if (argc < 3) {
    fprintf(stderr,
            "Usage: hello host port\n");
    return 1;
  }

  hostname = argv[1];
  port = atoi(argv[2]);

  conn = amqp_new_connection();

  socket = amqp_tcp_socket_new(conn);
  if (!socket) {
    die("creating TCP socket");
  }

  status = amqp_socket_open(socket, hostname, port);
  if (status) {
    die("opening TCP socket");
  }

  die_on_amqp_error(amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN,
                               "guest", "guest"),
                    "Logging in");
  amqp_channel_open(conn, 1);
  die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel");

  amqp_basic_properties_t m_properties;
  memset(&m_properties, 0, sizeof m_properties);
  m_properties._flags = AMQP_BASIC_DELIVERY_MODE_FLAG;
  m_properties.delivery_mode = 1;
  m_properties.content_type = amqp_cstring_bytes( "application/json");
  m_properties._flags |= AMQP_BASIC_CONTENT_TYPE_FLAG;
  m_properties.content_encoding = amqp_cstring_bytes("utf-8");
  m_properties._flags |= AMQP_BASIC_CONTENT_ENCODING_FLAG;
  m_properties.reply_to = amqp_cstring_bytes("3149beef-be66-4b0e-ba47-2fc46e4edac3");
  m_properties._flags |= AMQP_BASIC_REPLY_TO_FLAG;

  amqp_table_t *table = &m_properties.headers;
  table->num_entries = 2;
  table->entries = calloc(2, sizeof(amqp_table_entry_t));
  table->entries[0].key = amqp_cstring_bytes("id");
  table->entries[0].value.kind = AMQP_FIELD_KIND_UTF8;
  table->entries[0].value.value.bytes = amqp_cstring_bytes("3149beef-be66-4b0e-ba47-2fc46e4edac3");
  table->entries[1].key = amqp_cstring_bytes("task");
  table->entries[1].value.kind = AMQP_FIELD_KIND_UTF8;
  table->entries[1].value.value.bytes = amqp_cstring_bytes("dear.exec.add");
  m_properties._flags |= AMQP_BASIC_HEADERS_FLAG;

  amqp_bytes_t message_bytes = amqp_cstring_bytes("[[1,2],{},{}]");

  die_on_error(amqp_basic_publish(conn, 1, amqp_cstring_bytes("celery"),
                                  amqp_cstring_bytes("celery"), 0, 0, &m_properties,
                                  message_bytes),
               "Publishing");
  free(m_properties.headers.entries);

  amqp_queue_declare_ok_t *reply = amqp_queue_declare(
      conn, 1, amqp_cstring_bytes("3149beef-be66-4b0e-ba47-2fc46e4edac3"),
      0, 1, 0, 0, amqp_empty_table);
  if (reply == NULL) {
    die_on_amqp_error(amqp_get_rpc_reply(conn), "queue.declare");
  }
  amqp_basic_consume(
      conn, 1, amqp_cstring_bytes("3149beef-be66-4b0e-ba47-2fc46e4edac3"),
      amqp_empty_bytes, 0, 0, 0, amqp_empty_table);
  die_on_amqp_error(amqp_get_rpc_reply(conn), "Consuming");

  amqp_rpc_reply_t res;
  amqp_envelope_t envelope;

  amqp_maybe_release_buffers(conn);

  res = amqp_consume_message(conn, &envelope, NULL, 0);

  if (AMQP_RESPONSE_NORMAL != res.reply_type) {
    printf("not a normal response\n");
  }

  printf("Delivery %u, exchange %.*s routingkey %.*s\n",
         (unsigned)envelope.delivery_tag, (int)envelope.exchange.len,
         (char *)envelope.exchange.bytes, (int)envelope.routing_key.len,
         (char *)envelope.routing_key.bytes);

  if (envelope.message.properties._flags & AMQP_BASIC_CONTENT_TYPE_FLAG) {
    printf("Content-type: %.*s\n",
           (int)envelope.message.properties.content_type.len,
           (char *)envelope.message.properties.content_type.bytes);
  }
  printf("----\n");

  amqp_dump(envelope.message.body.bytes, envelope.message.body.len);

  amqp_destroy_envelope(&envelope);

  die_on_amqp_error(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS),
                    "Closing channel");
  die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS),
                    "Closing connection");
  die_on_error(amqp_destroy_connection(conn), "Ending connection");
  return 0;
}
