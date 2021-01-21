## Challenge Description 

This was a classic challenge that required some input for the user to be given the flag
> #### Description: Mom  How can I pass my input to a computer program?

> #### Connection: sshpass -p 'guest' ssh input2@pwnable.kr -p 2222 

Ssh into the server we had the following source code alogether

```
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc, char* argv[], char* envp[]){
        printf("Welcome to pwnable.kr\n");
        printf("Let's see if you know how to give input to program\n");
        printf("Just give me correct inputs then you will get the flag :)\n");

        // argv
        if(argc != 100) return 0;
        if(strcmp(argv['A'],"\x00")) return 0;
        if(strcmp(argv['B'],"\x20\x0a\x0d")) return 0;
        printf("Stage 1 clear!\n");

        // stdio
        char buf[4];
        read(0, buf, 4);
        if(memcmp(buf, "\x00\x0a\x00\xff", 4)) return 0;
        read(2, buf, 4);
        if(memcmp(buf, "\x00\x0a\x02\xff", 4)) return 0;
        printf("Stage 2 clear!\n");

        // env
        if(strcmp("\xca\xfe\xba\xbe", getenv("\xde\xad\xbe\xef"))) return 0;
        printf("Stage 3 clear!\n");

        // file
        FILE* fp = fopen("\x0a", "r");
        if(!fp) return 0;
        if( fread(buf, 4, 1, fp)!=1 ) return 0;
        if( memcmp(buf, "\x00\x00\x00\x00", 4) ) return 0;
        fclose(fp);
        printf("Stage 4 clear!\n");

        // network
        int sd, cd;
        struct sockaddr_in saddr, caddr;
        sd = socket(AF_INET, SOCK_STREAM, 0);
        if(sd == -1){
                printf("socket error, tell admin\n");
                return 0;
        }
        saddr.sin_family = AF_INET;
        saddr.sin_addr.s_addr = INADDR_ANY;
        saddr.sin_port = htons( atoi(argv['C']) );
        if(bind(sd, (struct sockaddr*)&saddr, sizeof(saddr)) < 0){
                printf("bind error, use another port\n");
                return 1;
        }
        listen(sd, 1);
        int c = sizeof(struct sockaddr_in);
        cd = accept(sd, (struct sockaddr *)&caddr, (socklen_t*)&c);
        if(cd < 0){
                printf("accept error, tell admin\n");
                return 0;
        }
        if( recv(cd, buf, 4, 0) != 4 ) return 0;
        if(memcmp(buf, "\xde\xad\xbe\xef", 4)) return 0;
        printf("Stage 5 clear!\n");

        // here's your flag
        system("/bin/cat flag");
        return 0;
}

```

#### Stage One 
- Here all we had to do was pass some args to the program.

```
if(argc != 100) return 0;
        if(strcmp(argv['A'],"\x00")) return 0;
        if(strcmp(argv['B'],"\x20\x0a\x0d")) return 0;
        printf("Stage 1 clear!\n");
```
- We can see that the program requires the argument count `argc` to be `100`.
- The `argv[65]` this should be equal to `\x00`
- The `argv[66]` this should be equal to `\x20\x0a\x0d`
- Using `execve` we can call another process in linux. `man exceve` to learn more about this syscall.
- We will create a simple 'c' script that will be used to pass this stage and build upon it

```
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char **argv){
  char *args[99+1]= {NULL};
  for(int i = 0;i < 99;i++){
    args[i] = "A";
  }
  args[65] = "\x00"; /* 65 is the same as 'A' in ascii*/
  args[66] = "\x20\x0a\x0d"; /* 66 is the same as 'B' in ascii*/
 
  */ 
  according to the man page, execve takes 3 args
  1. The process to the program to call   
  2. The cmdline arguments to be passed to the process that was called
  3. The environment variables
  */
  execve("./input", argv, NULL);
  return 0;
}
```
#### Stage Two
- In this stage we are required to provide some input to the program to pass this stage.

```
char buf[4];
read(0, buf, 4);
if(memcmp(buf, "\x00\x0a\x00\xff", 4)) return 0;
  read(2, buf, 4);
if(memcmp(buf, "\x00\x0a\x02\xff", 4)) return 0;
  printf("Stage 2 clear!\n");
```
- Here the program uses the read function to read input from `stdin`.
- Using `pipe` we can pass out input to the program and pass this stage
- Using fork() to create a child and parent process to effectively pass our input to the program

```
#include <unistd.h> 
#include <sys/types.h> 
void pass_input{
  int pipe_stdin[2], pipe_stdout[2];
  pid_t child_pid;
  
 if ((child_pid = fork()) < 0){
  perror("There was an error creating the child process :(");
 }
 if (child_pid == 0){
  close(pipe_stdin[0]); close(pipe_stdout[0]); //we close the stdin of the pipes since we are writing
  write(pipe_stdin[0], "\x00\x0a\x00\xff", 4);
  write(pipe_stdin[1], "\x00\x0a\x02\xff", 4);
 }else{
  close(pipe_stdin[1]); close(pipe_stdout[1]);
  dup2(pipe_stdin[0], 0); // This are used to duplicate the fd to stdin and stderr respcetively are read by the program we are calling
  dup2(pipe_stdout[0], 2);
 }
 return 0;
}
```
#### Stage 3
- We are required to pass some evironment variable to the process we called usin execve()

```
  if(strcmp("\xca\xfe\xba\xbe", getenv("\xde\xad\xbe\xef"))) return 0;
        printf("Stage 3 clear!\n")
```
- using `setenv` we will be able to pass the required input to the environment variable

```
setenv("\xde\xad\xbe\xef", "\xca\xfe\xba\xbe", 1);
extern char **environ;
exceve("./input", args,environ);
exceve("./input", argc, environ);
```

#### Stage 4
- We are required to create a file and pass some input to the file 

```
FILE* fp = fopen("\x0a", "r");
        if(!fp) return 0;
        if( fread(buf, 4, 1, fp)!=1 ) return 0;
        if( memcmp(buf, "\x00\x00\x00\x00", 4) ) return 0;
        fclose(fp);
        printf("Stage 4 clear!\n");
```

- We create a file called `fp`. Here we just reverse whatever the program does

```
void create_file(){
  FILE *fp = fopen("\x0a", "r");
  if(!fp){
    perror("There was an error creating the file");
  }
  fwrite("\x00\x00\x00\x00", 4, 1, fp);
  fclose(fp);
}
```
#### Stage 5
- We are required to connect to as the client and send some input

```
if(bind(sd, (struct sockaddr*)&saddr, sizeof(saddr)) < 0){
                printf("bind error, use another port\n");
                return 1;
        }
        listen(sd, 1);
        int c = sizeof(struct sockaddr_in);
        cd = accept(sd, (struct sockaddr *)&caddr, (socklen_t*)&c);
        if(cd < 0){
                printf("accept error, tell admin\n");
                return 0;
        }
        if( recv(cd, buf, 4, 0) != 4 ) return 0;
        if(memcmp(buf, "\xde\xad\xbe\xef", 4)) return 0;
        printf("Stage 5 clear!\n");
```

- There is a server that is waiting for us to connect to the user provided port 

```
#include <sys/sockets.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void main(){
   int sock_fd;
   struct sockaddr_in addr;
   sock_fd = socket(AF_INET, SOCK_STREAM, 0);
   if (sock_fd < 0){
    printf("Error creating socket !");
   }
   addr.sin_family = AF_INET;
   addr.sin_port = htons(atoi(argv['C']));
   addr.sin_addr.s_addr = inet_addr('127.0.0.1');
   if (connect(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0){
    printf("There was a connection error :(");
   }
   write(sock_fd, "\xde\xad\xbe\xef", 4);
   close(sock_fd);
}

```
- And the final stage is complete and we get the flag
- Check out the final code at 
