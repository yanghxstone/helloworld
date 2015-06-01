#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<sys/socket.h>
#include<netdb.h>
#include<fcntl.h>
#include<sys/epoll.h>
#include<string.h>

#defineMAXEVENTS 64

//����:
//����:�����Ͱ�һ��TCP socket
//����:�˿�
//����ֵ:������socket
static int
create_and_bind(char*port)
{
	struct addrinfo hints;
	struct addrinfo*result,*rp;
	int s, sfd;
	
	memset(&hints,0,sizeof(struct addrinfo));
	hints.ai_family=AF_UNSPEC;  /* Return IPv4 and IPv6 choices */
	hints.ai_socktype=SOCK_STREAM;/* We want a TCP socket */
	hints.ai_flags=AI_PASSIVE;  /* All interfaces */
	
	s= getaddrinfo(NULL, port,&hints,&result);
	if(s!=0)
	{
		fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(s));
		return-1;
	}
	
	for(rp= result; rp!= NULL; rp= rp->ai_next)
	{
		sfd= socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(sfd==-1)
			continue;
		
		s= bind(sfd, rp->ai_addr, rp->ai_addrlen);
		if(s==0)
		{
			/* We managed to bind successfully! */
			break;
		}
		
		close(sfd);
	}
	
	if(rp== NULL)
	{
		fprintf(stderr,"Could not bind\n");
		return-1;
	}
	
	freeaddrinfo(result);
	
	return sfd;
}


//����
//����:����socketΪ��������
static int
make_socket_non_blocking(int sfd)
{
	int flags, s;
	
	//�õ��ļ�״̬��־
	flags= fcntl(sfd, F_GETFL,0);
	if(flags==-1)
	{
		perror("fcntl");
		return-1;
	}
	
	//�����ļ�״̬��־
	flags|= O_NONBLOCK;
	s= fcntl(sfd, F_SETFL, flags);
	if(s==-1)
	{
		perror("fcntl");
		return-1;
	}
	
	return0;
}

//�˿��ɲ���argv[1]ָ��
int
main (int argc,char*argv[])
{
	int sfd, s;
	int efd;
	struct epoll_eventevent;
	struct epoll_event*events;
	
	if(argc!=2)
	{
		fprintf(stderr,"Usage: %s [port]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	sfd= create_and_bind(argv[1]);
	if(sfd==-1)
		abort();
	
	s= make_socket_non_blocking(sfd);
	if(s==-1)
		abort();
	
	s= listen(sfd, SOMAXCONN);
	if(s==-1)
	{
		perror("listen");
		abort();
	}
	
	//���˲���size��������,�˺�����epoll_create��ȫ��ͬ
	efd= epoll_create1(0);
	if(efd==-1)
	{
		perror("epoll_create");
		abort();
	}
	
	event.data.fd= sfd;
	event.events= EPOLLIN | EPOLLET;//����,��Ե������ʽ
	s= epoll_ctl(efd, EPOLL_CTL_ADD, sfd,&event);
	if(s==-1)
	{
		perror("epoll_ctl");
		abort();
	}
	
	/* Buffer where events are returned */
	events= calloc(MAXEVENTS,sizeofevent);
	
	/* The event loop */
	while(1)
	{
		int n, i;
		
		n= epoll_wait(efd, events,MAXEVENTS,-1);
		for(i=0; i<n; i++)
		{
			if((events[i].events& EPOLLERR)||
				(events[i].events& EPOLLHUP)||
				(!(events[i].events& EPOLLIN)))
			{
			/* An error has occured on this fd, or the socketis not
				readyfor reading (why were we notified then?) */
				fprintf(stderr,"epoll error\n");
				close(events[i].data.fd);
				continue;
			}
			
			elseif(sfd== events[i].data.fd)
			{
			/* We have a notification on the listening socket,which
				meansone or more incoming connections. */
				while(1)
				{
					struct sockaddr in_addr;
					socklen_t in_len;
					int infd;
					char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
					
					in_len=sizeofin_addr;
					infd= accept(sfd,&in_addr,&in_len);
					if(infd==-1)
					{
						if((errno== EAGAIN)||
							(errno== EWOULDBLOCK))
						{
						/* We have processed all incoming
							connections.*/
							break;
						}
						else
						{
							perror("accept");
							break;
						}
					}
					
					//����ַת��Ϊ���������߷�����
					s= getnameinfo(&in_addr, in_len,
						hbuf,sizeofhbuf,
						sbuf,sizeofsbuf,
						NI_NUMERICHOST| NI_NUMERICSERV);//flag����:������������
					//������ַ�ͷ����ַ
					
					if(s==0)
					{
						printf("Accepted connection on descriptor %d "
							"(host=%s,port=%s)\n", infd, hbuf,sbuf);
					}
					
					/* Make the incoming socket non-blocking and addit to the
					listof fds to monitor. */
					s= make_socket_non_blocking(infd);
					if(s==-1)
						abort();
					
					event.data.fd=infd;
					event.events= EPOLLIN | EPOLLET;
					s= epoll_ctl(efd, EPOLL_CTL_ADD,infd,&event);
					if(s==-1)
					{
						perror("epoll_ctl");
						abort();
					}
				}
				continue;
			}
			else
			{
			/* We have data on the fd waiting to be read. Readand
			displayit. We must read whatever data is available
			completely,as we are running in edge-triggered mode
			andwon't get a notification again for the same
				data.*/
				intdone=0;
				
				while(1)
				{
					ssize_t count;
					char buf[512];
					
					count= read(events[i].data.fd, buf,sizeof(buf));
					if(count==-1)
					{
					/* If errno == EAGAIN, that means we have read all
						data.So go back to the main loop. */
						if(errno!= EAGAIN)
						{
							perror("read");
							done=1;
						}
						break;
					}
					elseif(count==0)
					{
					/* End of file. The remote has closed the
						connection.*/
						done=1;
						break;
					}
					
					/* Write the buffer to standard output */
					s= write(1, buf, count);
					if(s==-1)
					{
						perror("write");
						abort();
					}
				}
				
				if(done)
				{
					printf("Closed connection on descriptor %d\n",
						events[i].data.fd);
					
						/* Closing the descriptor will make epoll removeit
					fromthe set of descriptors which are monitored. */
					close(events[i].data.fd);
				}
			}
    }
  }
  
  free(events);
  
  close(sfd);
  
  return EXIT_SUCCESS;
}