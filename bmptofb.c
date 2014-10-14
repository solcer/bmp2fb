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

	int fbfd = 0, tty = 0;;

    	struct fb_var_screeninfo vinfo;

    	struct fb_fix_screeninfo finfo;

    	long int screensize = 0;

    	char *fbp = 0;

    	int x = 0, y = 0, i = 0;

    	uint8_t pixel = 0;

    	bool stripe = true;

    	long int location = 0;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s image.bmp\n", argv[0]);
		return 1;
	}
	
	if ((getuid ()) != 0) {

        	printf ("You are not root, run this as root or sudo\n");

        	exit(1);

    	}


	fbfd = open("/dev/fb0", O_RDWR);

    	if (fbfd == -1) {

        	perror("Error: cannot open framebuffer device");

        	exit(1);

    	}

    	printf("The framebuffer device was opened successfully.\n");

 

 

    	tty = open("/dev/tty1", O_RDWR);

 

    	if(ioctl(tty, KDSETMODE, KD_GRAPHICS) == -1)

		printf("Failed to set graphics mode on tty1\n");

 

    	// Get fixed screen information

    	if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1) {

        	perror("Error reading fixed information");

        	exit(2);

    	}

 

    	// Get variable screen information

    	if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {

        	perror("Error reading variable information");

        	exit(3);

    	}

 

    	//printf("Fixed screen information: %dx%d, %dbpp\n", finfo.xres, finfo.yres, finfo.bits_per_pixel);

    	printf("Variable screen information: %dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

 

    	// Figure out the size of the screen in bytes

    	//screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

    	screensize = vinfo.yres_virtual * finfo.line_length;

    	printf("screensize: %d\n",screensize); 

 

 

    	// Map the device to memory

    	fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);

    	if ((int)fbp == -1) {

        	perror("Error: failed to map framebuffer device to memory");

        	exit(4);

    	}

	printf("The framebuffer device was mapped to memory successfully.\n");


	for(y=0;y<480;y++)
        {
                for(x=0;x<finfo.line_length;x++)
                {
                        pixel=0xff;
                        location = x+(y*finfo.line_length);
                         *((uint8_t*)(fbp + location)) = pixel;
                     //  *((uint8_t*)(fbp + location+1)) = pixel;

                }
        }
	nanosleep((struct timespec[]){{0, 50000000}}, NULL);

	/* create our bmp image */
	bmp_create(&bmp, &bitmap_callbacks);

	/* load file into memory */
	unsigned char *data = load_file(argv[1], &size);

	/* analyse the BMP */
	code = bmp_analyse(&bmp, size, data);
	if (code != BMP_OK) {
		warning("bmp_analyse", code);
		res = 1;
		goto cleanup;
	}

	/* decode the image */
	code = bmp_decode(&bmp);
	/* code = bmp_decode_trans(&bmp, TRANSPARENT_COLOR); */
	if (code != BMP_OK) {
		warning("bmp_decode", code);
		/* allow partially decoded images */
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
		image = (unsigned short int*) bmp.bitmap;
		// *(uint8_t*)fbp= (uint8_t *) bmp.bitmap;
		
		for (row = 0; row != bmp.height; row++) {
			for (col = 0; col != bmp.width; col++) {
				size_t z = (row * bmp.width + col) * BYTES_PER_PIXEL;
				//printf("%u %u %u** ",	image[z],image[z + 1]);
				//pixel=z;
				
				//pixel = * (image+ (row * bmp.width + col));
                        	location = col+(row*finfo.line_length);
                         	pixel = image[z];
				*((unsigned short int*)(fbp + location)) = pixel;
				//pixel = image[z+1];
				//*((unsigned short int*)(fbp + location)) = pixel;
				//printf("pixel no:%d,location:%d\n",pixel,location);
 				//nanosleep((struct timespec[]){{0, 50000000}}, NULL);

				// pixel = * (image+ (row * bmp.width + col+1));
                                //location = col+(row*finfo.line_length)+1; 
                                //*((uint8_t*)(fbp + location)) = pixel; 
				//printf("pixel no:%d,location:%d\n",pixel,location);


				//printf("Location: %d, pixel: %d\r",location, pixel);
			}
			//printf("\n");
		}
	}

cleanup:
	/* clean up */
	bmp_finalise(&bmp);
	free(data);
	
	munmap(fbp, screensize);

    	close(fbfd);


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

/***********************************************************************
* Function: convert24to16
*
* Purpose:  convert Windows BMP (24bpp GBR) to 16bpp 565 RGB
*
* Parameters:
*     read_buffer : read from bmp data buffer (uint8) = bmp-file + data_offset
*     write_buffer: write to picture buffer (uint16)
*     width    : picture width
*     height      : picture height
*
* Outputs: None
*
* Returns: Nothing

* Notes: this function is also changing byte order to avoid
*        wrong 'mirroring' of pictures
*
**********************************************************************/
/*static void convert24to16(uint8_t* read_buffer,uint16_t* write_buffer,uint32_t width,uint32_t height)
{
uint32_t p_x; //picture x position
uint32_t p_y; //picture y position
write_buffer += (width*height)-1-width;//start at last line
//read 24bit BGR to 16bit RGB
for(p_y=0;p_y>3;//read upper 5 bits blue
   read_buffer++; //next color byte
   *write_buffer |=((*read_buffer &0xFC)<<3);//read upper 6 bits green
   read_buffer++; //next color byte
   *write_buffer |=((*read_buffer & 0xF8)<<8);//add upper 5 bits red
   write_buffer++; //next picture int
   read_buffer++; //next color byte
  } //end pixel
  write_buffer-= 2*width; //2 lines back
} //end line
} //end function
*/
