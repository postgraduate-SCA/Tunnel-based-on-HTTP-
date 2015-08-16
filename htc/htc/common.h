#ifndef COMMON_H
#define COMMON_H
#include<errno.h>
#include<fcntl.h>
#include<stdio.h>
#include<string.h>
#include<WinSock2.h>

#include "tunnel.h"

#define DEFAULT_HOST_PORT 8888
#define DEFAULT_CONTENT_LENGTH (100*1024)
#define DEFAULT_KEEP_ALIVE 5//second
#define DEFAULT_MAX_CONNECTION_AGE 300//second


extern SOCKET server_socket (int port);
extern int set_address (struct sockaddr_in *address,
			const char *host, int port);
extern int handle_device_input (Tunnel *tunnel, SOCKET fd);
extern int handle_tunnel_input (Tunnel *tunnel, SOCKET fd);
/*
函数说明：从套接字fd处读len个字符到buf;
*/
static int read_all(SOCKET fd,void *buf,int len)
{
	int n,m,r;
    char *rbuf=(char *)buf;
	r=len;

	for(n=0;n<len;n+=m)
     {
	m=recv(fd,rbuf+n,len-n,0);
    if (m == 0)
	{
	  r = 0;
	  break;
	}
      else if (m == SOCKET_ERROR)
	{
		if(WSAGetLastError()==WSAEWOULDBLOCK)m=0;
		else
		{
	        r=-1;
	        break;
		}
	}
    }
	//rbuf[r] = '\0';
	//printf("read_all 函数读取到：%s\n",(char * ) buf );
    return r;
}
/*
函数说明：往套接字fd中写入len的数据；
*/
static  int write_all(SOCKET fd,void *data,int len)
{
	int n,m;
	char *wdata=(char *)data;
	//wdata[len] = '\0';
	for(n=0;n<len;n+=m)
	{
       m=send(fd,wdata+n,len-n,0);
	   if(m==0)return 0;
	   else if(m==SOCKET_ERROR)
	   {
		   if(WSAGetLastError()==WSAEWOULDBLOCK)m=0;
		   else 
		   {
			  return -1;
		   }
	   }
	}
	//printf("write_all 函数发送:%s\n",(char *)data);
	return len;
}
/*
函数说明：往目标地址发起一次连接；
*/
static  SOCKET do_connect(struct sockaddr_in *address)
{
	SOCKET fd;
	fd=socket(AF_INET,SOCK_STREAM,0);
	if(fd==INVALID_SOCKET)return INVALID_SOCKET;
	if(connect(fd,(struct sockaddr *)address,sizeof(sockaddr))==SOCKET_ERROR)
	{
		printf(" 连接到[%s]:[%hu]失败!,错误码：%d\n",inet_ntoa(address->sin_addr),ntohs(address->sin_port),WSAGetLastError());
		closesocket(fd);
		return INVALID_SOCKET;
	}
	return fd;
}
#endif