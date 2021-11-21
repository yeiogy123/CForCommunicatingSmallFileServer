#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#define SERVER 0
#define CLIENT 1
char web_page[8192]="HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n"
                    "<!DOCTYPE html>\r\n"
                    "<img src=\"spongebob.jpeg\" alt=\"Revealing\" title=\"Demo picture\" width=\"200px\">\r\n"
                    "<form method=\"post\" enctype=\"multipart/form-data\">\r\n"
                    "<p><input type=\"file\" name=\"upload\"></p>\r\n"
                    "<p><input type=\"submit\" value=\"submit\"></p>\r\n"
                    "</center>\r\n"
                    "</body></html>\r\n";
void info(int fdsock, int identifier){
    socklen_t address_len;
    struct sockaddr_in address;
    if(identifier == 1)
        printf("\n\tServer listen address:");
    else if (identifier == 0)
        printf("Connected server address:");
    /**
     * use the socket_address_in struct to get the name
     * with forcing changing into socket address struct
     */
    getsockname(fdsock, (struct sockaddr*)&address, &address_len);
    /*
     * inet_ntoa()->所指的网络二进制的数字转换成网络地址, 然后将指向此网络地址字符串的指针返回.
     * ntoh()->本函式將一個16位數由網路位元組順序轉換為主機位元組順序
     */
    printf("%s:%d\n",inet_ntoa(address.sin_addr), ntohs(address.sin_port));
}
void signalchild_handler(){
    pid_t pid;
    while((pid = waitpid(-1, NULL, WNOHANG))>0);
}
void process(int client_socket){
    char buf[65536], filename[1024];
    char *ptr, *qtr;
    int img_fd, len, count=0;
    FILE *fptr;
    memset(buf, 0, sizeof(buf));
    read(client_socket, buf, sizeof(buf));
    if(strncmp(buf, "GET /spongebob.jpeg", 18) ==0){
        img_fd  = open("spongebob.jpeg", O_RDONLY);
        sendfile(client_socket, img_fd, NULL, 5000000);
        close(img_fd);
        printf("GET / of buf is [%s]\n",buf);
    }else if(strncmp(buf, "POST /", 6)==0) {
        ptr = strstr(buf, "filename");
        if (ptr != NULL) {
            len = strlen("filename");
            ptr += len + 2;
            while (*ptr != '\"') {
                filename[count++] = *ptr;
                ptr++;
            }
            ptr++;
            filename[count] = '\0';
            while (!(*(ptr - 4) == '\r' && *(ptr - 3) == '\n' && *(ptr - 2) == '\r' && *(ptr - 1) == '\n')) ptr++;
            printf("receive filename:%s\n", filename);
            fptr = fopen(filename, "w");
            if (fptr==NULL) {
                printf("error:file");
                exit(1);
            }
            qtr = buf + strlen(buf) - 3;
            while (*qtr != '\r') --qtr;
            --qtr;
            while (ptr != qtr) {
                fputc(*ptr, fptr);
                ptr++;
            }
            fputc(*ptr, fptr);
            fclose(fptr);
            write(client_socket, web_page, sizeof(web_page));
        } else
            write(client_socket, web_page, sizeof(web_page));
        printf("POST / of buf is [%s]\n",buf);
    }
    write(client_socket, web_page, sizeof(web_page));
}
int main(){
    int server_socket, client_socket, listening;
    struct sockaddr_in server_address;
    /**
     * create a socket with IPv4, double-sided, and default protocol
     */
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    /**
     * initialize the structure for server
     * bit zero which is like memset
     * use ipv4, port with 8080, and set anyone can get in
     */
    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(8080);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    /**
     * bind server socket with IP, port
     */
    bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address));
    /**
     * set the max connection to server socket
     */
     listening = listen(server_socket, 10000);
     signal(SIGCHLD, signalchild_handler);
     info(listening, SERVER);
     while(1){
         client_socket = accept(server_socket, NULL, NULL);
         pid_t child = fork();
         if(child == 0){
             close(listening);
             info(client_socket, CLIENT);
             process(client_socket);
             close(client_socket);
             exit(0);
         }
         close(client_socket);
     }
     return 0;
}
