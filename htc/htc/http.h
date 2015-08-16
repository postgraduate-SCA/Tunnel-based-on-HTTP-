#ifndef HTTP_H
#define HTTP_H
#include <WinSock2.h>
#include <stdio.h>
typedef enum
{
  HTTP_GET,
  HTTP_PUT,
  HTTP_POST,
  HTTP_OPTIONS,
  HTTP_HEAD,
  HTTP_DELETE,
  HTTP_TRACE,
  HTTP_ERROR=-1
} Http_method;

typedef struct http_header Http_header;
struct http_header
{
  const char *name;
  const char *value;
  Http_header *next; /* FIXME: this is ugly; need cons cell. */
};

typedef struct
{
  Http_method method;
  const char *uri;
  int major_version;
  int minor_version;
  Http_header *header;
} Http_request;

typedef struct
{
   int major_version;
   int minor_version;
   int status_code;
   const char *status_message;
   Http_header *header;
} Http_response;

typedef struct
{
  const char *host_name;
  int host_port;
  const char *user_agent;
} Http_destination;
//typedef int ssize_t;
extern int http_get (SOCKET fd, Http_destination *dest);
extern int http_put (SOCKET fd, Http_destination *dest,
			 int content_length);
extern int http_post (SOCKET fd, Http_destination *dest,
			  int content_length);
extern int http_error_to_errno (int err);

extern Http_response *http_create_response (int major_version,
					    int minor_version,
					    int status_code,
					    const char *status_message);
extern int http_parse_response(SOCKET fd,Http_response **response);
extern void http_destroy_response (Http_response *response);

extern Http_header *http_add_header (Http_header **header,
				     const char *name,
				     const char *value);

extern Http_request *http_create_request (Http_method method,
					  const char *uri,
					  int major_version,
					  int minor_version);
extern int http_parse_request (SOCKET fd, Http_request **request);
extern int http_write_request (SOCKET fd, Http_request *request);
extern void http_destroy_request (Http_request *resquest);

extern const char *http_header_get (Http_header *header, const char *name);

#endif