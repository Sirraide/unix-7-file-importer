#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>

#define ever ;;

#define MAXPLEN 256

char the_path[MAXPLEN];
int plen;
struct stat statbuf;

strncpy(dest, src, n)
     char *src;
     char *dest;
{
  register i;
  for (i = 0; i < n - 1 && src[i]; i++) {
    dest[i] = src[i];
  }
  dest[i] = 0;
  return i;
}

/*
PERM
FILENAME
HEXDATA.............................
PERM
FILENAME
HEXDATA.............................
*/

XpD() {
  register prev, file;
  static struct direct dentry;
  prev = plen;
  file = open(the_path, 0);
  if (file < 0) return;
  for (ever) {
    /* If we are done quit */
    if (read(file, (char *)&dentry, sizeof(dentry)) != sizeof(dentry)) break;
    /* If the file name or inode is BS, skip */
    if (dentry.d_ino == 0 ||
dentry.d_name[0] == '.' && (dentry.d_name[1] == 0 ||
    dentry.d_name[1] == '.')) continue;
    /* It looks good, proceed */
    the_path[plen] = '/';
    plen++;
    plen += strncpy(the_path + plen, dentry.d_name, MAXPLEN - plen);
    Process();
    the_path[prev] = 0;
    plen = prev;
  }
  close(file);
  
}

XpF() {
  int file, rdd;
  char next[64];
  register i;
  i = 0;
  file = open(the_path, 0);
  if (file < 0) return;
  printf("%o\n", statbuf.st_mode & 0777);
  printf("%s\n", the_path);
  printf("%ld\n", statbuf.st_size);
  while ((rdd = read(file, next, 64))) {
    for (i = 0; i < rdd; i++) {
        printf("%02x", (unsigned int) next[i]);
    }
    printf("\n");
  }
  close(file);
}

Process(){
  register cantstat;
  cantstat = stat(the_path, &statbuf);
  if (cantstat) return;
  if (statbuf.st_mode & S_IFDIR) {
    XpD();
  } else if (statbuf.st_mode & S_IFREG) {
    XpF();
  } else {}
}

main(argc,argv)
     char *argv[];
{
  plen = strncpy(the_path, *++argv, MAXPLEN);
  Process();
  return 0;
}
