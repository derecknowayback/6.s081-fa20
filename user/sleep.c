#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  /*  rm的例子: 
      if(argc < 2){
        fprintf(2, "Usage: rm files...\n");
        exit(1);
      }*/
  if(argc < 2){
    fprintf(2,"usage: sleep ...\n");
    // fprintf(2,"sleep: failed to sleep...\n");
    exit(1);
  }
  /*从1开始，忽略参数0;*/  
  int time = atoi(argv[1]);
  //! \bug 如果time是负数怎么办？
  sleep(time);
  exit(0);
}
