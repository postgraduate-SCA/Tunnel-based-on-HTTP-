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
	SetConsoleTitle ( _T ( "HTTP ������������"));
	if ( WantToUseSocket() ) return 0;
	printf ("             HTTP���������������� ��developed by Tangjiehui )\n");
	printf("�������������˶˿ںţ�\n");
	scanf("%hu",&port);
	printf ( "���������������ַ��\n");
	scanf("%s",hostname);
	printf("���������˿ںţ�\n");
	scanf("%hu",&forward_port);

	tunnel = tunnel_new_server ( port );
	if ( tunnel == NULL )
		{
			printf("������������ʧ�ܣ�\n");
			return 0;
	    }
	printf("\n�����������˳ɹ�!\n");
	while (1 )
	{

		if  ( tunnel_accept ( tunnel ) == -1 )
		{
			printf ( " ���δ�ܳɹ�����!\n");
			continue;
		}
		printf ( "\nHTTP ��������ɹ�����,�������ӵ��ⲿ����.....\n");

		if ( set_address (&addr, hostname, forward_port )== -1)
		{
			printf("δʶ��Ĵ���Ŀ������!\n");
			return 0;
		}

		fd = do_connect ( &addr );
		
		if( fd == INVALID_SOCKET )
		{
			printf ( " �޷����ӵ�����Ŀ������!\n");
			return 0;
		}
		printf("\n�ɹ����ӵ��ⲿ����[%s]:[%hu]\n",inet_ntoa(addr.sin_addr),ntohs(addr.sin_port));
		printf("\n���潫��ʼ���ݵĴ���!\n");
		closed =FALSE ;
 
		while( !closed )
		{
			FD_ZERO( &fs);
			FD_SET( fd, &fs );
			FD_SET( tunnel ->in_fd, &fs);

			n= select ( 0 ,&fs ,NULL, NULL, &t);

			if ( n== SOCKET_ERROR ) return 0;
			printf("\n~~~~~~~~~~~~~~~~~~~~~~~~����handle_device_input����~~~~~~~~~~~~\n");
			n = handle_device_input( tunnel, fd );
			if ( n <= 0 ) closed =TRUE;
			printf("~~~~~~~~~~~~~~~~~~~~~~~�˳�handle_device_input����~~~~~~~~~~~~~\n");
			printf("\n~~~~~~~~~~~~~~~~~~~~~~~~����handle_tunnel_input����~~~~~~~~~~~~\n");
			n= handle_tunnel_input ( tunnel, fd );
			printf("~~~~~~~~~~~~~~~~~~~~~~~~�˳�handle_tunnel_input����~~~~~~~~~~~~\n");
			if ( n <= 0 ) closed = TRUE;
		}
		if ( fd != INVALID_SOCKET  )closesocket ( fd );
		tunnel_close ( tunnel );
	}

	tunnel_destory( tunnel );
	return 0;
}