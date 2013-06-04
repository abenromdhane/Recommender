#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <assert.h>
#include "recommender.h"
#include "server_parameters.h"
#include "libuv/include/uv.h"
#include "http-parser/http_parser.h"
#include "post_parse.h"

#define CHECK(r, msg) \
if (r) { \
uv_err_t err = uv_last_error(uv_loop); \
fprintf(stderr, "%s: %s\n", msg, uv_strerror(err)); \
exit(1); \
}
#define UVERR(err, msg) fprintf(stderr, "%s: %s\n", msg, uv_strerror(err))
#define LOG(msg) puts(msg);
#define LOGF(fmt, params...) printf(fmt "\n", params);
#define LOG_ERROR(msg) puts(msg);

#define HEADER \
"HTTP/1.1 200 OK\r\n" \
"Content-Type: text/plain\r\n" \
"Content-Length: "

#define RESPONSE \
"HTTP/1.1 200 OK\r\n" \
"Content-Type: text/plain\r\n" \
"Content-Length: 12\r\n" \
"\r\n" \
"hello world\n"

static uv_loop_t* uv_loop;
static uv_tcp_t server;
static http_parser_settings parser_settings;
static uv_mutex_t mutex;

struct learned_factors* factors;
struct learned_factors* factors_backup;
training_set_t* training_set;
server_parameters_t* server_param;

typedef struct
{
	uv_tcp_t handle;
	http_parser parser;
	uv_write_t write_req;
	int request_num;
} client_t;

void on_close (uv_handle_t* handle)
{
	client_t* client = (client_t*) handle->data;

	//LOGF ("[ %5d ] connection closed", client->request_num);

	free (client);
}

uv_buf_t on_alloc (uv_handle_t* client, size_t suggested_size)
{
	uv_buf_t buf;
	buf.base = malloc (suggested_size);
	buf.len = suggested_size;
	return buf;
}

void on_read (uv_stream_t* tcp, ssize_t nread, uv_buf_t buf)
{
	size_t parsed;

	client_t* client = (client_t*) tcp->data;

	if (nread >= 0)
	{
		parsed = http_parser_execute (
		             &client->parser, &parser_settings, buf.base, nread);
		if (!parsed)
		{
			LOGF ("parse error %u %u",parsed,nread);
			uv_close ( (uv_handle_t*) &client->handle, on_close);
		}
	}
	else
	{
		uv_err_t err = uv_last_error (uv_loop);
		if (err.code != UV_EOF)
		{
			UVERR (err, "read");
		}
	}

	free (buf.base);
}

static int request_num = 1;

void on_connect (uv_stream_t* server_handle, int status)
{
	CHECK (status, "connect");

	int r;

	assert ( (uv_tcp_t*) server_handle == &server);

	client_t* client = malloc (sizeof (client_t) );
	client->request_num = request_num;

	//LOGF ("[ %5d ] new connection", request_num++);

	uv_tcp_init (uv_loop, &client->handle);
	http_parser_init (&client->parser, HTTP_REQUEST);

	client->parser.data = client;
	client->handle.data = client;

	r = uv_accept (server_handle, (uv_stream_t*) &client->handle);
	CHECK (r, "accept");

	uv_read_start ( (uv_stream_t*) &client->handle, on_alloc, on_read);
}

void after_write (uv_write_t* req, int status)
{
	CHECK (status, "write");

	uv_close ( (uv_handle_t*) req->handle, on_close);
}
static int* complete;
int on_headers_complete (http_parser* parser)
{
	client_t* client = (client_t*) parser->data;

	LOGF ("[ %5d ] http message parsed", client->request_num);

	/*uv_write(
	    &client->write_req,
	    (uv_stream_t*)&client->handle,
	    &resbuf,
	    1,
	    NULL);*/
	//complete=1;
	return 1;
}

int on_url (http_parser* parser, const char *at, size_t length)
{
	client_t* client = (client_t*) parser->data;

	//LOGF ("[ %5d ] url parsed", client->request_num);
	struct http_parser_url *u = malloc (sizeof (struct http_parser_url) );
	uv_tcp_t* handle = &client->handle;
	char* path = malloc (length * sizeof (char) );
	char * content = malloc (1024);
	char * buffer = malloc (1024);
	uv_buf_t resbuf;
	strncpy (path, at, length);
	printf ("%s \n", path);
	size_t user_id;
	sscanf (path, "/user/%u", &user_id);
	if (user_id > server_param->model.parameters.users_number)
	{
		strcpy (content, "{ \n"\
		        "\"error\": { \n"\
		        "\"message\": \"Unsupported get request.\", \n"\
		        "\"type\": \"User Not Valid\", \n"\
		        "}\n"\
		        "}");
	}
	else
	{
		rating_estimator_parameters_t* estim_param = malloc (sizeof (rating_estimator_parameters_t) );
		if (uv_mutex_trylock(&mutex))
		{
		estim_param->lfactors = factors_backup;
		}else
		{
		estim_param->lfactors = factors;
		}
		estim_param->tset = training_set;
		estim_param->user_index = user_id;
		recommended_items_t* rec = recommend_items (estim_param, server_param->model, 4);
		uv_mutex_unlock(&mutex);
		//LOG ("recommendation completed");
		int i;
		sprintf (content, "{ \n \"user_index\" : %u, \n",user_id);
		sprintf (content, "%s \"recommended_items\" : [ \n",content);
		for (i = 0; i < rec->items_number - 1; i++)
		{
			sprintf (content, "%s { \"item\" : %u , \"rating\" : %lf }, \n", content, rec->items[i].index, rec->items[i].rating);
		}
		sprintf (content, "%s { \"item\" : %u , \"rating\" : %lf } \n", content, rec->items[rec->items_number - 1].index, rec->items[rec->items_number - 1].rating);
		sprintf (content, "%s ] \n }", content);
	}
	sprintf (buffer, "%s %u \r\n\r\n%s\n", HEADER, strlen (content), content);

		resbuf.base = buffer;
		resbuf.len = strlen (resbuf.base);
		printf ("%d", resbuf.len);
		uv_write (
	    &client->write_req,
	    (uv_stream_t*) &client->handle,
	    &resbuf,
	    1,
	    after_write);
	free (content);
	free (path);
	return 0;
}
static void timer_close_cb(uv_handle_t* handle) {
	printf("TIMER_CLOSE_CB\n");
}
static void thread_method (void* arg)
{
  uv_mutex_trylock(&mutex);
  free_learned_factors(factors);
  compile_training_set (training_set);
  factors = learn (training_set, server_param->model);
  //sleep(5);
  uv_mutex_unlock(&mutex);
  free_learned_factors(factors_backup);
  factors_backup = copy_learned_factors (factors);
  //LOG ("Learning completed");
}
static void timer_cb(uv_timer_t* handle, int status) {
  printf("TIMER_CB\n");

  assert(handle != NULL);
  assert(status == 0);
  uv_thread_t thread_id;
  uv_thread_create(&thread_id,thread_method,NULL);
  
  
}
int on_value (http_parser* parser, const char *at, size_t length)
{	
	client_t* client = (client_t*)(parser->data);

   
char* a;
if(!complete[client->request_num])
{

a = parse_post_request(at,"(user=[[:digit:]]{0,4}&item=[[:digit:]]{0,4})");
	if(a!=NULL)
		complete[client->request_num]=1;
	else
		return 0;
	LOGF("%s \n", a);
	LOGF("%u %u \n", length, client->request_num);
}

	return 0;
}
int main (int argc, char** argv)
{
	int r;
	server_param = parse_arguments (argc, argv);
	LOG ("Extracting data ...");
	training_set = server_extract_data (server_param);
	LOG ("Learning ...");
	compile_training_set (training_set);
	factors = learn (training_set, server_param->model);
	factors_backup = copy_learned_factors (factors);
	LOG ("Learning completed");
	complete = malloc(20 *sizeof(int));
	memset(complete,0,20 *sizeof(int));
	parser_settings.on_headers_complete = on_headers_complete;
	parser_settings.on_url = on_url;
	parser_settings.on_header_value = on_value;
	uv_loop = uv_default_loop();

	r = uv_tcp_init (uv_loop, &server);
	CHECK (r, "bind");

	struct sockaddr_in address = uv_ip4_addr ("0.0.0.0", server_param->port);

	r = uv_tcp_bind (&server, address);
	CHECK (r, "bind");
	uv_listen ( (uv_stream_t*) &server, 128, on_connect);

	LOGF ("listening on port %u", server_param->port);
    uv_timer_t timer;
    r = uv_timer_init(uv_default_loop(), &timer);
    assert(r == 0);
   // r = uv_timer_start(&timer, timer_cb, 10000, 10000);
    assert(r == 0);
	
  	r = uv_mutex_init(&mutex);
  	assert(r == 0);
	uv_run (uv_loop);
}
