/*
 * Copyright 2008 Sean Fox <dyntryx@gmail.com>
 * Copyright 2008 James Bursa <james@netsurf-browser.org>
 *
 * This file is part of NetSurf's libnsbmp, http://www.netsurf-browser.org/
 * Licenced under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 */


//selim Ölçer 14/10/2014 bmp to framebuffer
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

#define BYTES_PER_PIXEL 4
#define TRANSPARENT_COLOR 0xffffffff


unsigned char *load_file(const char *path, size_t *data_size);
void warning(const char *context, bmp_result code);
void *bitmap_create(int width, int height, unsigned int state);
unsigned char *bitmap_get_buffer(void *bitmap);
size_t bitmap_get_bpp(void *bitmap);
void bitmap_destroy(void *bitmap);
void showBitmap(uint8_t *resim, char *fbPointer);

void handler (int sig)

{

  printf ("nexiting...(%d)n", sig);

  exit (0);

}

 

void perror_exit (char *error)

{

  perror (error);

  handler (9);

}

#define EKRANADEDI 6

int main(int argc, char *argv[])
{
	bmp_bitmap_callback_vt bitmap_callbacks = {
		bitmap_create,
		bitmap_destroy,
		bitmap_get_buffer,
		bitmap_get_bpp
	};
	bmp_result code;
	bmp_image bmp;
	size_t size;
	unsigned short res = 0;
	
    	/* framebuffer */

	int fbfd[EKRANADEDI] , tty[EKRANADEDI] ;

    	struct fb_var_screeninfo vinfo[EKRANADEDI];

    	struct fb_fix_screeninfo finfo[EKRANADEDI];

	struct fb_con2fbmap c2m[EKRANADEDI];			//tty leri framebuffer a atamak için kullanılıyor
	
    	long int screensize[EKRANADEDI] ;

    	char *fbp[EKRANADEDI] ;

    	int x = 0, y = 0, i = 0;

    	uint8_t pixel = 0;

    	bool stripe = true;

    	long int location = 0;
	
	uint8_t buffer[10];
	unsigned char *data[EKRANADEDI];				//resim datasını içeren değişken

	for(i=0;i<EKRANADEDI;i++)
	{
	  fbp[i]=0;
	  fbfd[i]=0;
	  tty[i]=0;
	  screensize[i]=0;
	}
	if (argc != 2) {
		fprintf(stderr, "Usage: %s image.bmp\n", argv[0]);
		return 1;
	}
	
	if ((getuid ()) != 0) {

        	printf ("You are not root, run this as root or sudo\n");

        	exit(1);

    	}

	for(i=0;i<EKRANADEDI;i++)				//ekranları tty lere atayıp ram'de pointer a işaretliyor
	{
		//printf("i:%d\n",i);		

		c2m[i].console = (uint32_t) 10+i;			//framebuffer a atanacak consol numarası
		c2m[i].framebuffer = (uint32_t) i;		//framebuffer numarası

		sprintf(&buffer[0],"/dev/fb%d",i);
		fbfd[i] = open(buffer, O_RDWR);	//framebuffer ı açıyoru
		if (fbfd[i] == -1) 			//hata geldi mi?
		{
			perror("Error: cannot open framebuffer device :" );
			exit(1);
		}
		printf("The framebuffer %d  was opened successfully.\n",i);

		if (ioctl(fbfd[i], FBIOPUT_CON2FBMAP, &c2m[i])) 			//framebufferı tty lere assign et
		{
			//fprintf(stderr, "%s: Cannot set console mapping\n", progname);
			perror("Error: cannot assign framebuffer to tty device : " );
			exit(1);
			
		}
		printf("The framebuffer %d  was assigned successfully to tty%d.\n",i,i+10);


		sprintf(&buffer[0],"/dev/tty%d",i+10);
		tty[i] = open(buffer, O_RDWR);
		if(ioctl(tty[i], KDSETMODE, KD_GRAPHICS) == -1)
			printf("FAILED to set graphics mode on tty%d\n",i+10);



		if (ioctl(fbfd[i], FBIOGET_FSCREENINFO, &finfo[i]) == -1) {

			perror("Error reading fixed information :");

			exit(2);

		}

		// Get variable screen information
		if (ioctl(fbfd[i], FBIOGET_VSCREENINFO, &vinfo[i]) == -1) {

			perror("Error reading variable information :" );

			exit(3);
		}
		printf("Variable screen information fb%d: %dx%d, %dbpp\n",i, vinfo[i].xres, vinfo[i].yres, vinfo[i].bits_per_pixel);


		// Figure out the size of the screen in bytes

		screensize[i] = vinfo[i].yres_virtual * finfo[i].line_length;

		printf("screensize[%d]: %d\n",i,screensize[i]); 





		// Map the device to memory

		fbp[i] = (char *)mmap(0, screensize[i], PROT_READ | PROT_WRITE, MAP_SHARED, fbfd[i], 0);

		if ((int)fbp[i] == -1) {

		perror("Error: failed to map framebuffer device to memory");

		exit(4);

		}
		printf("The framebuffer[%d] was mapped to memory successfully.\n",i);


	}
	
	// create our bmp image 
	bmp_create(&bmp, &bitmap_callbacks);
	//showBitmap();
	for(i=0;i<EKRANADEDI;i++)		//bitmap işleri
	{  
		// load file into memory 
		sprintf(&buffer[0],"./images/samplescreen%d.bmp",i);
		data[i] = load_file(buffer, &size);
		// analyse the BMP 
		code = bmp_analyse(&bmp, size, data[i]);
		if (code != BMP_OK) {
			warning("bmp_analyse", code);
			res = 1;
			goto cleanup;
		}
	}
	

	
/*
	

	// decode the image 
	code = bmp_decode(&bmp);
	// code = bmp_decode_trans(&bmp, TRANSPARENT_COLOR); 
	if (code != BMP_OK) {
		warning("bmp_decode", code);
		// allow partially decoded images 
		if (code != BMP_INSUFFICIENT_DATA) {
			res = 1;
			goto cleanup;
		}
	}

	printf("P3\n");
	printf("# %s\n", argv[1]);
	printf("# width                %u \n", bmp.width);
	printf("# height               %u \n", bmp.height);
	printf("# size			%u\n", bmp.buffer_size);
	printf("# bpp			%d\n", bmp.bpp);
	printf("# Encoding		%d\n", bmp.encoding);
	
	printf("%u %u 256\n", bmp.width, bmp.height);

	{
		uint16_t row, col,pixelCounter=0;
		uint8_t *image;
		image = (uint8_t *) bmp.bitmap;
		// *(uint8_t*)fbp= (uint8_t *) bmp.bitmap;
		switch(vinfo.bits_per_pixel)
		{
		  case 16:						//ekran çözünürlüğü 16bpp ise
		    //printf("Ekran 16bpp\n");
		    if(bmp.bpp==16){					//resim çözünülüğü 16bpp mi?
		      for (row = 0; row != bmp.height; row++) {
			      for (col = 0; col != bmp.width; col++) {
				      size_t z = (row * bmp.width + col) * BYTES_PER_PIXEL;		//bmp içerisinde bpp ne olursa olsun her bir pixel bilgisi 4 byte uzunlugundadir. burada pixel başlangıcı hesaplanıyor.
				      //printf("byte %d %d %d**  \n",	image[z],image[z + 1],image[z + 2]);
				      location = col*2+(row*finfo.line_length);			//her bir pixel 2 byte olduğu için col*2 yaptım.
				      *((uint16_t*)(fbp + location)) = image[z]<<8|image[z+1]<<3|image[z+2]>>3;		//ram'e datayı yazıyorum.
			      } 
		      }
		    }else if(bmp.bpp==24) 		//resim çözünürlüğü 24bpp mi?
		    {
			//printf("16bpp ekrana 24bpp resim...");
		      for (row = 0; row != bmp.height; row++) {
			      for (col = 0; col != bmp.width; col++) {
				    size_t z = (row * bmp.width + col) * BYTES_PER_PIXEL;		//bmp içerisinde bpp ne olursa olsun her bir pixel bilgisi 4 byte uzunlugundadir. burada pixel başlangıcı hesaplanıyor.
				    location = col*2+(row*finfo.line_length);			//her bir pixel 2 byte olduğu için col*2 yaptım.
				    *((uint16_t*)(fbp + location)) = ((uint16_t)(image[z] << 8) &  0xf800) | ((uint16_t)(image[z+1] << 3) & 0x7E0) |(uint16_t)((image[z+2]>>3) & 0x1f);
				     
			      } 
		      }
		    }
		      break;
		  case 24:
		    printf("Ekran 24bpp\n");
		    for (row = 0; row != bmp.height; row++) {
			    for (col = 0; col != bmp.width; col++) {
				    size_t z = (row * bmp.width + col) * BYTES_PER_PIXEL;		//bmp içerisinde bpp ne olursa olsun her bir pixel bilgisi 4 byte uzunlugundadir. burada pixel başlangıcı hesaplanıyor.
				    location = col*3+(row*finfo.line_length);
				    *((uint32_t*)(fbp + location)) = image[z];
			    }
		    }
		    break;
		}
	}
*/
cleanup:
	// clean up 
	bmp_finalise(&bmp);
	
	
	for(i=0;i<EKRANADEDI;i++)
	{
		free(data[i]);
		munmap(fbp[i], screensize[i]); 
		close(fbfd[i]);
	}

	return res;
}


unsigned char *load_file(const char *path, size_t *data_size)
{
	FILE *fd;
	struct stat sb;
	unsigned char *buffer;
	size_t size;
	size_t n;

	fd = fopen(path, "rb");
	if (!fd) {
		perror(path);
		exit(EXIT_FAILURE);
	}

	if (stat(path, &sb)) {
		perror(path);
		exit(EXIT_FAILURE);
	}
	size = sb.st_size;

	buffer = malloc(size);
	if (!buffer) {
		fprintf(stderr, "Unable to allocate %lld bytes\n",
				(long long) size);
		exit(EXIT_FAILURE);
	}

	n = fread(buffer, 1, size, fd);
	if (n != size) {
		perror(path);
		exit(EXIT_FAILURE);
	}

	fclose(fd);

	*data_size = size;
	return buffer;
}


void warning(const char *context, bmp_result code)
{
	fprintf(stderr, "%s failed: ", context);
	switch (code) {
		case BMP_INSUFFICIENT_MEMORY:
			fprintf(stderr, "BMP_INSUFFICIENT_MEMORY");
			break;
		case BMP_INSUFFICIENT_DATA:
			fprintf(stderr, "BMP_INSUFFICIENT_DATA");
			break;
		case BMP_DATA_ERROR:
			fprintf(stderr, "BMP_DATA_ERROR");
			break;
		default:
			fprintf(stderr, "unknown code %i", code);
			break;
	}
	fprintf(stderr, "\n");
}


void *bitmap_create(int width, int height, unsigned int state)
{
	(void) state;  /* unused */
	return calloc(width * height, BYTES_PER_PIXEL);
}


unsigned char *bitmap_get_buffer(void *bitmap)
{
	assert(bitmap);
	return bitmap;
}


size_t bitmap_get_bpp(void *bitmap)
{
	(void) bitmap;  /* unused */
	return BYTES_PER_PIXEL;
}


void bitmap_destroy(void *bitmap)
{
	assert(bitmap);
	free(bitmap);
}

void showBitmap(uint8_t *resim, char *fbPointer)
{
	
}
