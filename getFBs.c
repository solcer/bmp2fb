#include <string.h>
#include <stdio.h>
#include <stdlib.h>
int main(const int argc, const char* const* const argv) {
 int res;
 int err;
 unsigned char buffer[1000];
 res = system("ls /dev/fb*");
 //sprintf(buffer,"%s",stdout);
 //printf("boyut %d:\n %s",strlen(buffer),buffer);
 
/* 
 
 //fprintf(stdout,"execution returned %d.\n",res);
 if ((-1 != res) && (127 != res))
  fprintf(stdout,"now would be a good time to check out 'man split' to check what the resulting return-value (%d) means.\n",res);
 return res;*/
}