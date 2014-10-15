    #include <stdlib.h>
    #include <unistd.h>
    #include <stdio.h>
    #include <fcntl.h>
    #include <linux/fb.h>
    #include <sys/mman.h>
    #include <sys/ioctl.h>

    int main()
    {
        int fbfd = 0;
        struct fb_var_screeninfo vinfo;
        struct fb_fix_screeninfo finfo;
        long int screensize = 0;
        char *fbp = 0;
        int x = 0, y = 0;
        long int location = 0;
        int xsize, ysize;

        unsigned short b,g,r;
        unsigned short int t;
        unsigned short int reps;

        // Open the file for reading and writing
        fbfd = open("/dev/fb0", O_RDWR);
        if (fbfd == -1) {
            perror("Error: cannot open framebuffer device");
            exit(1);
        }

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

        xsize = vinfo.xres;
        ysize = vinfo.yres;

        // Figure out the size of the screen in bytes
        screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

        // Map the device to memory
        fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
        if ((int)fbp == -1) {
            perror("Error: failed to map framebuffer device to memory");
            exit(4);
        }
        printf("The framebuffer device was mapped to memory successfully.\n");

        // Figure out where in memory to put the pixel
        for (reps = 0; reps < 1000; reps++) {
         for (y = 0; y <  ysize; y++) {
            for (x = 0; x < xsize; x++) {

                location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) +
                           (y+vinfo.yoffset) * finfo.line_length;

                b = reps;
                g = (reps+x-100)/6;     // A little green
                r = 31-((y+b/2)-100)/16;    // A lot of red
                t = r<<11 | g << 5 | b;
                *((unsigned short int*)(fbp + location)) = t;

            } // end for x..
          } // end for y..
        } // end for reps...
        munmap(fbp, screensize);
        close(fbfd);
        printf("Screensize: %dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

        return 0;
    }
