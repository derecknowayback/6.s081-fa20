#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

/**
 * Write a simple version of the UNIX xargs program: read lines from the standard input and run a command for each line, 
 * supplying the line as arguments to the command.
 * Your solution should be in the file user/xargs.c.
 * Use fork and exec to invoke the command on each line of input. 
 * Use wait in the parent to wait for the child to complete the command.
 * To read individual lines of input, read a character at a time until a newline ('\n') appears.
 * kernel/param.h declares MAXARG, which may be useful if you need to declare an argv array.
 * Add the program to UPROGS in Makefile.
  Changes to the file system persist across runs of qemu; to get a clean file system run make clean and then make qemu.
*/

int k = 0;


int
xargs(int base,int argc,char *argv[])
{
  char arguments[MAXARG][50],c;
  int line = 0, col = 0, full = 0;
  while (read(0,&c,1) == 1)
  {
    if(c == '\n' || c == 0) break; // 读取结束了;
    if(full) continue; // 太多参数就不要了;
    if(c == ' '){
      if(col == 0) continue; // 清除连续的空格;
      col = col >= 49 ? 49 : col;
      arguments[line][col] = 0;
      line++;
      col = 0;  
      if(line == MAXARG) {
        full = 1;
        continue;
      }
    }
    if(col < 50)
      arguments[line][col++] = c;
  }
  if(col == 0) line--; // 将line修正为 "有意义"的最后一行;
  if(line == -1) return 0; // 如果 line修正后是-1的话,说明已经没有可以读的了;
  if(col < 50) arguments[line][col] = 0; // c语言字符串,修正末尾

  // 拼接参数
  char *pass[MAXARG];
  int i;
  for ( i = 0; i + base < argc && i < MAXARG; i++)
  {
    pass[i] = argv[i + base];
  }
  int j;
  for (j = 0 ; i < MAXARG && j <= line; i++,j++)
  {
    pass[i] = arguments[j];
    printf("arrgument-%d: %s\n",j,pass[i]);
  }
  if(i < MAXARG)
    pass[i] = 0;
    
  // fork子线程执行  
  if(fork()==0){
    exec(argv[1],pass);
  }else{
    int status;
    while (wait(&status) > 0);
  }

  // 如果是 c == '\n'的话,返回1,表示可能要继续做;
  if(c == '\n') return 1;
  return 0;
}


int
main(int argc, char *argv[])
{
  // printf("%d\n",argc);
  int offset = 1;
  if(argc < 2)
  {
    printf("xargs: need one command...\n");
    exit(1);
  }
  // ignore "-n 1"
  if(strcmp(argv[1],"-n") == 0){
    offset = 3; // jump to "1", after continue "i == 3";
  }

  while(xargs(offset,argc,argv));
  exit(0);
}