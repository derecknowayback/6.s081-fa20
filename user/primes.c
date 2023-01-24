#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

/**
 * Your goal is to use pipe and fork to set up the pipeline. 
 * The first process feeds the numbers 2 through 35 into the pipeline. 
 * For each prime number, you will arrange to create one process that reads from its left neighbor over a pipe and writes to its right neighbor over another pipe. 
 * Since xv6 has limited number of file descriptors and processes, the first process can stop at 35.
*/

void feed(int is_init,int p0,int p1,int count){
  if(is_init){
    int p[2];
    pipe(p);
    if(fork() == 0){
      feed(0,p[0],p[1],17);
    }else{
      int i;
      fprintf(1,"prime %d\n",2);
      close(p[0]);
      for (i = 2; i <= 35; i++)
      {
        if(i%2)
          write(p[1],&i,sizeof(int));
      }
      close(p[1]);
      // 等待孩子;
      int status;
      while (wait(&status) > 0);
    }
  }else{
    // 打印本轮数字
    int prime, k, has_feed = 0;
    read(p0,&prime,4); // 按照四字节读取
    count--;
    fprintf(1,"prime %d\n",prime);
    int p[2]; // 新建管道
    pipe(p);

    // 读取父进程传来的数字
    while (count)
    {
      read(p0,&k,sizeof(int));
      count--; //! \attention 别忘了这里要--;
      if(k%prime){
        write(p[1],&k,sizeof(int));
        has_feed += 1; 
      }
    }
    close(p0); 
    close(p1);

    // 看看要不要fork一个进程出来: 
    if(has_feed){
      if(fork() == 0){
        feed(0,p[0],p[1],has_feed);
      }else{
        close(p[0]);
        close(p[1]);
      }
    }
    // 等待孩子;
    int status;
    while (wait(&status) > 0);
    exit(0);
  }
}

int
main(){
  feed(1,-1,-1,0);
  exit(0);
}