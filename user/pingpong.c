#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

/**
 * Write a program that uses UNIX system calls to ''ping-pong'' a byte between two processes over a pair of pipes, one for each direction. 
 * The parent should send a byte to the child;
 * the child should print "<pid>: received ping", where <pid> is its process ID, write the byte on the pipe to the parent, and exit;
 * the parent should read the byte from the child, print "<pid>: received pong", and exit. 
 * Your solution should be in the file user/pingpong.c.
*/

int
main(){
  // 创建管道;
  int p[2];
  char buf[1];
  pipe(p);
  // 创建子进程;
  if(fork() == 0){
    // 读取 ping
    int r_num = read(p[0],buf,1);
    if(r_num < 1){
      fprintf(2,"ping-pong error: ping received wrong...\n"); // 处理错误;
      exit(1);
    }
    fprintf(1,"%d: received ping\n",getpid());
    close(p[0]);
    // 发送 pong
    write(p[1],"1",1);
    close(p[1]); // 发完就关;
  }else{
    write(p[1],"0",1); // "ping"
    close(p[1]); // 写完立马关闭自己的写
    int status;
    wait(&status); //! \attention 需要我们等待子线程,不然父线程"自投自抢"
    // 读取 pong
    int r_num = read(p[0],buf,1);
    if(r_num < 1){
      fprintf(2,"ping-pong error: pong received wrong...\n"); // 处理错误;
      exit(1);
    }
    fprintf(1,"%d: received pong\n",getpid());
    close(p[0]);
  }
  exit(0);
}