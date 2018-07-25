//header files--c library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <signal.h>

//-------------------------------------------Defines------------------------------------------//
#define FAIL -1
#define SUCCESS 0
#define LINBUF 4096

#ifndef chatserver_c
#define chatserver_c

//-------------------------------------------structs------------------------------------------//
struct slist_node
{
	int data; // Pointer to data of this node
	struct slist_node *next; // Pointer to next node on list
};
typedef struct slist_node slist_node_t;
struct slist
{
	slist_node_t *head; // Pointer to head of list
	slist_node_t *tail; // Pointer to tail of list
	unsigned int size; // The number of elements in the list
};

typedef struct slist slist_t;
//------------------------------Declaration of functions------------------------------------//
int checkInput(int argc,char* argv[]);
void error(char *err,int flg);
int goOverList(fd_set *readfds ,fd_set *writefds,slist_t *list,int maxfd);
void readWrite(fd_set *readfds ,fd_set *writefds,slist_t *list);
int add(slist_t *list,int data);
void slist_init(slist_t *list);
int isEmpty(slist_t *list);
void removeFromList(slist_t *list,int fd);
void slist_destroy(slist_t *list);
void sig_handler(int signo);

//-------------------------------------------Global variables------------------------------------------//

slist_t *signalList;
int sockfd,flag=-1;

//===================================================================MAIN======================================================================================//
int main(int argc,char *argv[])
{
	if(checkInput(argc,argv)==FAIL)
		error("Usage: server <port>",0);

	fd_set readfds,writefds;//represent the bitmap of ready to read or ready to write.
	slist_t *list=(slist_t *)malloc(sizeof(slist_t));//allocate memory to the list of file descriptors
	if(list==NULL){error("malloc",0);}
		
	int maxfd,newSocket;
	struct sockaddr_in serv_addr;
	struct sockaddr_in client_addr;
	socklen_t clilen=sizeof(client_addr);
	
	signalList=list;
	signal(SIGINT,sig_handler);//handle signal 
	signal(SIGPIPE,sig_handler);//handle signal 
	slist_init(list);
	if((sockfd=socket(AF_INET,SOCK_STREAM,0))==FAIL){error("socket",1);}//create new main socket
		
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr =htonl(INADDR_ANY);
	serv_addr.sin_port = htons(atoi(argv[1]));// use the input as port server

	if(bind(sockfd, (struct sockaddr*) &serv_addr,sizeof(serv_addr))<0){error("bind",2);}	
	if(listen(sockfd,5)<0){error("listen",2);}//listen to maximum 5 requests each time.
	
	maxfd=sockfd;//set the maximum to the main sockfd
	while(1)
	{
		if(list->head==NULL)//in case it's the first iteration
		{
			if(add(list,sockfd)==FAIL){error(NULL,-1);}	
		}
		FD_ZERO(&readfds);//set readfds bitMap to zeros
		FD_ZERO(&writefds);//set writefds bitMap to zeros
		maxfd=goOverList(&readfds ,&writefds,list,maxfd);//set the maximum to the maximum sockfd
		if(select(maxfd+1,&readfds ,&writefds,NULL,NULL)<=0){error("select",2);}//use select function	
		else
		{
			if(FD_ISSET(sockfd,&readfds))//in case the main sockfd is readble
			{
				if((newSocket=accept(sockfd,(struct sockaddr*)&client_addr,&clilen))<0){error("accept",2);}//accept client request.
				if(add(list,newSocket)==FAIL){error(NULL,-1);}//add the client sockfd to the list 
				FD_SET(newSocket,&readfds);//set the new client sockfd to 1
				FD_SET(newSocket,&writefds);//set the new client sockfd to 1
				if(maxfd<newSocket)
					maxfd = newSocket;
			}
			else
				readWrite(&readfds ,&writefds,list);//the server read and write the clients masseges.
		}	
	}
return 0;
}


//============================================================================FUNCTIONS================================================================================================//

//----------------------------------------------------sig_handler------------------------------------------------------//
void sig_handler(int signo)//the method handle signals
{
	if(signo==SIGINT)
		error(NULL,-1);
	if(signo==SIGPIPE)
		flag=1;
		
}
//----------------------------------------------------isEmpty---------------------------------------------------------//
int isEmpty(slist_t *list)//the method check if the list head is NULL.
{
	if(list->head==NULL)
		return 0;
return 1;
}
//----------------------------------------------------slist_init------------------------------------------------------//
void slist_init(slist_t *list)//the methos initilaize the list
{	
	if(list==NULL)
		return;
	list->head=NULL;
	list->tail=NULL;
	list->size=0;	
}
//----------------------------------------------------checkInput------------------------------------------------------//
int checkInput(int argc,char* argv[])//this method check the input.
{
	if(argc>2 || argc<2)
		return FAIL;
	int i;
	char *str;
	str=argv[1];
	for(i=0;i<(int)strlen(str);i++)//O(n)
	{
		if(!isdigit(str[i]))
			return FAIL;
	}
return SUCCESS;   
}
//----------------------------------------------------error-----------------------------------------------------------//
void error(char *err,int flg)//taking care of all the errors , free memory and exit.
{
	switch(flg)
	{
		case 0:
			perror(err);
			break;
		case 1:
			perror(err);
			slist_destroy(signalList);
			break;
		case 2:
			perror(err);
			slist_destroy(signalList);
			close(sockfd);
			break;
		default:
			slist_destroy(signalList);
			close(sockfd);
	}
	exit(1);
}
//----------------------------------------------------goOverList------------------------------------------------------//
int goOverList(fd_set *readfds ,fd_set *writefds,slist_t *list,int maxfd)//the method set all clients file descriptors to 1 for read and write 
{
	slist_node_t *tmp=list->head;
	while(tmp!=NULL)
	{
		int data = tmp->data;
		FD_SET(data,writefds);
		FD_SET(data,readfds);
		if(data>maxfd)
			maxfd=data;
		tmp=tmp->next;
	}
return maxfd;
}
//----------------------------------------------------readWrite------------------------------------------------------//
void readWrite(fd_set *readfds ,fd_set *writefds,slist_t *list)//in this methd the server read clients masseges and write the masseges to all clients.
{
	int fdread,fdwrite,rc,size,wt;
	char buf[LINBUF];
	char response[LINBUF];
	slist_node_t *tmp,*tmp1,*p=list->head;
	
	for(tmp=list->head->next;tmp!=NULL;tmp=tmp->next)	
	{
		fdread= tmp->data;
		if(FD_ISSET(fdread,readfds))//in case the client file descriptor is readble.
		{
			printf("server is ready to read from socket %d\n",fdread);	
			if((rc = recv(fdread,buf,LINBUF,MSG_PEEK))<=0)//read massege from client.
			{
				if(rc<0){error("recv",2);}
				close(fdread);
				FD_CLR(fdread,readfds);
				FD_CLR(fdread,writefds);
				removeFromList(list,fdread);
				tmp=p;
			}
			else
			{
				size = strstr(buf,"\r\n")-buf+2;//save the massege size until the end of line.
				if((rc = recv(fdread,buf,size,0))<=0)//read only one line from the client massege. 
					if(rc<0){slist_destroy(list);error("recv",2);}
				buf[rc]='\0';
				sprintf(response,"gust%d:%s",fdread,buf);//create response to all clients.
				slist_node_t *q=list->head;
				for(tmp1=list->head->next;tmp1!=NULL;tmp1=tmp1->next)//go over all clients.	
				{
					fdwrite= tmp1->data;
					if(FD_ISSET(fdwrite,writefds))//check if file descriptor is writeable.
					{
						printf("server is ready to write to socket %d\n",fdwrite);
						if((wt=write(fdwrite,response,(int)strlen(response)))!=(int)strlen(response))//write line to all clients.
						{
							if(wt==-1 && errno==EINTR)
							{
								if(flag!=1){error("write",2);}
								else{flag=-1;}//in case of signal SIGPIPE
							}
							close(fdwrite);
							FD_CLR(fdwrite,readfds);
							FD_CLR(fdwrite,writefds);
							removeFromList(list,fdwrite);
							tmp1=q;
						}
					}
					q=tmp1;
				}
			}
		}
		p=tmp;
	}
}

//----------------------------------------------------add-------------------------------------------------------------//
int add(slist_t *list,int data)//the methos add new node to a linked list.
{
	slist_node_t *new_node=(slist_node_t*)calloc(1,sizeof(slist_node_t));
	new_node->data=data;
	if(new_node==NULL || list==NULL)
		return -1;
	new_node->next=NULL;
	if(isEmpty(list)==0)
		list->head=list->tail=new_node;
	else
	{
		list->tail->next=new_node;
		list->tail=list->tail->next;
	}
	list->size+=1;

return 0;
}
//----------------------------------------------------removeFromList--------------------------------------------------//
void removeFromList(slist_t *list,int fd)//the methos remove node from linked list.
{
	slist_node_t *tmp,*p=NULL;
	for(tmp=list->head;tmp!=NULL;tmp=tmp->next)
	{
		if(list->head->data==fd && tmp==list->head)
		{
			list->head=list->head->next;
			tmp->next=NULL;
			free(tmp);
			return;
		}
		else if(list->tail->data==fd && tmp==list->tail)
		{
			list->tail=p;
			list->tail->next=NULL;
			free(tmp);
			return;
		}
		else if(list->tail->data!=fd && list->head->data!=fd && tmp->data==fd)
		{
			p->next=tmp->next;
			tmp->next=NULL;
			free(tmp);
			return;
		}
		p=tmp;
	}
}
//----------------------------------------------------slist_destroy--------------------------------------------------//
void slist_destroy(slist_t *list)//the method free all list memory.
{
	if(list==NULL)
		return;
	while(list->head!=NULL)
	{
		slist_node_t *temp;
		temp=list->head;
		list->head=list->head->next;
		close(temp->data);
		free(temp);
		temp=NULL;
	}
free(list);
}
#endif

