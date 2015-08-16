#include<stdio.h>
#include<fcntl.h>
#include<stdlib.h>
#include<WinSock2.h>
#include<time.h>

//typedef unsigned char Request;
//typedef unsigned short Length;

#include"http.h"
#include"tunnel.h"
#include"common.h"

#define DEFAULT_OUT_CONNECTION_TIME 5//seconds
#define ACCEPT_TIME 10//second.
static const int sizeof_header = sizeof (int) + sizeof (int);
enum tunnel_request
{
  TUNNEL_SIMPLE = 0x40,
  TUNNEL_OPEN = 0x01,
  TUNNEL_DATA = 0x02,
  TUNNEL_PADDING = 0x03,
  TUNNEL_ERROR = 0x04,
  TUNNEL_PAD1 = TUNNEL_SIMPLE | 0x05,
  TUNNEL_CLOSE = TUNNEL_SIMPLE | 0x06,
  TUNNEL_DISCONNECT = TUNNEL_SIMPLE | 0x07
};


/*
创建隧道对象函数
*/
Tunnel *tunnel_new_client (const char *host, int host_port)
{
	
	const char *remote;
	int remote_port;
	Tunnel *tunnel;
	tunnel=(Tunnel *)malloc(sizeof(Tunnel));
	if(tunnel==NULL)
	{
		return NULL;
	}
	tunnel->in_fd=INVALID_SOCKET;
	tunnel->out_fd=INVALID_SOCKET;
	tunnel->server_socket=INVALID_SOCKET;
	tunnel->dest.host_name=host;
	tunnel->dest.host_port=host_port;
	tunnel->dest.user_agent=NULL;
	
	tunnel->content_length=DEFAULT_CONTENT_LENGTH;
	tunnel->buf_ptr=tunnel->buf;
    tunnel->buf_len=0;
	/*
    tunnel->in_total_raw=0;
	tunnel->in_total_data=0;
	tunnel->out_total_raw=0;
	tunnel->out_total_data=0;
	*/
	tunnel->strict_content_length=FALSE;
	tunnel->bytes=0;
	remote=tunnel->dest.host_name;
	remote_port=tunnel->dest.host_port;
	if(set_address(&tunnel->address,remote,remote_port)==-1)
	{
		free(tunnel);
		return NULL;
	}
	//tunnel->keep_alive = DEFAULT_KEEP_ALIVE;
	//tunnel->max_connection_age =  DEFAULT_MAX_CONNECTION_AGE;
	//tunnel->out_connect_time = DEFAULT_OUT_CONNECTION_TIME;
	return tunnel;
}
Tunnel * tunnel_new_server (int port )
{
	Tunnel *tunnel;
	tunnel = ( Tunnel *) malloc (sizeof (Tunnel ));
	if ( tunnel == NULL )
		return NULL ;
	//if ( content_length == 0 )
		//content_length = DEFAULT_CONTENT_LENGTH;
	tunnel->in_fd = INVALID_SOCKET;
	tunnel->out_fd = INVALID_SOCKET;
	tunnel->server_socket = INVALID_SOCKET;
	tunnel->dest.host_name = NULL;
	tunnel->dest.host_port = port;
	tunnel->buf_ptr = tunnel->buf;
	tunnel->buf_len = 0;
	
	tunnel->content_length = DEFAULT_CONTENT_LENGTH - 1;
	/*
	tunnel->in_total_raw = 0;
	tunnel->in_total_data = 0;
	tunnel->out_total_raw = 0;
	tunnel->out_total_data = 0;
	*/
	tunnel->strict_content_length=FALSE;
	tunnel->bytes = 0;
	
	tunnel->server_socket = server_socket (port);

	if ( tunnel->server_socket == INVALID_SOCKET)
		return NULL;
	return tunnel;
}
/*
隧道参数类函数；

static int tunnel_opt(Tunnel *tunnel,const char *opt,void *data,int get_flag)
{
	if(strcmp(opt,"strict_content_length")==0)
	{
		 if (get_flag)*(int *)data = tunnel->strict_content_length;
         else tunnel->strict_content_length = *(int *)data;

	}
	else if (strcmp (opt, "keep_alive") == 0)
    {
      if (get_flag)*(int *)data = tunnel->keep_alive;
      else tunnel->keep_alive = *(int *)data;
    }
	else if (strcmp (opt, "max_connection_age") == 0)
    {
      if (get_flag) *(int *)data = tunnel->max_connection_age;
      else tunnel->max_connection_age = *(int *)data;
    }
	else if (strcmp (opt, "user_agent") == 0)
    {
      if (get_flag)
	{
	  if (tunnel->dest.user_agent == NULL)*(char **)data = NULL;
	  else *(char **)data = _strdup (tunnel->dest.user_agent);
	}
      else
	{
	  if (tunnel->dest.user_agent != NULL)free ((char *)tunnel->dest.user_agent);
	  tunnel->dest.user_agent = _strdup ((char *)data);
	  if (tunnel->dest.user_agent == NULL)
	    return -1;
	}
    }
	else return -1;
	return 0;
}
int tunnel_setopt (Tunnel *tunnel, const char *opt, void *data)
{
  return tunnel_opt (tunnel, opt, data, FALSE);
}

int tunnel_getopt (Tunnel *tunnel, const char *opt, void *data)
{
  return tunnel_opt (tunnel, opt, data, TRUE);
}
*/
/*
隧道状态类函数
*/
static  int tunnel_is_disconnected(Tunnel *tunnel)
{
	return tunnel->out_fd==INVALID_SOCKET;
}
static  int tunnel_is_connected(Tunnel *tunnel)
{
	return !tunnel_is_disconnected(tunnel);
}
static  int tunnel_is_server(Tunnel *tunnel)
{
	return tunnel->server_socket!=INVALID_SOCKET;
}
static  int tunnel_is_client(Tunnel *tunnel)
{
	return !tunnel_is_server(tunnel);
}
/*
隧道单连接操作类函数
/*
函数说明：断开隧道的输出连接；
*/
static void tunnel_out_disconnect(Tunnel *tunnel)
{
	if(tunnel_is_disconnected(tunnel))return;
	closesocket(tunnel->out_fd);
	tunnel->out_fd=-1;
	tunnel->bytes=0;
	tunnel->buf_ptr=tunnel->buf;
	tunnel->buf_len=0;
}
static void tunnel_in_disconnect(Tunnel *tunnel)
{
	if(tunnel->in_fd==INVALID_SOCKET)return;
	closesocket(tunnel->in_fd);
	tunnel->in_fd=INVALID_SOCKET;
}
static int tunnel_in_connect(Tunnel *tunnel)
{
	Http_response *response;
	int n;
	if(tunnel->in_fd!=INVALID_SOCKET)return -1;
	tunnel->in_fd=do_connect(&tunnel->address);
	if(tunnel->in_fd==INVALID_SOCKET)return -1;
	//发送get请求；
	if(http_get(tunnel->in_fd,&tunnel->dest)==-1)return -1;
	//解析接收到的响应；
	n=http_parse_response(tunnel->in_fd,&response);
	if(n<=0);
	else if(response->major_version!=1||(response->minor_version!=1&&response->minor_version!=0))//不是1.1或者1.0
	{
		n=-1;
	}
	else if(response->status_code!=200)
	{
		n=-1;
	}
	if(response!=NULL)http_destroy_response(response);
	/*
	if(n>0)
	{
		
		//tunnel->in_total_raw+=n;
	}
	else */
	if ( n <= 0 ) return n;
	return 1;
}

/*
函数说明：重新发起一个隧道连接；
*/
static int tunnel_out_connect(Tunnel *tunnel)
{
	int n;
	if(tunnel_is_connected(tunnel))
	{
		tunnel_out_disconnect(tunnel);
	}
	tunnel->out_fd=do_connect(&tunnel->address);
	if(tunnel->out_fd==INVALID_SOCKET)
	{
		//printf("tunnel_out_connect connect 失败!\n");
		return -1;
    }
	//printf("进入http_post函数!\n");
	n=http_post(tunnel->out_fd,&tunnel->dest,tunnel->content_length+1);
	//printf("http_post函数退出!\n");
	if(n==-1)
		{
			printf(" tunnel_out_connect http_post 失败!\n");
			return -1;
	    }
	//tunnel->out_total_raw+=n;
	tunnel->bytes=0;
	tunnel->buf_len=0;
	tunnel->padding_only=TRUE;
	//time(&tunnel->out_connect_time);
}

/*
隧道IO操作函数；
*/

/*
函数说明：往隧道写入length的数据；
*/
static  int tunnel_write_data(Tunnel *tunnel,void *data,int length)
{
	if(write_all(tunnel->out_fd,data,length)==-1)return -1;
	tunnel->bytes+=length;
	return length;
}
/*
函数说明：往隧道中写入一个隧道请求命令；
*/
static int tunnel_write_request(Tunnel *tunnel,char request,void *data,u_short length)
{
	//if(tunnel->bytes+sizeof(request)+( data == NULL ?sizeof(length)+length:0)>tunnel->content_length)
		//tunnel_padding(tunnel,tunnel->content_length-tunnel->bytes);
	//如果要写入的字节数(命令字+长度值+数据）大于可写入的字节数

	
		/*
		time_t t;
		t=time(NULL);
		
		if(tunnel_is_client(tunnel)&&
			tunnel_is_connected(tunnel)&&
			t-tunnel->out_connect_time>tunnel->max_connection_age)
		{
			char c=TUNNEL_DISCONNECT;
			if(tunnel->strict_content_length)
			{
				int l=tunnel->content_length-tunnel->bytes-1;
				if(l>3)
				{
					char c;
					short s;
					int i;

					c=TUNNEL_PADDING;
					tunnel_write_data(tunnel,&c,sizeof(c));
					s=htons(l-2);
					tunnel_write_data(tunnel,&s,sizeof(s));
					l-=2;
					c=0;
					for(i=0;i<l;i++)
						tunnel_write_data(tunnel,&c,sizeof(c));
				}
				else
				{
					char c=TUNNEL_PAD1;
					int i;
					for(i=0;i<l;i++)
					tunnel_write_data(tunnel,&c,sizeof(c));

				}
			}
			if(tunnel_write_data(tunnel,&c,sizeof(c))<=0)return -1;
			tunnel_out_disconnect(tunnel);
		}
		*/
	
	//如果是客户端数据并且正在连接超时的话， 断开;
	//char str[100];
	if(tunnel_is_disconnected(tunnel))
	{
		if(tunnel_is_client(tunnel))
		{
			if(tunnel_out_connect(tunnel)==-1)return -1;
		}
		else 
		{
               if(tunnel_accept(tunnel)==-1)
				   return -1;
		}
	}
	//printf("tunnel_write_request 开始写数据!\n"); 
	
	// 如果是客户端重连，服务器端接收连接：
	if(request!=TUNNEL_PADDING&&request!=TUNNEL_PAD1)tunnel->padding_only=FALSE;
	//发送隧道请求命令；
	//str[0]=request;
	//str[1]='\0';
	if(request != TUNNEL_OPEN)
	{
	if(tunnel_write_data(tunnel,(char *)&request,sizeof(request))==-1)
	//if(tunnel_write_data(tunnel,str,1)==-1)
		{
			return -1;
	    }
	    //printf("命令：%d\n",request);
	}
	
	 //发送隧道数据：长度+数据；
	if(data!=NULL)
	{
	    
		u_short network_length=htons(length);
		
		if(tunnel_write_data(tunnel,(char *)&network_length,sizeof(u_short))==-1)
			return -1;
		//printf("数据长度:%hu---%hu\n",network_length,length);
		if(tunnel_write_data(tunnel,data,(int)length)==-1)return -1;
		//printf("数据：%s\n",data);
	}
	/*
	if(data)
	{
		tunnel->out_total_raw+=3+length;
	    
	}
	else tunnel->out_total_raw+=1;
	*/

	if(tunnel->bytes >= tunnel->content_length)
	{
		char c=TUNNEL_DISCONNECT;
		tunnel_write_data(tunnel,&c,sizeof(c));
		tunnel_out_disconnect(tunnel);
		if(tunnel_is_server(tunnel))tunnel_accept(tunnel);
	}
}
/*
函数说明：根据命令对tunnel进行填充或写入；
*/
static int tunnel_write_or_padding(Tunnel *tunnel,char request,void *data,u_short length)
{
	static char padding[65536];
	int n,remaining;
	char *wdata=(char *)data;
	for(remaining=length;remaining>0;remaining-=n,wdata+=n)
	{
		/*
		if(tunnel->bytes+remaining>tunnel->content_length-sizeof_header&&
			tunnel->content_length-tunnel->bytes>sizeof_header)
		n=tunnel->content_length-sizeof_header-tunnel->bytes;
		else if(remaining>tunnel->content_length-sizeof_header)
		n=tunnel->content_length-sizeof_header;
		else n=remaining;
		*/
		n=remaining;
		n=min(n,tunnel->content_length-tunnel->bytes);
		n=min(n,65535);
		n=max(n,0);
		//n= min{隧道的可载长度，需要写的长度，0,65535}
		if(n>65535)n=65535;
		
		if(request==TUNNEL_PADDING)
		{
			if(n+ sizeof_header>remaining)n=remaining-sizeof_header;
			if(tunnel_write_request(tunnel,request,padding,n)==-1)break;
			n+=sizeof_header;
		}
		else 
		{
			if(tunnel_write_request(tunnel,request,wdata,n)==-1)break;

		}
	}
	return length-remaining;
}
/*
函数说明：往tunnel中写入length个字符
*/
int tunnel_write(Tunnel *tunnel,void *data,int length)
{
	int n;
	n=tunnel_write_or_padding(tunnel,TUNNEL_DATA,data,length);
	//tunnel->out_total_data+=length;
	return n;
}
/*
函数说明：往tunnel中填充length bytes填充字符
*/
int tunnel_padding(Tunnel *tunnel,int length)
{
	if(length<sizeof_header+1)
	{
		int i;
		for(i=0;i<length;i++)
			tunnel_write_request(tunnel,TUNNEL_PAD1,NULL,0);
		return length;
	}
	return tunnel_write_or_padding(tunnel,TUNNEL_PADDING,NULL,length);
}

int tunnel_maybe_pad(Tunnel *tunnel,int length)
{
	int padding;
	if(tunnel_is_disconnected(tunnel)||tunnel->bytes%length==0||tunnel->padding_only)
		return 0;
	padding = length - tunnel->bytes % length;
	if ( padding > tunnel->content_length - tunnel->bytes )
		padding=tunnel->content_length - tunnel->bytes;
	return tunnel_padding ( tunnel , padding );
}
static int tunnel_read_request ( Tunnel *tunnel,int *request,
	                              char *buf, int *length)
{
	char tmp[100];
	int req;
	int len;
	int n;
	n = recv (tunnel->in_fd, tmp, sizeof(int), 0);
	if(n==SOCKET_ERROR)return n;
	else if ( n==0 )
	{
		tunnel_in_disconnect ( tunnel );
		if( tunnel_is_client ( tunnel ) && tunnel_in_connect ( tunnel ) == -1)
			return -1;
		return -1;
	}
	
	req = tmp[0];
	(*request ) = req;
	if( req & TUNNEL_SIMPLE )
	{
		*length = 0;
		return -1;
	}
	//先读取命令字；
	n = read_all ( tunnel->in_fd ,&len, sizeof(u_short) );
	if ( n <= 0 )return -1;
	//再读长度值；
	len = ntohs ( len );
	*length = len;
	//tunnel->in_total_raw += n;

	if ( len > 0 )
	{
		n =read_all ( tunnel->in_fd, buf , len );
		if( n <= 0) return -1;
		//tunnel->in_total_raw  += n;
    }
	//再读数据块；
	return 1;
}
/*
函数说明：从指定隧道tunnel中读取一定的数据；
*/
int tunnel_read ( Tunnel * tunnel, void *data, int length )
{
	int req;
	int len;
	int n;
	if(tunnel->buf_len > 0 )
	{
		n = min ( tunnel-> buf_len ,length );
		memcpy (data ,tunnel->buf_ptr , n);
		tunnel-> buf_ptr += n;
		tunnel->buf_len -= n;
		return n;
	}
	if (tunnel->in_fd == INVALID_SOCKET )
	{
		if ( tunnel_is_client ( tunnel ) )
		{
			if ( tunnel_in_connect ( tunnel ) == -1 ) return -1;
		}
		else 
		{
			if ( tunnel_accept ( tunnel  ) == -1 )return -1;
		}
		return -1;
	}
	if ( tunnel->out_fd == INVALID_SOCKET && tunnel_is_server (tunnel ) )
	{
		tunnel_accept ( tunnel );
		return -1;
	}
	if ( tunnel_read_request ( tunnel, &req, tunnel->buf, &len ) <=0 )
		return -1;
	switch ( req )
	{
	case TUNNEL_OPEN:break;
    
	case TUNNEL_DATA:
		tunnel->buf_ptr = tunnel->buf;
		tunnel->buf_len = len;
		//tunnel->in_total_data += len;
		return tunnel_read ( tunnel ,data, length);
	case TUNNEL_PADDING:break;

	case TUNNEL_PAD1:break;

	case TUNNEL_ERROR:
		tunnel->buf[len] = 0;
		return -1;

	case TUNNEL_CLOSE:
		return 0;

	case TUNNEL_DISCONNECT:
		tunnel_in_disconnect( tunnel );

		if ( tunnel_is_client (tunnel )&& tunnel_in_connect (tunnel) == -1)return -1;
	default :
		return -1;
    }
return -1;
}
/*
隧道操作函数
*/
/*
函数说明：隧道服务端接收一个隧道连接：
一个隧道连接包含两个TCP连接in_fd（接收数据）,out_fd（发送数据）;
两个TCP连接如何建立，其流程如此：

在接收TCP连接时，如果还没成功接收到一个TCP连接（in_fd&&out_fd==-1)则测试时间为无穷；
否则只等待10s时间来接收下一个TCP连接；
1、首先接收一个TCP连接：
2、然后解析一个HTTP request，如果method是 -1，转到1
3、如果是post ,put,则将in_fd赋值，转到1；
4、如果是get，则将out_fd赋值，转到1；
5、其他，关闭连接，转到1；
*/
int tunnel_accept(Tunnel *tunnel)
{
	if(tunnel->in_fd!=INVALID_SOCKET&&tunnel->out_fd!=INVALID_SOCKET)
	{
		return 0;
	}
	//假如该服务端正在连接中，不接受；
	/*
	循环说明：
	终止条件：tunnel->in_fd!=INVALID_SOCKET&&tunnel->out_fd!=INVALID_SOCKET
	即已建立起两条TCP连接，一条用于发送、一条用于接收数据！
	步骤：
	1、accept接收一个tcp连接；
	2、从该连接中解析一个http request数据包：http_parse_request;
	3、根据request数据包中method做出不同的处理：switch(method):
	case -1,非法数据包，关闭连接；
	case POST \PUT ：
	*/
	while(tunnel->in_fd==INVALID_SOCKET||tunnel->out_fd==INVALID_SOCKET)
	{
		
		struct sockaddr_in addr;
		Http_request *request;
		//fd_set fs;
		int m;

		//timeval t;
		int len,n;
		SOCKET s;
	/*
		t.tv_sec=2;
		t.tv_usec=0;
		FD_ZERO(&fs);
		FD_SET(tunnel->server_socket,&fs);
		n=select(0,&fs,NULL,NULL,((tunnel->in_fd!=INVALID_SOCKET||tunnel->out_fd!=INVALID_SOCKET)?&t:NULL));
		
		if(n==SOCKET_ERROR)
		{
			return -1;
		}
		else if(n==0)break;
	*/
		/*
		测试服务器套接字是否有connect请求到来；
		如果已经连接上一个TCP连接了，就只等ACCEPT_TIME;
		否则则一直等待下去；
		*/
		len=sizeof(addr);
		s=accept(tunnel->server_socket,(struct sockaddr *)&addr,&len);
		if(s==INVALID_SOCKET)return -1;
		m=http_parse_request(s,&request);
		//从连接套接字s读取http request；
		if(m<=0)return m;
		if(request->method==-1)
		{
			closesocket(s);
		}
		else if(request->method==HTTP_POST||request->method==HTTP_PUT)
		{
			if(tunnel->in_fd==INVALID_SOCKET)
				{
					tunnel->in_fd=s;
                   // tunnel->in_total_raw+=m;
			    }
			else closesocket(s);
		}
		/*
		客户端是准备传数据过来，即Post和Put操作；
		如果服务端输入in_fd已经处于连接状态了，关闭此次TCP连接；
		否则将其赋值给in_fd;
		*/
		else if(request->method==HTTP_GET)
		{
		if(tunnel->out_fd==INVALID_SOCKET)
		{
			char str[1024];
			tunnel->out_fd=s;
			sprintf(str,"HTTP/1.1 200 OK\r\n"
				        "Content-Length: %d\r\n"
                         "Connection: close\r\n"
				          "Pragma: no-cache\r\n"
						  "Cache-Control: no-cache, no-store, must-revalidate\r\n"
						  "Expires: 0\r\n"
						  "Content-Type: text/html\r\n"
						  "\r\n",tunnel->content_length+1);
			if(write_all(tunnel->out_fd,str,strlen(str))<=0)
			{
				closesocket(tunnel->out_fd);
				tunnel->out_fd=INVALID_SOCKET;
			}
			else 
			{
				tunnel->bytes=0;
				tunnel->buf_len=0;
				tunnel->buf_ptr=tunnel->buf;
				//tunnel->out_total_raw+=strlen(str);

			}
		}
		else closesocket(s);
		
		}
		else closesocket(s);
        http_destroy_request(request);
	}
	if(tunnel->in_fd==INVALID_SOCKET||tunnel->out_fd==INVALID_SOCKET)
		//隧道连接未能成功建立只建立了一半，断开隧道连接；
	{
		if(tunnel->in_fd!=INVALID_SOCKET)closesocket(tunnel->in_fd);
		tunnel->in_fd=INVALID_SOCKET;
		tunnel_out_disconnect(tunnel);
		return -1;
	}
	return 0;
}
/*
函数说明：发起隧道连接：
*/
int  tunnel_connect(Tunnel *tunnel)
{
	if(tunnel_is_connected(tunnel))return -1;
	if(tunnel_write_request(tunnel,TUNNEL_OPEN,NULL,0)==-1)return -1;
	if(tunnel_in_connect(tunnel)<=0)return -1;
	return 0;
}
/*
函数说明：摧毁一个隧道实例；
*/
int tunnel_close ( Tunnel *tunnel)
{
	fd_set fs;
	char buf[10240];
	int n;
	timeval t;
	t.tv_sec=1;
	t.tv_usec=0;
	tunnel_write_request(tunnel, TUNNEL_CLOSE,NULL, 0);
	
	tunnel_out_disconnect ( tunnel );
	FD_ZERO(&fs);
	FD_SET(tunnel->in_fd,&fs);
	while( select( 0, &fs,NULL,NULL,&t) >0)
	{
		n = recv( tunnel->in_fd, buf, sizeof (buf ), 0);
	   if ( n <= 0 ) break;
	}
	tunnel_in_disconnect ( tunnel );
	tunnel->buf_len = 0;
	return 0;
}
void tunnel_destory(Tunnel * tunnel )
{
	if ( tunnel_is_connected( tunnel ) || tunnel->in_fd != INVALID_SOCKET )
		tunnel_close( tunnel );
	if ( tunnel->server_socket != INVALID_SOCKET )
		closesocket( tunnel->server_socket );
	free( tunnel );
 }




