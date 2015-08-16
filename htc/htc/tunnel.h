#ifndef TUNNEL_H
#define TUNNEL_H
#define DEFAULT_CONNECTION_MAX_TIME 300//millsecond;
#include"http.h"

struct Tunnel
{
  SOCKET in_fd, out_fd;
  SOCKET server_socket;
  Http_destination dest;
  struct sockaddr_in address;
  int bytes;
  int  content_length;
  char buf[65536];
  char *buf_ptr;
  int  buf_len;
  int padding_only;
 /* size_t in_total_raw;
  size_t in_total_data;
  size_t out_total_raw;
  size_t out_total_data;
   */
  //time_t out_connect_time;
  int strict_content_length;
  //int keep_alive;
  //int max_connection_age;
 
};

static  int tunnel_is_disconnected(Tunnel *tunnel);
static  int tunnel_is_connected(Tunnel *tunnel);
static  int tunnel_is_server(Tunnel *tunnel);
static  int tunnel_is_client(Tunnel *tunnel);
static void tunnel_out_disconnect(Tunnel *tunnel);
static void tunnel_in_disconnect(Tunnel *tunnel);
static int tunnel_in_connect(Tunnel *tunnel);
static int tunnel_out_connect(Tunnel *tunnel);
static  int tunnel_write_data(Tunnel *tunnel,void *data,int length);

extern int tunnel_read ( Tunnel * tunnel, void *data, int length );
extern int tunnel_write(Tunnel *tunnel,void *data,int length);
extern int tunnel_padding(Tunnel *tunnel,int length);
extern int tunnel_maybe_pad(Tunnel *tunnel,int length);
extern Tunnel *tunnel_new_client (const char *host, int host_port);
extern Tunnel *tunnel_new_server ( int port );
extern int tunnel_setopt (Tunnel *tunnel, const char *opt, void *data);
extern int tunnel_getopt (Tunnel *tunnel, const char *opt, void *data);
extern int tunnel_connect (Tunnel *tunnel);
extern int tunnel_accept (Tunnel *tunnel);
extern int tunnel_connect (Tunnel *tunnel);
extern void tunnel_destory (Tunnel *tunnel);
#endif