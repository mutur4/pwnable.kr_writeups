#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int main(int argc, char **argv){
        char *args[100] = {};
        pid_t childpid;
        int pipe_stdin[2], pipe_stderr[2];

        for(int i = 0;i < 100;i++){
                args[i] = "A";
        }
        args[100] = NULL; args['A'] = "\x00"; args['B'] = "\x20\x0a\x0d";
        args['C'] = "4444";
        int sock_fd; struct sockaddr_in addr;

        setenv("\xde\xad\xbe\xef", "\xca\xfe\xba\xbe", 1);
        //char *environ[] = {"\xde\xad\xbe\xef=\xca\xfe\xba\abe", NULL};
        extern char **environ;

        /*create a file :P*/
        FILE* fp = fopen("\x0a", "w");
        if (fp < 0){
                perror("Error Opening file!!");
                return -1;
        }

        fwrite("\x00\x00\x00\x00", 4, 1, fp);
        fclose(fp);

        pipe(pipe_stdin);
        pipe(pipe_stderr);
        childpid = fork();

        if (childpid < 0){
                perror("Error in Fork()!");
        }

        if (childpid == 0){
                close(pipe_stdin[0]);
                close(pipe_stderr[0]);

                write(pipe_stdin[1], "\x00\x0a\x00\xff", 4);
                write(pipe_stderr[1], "\x00\x0a\x02\xff", 4);

                sock_fd = socket(AF_INET, SOCK_STREAM, 0);
                if(sock_fd < 0){
                        puts("Socket Failed Creation !");
                }

                // set the port to connect to and the adddress
                addr.sin_family = AF_INET;
                addr.sin_port = htons(atoi(args['C']));
                addr.sin_addr.s_addr = inet_addr("127.0.0.1");

                if(connect(sock_fd, (struct sockaddr *) &addr, sizeof(addr)) < 0){
                        printf("Connection Failed Too Bad !");
                }
                write(sock_fd, "\xde\xad\xbe\xef", 4);
                close(sock_fd);
                return 0;
        }else{
                close(pipe_stdin[1]); close(pipe_stderr[1]);
                dup2(pipe_stdin[0], 0);
                dup2(pipe_stderr[0], 2);

                close(pipe_stdin[0]); close(pipe_stderr[0]);
                execve("./name", args, environ);
        }
        return 0;
}
