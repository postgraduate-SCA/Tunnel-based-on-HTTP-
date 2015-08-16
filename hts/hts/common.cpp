#include <WinSock2.h>
#include "common.h"
/*
����˵���� ���׽���fd�����վ�Ȼ����ͨ�����ת����ȥ��
*/
int handle_device_input(Tunnel *tunnel,SOCKET fd)
{
	char buf[10240];
	int n,m;
    n=recv(fd,buf,sizeof(buf),0);
	if(n==0||n==SOCKET_ERROR)return n;
	buf[n] = '\0';
	printf("HTTP �������˴��ⲿ������յ�����Ϊ��%s\n",buf);
	m=tunnel_write(tunnel,buf,n);
	return m;
}
int handle_tunnel_input(Tunnel *tunnel,SOCKET fd)
{
	char buf[10240];
	int n,m;
	n=tunnel_read( tunnel,buf,sizeof(buf));
    if(n<=0)return n;
	buf[n] = '\0';
	printf("HTTP �������˴�������յ�������Ϊ:%s\n",buf);
	m = write_all ( fd, buf , n  );
	return  m;
}
/*
 ����˵��������һ���������׽��֣���ʹ���ڶ˿�port������
*/
SOCKET server_socket ( int port)
{
	struct sockaddr_in address;
	int i;
	SOCKET s;
	s=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(INVALID_SOCKET==s)
	{
	 return INVALID_SOCKET;
	}
	i=1;
	memset(&address,0,sizeof(address));
	address.sin_family=AF_INET;
	address.sin_port=htons((u_short)port);
	address.sin_addr.s_addr=htonl(ADDR_ANY);
	if(bind(s,(struct sockaddr *)&address,sizeof(address))==SOCKET_ERROR)
	{
		closesocket(s);
		return INVALID_SOCKET;
	}
	if(listen(s,SOMAXCONN)==-1)
	{
		closesocket(s);
		return INVALID_SOCKET;
	}
	return s;

}
/*
����˵�������������˿ں�==>sockaddr_in�ṹ�壻
*/
int set_address (struct sockaddr_in *address,
			const char *host, int port)
{
   memset(address,0,sizeof(*address));
   address->sin_family=AF_INET;
   address->sin_port=htons(port);
   address->sin_addr.s_addr=inet_addr(host);
   if(address->sin_addr.s_addr==INADDR_NONE)
   {
	   struct hostent *ent;
       ent=gethostbyname(host);
	   if(ent==NULL)return -1;
	   memcpy(&(address->sin_addr.s_addr),ent->h_addr,(unsigned)ent->h_length);
	}
   return 0;
}