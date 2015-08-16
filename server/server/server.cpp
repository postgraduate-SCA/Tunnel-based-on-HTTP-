// server.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
bool WantToUseSock();
#define MAX_LEN 1000
#define MAX_CON 100
DWORD Send_thread(LPVOID lpParameter);
DWORD Recv_thread(LPVOID lpParameter);
//int cnt;//当前活动的连接的个数；
int _tmain(int argc, _TCHAR* argv[])
{
	SOCKET server_sock=INVALID_SOCKET;
	SOCKET tmp_sock=INVALID_SOCKET;
	SOCKADDR_IN skaddr={0};
	char inbuff[MAX_LEN];
	int byte_recv,addr_len;
    printf("~~~~~~~~~~~~~~~~~~the server start up!~~~~~~~~~~~\n");
	if(WantToUseSock())
	{
     printf("WSAStartup failed!\n");     
	 return 0;
	}
	server_sock=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(server_sock==INVALID_SOCKET)
	{
       printf("create server socket failed!\n");
	   WSACleanup();
	   return 0;
	}
	skaddr.sin_family=AF_INET;
	skaddr.sin_port=htons(5000);
	skaddr.sin_addr.s_addr=htonl(ADDR_ANY);
	if(bind(server_sock,(sockaddr *)&skaddr,sizeof (sockaddr))==SOCKET_ERROR)
	{
        printf("bind failed!\n");
		closesocket(server_sock);
		WSACleanup();
	    return 0;
	}
	printf("bind success!\n");
	if(listen(server_sock,SOMAXCONN)==SOCKET_ERROR)
	{
		printf("listen failed!\n");
		closesocket(server_sock);
		WSACleanup();
		return 0;
	}
	printf("listen success on port [5000]!\n");
	//cnt=0;
	/*while(1)
	{
		addr_len=sizeof(sockaddr);
		tmp_sock=accept(server_sock,(sockaddr *)&skaddr,&addr_len);
		if(tmp_sock==INVALID_SOCKET)
		{
        printf("the connection request failed:%d\n",WSAGetLastError());
		}
		else 
		{
           
		}
	}
	*/
	addr_len=sizeof(sockaddr);
	tmp_sock=accept(server_sock,(sockaddr *)&skaddr,&addr_len);
	if(tmp_sock==INVALID_SOCKET)
	{
       printf("accept failed: %d!\n",WSAGetLastError());
	   closesocket(server_sock);
	   WSACleanup();
	   return 0;
    }
	printf("accept connection from [%s]:[%hu]\n",inet_ntoa(skaddr.sin_addr),ntohs(skaddr.sin_port));
	HANDLE thread[2];
	int cnt=0;
	thread[cnt]=CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)Send_thread,&tmp_sock,0,NULL);
	if(thread[cnt]==NULL)
	 {
		 printf("create send_thread  failed:%d!sorry ,you can't send data\n",GetLastError());
	 }
	 else cnt++;
	thread[cnt]=CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)Recv_thread,&tmp_sock,0,NULL);
	if(thread[cnt]==NULL)
	 {
		 printf("create send_thread  failed:%d!sorry ,you can't send data\n",GetLastError());
	 }
	else cnt++;
    if(cnt)WaitForMultipleObjects(cnt,thread,true,INFINITE);
     for(int i=0;i<cnt;i++)CloseHandle(thread[i]);
	/*byte_recv=recv(tmp_sock,inbuff,MAX_LEN,NULL);
	  if(byte_recv==SOCKET_ERROR)
	    {
       printf("read from socket error: %d!\n",WSAGetLastError());
	   closesocket(tmp_sock);
	   closesocket(server_sock);
	   WSACleanup();
	   return 0;
	   }
	   printf("read %d bytes:%s\n",byte_recv,inbuff);
	*/
	closesocket(tmp_sock);
	closesocket(server_sock);
	WSACleanup();
	system("pause");
	return 0;
}
bool WantToUseSock()
{
	WSADATA wsa;
	if(WSAStartup(WINSOCK_VERSION,&wsa ))return true;
	return false;
}
/*
定义三个线程函数：接收连接、发送数据、接收数据线程函数；
*/
/*
DWORD run_thread(LPVOID lpParameter)
{
  SOCKET sockfd=*((SOCKET *)lpParameter);


}
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
		 gets_s(data,MAX_LEN-1);
		 byte_send=strlen(data);
		 data[byte_send]='\r';
		 data[byte_send+1]='\n';
		 data[byte_send+2]=0;
		 //scanf_s("%s",data,MAX_LEN-1);
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
	    // else printf("send %d bytes\n",byte_send);
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
		 printf("client said:%s\n",data);
		 //printf("the client said: %s[%d bytes]\n",data,byte_recv);
	    }
	//Sleep(10000);
	}
}

