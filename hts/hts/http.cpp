#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include<malloc.h>
#include <string.h>


#include "http.h"
#include "common.h"


/*
HTTP method 函数类
*/
static  int http_method (SOCKET fd, Http_destination *dest,
	     Http_method method, int length)
{
  char str[1024]; /* FIXME: possible buffer overflow */
  Http_request *request;
  int n;

  if (fd == INVALID_SOCKET)
    {
      return -1;
    }

  n = 0;
 

  request = http_create_request (method, str, 1, 1);
  if (request == NULL)
    return -1;

  sprintf (str, "%s:%d", dest->host_name, dest->host_port);
  http_add_header (&request->header, "Host", str);

  if (length >= 0)
    {
      sprintf (str, "%d", length);
      http_add_header (&request->header, "Content-Length", str);
    }

  http_add_header (&request->header, "Connection", "close");
   if (dest->user_agent)
    {
      http_add_header (&request->header,
		       "User-Agent",
		       dest->user_agent);
    }

  n = http_write_request (fd, request);
  http_destroy_request (request);
  return n;
}
int http_get (SOCKET fd, Http_destination *dest)
{

  return http_method (fd, dest, HTTP_GET, -1);
}

int http_put (SOCKET fd, Http_destination *dest, int length)
{
  return http_method (fd, dest, HTTP_PUT, length);
}

int http_post (SOCKET fd, Http_destination *dest, int length)
{
  return http_method (fd, dest, HTTP_POST, length);
}

/*
method <=>string 类
*/
static Http_method http_string_to_method (const char *method, int n)
{
  if (strncmp (method, "GET", n) == 0)
    return HTTP_GET;
  if (strncmp (method, "PUT", n) == 0)
    return HTTP_PUT;
  if (strncmp (method, "POST", n) == 0)
    return HTTP_POST;
  if (strncmp (method, "OPTIONS", n) == 0)
    return HTTP_OPTIONS;
  if (strncmp (method, "HEAD", n) == 0)
    return HTTP_HEAD;
  if (strncmp (method, "DELETE", n) == 0)
    return HTTP_DELETE;
  if (strncmp (method, "TRACE", n) == 0)
    return HTTP_TRACE;
  return HTTP_ERROR;
}
static const char * http_method_to_string (Http_method method)
{
  switch (method)
    {
    case HTTP_GET: return "GET";
    case HTTP_PUT: return "PUT";
    case HTTP_POST: return "POST";
    case HTTP_OPTIONS: return "OPTIONS";
    case HTTP_HEAD: return "HEAD";
    case HTTP_DELETE: return "DELETE";
    case HTTP_TRACE: return "TRACE";
    }
  return "(uknown)";
}
/*
request io 类函数
*/
static int read_until (SOCKET fd, int ch, unsigned char **data)
{
  unsigned char *buf, *buf2;
  int n, len, buf_size;

  *data = NULL;

  buf_size = 100;
  buf = (unsigned char *)malloc (buf_size);
  if (buf == NULL)
    {
      return -1;
    }

  len = 0;
  while ((n = read_all (fd, buf + len, 1)) == 1)
    {
      if (buf[len++] == ch)
	break;
      if (len + 1 == buf_size)
	{
	  buf_size *= 2;
	  buf2 = (unsigned char * )realloc (buf, buf_size);
	  if (buf2 == NULL)
	    {
	       free (buf);
	      return -1;
	    }
	  buf = buf2;
	}
    }
  if (n <= 0)
    {
      free (buf);
       return n;
    }

  /* Shrink to minimum size + 1 in case someone wants to add a NUL. */
  buf2 = ( unsigned char * ) realloc (buf, len + 1);
  if (buf2 == NULL);
  else
    buf = buf2;
   *data = buf;
  return len;
}
static int http_write_header (SOCKET fd, Http_header *header)
{
   int n = 0, m;

  if (header == NULL)
    return write_all (fd, "\r\n", 2);

  m = write_all (fd, (void *)header->name, strlen (header->name));
  if (m == -1)
    {
      return -1;
    }
  n += m;

  m = write_all (fd, ": ", 2);
  if (m == -1)
    {
      return -1;
    }
  n += m;

  m = write_all (fd, (void *)header->value, strlen (header->value));
  if (m == -1)
    {
      return -1;
    }
  n += m;

  m = write_all (fd, "\r\n", 2);
  if (m == -1)
    {
      return -1;
    }
  n += m;

  m = http_write_header (fd, header->next);
  if (m == -1)
    {
      return -1;
    }
  n += m;

  return n;
}
int http_write_request (SOCKET fd, Http_request *request)
{
  char str[1024]; /* FIXME: buffer overflow */
  int n = 0;
  int m;

  m = sprintf (str, "%s %s HTTP/%d.%d\r\n",
	       http_method_to_string (request->method),
	       request->uri,
	       request->major_version,
	       request->minor_version);
  m = write_all (fd, str, m);
  
  if (m == -1)
    {
    
      return -1;
    }
  n += m;

  m = http_write_header (fd, request->header);
  if (m == -1)
    {
      return -1;
    }
  n += m;

  return n;
}

/*
request and response 构造函数
*/
static  Http_header * http_alloc_header (const char *name, const char *value)
{
  Http_header *header;

  header = (Http_header * )malloc (sizeof (Http_header));
  if (header == NULL)
    return NULL;

  header->name = header->value = NULL;
  header->name = _strdup (name);
  header->value = _strdup (value);
  if (name == NULL || value == NULL)
    {
      if (name == NULL)
	free ((char *)name);
      if (value == NULL)
	free ((char *)value);
      free (header);
      return NULL;
    }
  return header;
}
static  Http_response * http_allocate_response (const char *status_message)
{
  Http_response *response;

  response = (Http_response * ) malloc (sizeof (Http_response));
  if (response == NULL)
    return NULL;

  response->status_message = _strdup (status_message);
  if (response->status_message == NULL)
    {
      free (response);
      return NULL;
    }

  return response;
}

Http_response * http_create_response (int major_version,
		     int minor_version,
		     int status_code,
		     const char *status_message)
{
  Http_response *response;

  response = http_allocate_response (status_message);
  if (response == NULL)
    return NULL;

  response->major_version = major_version;
  response->minor_version = minor_version;
  response->status_code = status_code;
  response->header = NULL;

  return response;
}
static  Http_request*  http_allocate_request (const char *uri)
{
  Http_request *request;
  Http_request tmp;
  request =(Http_request *)malloc (sizeof (tmp));
  if (request == NULL)
    return NULL;

  request->uri = _strdup (uri);
  if (request->uri == NULL)
    {
      free (request);
      return NULL;
    }

  return request;
}

Http_request * http_create_request (Http_method method,
		     const char *uri,
		     int major_version,
		     int minor_version)
{
  Http_request *request;

  request = http_allocate_request (uri);
  if (request == NULL)
    return NULL;

  request->method = method;
  request->major_version = major_version;
  request->minor_version = minor_version;
  request->header = NULL;

  return request;
}
/*
解析函数
*/
static int parse_header (SOCKET fd, Http_header **header)
{
  unsigned char buf[2];
  unsigned char *data;
  Http_header *h;
  int len;
  int n;

  *header = NULL;

  n = read_all (fd, buf, 2);
  if (n <= 0)
    return n;
  if (buf[0] == '\r' && buf[1] == '\n')
    return n;

  h = ( Http_header * ) malloc (sizeof (Http_header));
  if (h == NULL)
    {
     
      return -1;
    }
  *header = h;
  h->name = NULL;
  h->value = NULL;

  n = read_until (fd, ':', &data);
  if (n <= 0)
    return n;
  data = (unsigned char * ) realloc (data, n + 2);
  if (data == NULL)
    {
    
      return -1;
    }
  memmove (data + 2, data, n);
  memcpy (data, buf, 2);
  n += 2;
  data[n - 1] = 0;
  h->name = (const char * ) data;
  len = n;

  n = read_until (fd, '\r', &data);
  if (n <= 0)
    return n;
  data[n - 1] = 0;
  h->value =  (const char * )data;
  len += n;

  n = read_until (fd, '\n', &data);
  if (n <= 0)
    return n;
  free (data);
  if (n != 1)
    {
   
      return -1;
    }
  len += n;

  

  n = parse_header (fd, &h->next);
  if (n <= 0)
    return n;
  len += n;

  return len;
}
int http_parse_request (SOCKET fd, Http_request **request_)
{
  Http_request *request;
  unsigned char *data;
  int len;
  int n;

  (*request_) = NULL;

  request = ( Http_request * )malloc (sizeof (Http_request));
  if (request == NULL)
    {
      return -1;
    }

  request->method = HTTP_ERROR;
  request->uri = NULL;
  request->major_version = -1;
  request->minor_version = -1;
  request->header = NULL;

  n = read_until (fd, ' ', &data);
  if (n <= 0)
    {
      free (request);
      return n;
    }
  request->method = http_string_to_method ((const char *)data, n - 1);
  if (request->method == -1)
    {
     
      free (data);
      free (request);
      return -1;
    }
  data[n - 1] = 0;
  free (data);
  len = n;

  n = read_until (fd, ' ', &data);
  if (n <= 0)
    {
      free (request);
      return n;
    }
  data[n - 1] = 0;
  request->uri = ( const char * )data;
  len += n;
  

  n = read_until (fd, '/', &data);
  if (n <= 0)
    {
      http_destroy_request (request);
      return n;
    }
  else if (n != 5 || memcmp (data, "HTTP", 4) != 0)
    {
     
      free (data);
      http_destroy_request (request);
      return -1;
    }
  free (data);
  len = n;

  n = read_until (fd, '.', &data);
  if (n <= 0)
    {
      http_destroy_request (request);
      return n;
    }
  data[n - 1] = 0;
  request->major_version = atoi ((const char * )data);
  free (data);
  len += n;

  n = read_until (fd, '\r', &data);
  if (n <= 0)
    {
      http_destroy_request (request);
      return n;
    }
  data[n - 1] = 0;
  request->minor_version = atoi ( (const char * )data);
  
  free (data);
  len += n;

  n = read_until (fd, '\n', &data);
  if (n <= 0)
    {
      http_destroy_request (request);
      return n;
    }
  free (data);
  if (n != 1)
    {
    
      http_destroy_request (request);
      return -1;
    }
  len += n;

  n = parse_header (fd, &request->header);
  if (n <= 0)
    {
      http_destroy_request (request);
      return n;
    }
  len += n;

  *request_ = request;
 return len;
}
int http_parse_response (SOCKET fd, Http_response **response_)
{
  Http_response *response;
  unsigned char *data;
  int len;
  int n;

  *response_ = NULL;

  response =(Http_response *)malloc (sizeof (Http_response));
  if (response == NULL)
    {
   
      return -1;
    }

  response->major_version = -1;
  response->minor_version = -1;
  response->status_code = -1;
  response->status_message = NULL;
  response->header = NULL;

  n = read_until (fd, '/', &data);
  if (n <= 0)
    {
      free (response);
      return n;
    }
  else if (n != 5 || memcmp (data, "HTTP", 4) != 0)
    {
    
      free (data);
      free (response);
      return -1;
    }
  free (data);
  len = n;

  n = read_until (fd, '.', &data);
  if (n <= 0)
    {
      free (response);
      return n;
    }
  data[n - 1] = 0;
  response->major_version = atoi(( const char *)data);
  
  free (data);
  len += n;

  n = read_until (fd, ' ', &data);
  if (n <= 0)
    {
      free (response);
      return n;
    }
  data[n - 1] = 0;
  response->minor_version = atoi (( const char * )data);
 
  free (data);
  len += n;

  n = read_until (fd, ' ', &data);
  if (n <= 0)
    {
      free (response);
      return n;
    }
  data[n - 1] = 0;
  response->status_code = atoi ((const char * )data);
 
  free (data);
  len += n;

  n = read_until (fd, '\r', &data);
  if (n <= 0)
    {
      free (response);
      return n;
    }
  data[n - 1] = 0;
  response->status_message = (const char * )data;
  
  len += n;

  n = read_until (fd, '\n', &data);
  if (n <= 0)
    {
      http_destroy_response (response);
      return n;
    }
  free (data);
  if (n != 1)
    {
     
      http_destroy_response (response);
      return -1;
    }
  len += n;

  n = parse_header (fd, &response->header);
  if (n <= 0)
    {
      http_destroy_response (response);
      return n;
    }
  len += n;

  *response_ = response;
  return len;
}
/*
destory 函数
*/
static void http_destroy_header (Http_header *header)
{
  if (header == NULL)
    return;

  http_destroy_header (header->next);

  if (header->name)
    free ((char *)header->name);
  if (header->value)
    free ((char *)header->value);
  free (header);
}

void http_destroy_response (Http_response *response)
{
  if (response->status_message)
    free ((char *)response->status_message);
  http_destroy_header (response->header);
  free (response);
}

void http_destroy_request (Http_request *request)
{
  if (request->uri)
    free ((char *)request->uri);
  http_destroy_header (request->header);
  free (request);
}

/*
http header操作函数
*/
Http_header * http_add_header (Http_header **header, const char *name, const char *value)
{
  Http_header *new_header;

  new_header = http_alloc_header (name, value);
  if (new_header == NULL)
    return NULL;

  new_header->next = NULL;
  while (*header)
    header = &(*header)->next;
  *header = new_header;

  return new_header;
}
static Http_header * http_header_find (Http_header *header, const char *name)
{
  if (header == NULL)
    return NULL;

  if (strcmp (header->name, name) == 0)
    return header;

  return http_header_find (header->next, name);
}
const char * http_header_get (Http_header *header, const char *name)
{
  Http_header *h;

  h = http_header_find (header, name);
  if (h == NULL)
    return NULL;

  return h->value;
}


void output_header ( Http_header *header)
{
	if(header == NULL )
		printf("\r\n");
	else 
		{
			printf( " %s: %s\r\n",header->name,header->value);
			output_header ( header->next);    
	}
}
void output ( Http_request *request)
{
	char str[1024];
	printf("%s %s HTTP/%d.%d\r\n",http_method_to_string(request->method),request->uri,request->major_version,request->minor_version);
	output_header(request->header);
}
/*
void http_header_set (Http_header **header, const char *name, const char *value)
{
  Http_header *h;
  size_t n;
  char *v;

  n = strlen (value);
  v = (char *)malloc (n + 1);
  if (v == NULL)return;
    
  memcpy (v, value, n + 1);

  h = http_header_find (*header, name);
  if (h == NULL)
    {
      Http_header *h2;

      h2 = malloc (sizeof (Http_header));
      if (h2 == NULL)return;

      n = strlen (name);
      h2->name = malloc (strlen (name) + 1);
      if (h2->name == NULL)return;
      memcpy (h2->name, name, n + 1);

      h2->value = v;

      h2->next = *header;

      *header = h2;

      return NULL;
    }
  else
    {
      free (h->value);
      h->value = v;
    }
}
*/