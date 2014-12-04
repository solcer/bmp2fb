/*
 * initialise Frame Buffer in Xpeed LX3
 * 
 * This is compilation of code from enigma2: https://github.com/openatv/enigma2
 * 
 * Licensed under GPLv2
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <memory.h>
#include <linux/kd.h>
#include <linux/fb.h>

main() {
	int xres=848;
	int yres=480;
	int nbpp=8;
	struct fb_var_screeninfo screeninfo;
	int fbFd;
	unsigned char *lfb;
	
	fbFd=open("/dev/fb5", O_RDWR);
	if (fbFd<0)
	{
		perror("open");
		exit(1);
	}
	
	
	if (ioctl(fbFd, FBIOGET_VSCREENINFO, &screeninfo)<0)
	{
		perror("FBIOGET_VSCREENINFO");
		exit(1);
	}
	
	struct fb_fix_screeninfo fix;
	if (ioctl(fbFd, FBIOGET_FSCREENINFO, &fix)<0)
	{
		perror("FBIOGET_FSCREENINFO");
		exit(1);
	}
	
	printf("%dk video mem\n", fix.smem_len/1024);
	lfb=(unsigned char*)mmap(0, fix.smem_len, PROT_WRITE|PROT_READ, MAP_SHARED, fbFd, 0);
	if (!lfb)
	{
		perror("mmap");
		exit(1);
	}
	
	
	/*
	int fd=open("/dev/tty0", O_RDWR);
	int state = 0;
	if(ioctl(fd, KDSETMODE, state?KD_TEXT:KD_GRAPHICS)<0)
	{
		puts("setting /dev/tty0 status failed.");
	}
	*/
	
	screeninfo.xres_virtual=screeninfo.xres=xres;
	screeninfo.yres_virtual=(screeninfo.yres=yres)*2;
	screeninfo.height=0;
	screeninfo.width=0;
	screeninfo.xoffset=screeninfo.yoffset=0;
	screeninfo.bits_per_pixel=nbpp;
	
	/*switch (nbpp) {
	case 16:
		// ARGB 1555
		screeninfo.transp.offset = 15;
		screeninfo.transp.length = 1;
		screeninfo.red.offset = 10;
		screeninfo.red.length = 5;
		screeninfo.green.offset = 5;
		screeninfo.green.length = 5;
		screeninfo.blue.offset = 0;
		screeninfo.blue.length = 5;
		break;
	case 32:
		// ARGB 8888
		screeninfo.transp.offset = 24;
		screeninfo.transp.length = 8;
		screeninfo.red.offset = 16;
		screeninfo.red.length = 8;
		screeninfo.green.offset = 8;
		screeninfo.green.length = 8;
		screeninfo.blue.offset = 0;
		screeninfo.blue.length = 8;
		break;
	}*/
	
	if (ioctl(fbFd, FBIOPUT_VSCREENINFO, &screeninfo)<0)
	{
		perror("FBIOPUT_VSCREENINFO");
		// try single buffering
		screeninfo.yres_virtual=screeninfo.yres=yres;
		
		if (ioctl(fbFd, FBIOPUT_VSCREENINFO, &screeninfo)<0)
		{
			perror("FBIOPUT_VSCREENINFO 2");
			exit(1);
		}
		puts(" - double buffering not available.");
	} else
		puts(" - double buffering available!");
	
	
	ioctl(fbFd, FBIOGET_VSCREENINFO, &screeninfo);
	
	if ((screeninfo.xres!=xres) && (screeninfo.yres!=yres) && (screeninfo.bits_per_pixel!=nbpp))
	{
		printf("SetMode failed: wanted: %dx%dx%d, got %dx%dx%d\n",
			xres, yres, nbpp,
			screeninfo.xres, screeninfo.yres, screeninfo.bits_per_pixel);
	}
	
	printf ("Got screen %dx%d@%d\n", screeninfo.xres, screeninfo.yres, screeninfo.bits_per_pixel);
	
	if (ioctl(fbFd, FBIOGET_FSCREENINFO, &fix)<0)
	{
		perror("FBIOGET_FSCREENINFO");
		printf("fb failed\n");
	}
	memset(lfb, 0, fix.line_length * screeninfo.yres);
	
	//if (ioctl(fbFd, FBIO_BLIT) < 0)
	//      perror("FBIO_BLIT");
	
	return 0;
}
