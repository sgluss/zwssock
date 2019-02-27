#include <czmq.h>

#include "zwssock.h"
#include "hiredis/hiredis.h"

#define ZMQ_STATIC

static char *listen_on = "tcp://127.0.0.1:8000";

redisContext* gimmeRedis() {
	redisContext *c;
    redisReply *reply;

    printf("setting host\n");
    const char *hostname = "127.0.0.1";

    int port = 6379;

    struct timeval timeout = { 1, 500000 }; // 1.5 seconds

	printf("establishing connection to %s\n", hostname);
	c = redisConnectWithTimeout(hostname, port, timeout);

    if (c == NULL || c->err) {
        if (c) {
            printf("Connection error: %s\n", c->errstr);
            redisFree(c);
        } else {
            printf("Connection error: can't allocate redis context\n");
        }
        exit(1);
    }

    /* PING server */
    reply = redisCommand(c,"PING");
    printf("PING: %s\n", reply->str);
    freeReplyObject(reply);

    if (strcmp(reply->str, "PONG")) {
    	printf("redis connection established successfully!\n\n");
		return c;
	} else {
		printf("failed to connect to redis!\n\n");
		return NULL;
    }
}

int main(int argc, char **argv)
{
	//redisContext* redis = gimmeRedis();

	zwssock_t *sock;

	zsock_t *dealer = zsock_new (ZMQ_DEALER);
	int dealer_port = zsock_connect (dealer, "tcp://127.0.0.1:5570");

	printf("dealer is connected to port %d\n", dealer_port);

	char *l =  argc > 1 ? argv[1] : listen_on;

	int major, minor, patch;
	zsys_version (&major, &minor, &patch);
	printf("built with: ØMQ=%d.%d.%d czmq=%d.%d.%d\n",
	       major, minor, patch,
	       CZMQ_VERSION_MAJOR, CZMQ_VERSION_MINOR,CZMQ_VERSION_PATCH);


	sock = zwssock_new_router();

	zwssock_bind(sock, l);

	zmsg_t* msg;
	zframe_t *id;

	while (!zsys_interrupted)
	{
		msg = zwssock_recv(sock);

		if (!msg)
			break;

		// first message is the routing id
		id = zmsg_pop(msg);

		while (zmsg_size(msg) != 0)
		{
			char * str = zmsg_popstr(msg);

			printf("%s\n", str);

			free(str);
		}

		zmsg_destroy(&msg);

		msg = zmsg_new();

		zmsg_push(msg, id);
		zmsg_addstr(msg, "hello back");

		int rc = zwssock_send(sock, &msg);
		if (rc != 0)
			zmsg_destroy(&msg);
	}

	zwssock_destroy(&sock);
}
