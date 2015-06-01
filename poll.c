

#define TIMES 50
void error(const char * msg);
int main(){
	int pipefds[2];
	if(-1 == pipe(pipefds)){
		error("failed to pipe");
	}
	
	pid_t pid;
	if((pid = fork()) < 0){
		error("failed to fork");
	}
	if(pid = 0)              // child
	{
		printf("child is running");
		close(pipefds[0]);
		for( int i = 0;i < Times;i++){
			write(pipefds[1],"child is write",strlen("child is write");
			sleep(1);
		}
	}
	else{                    //parent
		printf("parent is running");
		char buf[256];
		close(pipefds[1]);
		struct pollfd pf[2];
		pf[0].fd = 0;
		pf[0].events = POLLIN;
		pf[1].fd = pipefds[0];
		pf[1].events = POLLIN;
		
		for(int i = 0;i < Times;i++){
			poll(pf,2,500);
			printf("Testing ....");
			if(pf[1].revents & POLLIN){
				buf[ read(pipefds[0],buf,256) ] = '\0';
				printf("%s\n",buf);
			}
			if(pf[0].revents & POLLIN){
				buf[ read(pipefds[0],buf,256) ] = '\0';
				printf("%s\n",buf);
			}
		}
		int status;
		wait(&status);
		
	}
}







