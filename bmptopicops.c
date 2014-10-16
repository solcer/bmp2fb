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

	int fbfd[EKRANADEDI] , tty = 0;;

    	struct fb_var_screeninfo vinfo[EKRANADEDI];

    	struct fb_fix_screeninfo finfo[EKRANADEDI];

	struct fb_con2fbmap c2m[EKRANADEDI];			//tty leri framebuffer a atamak için kullanılıyor
	
    	long int screensize = 0;

    	char *fbp[EKRANADEDI] ;

    	int x = 0, y = 0, i = 0;

    	uint8_t pixel = 0;

    	bool stripe = true;

    	long int location = 0;
	
	uint8_t buffer[10];

	for(i=0;i<EKRANADEDI;i++)
	{
	  fbp[i]=0;
	  fbfd[i]=0;
	}
	if (argc != 2) {
		fprintf(stderr, "Usage: %s image.bmp\n", argv[0]);
		return 1;
	}
	
	if ((getuid ()) != 0) {

        	printf ("You are not root, run this as root or sudo\n");

        	exit(1);

    	}

	for(i=0;i<EKRANADEDI;i++)
	{
		sprintf(&buffer[0],"/dev/fb%d",i);
		
		c2m[i].console = (uint32_t) i;			//framebuffer a atanacak consol numarası
		c2m[i].framebuffer = (uint32_t) i;		//framebuffer numarası
		
		fbfd[EKRANADEDI] = open(buffer, O_RDWR);	//framebuffer ı açıyoru
		if (fbfd[EKRANADEDI] == -1) 			//hata geldi mi?
		{
		      perror("Error: cannot open framebuffer device :" );
		      exit(1);
		}
		
		if (ioctl(fbfd[EKRANADEDI], FBIOPUT_CON2FBMAP, &c2m[EKRANADEDI])) 			//framebufferı tty lere assign et
		{
		  //fprintf(stderr, "%s: Cannot set console mapping\n", progname);
		  perror("Error: cannot assign framebuffer to tty device : " );
		  exit(1);
		  
		}
		
		
		if (ioctl(fbfd[EKRANADEDI], FBIOGET_FSCREENINFO, &finfo[EKRANADEDI]) == -1) {

		    perror("Error reading fixed information :");

		    exit(2);

		}

		// Get variable screen information
		if (ioctl(fbfd[EKRANADEDI], FBIOGET_VSCREENINFO, &vinfo[EKRANADEDI]) == -1) {

			perror("Error reading variable information :" );

			exit(3);
		}
		printf("Variable screen information fb%d: %dx%d, %dbpp\n",i, vinfo[EKRANADEDI].xres, vinfo[EKRANADEDI].yres, vinfo[EKRANADEDI].bits_per_pixel);
	}
	

    	

    	printf("The framebuffer devices was opened successfully.\n");

 

 
/*
    	tty = open("/dev/tty1", O_RDWR);

 

    	if(ioctl(tty, KDSETMODE, KD_GRAPHICS) == -1)

		printf("Failed to set graphics mode on tty1\n");

 

    	// Get fixed screen information

    	
    	printf("Variable screen information: %dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

 

    	// Figure out the size of the screen in bytes

    	screensize = vinfo.yres_virtual * finfo.line_length;

    	printf("screensize: %d\n",screensize); 

 

 

    	// Map the device to memory

    	fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);

    	if ((int)fbp == -1) {

        	perror("Error: failed to map framebuffer device to memory");

        	exit(4);

    	}

	printf("The framebuffer device was mapped to memory successfully.\n");


//	nanosleep((struct timespec[]){{0, 50000000}}, NULL);

	// create our bmp image 
	bmp_create(&bmp, &bitmap_callbacks);

	// load file into memory 
	unsigned char *data = load_file(argv[1], &size);

	// analyse the BMP 
	code = bmp_analyse(&bmp, size, data);
	if (code != BMP_OK) {
		warning("bmp_analyse", code);
		res = 1;
		goto cleanup;
	}

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

cleanup:
	// clean up 
	bmp_finalise(&bmp);
	free(data);
	
	munmap(fbp, screensize); 
	*/
	for(i=0;i<EKRANADEDI;i++)
	{
	    close(fbfd[EKRANADEDI]);
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
