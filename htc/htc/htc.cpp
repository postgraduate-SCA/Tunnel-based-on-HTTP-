#include<tchar.h>
#include<stdio.h>
#include<WinSock2.h>
#include<Windows.h>
#include<time.h>

#include"tunnel.h"
#include"http.h"
#include"common.h"


#define DEFAULT_PROXY_PORT 8080
#define DEFAULT_PROXY_BUFFER_TIMEOUT 500 //millisecond;
bool WantToUseSocket()
{
	WSADATA wsa;
	if(WSAStartup(WINSOCK_VERSION,&wsa))return true;
	else return false;
}
static SOCKET wait_for_connection_on_socket( SOCKET s )
{
	sockaddr_in addr;
	int len;
	SOCKET t;

	len = sizeof( addr );
	t = accept(s, (sockaddr *)&addr ,&len );
	
	if( t == INVALID_SOCKET )
		{
			printf ( "wait_for_connection_on_socket 出错!\n");
			return INVALID_SOCKET;
	    }
	printf("成功接收来自地址为{ [%s]:[%hu] }外部程序的连接\n",inet_ntoa(addr.sin_addr),ntohs(addr.sin_port));
	return t;
}
int _tmain(int argc,TCHAR *arg[])
{
	SOCKET s = INVALID_SOCKET;
	SOCKET fd = INVALID_SOCKET;
	Tunnel *tunnel = NULL;
	fd_set fs;
	unsigned short forward_port;
    bool closed ;
	char hostname[255];
	unsigned short port;
	int n;
	timeval t;
	t.tv_sec = 5;
	t.tv_usec = 0;
	SetConsoleTitle(_T("HTTP 加密隧道客户端" ));
	if( WantToUseSocket())
	{
            //printf("WSASocket Start up failed!\n");
			return 0;
	}
	//else printf( "WSASocket Start up successed!\n");
	printf("              HTTP 加密隧道客户端启动 (developed by TangJieHui)\n ");
    printf("请输入隧道服务端地址:\n");
	scanf("%s",hostname);
	printf("请输入隧道服务端端口地址:\n");
	scanf("%hu",&port);
	printf("请输入代理端口地址:\n");
	scanf("%hu",&forward_port);
	//输入块结束：
	/*
	测试开始；
	tunnel = tunnel_new_client(hostname, port);
	if ( tunnel_connect ( tunnel ) == -1 )
			{
				return 0;
		    }
		printf(" HTTP 加密隧道成功建立,下面开始数据的传输!\n");
	测试结束；
	*/
	s=server_socket(forward_port);
	
	if( s == INVALID_SOCKET ) return 0;
	while (1 )
	{
        printf ("\n代理端口建立完毕，正等待外部程序的连接............\n");
		fd = wait_for_connection_on_socket( s );
		if ( fd == INVALID_SOCKET )return 0;
		printf( "\n正在为该外部程序的通信建立 HTTP 加密隧道............\n");
		tunnel = tunnel_new_client(hostname, port);
		if ( tunnel == NULL ) return 0;

		if ( tunnel_connect ( tunnel ) == -1 )
			{

				return 0;
		    }
		printf("\n HTTP 加密隧道成功建立,下面开始数据的传输!\n");
		closed = FALSE;
		while ( !closed )
		{
			FD_ZERO(&fs);
			FD_SET(fd,&fs);
			FD_SET( tunnel->in_fd, &fs );
			n = select ( 0 , &fs, NULL , NULL , &t);
			if ( n == SOCKET_ERROR ) return 0;
			 printf("\n~~~~~~~~~~~~~~~~~~~~~~~~进入handle_device_input函数~~~~~~~~~~~~\n");
			n = handle_device_input ( tunnel, fd );
			if( n <= 0) closed=TRUE;
			printf("~~~~~~~~~~~~~~~~~~~~~~~退出handle_device_input函数~~~~~~~~~~~~~\n");
			printf("\n~~~~~~~~~~~~~~~~~~~~~~~~进入handle_tunnel_input函数~~~~~~~~~~~~\n");
			n = handle_tunnel_input ( tunnel ,fd );
			printf("~~~~~~~~~~~~~~~~~~~~~~~~退出handle_tunnel_input函数~~~~~~~~~~~~\n");
			if( n <= 0) closed=TRUE;
		}
		if( fd != INVALID_SOCKET )closesocket ( fd );
		tunnel_destory( tunnel );
	}
	closesocket (s );
	WSACleanup();
	return 0;
}