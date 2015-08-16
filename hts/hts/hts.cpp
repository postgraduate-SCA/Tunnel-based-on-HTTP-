#include<tchar.h>
#include<stdio.h>
#include<WinSock2.h>
#include<Windows.h>
#include<time.h>

#include"tunnel.h"
#include"http.h"
#include"common.h"

bool WantToUseSocket()
{
	WSADATA wsa;
	if(WSAStartup(WINSOCK_VERSION,&wsa))return true;
	else return false;
}

int _tmain(int argc,TCHAR *arg[])
{
	sockaddr_in addr;
	SOCKET fd = INVALID_SOCKET;
	Tunnel *tunnel = NULL;
	fd_set fs;
	unsigned short forward_port;
    bool closed ;
	char hostname[255];
	unsigned short port;
	int n;
	timeval t;
	t.tv_sec=5;
	t.tv_usec= 0;
	SetConsoleTitle ( _T ( "HTTP 加密隧道服务端"));
	if ( WantToUseSocket() ) return 0;
	printf ("             HTTP加密隧道服务端启动 （developed by Tangjiehui )\n");
	printf("请输入隧道服务端端口号：\n");
	scanf("%hu",&port);
	printf ( "请输入代理主机地址：\n");
	scanf("%s",hostname);
	printf("请输入代理端口号：\n");
	scanf("%hu",&forward_port);

	tunnel = tunnel_new_server ( port );
	if ( tunnel == NULL )
		{
			printf("建立隧道服务端失败！\n");
			return 0;
	    }
	printf("\n建立隧道服务端成功!\n");
	while (1 )
	{

		if  ( tunnel_accept ( tunnel ) == -1 )
		{
			printf ( " 隧道未能成功建立!\n");
			continue;
		}
		printf ( "\nHTTP 加密隧道成功建立,正在连接到外部程序.....\n");

		if ( set_address (&addr, hostname, forward_port )== -1)
		{
			printf("未识别的代理目标主机!\n");
			return 0;
		}

		fd = do_connect ( &addr );
		
		if( fd == INVALID_SOCKET )
		{
			printf ( " 无法连接到代理目标主机!\n");
			return 0;
		}
		printf("\n成功连接到外部程序：[%s]:[%hu]\n",inet_ntoa(addr.sin_addr),ntohs(addr.sin_port));
		printf("\n下面将开始数据的传输!\n");
		closed =FALSE ;
 
		while( !closed )
		{
			FD_ZERO( &fs);
			FD_SET( fd, &fs );
			FD_SET( tunnel ->in_fd, &fs);

			n= select ( 0 ,&fs ,NULL, NULL, &t);

			if ( n== SOCKET_ERROR ) return 0;
			printf("\n~~~~~~~~~~~~~~~~~~~~~~~~进入handle_device_input函数~~~~~~~~~~~~\n");
			n = handle_device_input( tunnel, fd );
			if ( n <= 0 ) closed =TRUE;
			printf("~~~~~~~~~~~~~~~~~~~~~~~退出handle_device_input函数~~~~~~~~~~~~~\n");
			printf("\n~~~~~~~~~~~~~~~~~~~~~~~~进入handle_tunnel_input函数~~~~~~~~~~~~\n");
			n= handle_tunnel_input ( tunnel, fd );
			printf("~~~~~~~~~~~~~~~~~~~~~~~~退出handle_tunnel_input函数~~~~~~~~~~~~\n");
			if ( n <= 0 ) closed = TRUE;
		}
		if ( fd != INVALID_SOCKET  )closesocket ( fd );
		tunnel_close ( tunnel );
	}

	tunnel_destory( tunnel );
	return 0;
}