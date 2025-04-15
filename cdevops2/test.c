#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>

int main(int argc , char *argv[]){
    if(argc < 2){
      printf("empty arguments \n");
      return 0;
    }
    int fd;
    fd = open(argv[1],O_RDONLY);
    if(fd < 0){
        printf("error openinng file in rd \n");
        return 0;
    }
    close(fd);
    fd = open(argv[1],O_WRONLY | O_NONBLOCK);
    if(fd < 0){
        printf("error openinng file nonblock \n");
        return 0;
    }
    close(fd);
    fd = open(argv[1],O_SYNC | O_RDWR);
    if(fd < 0){
        printf("error openinng file sync \n");
        return 0;
    }
    close(fd);
}