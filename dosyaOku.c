#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "libnsbmp.c"


#include <unistd.h>

#include <fcntl.h>

#include <linux/fb.h>

#include <sys/mman.h>

#include <sys/ioctl.h>

#include <linux/input.h>

#include <linux/kd.h>

#include "rs232.c" 

signed int offsetler[17]={0,18,50,48,36,32,45,44,55,54,44,20,59,47,51,30,53};			//3. ayar
void ofsetleriAl(void);
char buffer[100];
main()
{
	
   ofsetleriAl();
	// test for files not existing. 
	printf("buffer: %s",buffer);
	return 0;
}

void ofsetleriAl(void)
{
FILE *ofsetDosyasi;
char * pch,i;
	ofsetDosyasi  = fopen("./ofsetler.dat", "w"); // 
	if (ofsetDosyasi == NULL ) 
	{   
	  printf("Error! Could not open file\n"); 
	  exit(-1); // must include stdlib.h 
	} 
	for (i = 0; i < 17; i++)
    {
        fscanf(ofsetDosyasi, "%d,", &offsetler[i] );

    }
	fclose(ofsetDosyasi);
}