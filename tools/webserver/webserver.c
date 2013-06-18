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
"Content-Type: application/json \r\n" \
"Access-Control-Allow-Origin: * \r\n" \
"Content-Length: "

#define POST_SUCCESS \
"HTTP/1.1 200 OK\r\n" \
"Content-Type: application/json\r\n" \
"Access-Control-Allow-Origin: * \r\n" \
"Content-Length: 16 \r\n" \
"\r\n"\
"{\"status\": \"OK\"}\n" 


#define POST_ERROR \
"HTTP/1.1 200 OK\r\n" \
"Content-Type: application/json\r\n" \
"Access-Control-Allow-Origin: * \r\n" \
"Content-Length: 17 \r\n" \
"\r\n"\
"{status: \"ERROR\"}\n" 

static uv_loop_t* uv_loop;
static uv_tcp_t server;
static http_parser_settings parser_settings;
static uv_mutex_t factors_mutex;
static uv_mutex_t factors_backup_mutex;
static uv_mutex_t tset_mutex;

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

	LOGF ("[ %5d ] new connection", request_num++);

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
	if(parser->method != 1)
		return 0;
	LOGF ("[ %5d ] url parsed", client->request_num);
	struct http_parser_url *u = malloc (sizeof (struct http_parser_url) );
	uv_tcp_t* handle = &client->handle;
	char* path = malloc ((length +1)* sizeof (char) );
	char * content = malloc (304800);
	char * buffer = malloc (304800);
	uv_buf_t resbuf;
	strncpy (path, at, length);
	size_t user_id=server_param->model.parameters.users_number;
	path[length]=0;
	sscanf (path, "/user=%u", &user_id);
	
	if (user_id >= server_param->model.parameters.users_number)
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
		int backup=0;
		if (uv_mutex_trylock(&factors_mutex))
		{
		uv_mutex_lock(&factors_backup_mutex);
		backup=1;
		estim_param->lfactors = factors;
		printf("Backup \n");
	        printf("factors_backup_mutex locked outside of thread \n");
		}else
		{
		estim_param->lfactors = factors_backup;
		}
		estim_param->tset = training_set;
		estim_param->user_index = user_id;
		recommended_items_t* rec = recommend_items (estim_param, server_param->model, 4);
		uv_mutex_unlock(&factors_mutex);
		if(backup==1)
		{
			uv_mutex_unlock(&factors_backup_mutex);
		}
		LOG ("recommendation completed");
		int i;
		sprintf (content, "{ \n \"user_index\" : %u, \n",user_id);
		
		sprintf (content, "%s \"recommended_items\" : [ \n",content);
		for (i = 0; i < rec->items_number - 1; i++)
		{
			sprintf (content, "%s { \"item\" : %u , \"rating\" : %lf }, \n", content, rec->items[i].index, rec->items[i].rating);
		}
		sprintf (content, "%s { \"item\" : %u , \"rating\" : %lf } \n ], \n", content, rec->items[rec->items_number - 1].index, rec->items[rec->items_number - 1].rating);
		coo_matrix_t* top_rated_items=get_top_rated_items(training_set,user_id,6);
		sprintf (content, "%s \"top_rated_items\" : [ \n",content);
		for(i=0;i < top_rated_items->size - 1; i++)
		{
			sprintf (content, "%s { \"item\" : %u , \"rating\" : %lf }, \n", 
				 content, top_rated_items->entries[i].row_i, top_rated_items->entries[i].value);
		}
			sprintf (content, "%s { \"item\" : %u , \"rating\" : %lf } \n ], \n", 
				 content, top_rated_items->entries[top_rated_items->size-1].row_i, top_rated_items->entries[top_rated_items->size-1].value);
		coo_matrix_t* rated_items=get_rated_items(training_set,user_id);
		sprintf (content, "%s \"rated_items\" : [ \n",content);
		for(i=0;i < rated_items->current_size; i++)
		{
			sprintf (content, "%s { \"item\" : %u , \"rating\" : %lf }, \n", 
				 content, rated_items->entries[i].row_i, rated_items->entries[i].value);
		}
			sprintf (content, "%s { \"item\" : %u , \"rating\" : %lf } \n ] \n", 
				 content, rated_items->entries[rated_items->current_size-1].row_i, rated_items->entries[rated_items->current_size-1].value);
		sprintf (content, "%s  \n }", content);
		free_coo_matrix(rated_items);
		free_coo_matrix(top_rated_items);
		
	}
		sprintf (buffer, "%s %u \r\n\r\n%s\n", HEADER, strlen (content), content);

		resbuf.base = buffer;
		resbuf.len = strlen (resbuf.base);
		
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
  uv_mutex_lock(&factors_mutex);
  free_learned_factors(factors);
  compile_training_set (training_set);
  factors = learn (training_set, server_param->model);
  
  uv_mutex_lock(&factors_backup_mutex);
  printf("factors_backup_mutex locked inside of thread \n");
  free_learned_factors(factors_backup);
  factors_backup = copy_learned_factors (factors);
  uv_mutex_unlock(&factors_backup_mutex);
  uv_mutex_unlock(&factors_mutex);
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
	if(parser->method!=3)
		return 0;
	client_t* client = (client_t*)(parser->data);
	uv_tcp_t* handle = &client->handle;
	uv_buf_t resbuf;
	coo_entry_t* rating;
	if(!complete[client->request_num])
	{
	rating = get_rating_from_http(at,"(user=[[:digit:]]{0,4}&item=[[:digit:]]{0,4}&rating=[[:digit:]])");
	complete[client->request_num]=1;
	if(rating!=NULL)
	{
		LOGF("%u %u %lf \n", rating->row_i,rating->column_j, rating->value);
		uv_mutex_lock(&tset_mutex);
		compile_training_set(training_set);
		add_rating(rating->row_i,rating->column_j,rating->value,training_set);
		uv_mutex_unlock(&tset_mutex);
		resbuf.base = malloc(sizeof(POST_SUCCESS));
		strcpy(resbuf.base,POST_SUCCESS);
		resbuf.len=sizeof(POST_SUCCESS);
		printf("%s",resbuf.base);
		free(rating);
	}else
	{
		LOG("Parameters incorrect");
		resbuf.base=POST_ERROR;
		resbuf.len=sizeof(POST_ERROR);
	}
	
	uv_write(&client->write_req, (uv_stream_t*) handle, &resbuf, 1, after_write);
	

	}else return 1;

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
	//parser_settings.on_headers_complete = on_headers_complete;
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
   	r = uv_timer_start(&timer, timer_cb, 10000, 10000);
    	assert(r == 0);
	
  	r = uv_mutex_init(&factors_mutex);
  	assert(r == 0);
  	r = uv_mutex_init(&factors_backup_mutex);
  	assert(r == 0);
  	r = uv_mutex_init(&tset_mutex);
  	assert(r == 0);
	uv_run (uv_loop);
}
