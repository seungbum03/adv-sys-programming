#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <termios.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <errno.h>

#define IP "127.0.0.1"
#define PORT 3000
#define MAX_CLIENT 1024
#define MAX_DATA 1024

#define EPOLL_SIZE 50
#define BUFFER_SIZE 1024
#define USER_NAME_SIZE 20
#define DEFAULT_ALLOCATED_FD_AMOUNT 6

char user_name[USER_NAME_SIZE]="DEFAULT";

void resetTermios(void);
void clear_buffer(void);

int launch_client(char* num_client);
int launch_server(void);
void* epoll_thread_main(void *arg);
void* send_msg_thread_main(void *arg);
void* send_read_msg(void *arg);
void* write_read_msg(void *arg);
void error_handling(char *msg);
void setnonblockingmode(int fd);

//구조체 선언
typedef struct epoll_arg_t{
    int fd;
    struct epoll_event *ep_evts;
    struct epoll_event evt;
    int manager_fd;
}epoll_arg_t;

int main(int argc, char *argv[])
{
    int ret = -1;
    struct timeval before, after;
    int duration;

    if ((argc != 2) && (argc != 3)) {
usage:  fprintf(stderr, "usage: %s s|c user_num(01~20)\n", argv[0]);
        goto leave;
    }

    if ((strlen(argv[1]) != 1))
        goto usage;
    setbuf(stdout,NULL);

    gettimeofday(&before,NULL);

    switch (argv[1][0]) {
        case 's': // Launch Server
            ret = launch_server();
            break;
        case 'c':
            ret = launch_client(argv[2]);
            break;
        default:
            goto usage;
    }

    gettimeofday(&after,NULL);

    duration = (after.tv_sec - before.tv_sec) * 1000000 + (after.tv_usec - before.tv_usec);
    printf("Processing time = %d.%06d sec\n", duration / 1000000, duration % 1000000);

leave:
    return ret;
}
void clear_buffer(void)
{
    while(getchar()!='\n');
}

int launch_client(char* num_client)
{
    int serverSock;
    struct sockaddr_in server_address;
    pthread_t send_thread, write_thread;
    void* thread_return_value;
    char startbuf[2];

    // user_name 초기화
    sprintf(user_name, "%s", num_client);
    serverSock=socket(PF_INET, SOCK_STREAM, 0);

    // 주소 초기화
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family=AF_INET;
    server_address.sin_addr.s_addr=inet_addr("127.0.0.1");
    server_address.sin_port=htons(PORT);

    // connect 요청
    if(connect(serverSock, (struct sockaddr*)&server_address, sizeof(server_address))==-1)
        error_handling("connect error");

    //서버에서 $가 올때까지 대기
    while(1){
        read(serverSock, startbuf, 2);
        if(!strcmp(startbuf, "$")||!strcmp(startbuf, "$\n")){
            break;
        }
    }

    // 쓰레드 생성
    pthread_create(&send_thread, NULL, send_read_msg, (void*)&serverSock);
    pthread_create(&write_thread, NULL, write_read_msg, (void*)&serverSock);

    // 쓰레드 종료대기
    pthread_join(send_thread, &thread_return_value);
    pthread_join(write_thread, &thread_return_value);

    // 소켓 닫기
    close(serverSock);  
    return 0;
}


int launch_server(void)
{
    int serverSock, acceptedSock;
    struct sockaddr_in Addr_server, Addr_server_as_client;
    int ret = 0, i = 1;
    int epfd;
    struct epoll_event *ep_events;
    struct epoll_event event;
    epoll_arg_t epoll_arg;
    pthread_t send_thread, epoll_thread;
    void* thread_return;

    // epoll 인스턴스 생성
    epfd = epoll_create(EPOLL_SIZE);
    ep_events = malloc(sizeof(struct epoll_event)*EPOLL_SIZE);

    // 소켓생성
    if((serverSock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        goto leave;
    }
    //소켓옵션 지정
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, (void *)&i, sizeof(i));


    //주소정보 초기화
    memset(&Addr_server, 0, sizeof(Addr_server));
    Addr_server.sin_family = AF_INET;
    Addr_server.sin_addr.s_addr = htonl(INADDR_ANY);
    Addr_server.sin_port = htons(PORT);

    //소켓에 IP주소와 포트번호를 연결   
    if( bind( serverSock, (struct sockaddr *)&Addr_server, sizeof(Addr_server) ) == -1) {
        perror("bind");
        goto error;
    }

    //클라이언트 접속 요청대기:연결요청 대기 큐의 크기 = 10
    if(listen(serverSock, 10)==-1) {
        perror("listen");
        goto error;
    }

    //nonblocking mode 설정
    setnonblockingmode(serverSock);

    //serverSock에서 수신할 데이터가 존재할때 이벤트를 확인
    event.events = EPOLLIN;
    event.data.fd = serverSock;

    //epoll 파일디스크립터에 서버fd와 위에 설정한 정보 등록
    epoll_ctl(epfd, EPOLL_CTL_ADD, serverSock, &event);

    //구조체 초기화
    epoll_arg.fd = epfd;
    epoll_arg.ep_evts = ep_events;
    epoll_arg.evt = event;
    epoll_arg.manager_fd = serverSock;

    //epoll 스레드 생성
    pthread_create(&epoll_thread, NULL, epoll_thread_main, (void*)&epoll_arg);

    //클라이언트들에게 로그를 뿌려주기 위한 server_as_client
    acceptedSock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&Addr_server_as_client, 0, sizeof(Addr_server_as_client));
    Addr_server_as_client.sin_family = AF_INET;
    Addr_server_as_client.sin_addr.s_addr = htonl(INADDR_ANY);
    Addr_server_as_client.sin_port = htons(PORT);
    if(connect(acceptedSock, (struct sockaddr*)&Addr_server_as_client, sizeof(Addr_server_as_client))==-1)
        error_handling("acceptedSock error");

    // 쓰레드 생성
    pthread_create(&send_thread, NULL, send_msg_thread_main, (void*)&acceptedSock);

    //쓰레드 종료 대기
    pthread_join(epoll_thread, &thread_return);
    pthread_join(send_thread, &thread_return);

    free(ep_events);
    close(acceptedSock);
error:
    close(serverSock);
leave:
    return ret;
}

// Non Block mode 설정
void setnonblockingmode(int fd)
{
    int flag=fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flag|O_NONBLOCK);
}

//메시지 발송 처리
void* send_msg_thread_main(void* arg)
{
    int socket=*((int*)arg);
    char messageToSend[BUFFER_SIZE];
    char messageWithName[BUFFER_SIZE];
    int cnt=0;
    fflush(stdout);

    while(1) 
    {
        fgets(messageToSend, BUFFER_SIZE, stdin);
        if(!strcmp(messageToSend,"@")||!strcmp(messageToSend,"@\n")) 
        {
            // 각클라이언트에게 메시지 마지막에 @를 수신
            cnt++;
            //20개 이상이면 %를 송신
            if(cnt>=20){
                sprintf(messageWithName, "q\n");
                write(socket, messageWithName, strlen(messageWithName));
            }
        } else{
            sprintf(messageWithName,"%s", messageToSend);
            write(socket, messageWithName, strlen(messageWithName));
            fflush(stdout);
        }
    }
    
    return NULL;
}


// 메시지 발송 처리
void* send_read_msg(void* arg)
{
    int socket=*((int*)arg);
    FILE *file;
    
    char filename[20];

    char messageToSend[BUFFER_SIZE];
    char messageWithName[USER_NAME_SIZE + BUFFER_SIZE];

    sprintf(filename, "/tmp/file_00%s", user_name);


    if((file = fopen(filename, "rt")) == NULL){
        perror("fopen error");
        exit(1);
    }

    //file에서 읽어서 보냄. 파일에서 읽을게 없으면 @전송
    while(fgets(messageToSend, BUFFER_SIZE, file)!=NULL){
        if(strlen(messageToSend) > 1){
            sprintf(messageWithName, "%s", messageToSend);
            write(socket, messageWithName, strlen(messageWithName));
        }else{
            sprintf(messageWithName, "@\n");
            write(socket, messageWithName, strlen(messageWithName));
            fclose(file);
            close(socket);
            return NULL;
        }
    }

    fclose(file);
    close(socket);
    return NULL;
}

// 메시지 수신 처리
void* write_read_msg(void* arg)
{
    int socket=*((int*)arg);
    char received_message[BUFFER_SIZE];
    int received_message_length;
    FILE *fout;
    char foutname[20];

    sprintf(foutname, "/tmp/fout_00%s", user_name);

    if((fout = fopen(foutname, "wr")) == NULL){
        perror("fopen error");
        exit(1);
    }
    
    while(1)
    {
        //q가 아니면 스트림에서 읽은걸 파일에 씀 
        received_message_length=read(socket, received_message, BUFFER_SIZE);
        if(!strcmp(received_message, "q")||!strcmp(received_message,"q\n")){
            fclose(fout);
            close(socket);
            fflush(stdout);
            return NULL;
        }
        if(received_message_length < 0){
            fflush(stdout);
            break;
        }
        received_message[received_message_length]=0;
        fputs(received_message, fout);
    }
    fclose(fout);
    close(socket);
    fflush(stdout);
    return NULL;
}

// 에러 발생시 출력구
void error_handling(char *msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}

void* epoll_thread_main(void *arg)
{
    int socket_count = 0;
    int server_socket, client_socket;
    int received_msg_len, i, j;

    char buffer[BUFFER_SIZE];
    char startbuf[2]="$";
    struct sockaddr_in client_address;
    socklen_t address_size;

    struct epoll_event *ep_events;
    struct epoll_event event;
    int epfd, event_cnt;

    int flag = 0;

    // epoll 구조체 초기화
    epoll_arg_t epoll_arg = *((epoll_arg_t*)arg);
    server_socket = epoll_arg.manager_fd;
    ep_events = epoll_arg.ep_evts;
    event = epoll_arg.evt;
    epfd = epoll_arg.fd;

    // 서버 실행
    while(1)
    {
        // epoll 이벤트 대기 
        event_cnt=epoll_wait(epfd, ep_events, EPOLL_SIZE, -1);
        if(event_cnt==-1)
        {
            puts("epoll_wait() error");
            break;

        }

        // epoll에서 return 된 이벤트 갯수만큼 반복
        for(i=0; i<event_cnt; i++)
        {
            int sock = ep_events[i].data.fd;

            //처음 접속자수가 20명이 됐을 때 client들에게 $전송
            if(flag == 0 && socket_count == 21)
            {
                for (j=DEFAULT_ALLOCATED_FD_AMOUNT; j<socket_count+DEFAULT_ALLOCATED_FD_AMOUNT; j++) {
                    write(j, startbuf, 2);
                }
                flag=1;
            }

            // 이벤트를 받은 소켓이 server_socket(연결요청) 이면
            // server_socket은 listen함수에 의해 등록된 file descriptor 임
            if(sock==server_socket)
            {
                address_size=sizeof(client_address);
                // 연결
                client_socket=accept(sock, (struct sockaddr*)&client_address, &address_size);
                if(client_socket < 0)
                    break;


                // 새로 연결된 client_socket을 EPOLLIN 상황에서 이벤트가 발생하도록 등록
                setnonblockingmode(client_socket);
                event.events=EPOLLIN|EPOLLET;
                event.data.fd=client_socket;
                epoll_ctl(epfd, EPOLL_CTL_ADD, client_socket, &event);

         
                socket_count++;
            }

            // 이벤트가 연결요청이 아니고 client가 모두 접속했을 때
            else if(flag)
            {
                while(1)
                {
                    client_socket = ep_events[i].data.fd;
                    memset(buffer, 0, BUFFER_SIZE);

                    received_msg_len=read(client_socket, buffer, BUFFER_SIZE);
                    // 종료 요청이 들어오면
                    if(received_msg_len==0)
                    {
                        epoll_ctl(epfd, EPOLL_CTL_DEL, client_socket, NULL);
                        close(client_socket);
                        printf("closed client: %d \n", client_socket);
                        socket_count--;
                        if(socket_count==1){
                            close(sock);
                            return NULL;
                        }

                    }
                    //read error발생시 nonblockingmode이므로 바로 리턴하므로 read error대응 
                    if(received_msg_len<0)
                    {
                        if(errno==EAGAIN || errno==EWOULDBLOCK){
                            break;
                        }
                    }
                    // error없을 시 메시지 전송
                    else
                    {   
                        for (j=DEFAULT_ALLOCATED_FD_AMOUNT; j<socket_count+DEFAULT_ALLOCATED_FD_AMOUNT; j++) {
                                 write(j, buffer, BUFFER_SIZE);
                        }               
                    }
                }
            }
        }
    }
    return NULL;
}
