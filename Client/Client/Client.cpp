// Client.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
/*
1、请求winsock服务；
2、建立一个socket，根据输入，连入到服务器；
3、向服务器发送数据；
4、over；
*/
#define MAX_LEN 10000
#define MAX_THREAD 100
bool GetHostIp(char domain_name[],in_addr &hostip);
bool  WantToUseSocket();
bool ConnectToHost(unsigned short port_num,in_addr ,SOCKET &sockfd);
DWORD Send_thread(LPVOID lpParameter);
DWORD Recv_thread(LPVOID lpParameter);
int _tmain(int argc, _TCHAR* argv[])
{
	SOCKET sockfd=INVALID_SOCKET;
	char server_name[256];
    in_addr server_address;
	unsigned short server_port;
	HANDLE thread[MAX_THREAD];
	int cnt=0;
	if(WantToUseSocket())
		{
			printf("WSASocket Start up failed!\n");
			return 0;
	    }
     printf("~~~~~~~~~~~~~~~~~~~~~~~~~~the client start!~~~~~~~~~~~~~~~~~~~~~~\n");
	 sockfd=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	 if(INVALID_SOCKET==sockfd)
	 {
		 printf("Create client socket error!\n");
		 if(INVALID_SOCKET!=sockfd)closesocket(sockfd);
		 WSACleanup();
		 return 0;
	 }
	 printf("please input the server name:\n");
	 scanf_s("%s",server_name,sizeof(server_name));
	 getchar();
	 //puts(server_name);
	 printf("please input the server port_number:\n");
	 scanf_s("%hu",&server_port,2);
	 getchar();
	 //printf("server name: %s  server port:%d\n",server_name,server_port);
	 if(GetHostIp(server_name,server_address))
	 {
       printf("bad server_name!\n");
	   if(INVALID_SOCKET!=sockfd)closesocket(sockfd);
	   WSACleanup();
	   return 0;
	 }
	 if(ConnectToHost(htons(server_port),server_address,sockfd))
	 {
		 printf("Can't connect to the server!\n");
		 if(INVALID_SOCKET!=sockfd)closesocket(sockfd);
		 WSACleanup();
		 return 0;
	 }
	 printf("Connect to [%s]:[%d] successs\n",server_name,server_port);
	 thread[cnt]=CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)Send_thread,&sockfd,0,NULL);
	 if(thread[cnt]==NULL)
	 {
		 printf("create send_thread  failed:%d!sorry ,you can't send data\n",GetLastError());
	 }
	 else cnt++;
	 thread[cnt]=CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)Recv_thread,&sockfd,0,NULL);
	 if(thread[cnt]==NULL)
		printf("create revc_thread failed:%d!sorry ,you cann't receive data\n",GetLastError());
	 else cnt++;
	 if(cnt)WaitForMultipleObjects(cnt,thread,true,INFINITE);
	 for(int i=0;i<cnt;i++)CloseHandle(thread[i]);
	 closesocket(sockfd);
	 WSACleanup();
	 system ("pause");
	 return 0;
}
bool WantToUseSocket()
{
	WSADATA wsa;
	if(WSAStartup(WINSOCK_VERSION,&wsa))return true;
	else return false;
}
bool GetHostIp(char domain_name[],in_addr &hostip)
{
	hostip.s_addr=inet_addr(domain_name);
	if(hostip.s_addr==INADDR_NONE)
	{
		hostent *phtent=NULL;
		phtent=gethostbyname(domain_name);
		if(phtent==NULL||phtent->h_addrtype!=AF_INET)return true;
		hostip=*((in_addr *)(phtent->h_addr));
		return false;
     }
	return false;
}
bool ConnectToHost(unsigned short port_num,in_addr server_address,SOCKET &sockfd)
{
	sockaddr_in sa={0};
	sa.sin_family=AF_INET;
	sa.sin_addr=server_address;
	sa.sin_port=port_num;
	return connect(sockfd,(sockaddr *)&sa,sizeof(sockaddr));
}
/*
定义两个线程函数：发送数据和接收数据线程函数；
*/
DWORD Send_thread(LPVOID lpParameter)
{
	char data[MAX_LEN];
	SOCKET sockfd=*((SOCKET *)lpParameter);
	int byte_send,error_code;
	printf("Send_thread start!\n");
	while(1)
	{  
		 //printf("please input what you want to send:\n");
		 //scanf_s("%s",data,MAX_LEN-1);
		 gets_s(data,MAX_LEN-1);
		// getchar(); 
		 byte_send=send(sockfd,data,strlen(data)+1,NULL);
		 if(strcmp(data,"OK")==0||strcmp(data,"ok")==0||strcmp(data,"Ok")==0||strcmp(data,"oK")==0)
			return 0;
	     if(SOCKET_ERROR==byte_send)
	     {
		 error_code=WSAGetLastError();
		 printf("send failed:%d!\n",error_code);
	     if(error_code==WSAEHOSTDOWN||error_code==WSAECONNABORTED)return 0;
		 }
	    //else printf("send %d bytes\n",byte_send);
	}
}
DWORD Recv_thread(LPVOID lpParameter)
{
	char data[MAX_LEN];
	int byte_recv,error_code;
	SOCKET sockfd=*((SOCKET *)lpParameter);
	printf("Recv_thread start !\n");
	while(1)
	{
	byte_recv=recv(sockfd,data,MAX_LEN-1,NULL);
	data[byte_recv]=0;
	if(byte_recv==SOCKET_ERROR)
	{
		error_code=WSAGetLastError();
        printf("read from socket error: %d!\n",error_code);
		if(error_code==WSAEHOSTDOWN||error_code==WSAECONNABORTED)return 0;
	}
	else 
		{
		 if(byte_recv==0||strcmp(data,"OK")==0||strcmp(data,"ok")==0||strcmp(data,"Ok")==0||strcmp(data,"oK")==0)return 0;
		 printf("the server said: %s\n",data,byte_recv);
	    }
    }
}