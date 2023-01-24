#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

/**
 * Write a simple version of the UNIX find program: find all the files in a directory tree with a specific name. 
 * Your solution should be in the file user/find.c.
 * Look at user/ls.c to see how to read directories.
   Use recursion to allow find to descend into sub-directories.
   Don't recurse into "." and "..".
   Changes to the file system persist across runs of qemu; to get a clean file system run make clean and then make qemu.
   Add the program to UPROGS in Makefile.
 * 
*/


void 
find(char *path,char *target)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0){
    fprintf(2, "find: cannot open %s\n", path);
    exit(1);
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    exit(1);
  }

  if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      fprintf(2,"find: path too long\n");
      close(fd);
      exit(1);
  }

  strcpy(buf, path);
  p = buf+strlen(buf);
  *p++ = '/';

  while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf("find: cannot stat %s\n", buf);
        continue;
      }
      // 如果是目录就递归("."和".."不递归):
      if(st.type == T_DIR && strcmp(".",de.name) && strcmp("..",de.name)){
        find(buf,target); //! \attention 这边要小心传参的问题;
      }
      // 否则是文件,就比较;
      else{
        if(strcmp(target,de.name) == 0){
          printf("%s\n",buf);
        }
      }
  }
  close(fd);
}


int 
main(int argc, char *argv[])
{
  // deal with no arguments
  if(argc < 3){
    fprintf(2,"usage: find command need as least 2 arguments...\n");
    exit(0);
  }
  find(argv[1],argv[2]);
  exit(0);
}