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
			printf ( "wait_for_connection_on_socket ����!\n");
			return INVALID_SOCKET;
	    }
	printf("�ɹ��������Ե�ַΪ{ [%s]:[%hu] }�ⲿ���������\n",inet_ntoa(addr.sin_addr),ntohs(addr.sin_port));
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
	SetConsoleTitle(_T("HTTP ��������ͻ���" ));
	if( WantToUseSocket())
	{
            //printf("WSASocket Start up failed!\n");
			return 0;
	}
	//else printf( "WSASocket Start up successed!\n");
	printf("              HTTP ��������ͻ������� (developed by TangJieHui)\n ");
    printf("�������������˵�ַ:\n");
	scanf("%s",hostname);
	printf("�������������˶˿ڵ�ַ:\n");
	scanf("%hu",&port);
	printf("���������˿ڵ�ַ:\n");
	scanf("%hu",&forward_port);
	//����������
	/*
	���Կ�ʼ��
	tunnel = tunnel_new_client(hostname, port);
	if ( tunnel_connect ( tunnel ) == -1 )
			{
				return 0;
		    }
		printf(" HTTP ��������ɹ�����,���濪ʼ���ݵĴ���!\n");
	���Խ�����
	*/
	s=server_socket(forward_port);
	
	if( s == INVALID_SOCKET ) return 0;
	while (1 )
	{
        printf ("\n����˿ڽ�����ϣ����ȴ��ⲿ���������............\n");
		fd = wait_for_connection_on_socket( s );
		if ( fd == INVALID_SOCKET )return 0;
		printf( "\n����Ϊ���ⲿ�����ͨ�Ž��� HTTP �������............\n");
		tunnel = tunnel_new_client(hostname, port);
		if ( tunnel == NULL ) return 0;

		if ( tunnel_connect ( tunnel ) == -1 )
			{

				return 0;
		    }
		printf("\n HTTP ��������ɹ�����,���濪ʼ���ݵĴ���!\n");
		closed = FALSE;
		while ( !closed )
		{
			FD_ZERO(&fs);
			FD_SET(fd,&fs);
			FD_SET( tunnel->in_fd, &fs );
			n = select ( 0 , &fs, NULL , NULL , &t);
			if ( n == SOCKET_ERROR ) return 0;
			 printf("\n~~~~~~~~~~~~~~~~~~~~~~~~����handle_device_input����~~~~~~~~~~~~\n");
			n = handle_device_input ( tunnel, fd );
			if( n <= 0) closed=TRUE;
			printf("~~~~~~~~~~~~~~~~~~~~~~~�˳�handle_device_input����~~~~~~~~~~~~~\n");
			printf("\n~~~~~~~~~~~~~~~~~~~~~~~~����handle_tunnel_input����~~~~~~~~~~~~\n");
			n = handle_tunnel_input ( tunnel ,fd );
			printf("~~~~~~~~~~~~~~~~~~~~~~~~�˳�handle_tunnel_input����~~~~~~~~~~~~\n");
			if( n <= 0) closed=TRUE;
		}
		if( fd != INVALID_SOCKET )closesocket ( fd );
		tunnel_destory( tunnel );
	}
	closesocket (s );
	WSACleanup();
	return 0;
}