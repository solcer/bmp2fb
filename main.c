/*
 * Copyright 2008 Sean Fox <dyntryx@gmail.com>
 * Copyright 2008 James Bursa <james@netsurf-browser.org>
 *
 * This file is part of NetSurf's libnsbmp, http://www.netsurf-browser.org/
 * Licenced under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 */

//selim �l�er 14/10/2014 bmp to framebuffer
//27.11.2014 staticGoruntu.c
//23.12.2014 main.c

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "libnsbmp.c"
#include "commands.h"


#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <linux/kd.h>
#include "rs232.c" 
#include <unistd.h>
#include <termios.h>

#define BYTES_PER_PIXEL 4
#define TRANSPARENT_COLOR 0xffffffff


unsigned char *load_file(const char *path, size_t *data_size);
void warning(const char *context, bmp_result code);
void *bitmap_create(int width, int height, unsigned int state);
unsigned char *bitmap_get_buffer(void *bitmap);
size_t bitmap_get_bpp(void *bitmap);
void bitmap_destroy(void *bitmap);
void showBitmap(uint8_t *resim, char *fbPointer);
uint8_t resimleriYenile(unsigned int sahneNo);			//0. sahne no calibrasyon resmi
void cleanUp(void);
void frameBufferTemizle(void);
void frameBufferYenile(unsigned char fbNo);
void ofsetleriAl(void);
void ofsetleriYaz(void);
void ttyleriHazirla(void);
void slitYenile(unsigned int slitNo, unsigned char *dt);
void fillSlit(unsigned char slit,unsigned int topOffset, unsigned int bottomOffset);
void slitDoldur(uint8_t projektorNo,uint16_t slit,uint8_t clear,uint8_t resim);
#define SOLGOZRESMI	0
#define SAGGOZRESMI 1
int getch(void);
uint16_t ttyOku(void);
void dataYaz(int * ,unsigned char *);
void dataOku(int * ,unsigned char *);
void framebufferAyarla(unsigned char );
uint8_t loadImage (char bufNo,char *imageName);
void tumFrameBufferlariYenile(char pNo);
void tekFrameBufferiYenile(char pNo);
void sendOfsetDataToPc(void);
#define EKRANADEDI 17
unsigned char SLITSIZE =	13;
#define PIXELADIMI	8
#define IKIGOZMESAFESI	25
  signed int offsetler[EKRANADEDI];//={0,18,50,48,36,32,45,44,55,54,44,20,59,47,51,30,53};			//3. ayar
//pixelData.dat :48,36,37,33,29,30,22,16,22,25,1,3,13,16,6,0,7,
//300,290,286,287,283,286,272,271,278,281,255,259,273,274,255,254,256,
//46,36,32,33,29,32,18,17,24,27,1,5,19,20,1,0,2,

bmp_bitmap_callback_vt bitmap_callbacks = {
	bitmap_create,
	bitmap_destroy,
	bitmap_get_buffer,
	bitmap_get_bpp
};
bmp_result code;
bmp_image bmp[EKRANADEDI];



/* framebuffer */

int calismaModu;
int uzaktty;
int fbfd[EKRANADEDI] , tty[EKRANADEDI] ;

struct fb_var_screeninfo vinfo[EKRANADEDI];

struct fb_fix_screeninfo finfo[EKRANADEDI];

struct fb_con2fbmap c2m[EKRANADEDI];			//tty leri framebuffer a atamak i�in kullan�l�yor

long int screensize[EKRANADEDI] ;

char *fbp[EKRANADEDI] ;

uint8_t *image;

unsigned char *data[EKRANADEDI];				//resim datas�n� i�eren de�i�ken
int projektorSlitNolari[EKRANADEDI];  //=296,284,285,281,277,278,270,264,270,273,249,251,261,264,254,248,255,



void handler (int sig){
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
	size_t size;
	
	unsigned short res = 0;
	
	int i = 0;
	

	uint8_t pixel = 0,dondurt=0;

	bool stripe = true;

	long int location = 0;
	
	uint8_t buffer[100],outputBuffer[100];
	
	int cport_nr=22;        /* /dev/ttyAMA0 */
	int bdrate=115200;       /* 9600 baud */

	//unsigned char uartBuf[4096];
	char mode[]={'8','N','1',0};
	uint16_t headPosX;
	uint16_t headPosY;
	uint16_t headPosZ;
	
	uint8_t pNo;
	uint16_t sltNo;
	uint16_t oncekiSltNo[EKRANADEDI];
	
	ofsetleriAl();						//ofsetler.dat dosyasindan ofsetleri al�r
	uzaktty =open("/dev/tty",O_RDWR);//O_RDONLY);
	if(uzaktty==-1)
	{
		printf("Can not open tty0\n");
	}
	//printf("Hazir\n");
	//write(uzaktty,buffer,sizeof(buffer));
	if(RS232_OpenComport(cport_nr, bdrate, mode))
	{
		printf("Can not open comport\n");
		return(0);
	}
	for(i=0;i<EKRANADEDI;i++)			//pointerlar�n ba�lang�� degeri
	{
	  fbp[i]=0;
	  fbfd[i]=0;
	  tty[i]=0;
	  screensize[i]=0;
	  oncekiSltNo[i]=0;
	}
	
	if ((getuid ()) != 0) {

		printf ("You are not root, run this as root or sudo\n");
		exit(1);

    }

	ttyleriHazirla();
	frameBufferTemizle();
	//resimleriYenile(0);
	loadImage(0,"/home/pi/selim/bmp2fb/AllContent/bmps/image0.bmp");
	loadImage(1,"/home/pi/selim/bmp2fb/AllContent/bmps/image1.bmp");
	loadImage(2,"/home/pi/selim/bmp2fb/calibrationPictureRed.bmp");
	loadImage(3,"/home/pi/selim/bmp2fb/calibrationPictureGreen.bmp");
	
	
 	dataOku(&projektorSlitNolari[0],"./pixelData.dat");	
//	for(i=0;i<EKRANADEDI;i++){
//		slitDoldur(i,projektorSlitNolari[i],1,SOLGOZRESMI);
//		slitDoldur(i,projektorSlitNolari[i]+IKIGOZMESAFESI,1,SAGGOZRESMI);
//	}
	RS232_flush(cport_nr);			//portta bekleyen datalar� temizle
	while(1){
		RS232_PollComport(cport_nr,&buffer[0],15);
		if(buffer[0] =='U' && buffer[1]=='U')
		{
			// printf("buffer[]= ");
			// for(i=0;i<15;i++){
				// printf("%d,",buffer[i]);
			// }
			// printf("\n");
			
			switch(buffer[2]){
				case COMMANDPOSITION:							//pc'den kafa posizyonlar�n� al�r
					//printf("COMMANDPOSITION\n");
					headPosX=(buffer[4]<<8)&0xff00 | buffer[3];
					if(headPosX>390) headPosX=390;
					//if(headPosX<90) headPosX=90;
					
					headPosY=(buffer[7]<<8)&0xff00 | buffer[8];
					headPosZ=(buffer[12]<<8)&0xff00 | buffer[11];
					printf("Head Pos: %d   onceki Slit Pos:%d , yeni sltNo:%d   \r",headPosX,oncekiSltNo[0], projektorSlitNolari[0]+headPosX);
					//frameBufferTemizle();
					for(i=0;i<EKRANADEDI;i++){
						slitDoldur(i,oncekiSltNo[i],0,SOLGOZRESMI);	//1. resimi sil
						slitDoldur(i,oncekiSltNo[i]+IKIGOZMESAFESI,0,SAGGOZRESMI);	//2. resimi sil
						oncekiSltNo[i]=projektorSlitNolari[i]+headPosX;
						slitDoldur(i,oncekiSltNo[i],1,SOLGOZRESMI);
						slitDoldur(i,oncekiSltNo[i]+IKIGOZMESAFESI,1,SAGGOZRESMI);		//2.slitler
						
						//slitDoldur(i,(projektorSlitNolari[i]-248)+headPosX+PIXELADIMI,0,SOLGOZRESMI);
						//slitDoldur(i,(projektorSlitNolari[i]-248)+headPosX+PIXELADIMI+IKIGOZMESAFESI,0,SAGGOZRESMI);	//2.slit
						//slitDoldur(i,(projektorSlitNolari[i]-248)+headPosX,1,SOLGOZRESMI);
						//slitDoldur(i,(projektorSlitNolari[i]-248)+headPosX+IKIGOZMESAFESI,1,SAGGOZRESMI);		//2.slitler
						
					}
					break;
				case COMMANDVERTICALCALIBRATIONSETPROJECTOR:			//projekt�rlerin yeni ofset degerini al
					switch(buffer[4]){			//yukar� veya asagi komutu geldi mi kontrol et
						case 1:
							//yukar� kaydir komutu geldiyse 
							//gelen ofset degeri kadar offseti kaydir
							printf("yukari:%d");
							offsetler[buffer[3]]+=buffer[5];
							break;
						case 2:
							//asagi komutu
							printf("asagi");
							offsetler[buffer[3]]-=buffer[5];
							break;
						default:
							break;
					}
					// outputBuffer[0]='U';
					// outputBuffer[1]='U';
					// outputBuffer[2]=COMMANDVERTICALCALIBRATIONSETPROJECTOR;
					sendOfsetDataToPc();
					//RS232_SendBuf(cport_nr,(char *) &offsetler[0],EKRANADEDI*2);		//offsetler degiskeninini char olarak pc'ye gonder.
					tekFrameBufferiYenile(buffer[3]);					// secilen projektorun rengini degistir
					dataYaz(&offsetler[0],"./ofsetler.dat");			
					break;
				case COMMANDVERTICALCALIBRATION:
					sendOfsetDataToPc();
					tumFrameBufferlariYenile(buffer[3]);
					break;
				case COMMANDSLITSIZE:
					//printf("COMMANDSLITSIZE\n");
					SLITSIZE=buffer[3];
					break;
				case COMMANDSLITDISTANCE:
					printf("COMMANDSLITDISTANCE\n");
					break;
				case COMMANDCLEARSCREEN:
					frameBufferTemizle();
					break;
			}
			RS232_flush(cport_nr);			//portta bekleyen datalar� temizle
			for(i=0;i<15;i++)
				buffer[i]=0;				//buffer i temizle
		}
	/*	switch(calismaModu){
			case 0:						//baslangic menusu
				printf("\n0: slitleri ayarlar");
				printf("\n1: ekran kaydirma ");
				printf("\nx: cikis \n");
				i = getch();
				switch(i){
					case '0':		
						calismaModu=10;
						//frameBufferTemizle();
						break;
					case '1':
						printf("\n<-- & -->\n");
						calismaModu=1;
						break;
					case 'x':				//'x' harfi
						goto cikis;
						break;
				}
			 	break;
			case 1:
				i = getch();
				if(i=='x'){
					calismaModu=0;
				}
				if(i==27){			//oklari bul
					i = getch();
					if(i==91){
						i = getch();
						switch(i){
							case 67:
								printf("\r-->           ");
								sltNo-=PIXELADIMI;
								for(i=0;i<EKRANADEDI;i++){
									slitDoldur(i,projektorSlitNolari[i]+sltNo+PIXELADIMI,0,SOLGOZRESMI);
									slitDoldur(i,projektorSlitNolari[i]+sltNo+PIXELADIMI+IKIGOZMESAFESI,0,SAGGOZRESMI);	//2.slit
									slitDoldur(i,projektorSlitNolari[i]+sltNo,1,SOLGOZRESMI);
									slitDoldur(i,projektorSlitNolari[i]+sltNo+IKIGOZMESAFESI,1,SAGGOZRESMI);		//2.slitler
								}
								break;
							case 68:								
								printf("\r<--           ");
								sltNo+=PIXELADIMI;
								for(i=0;i<EKRANADEDI;i++){
									slitDoldur(i,projektorSlitNolari[i]+sltNo-PIXELADIMI,0,SOLGOZRESMI);
									slitDoldur(i,projektorSlitNolari[i]+sltNo-PIXELADIMI+IKIGOZMESAFESI,0,SAGGOZRESMI);		//2.slitler
									slitDoldur(i,projektorSlitNolari[i]+sltNo,1,SOLGOZRESMI);
									slitDoldur(i,projektorSlitNolari[i]+sltNo+IKIGOZMESAFESI,1,SAGGOZRESMI);		//2.slitler
								}
								break;
						}
					}
				}
				break;
			case 10:
				printf("\n******p:projektor no gir: ");
				pNo=ttyOku();
				//printf("%d. projektor\n",pNo);
				printf("\n");
				calismaModu=11;	
				break;
			case 11:
				printf("\r<- sola kaydir, -> saga kaydir. 's' slit no gir. projektor secmek icin 'p'. x cikis. ");
				i=getch();
				if(i=='x'){
					printf("Cikis...\n");
					cleanUp();
				}
				if(i=='p'){			//'p'
					//printf("p geldi mode 2\n");
					dataYaz(&projektorSlitNolari[0],"./pixelData.dat");	
					for(i=0;i<EKRANADEDI;i++){
						printf("%d,",projektorSlitNolari[i]);
					}
					calismaModu=10;
					break;
				}
				if(i=='s'){			//'s'
					calismaModu=12;
					break;
				}
				if(i==27){			//oklari bul
					i = getch();
					if(i==91){
						i = getch();
						switch(i){
							case 67:
								printf("--> ");
								sltNo--;
								printf("slit:%d",sltNo);
								slitDoldur(pNo,sltNo+1,0,SOLGOZRESMI);
								slitDoldur(pNo,sltNo,1,SOLGOZRESMI);
								projektorSlitNolari[pNo]=sltNo;
								break;
							case 68:								
								printf("<-- ");
								sltNo++;
								printf("slit:%d",sltNo);
								slitDoldur(pNo,sltNo-1,0,SOLGOZRESMI);
								//printf("yaz");
								slitDoldur(pNo,sltNo,1,SOLGOZRESMI);
								projektorSlitNolari[pNo]=sltNo;
								break;
						}
					}
				}
				break;
			case 12:
				printf("Slit baslangic no gir: ");
				slitDoldur(pNo,sltNo,0,SOLGOZRESMI);
				sltNo=ttyOku();
				slitDoldur(pNo,sltNo,1,SOLGOZRESMI);
				projektorSlitNolari[pNo]=sltNo;
				//printf("slit baslangic no:%d \n",i);
				calismaModu=11;
				break;
		}*/
	}
	
cikis:
	cleanUp();
	return 0;
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

//tek bir resimi buffer'a yukler
uint8_t loadImage (char bufNo,char *imageName)
{
size_t size;
unsigned short res = 0;
	printf("bmp[%d] yenileniyor...: %s ",bufNo,imageName);
		// create our bmp image 
	bmp_create(&bmp[bufNo], &bitmap_callbacks);
	// load file into memory 

	//sprintf(&buffer[0],"/home/pi/selim/bmp2fb/AllContent/bmps/img%d.bmp",i+1);
	//sprintf(&buffer[0],"/home/pi/selim/bmp2fb/AllContent/bmps/image%d.bmp",i);
	//sprintf(&buffer[0],"/home/pi/selim/bmp2fb/calibrationPicture.bmp");
	
	data[bufNo] = load_file(imageName, &size);
	// analyse the BMP 
	code = bmp_analyse(&bmp[bufNo], size, data[bufNo]);
	if (code != BMP_OK) {
		warning("bmp_analyse", code);
		res = 1;
		//goto cleanup;
		cleanUp();
		return 0;
	}
	// decode the image
	code = bmp_decode(&bmp[bufNo]);
	// code = bmp_decode_trans(&bmp, TRANSPARENT_COLOR); 
	if (code != BMP_OK) {
		warning("bmp_decode", code);
		// allow partially decoded images 
		if (code != BMP_INSUFFICIENT_DATA) {
			res = 1;
			cleanUp();
			return 0;
		}	
	}
	printf("OK.\n");
}
uint8_t resimleriYenile(unsigned int sahneNo){
int i;
uint8_t buffer[128];
size_t size;
unsigned short res = 0;
	printf("resimler yenileniyor...");
	for(i=0;i<2;i++)					//resimleri import eden k�s�m.
	{
		// create our bmp image 
		bmp_create(&bmp[i], &bitmap_callbacks);
		// load file into memory 
	
		//sprintf(&buffer[0],"/home/pi/selim/bmp2fb/AllContent/bmps/img%d.bmp",i+1);
		sprintf(&buffer[0],"/home/pi/selim/bmp2fb/AllContent/bmps/image%d.bmp",i);
		//sprintf(&buffer[0],"/home/pi/selim/bmp2fb/calibrationPicture.bmp");
		
		data[i] = load_file(buffer, &size);
		// analyse the BMP 
		code = bmp_analyse(&bmp[i], size, data[i]);
		if (code != BMP_OK) {
			warning("bmp_analyse", code);
			res = 1;
			//goto cleanup;
			cleanUp();
			return 0;
		}
		// decode the image
		code = bmp_decode(&bmp[i]);
		// code = bmp_decode_trans(&bmp, TRANSPARENT_COLOR); 
		if (code != BMP_OK) {
			warning("bmp_decode", code);
			// allow partially decoded images 
			if (code != BMP_INSUFFICIENT_DATA) {
				res = 1;
				cleanUp();
				return 0;
			}	
		}
	}
	printf("OK.\n");
}
/*uint8_t resimleriYenile(unsigned int sahneNo){
int i;
uint8_t buffer[128];
size_t size;
unsigned short res = 0;
	printf("resimler yenileniyor...");
	for(i=0;i<EKRANADEDI;i++)					//resimleri import eden k�s�m.
	{
		// create our bmp image 
		bmp_create(&bmp[i], &bitmap_callbacks);
		// load file into memory 
		if(sahneNo==0)
		{
			sprintf(&buffer[0],"/home/pi/selim/bmp2fb/AllContent/bmps/img1.bmp");
			//sprintf(&buffer[0],"/home/pi/selim/bmp2fb/AllContent/bmps/image0.bmp");
			//sprintf(&buffer[0],"/home/pi/selim/bmp2fb/calibrationPicture.bmp");
		}else{
			sprintf(&buffer[0],"/home/pi/selim/bmp2fb/AllContent/v%d/samplescreen%d.bmp",sahneNo-1,i);
		}
		data[i] = load_file(buffer, &size);
		// analyse the BMP 
		code = bmp_analyse(&bmp[i], size, data[i]);
		if (code != BMP_OK) {
			warning("bmp_analyse", code);
			res = 1;
			//goto cleanup;
			cleanUp();
			return 0;
		}
		// decode the image
		code = bmp_decode(&bmp[i]);
		// code = bmp_decode_trans(&bmp, TRANSPARENT_COLOR); 
		if (code != BMP_OK) {
			warning("bmp_decode", code);
			// allow partially decoded images 
			if (code != BMP_INSUFFICIENT_DATA) {
				res = 1;
				cleanUp();
				return 0;
			}	
		}
	}
	printf("OK.\n");
}*/
void cleanUp(void){
	int i;
	// clean up 
	bmp_finalise(&bmp[i]);
	for(i=0;i<EKRANADEDI;i++)
	{
		free(data[i]);
	}
	 for(i=0;i<EKRANADEDI;i++)
	{
		munmap(fbp[i], screensize[i]); 
		close(fbfd[i]);
	}
}


void frameBufferYenile(unsigned char fbNo)
{
int i,row,col;
long int location = 0;
i=fbNo;
	if( finfo[i].line_length==1600)			//frame buffer'�n biri farkl� onu d�zg�n g�stermek i�in buray� yaz�yorum
	{
		//printf("1600 de\n");
		for (row = 0; row != 600; row++) {		//480
			for (col = 0; col != 800; col++) {		//848
				image = (uint8_t *) bmp[i].bitmap;
				size_t z = (row * 800 + col) * BYTES_PER_PIXEL;		//bmp i�erisinde bpp ne olursa olsun her bir pixel bilgisi 4 byte uzunlugundadir. burada pixel ba�lang�c� hesaplan�yor.
				location = (col+offsetler[i])*2+(row*finfo[i].line_length);					//her bir pixel 2 byte oldu�u i�in col*2 yapt�m.
				*((uint16_t*)(fbp[i] + location)) = ((uint16_t)(image[z] << 8) &  0xf800) | ((uint16_t)(image[z+1] << 3) & 0x7E0) |(uint16_t)((image[z+2]>>3) & 0x1f);
			}
		}
	}else{
		for (row = 0; row != bmp[0].height; row++) {		//480
			for (col = 0; col != bmp[0].width; col++) {		//848
				image = (uint8_t *) bmp[i].bitmap;
				size_t z = (row * bmp[i].width + col) * BYTES_PER_PIXEL;		//bmp i�erisinde bpp ne olursa olsun her bir pixel bilgisi 4 byte uzunlugundadir. burada pixel ba�lang�c� hesaplan�yor.
				location = (col+offsetler[i])*2+(row*finfo[i].line_length);					//her bir pixel 2 byte oldu�u i�in col*2 yapt�m.
				*((uint16_t*)(fbp[i] + location)) = ((uint16_t)(image[z] << 8) &  0xf800) | ((uint16_t)(image[z+1] << 3) & 0x7E0) |(uint16_t)((image[z+2]>>3) & 0x1f);
			}
		}
	}
}


/*
void frameBufferTemizle(void){
int i,y,x;	
long int location = 0;
	for(y=0;y<480;y++)				////ekrana ba�lang�� rengi veren k�s�m
	{
			for(x=0;x<finfo[0].line_length;x++)
			{
					//   pixel=0xf8;
					for(i=0;i<EKRANADEDI;i++)
					{
							location = x+(y*finfo[i].line_length);
							*((uint16_t*)(fbp[i] + location))=0;//xaaaa;
					}
			}
	}
}*/
void frameBufferTemizle(void){
int i,row,col;
long int location = 0;
	for(i=0;i<EKRANADEDI;i++){ 				//resimleri ekranlara basan kisim
		for (row = 0; row != 480; row++) {		//480
			for (col = 0; col != 848; col++) {		//848
				location = (col+offsetler[i])*2+(row*1696);					//her bir pixel 2 byte oldu�u i�in col*2 yapt�m.
				*((uint16_t*)(fbp[i] + location)) = 0;//((uint16_t)(image[z] << 8) &  0xf800) | ((uint16_t)(image[z+1] << 3) & 0x7E0) |(uint16_t)((image[z+2]>>3) & 0x1f);
				
			}
		}
	}
}

void ofsetleriAl(void)
{
FILE *ofsetDosyasi;
char * pch,i;
	ofsetDosyasi  = fopen("./ofsetler.dat", "r+"); // 
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


void ofsetleriYaz(void)
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
        fprintf(ofsetDosyasi, "%d,", &offsetler[i] );

    }
	fclose(ofsetDosyasi);
}


void dataOku(int *dtPointer, unsigned char *dosyaAdi)
{
FILE *dosya;
char i;
	dosya = fopen(dosyaAdi, "r+"); 
	if (dosya == NULL ) 
	{   
	  printf("Error! Could not open file\n"); 
	  exit(-1); // must include stdlib.h 
	} 
	for (i = 0; i < EKRANADEDI; i++){
        fscanf(dosya, "%d,", (dtPointer+i));
    }
	fclose(dosya);
}

void dataYaz( int *dtPointer,unsigned char *dosyaAdi)
{
FILE *dosya;
char i;
	dosya  = fopen(dosyaAdi, "w"); // 
	if (dosya == NULL ) 
	{   
	  printf("Error! Could not open file\n"); 
	  exit(-1); // must include stdlib.h 
	} 
	for (i = 0; i < 17; i++)
    {
        fprintf(dosya, "%d,", *(dtPointer+i) );

    }
	fclose(dosya);
}

void ttyleriHazirla(void)
{
int i;
uint8_t buffer[100];
unsigned short res = 0;
	for(i=0;i<EKRANADEDI;i++)				//ekranlar� tty lere atay�p ram'de pointer a i�aretliyor
	{
		//printf("i:%d\n",i);		

		c2m[i].console = (uint32_t) 10+i;			//framebuffer a atanacak consol numaras�
		c2m[i].framebuffer = (uint32_t) i;		//framebuffer numaras�

		sprintf(&buffer[0],"/dev/fb%d",i); 
		fbfd[i] = open(buffer, O_RDWR);	//framebuffer � a��yoru		
		if (fbfd[i] == -1) 			//hata geldi mi?
		{
			printf("acilamayan device no:%d",i);
			perror("Error: cannot open framebuffer device :" );
			exit(1);
		}
		//printf("The framebuffer %d  was opened successfully.\n",i);

		if (ioctl(fbfd[i], FBIOPUT_CON2FBMAP, &c2m[i])) 			//framebuffer� tty lere assign et
		{
			//fprintf(stderr, "%s: Cannot set console mapping\n", progname);
			perror("Error: cannot assign framebuffer to tty device : " );
			exit(1);
		}
		//printf("The framebuffer %d  was assigned successfully to tty%d.\n",i,i+10);


		sprintf(&buffer[0],"/dev/tty%d",i+10);
		tty[i] = open(buffer, O_RDWR);
		if(ioctl(tty[i], KDSETMODE, KD_GRAPHICS) == -1)
			printf("FAILED to set graphics mode on tty%d\n",i+10);



		if (ioctl(fbfd[i], FBIOGET_FSCREENINFO, &finfo[i]) == -1) {

			perror("Error reading fixed information :");

			exit(2);

		}
tekrarOlc:			//ekran ��z�n�rl��� tutmuyorsa ayarlay�p tekrar buraya gelecek
		// Get variable screen information
		if (ioctl(fbfd[i], FBIOGET_VSCREENINFO, &vinfo[i]) == -1) {

			perror("Error reading variable information :" );

			exit(3);
		}
		
		printf("%d. Variable screen fb%d: %dx%d, %dbpp - Fixed screen finfo.smem_len:0x%X, finfo.line_length:0x%X\n",i,i, vinfo[i].xres, vinfo[i].yres, vinfo[i].bits_per_pixel, finfo[i].smem_len,finfo[i].line_length);
		if(vinfo[i].xres!=848)		//eger resolution tutmuyorsa ayarla
		{
			printf("fb%d resolution yanlis. Duzeltiliyor...\n",i);
			sprintf(&buffer[0],"sudo fbset -fb /dev/fb%d 848x480-60",i);
			res = system(buffer);
			framebufferAyarla(i);
				
			goto tekrarOlc;
		}
			// Figure out the size of the screen in bytes

		screensize[i] = vinfo[i].yres_virtual * finfo[i].line_length;

		//printf("screensize[%d]: %d\n",i,screensize[i]); 

		// Map the device to memory

		fbp[i] = (char *)mmap(0, screensize[i], PROT_READ | PROT_WRITE, MAP_SHARED, fbfd[i], 0);

		if ((int)fbp[i] == -1) {

		perror("Error: failed to map framebuffer device to memory");

		exit(4);

		}
		//printf("The framebuffer[%d] was mapped to memory successfully.\n",i);


	}
}

void slitYenile(unsigned int slitNo, unsigned char *dt){
int i,row,col;
long int location = 0;
//int slitSize=13;
	for (row = 0; row != bmp[0].height; row++) {		//480
		for (col = 0; col != bmp[0].width; col++) {		//848
			location = (col+offsetler[i])*2+(row*finfo[i].line_length);					//her bir pixel 2 byte oldu�u i�in col*2 yapt�m.
			*((uint16_t*)(fbp[i] + location)) = 0;//((uint16_t)(image[z] << 8) &  0xf800) | ((uint16_t)(image[z+1] << 3) & 0x7E0) |(uint16_t)((image[z+2]>>3) & 0x1f);
		}
	}
}

void fillSlit(unsigned char slit,unsigned int topOffset, unsigned int bottomOffset)
{
unsigned int row,col;
unsigned char i;
long int location = 0;
static unsigned char oncekiSlit=0;
//	printf("slit:%d\n",slit);
	/*ilk �nce bir �nce hazirlanan slitlerin icerigini temizliyorum*/
	/*****************************************************************/
	/*for (row = oncekiSlit*SLITSIZE; row < oncekiSlit*SLITSIZE + SLITSIZE; row++) {
		for (col = 0; col != 848; col++) {
			for(i=0;i<EKRANADEDI;i++){		
				location = col*2+(row*finfo[i].line_length);			//her bir pixel 2 byte oldu�u i�in col*2 yapt�m.
				*((uint16_t*)(fbp[i] + location)) = 0;
			}
		}
	}*/
	/*2. sliti siliyor*/
	/*for (row = (oncekiSlit+2)*SLITSIZE; row < (oncekiSlit+2)*SLITSIZE + SLITSIZE; row++) {
		//for (col = 0; col != bmp[0].width; col++) {
		for (col = 0; col != 848; col++) {
			for(i=0;i<EKRANADEDI;i++){		
				location = col*2+(row*finfo[i].line_length);			//her bir pixel 2 byte oldu�u i�in col*2 yapt�m.
				*((uint16_t*)(fbp[i] + location)) = 0;
			}
		}
	}
	oncekiSlit=slit;				//Bir sonraki adimda onceki slit icerigini silme icin guncellenen slit numarasini oncekiSlit'e yaziyorum
	/*****************************************************************/

	/*sonra istenilen sliti guncelliyorum*/
	/*****************************************************************/
	
	for (row = slit*SLITSIZE; row < slit*SLITSIZE + SLITSIZE; row++) {
		for(i=0;i<16;i++){
			//buraya her bir projektor icin onceden belirlenmis offset degerini bir dosyadan okuyarak offset olarak yazacagim.
			for (col = 0; col != 848; col++) {
				image = (uint8_t *) bmp[i].bitmap;
				size_t z = (row * bmp[i].width + col) * BYTES_PER_PIXEL;		//bmp i�erisinde bpp ne olursa olsun her bir pixel bilgisi 4 byte uzunlugundadir. burada pixel ba�lang�c� hesaplan�yor.
				location = (col+offsetler[i])*2+(row*finfo[i].line_length);			//her bir pixel 2 byte oldu�u i�in col*2 yapt�m.
				*((uint16_t*)(fbp[i] + location)) = ((uint16_t)(image[z] << 8) &  0xf800) | ((uint16_t)(image[z+1] << 3) & 0x7E0) |(uint16_t)((image[z+2]>>3) & 0x1f);
			}
		}
	}
	/*for (row = (slit+2)*SLITSIZE; row < (slit+2)*SLITSIZE + SLITSIZE; row++) {
		for(i=0;i<10;i++){
			//buraya her bir projektor icin onceden belirlenmis offset degerini bir dosyadan okuyarak offset olarak yazacagim.
			for (col = 0; col != 848-offsetler[i]; col++) {
				image = (uint8_t *) bmp[i].bitmap;
				size_t z = (row * bmp[i].width + col) * BYTES_PER_PIXEL;		//bmp i�erisinde bpp ne olursa olsun her bir pixel bilgisi 4 byte uzunlugundadir. burada pixel ba�lang�c� hesaplan�yor.
				location = col*2+(row*finfo[i].line_length);			//her bir pixel 2 byte oldu�u i�in col*2 yapt�m.
				*((uint16_t*)(fbp[i] + location)) = ((uint16_t)(image[z] << 8) &  0xf800) | ((uint16_t)(image[z+1] << 3) & 0x7E0) |(uint16_t)((image[z+2]>>3) & 0x1f);
			}
		}
	}
	/*****************************************************************/
}


void slitDoldur(uint8_t projektorNo,uint16_t slit,uint8_t clear,uint8_t resim)
{
unsigned int row,col;
unsigned char i=projektorNo;
long int location = 0,slitBaslangici;
static unsigned char oncekiSlit=0;
//size_t z = (row * bmp[i].width + col) * BYTES_PER_PIXEL;
//size_t z=projektorNo*SLITSIZE*848;			//her projektor icin sabit bir resim icerigi gelecegi icin z her projektor icin sabit olacak
image = (uint8_t *) bmp[resim].bitmap;	
slitBaslangici=(projektorNo+10)*SLITSIZE*848 ;
	if(clear==1){
		for (row = slit; row < slit + SLITSIZE; row++) {
			//buraya her bir projektor icin onceden belirlenmis offset degerini bir dosyadan okuyarak offset olarak yazacagim.
			for (col = 0; col != 700; col++) {		//buradaki deger 848'e kadard�. �stteki bosluklar� almak icin 700'e kadar yazdiriyorum.					
				size_t z = (slitBaslangici +  ((row-slit) * bmp[resim].width)+  col) * BYTES_PER_PIXEL;
				//location = (col+offsetler[i])*2+(row*finfo[i].line_length);			//her bir pixel 2 byte oldu�u i�in col*2 yapt�m.
				location = (col+offsetler[i])*2+(row*1696);								//fb5'in line length degisik oldu�u icin 1696 sabit girdim 
				*((uint16_t*)(fbp[i] + location)) = ((uint16_t)(image[z] << 8) &  0xf800) | ((uint16_t)(image[z+1] << 3) & 0x7E0) |(uint16_t)((image[z+2]>>3) & 0x1f);
			}
		}
	}else{
		for (row = slit; row < slit + SLITSIZE; row++) {
			//buraya her bir projektor icin onceden belirlenmis offset degerini bir dosyadan okuyarak offset olarak yazacagim.
			for (col = 0; col != 700; col++) {
				location = (col+offsetler[i])*2+(row*1696);			//her bir pixel 2 byte oldu�u i�in col*2 yapt�m.
				*((uint16_t*)(fbp[i] + location)) = 0;//((uint16_t)(image[z] << 8) &  0xf800) | ((uint16_t)(image[z+1] << 3) & 0x7E0) |(uint16_t)((image[z+2]>>3) & 0x1f);
			}
		}	
	}
}

int getch(void){
int ch;
struct termios oldt;
struct termios newt;
	tcgetattr(STDIN_FILENO, &oldt); /*store old settings */
	newt = oldt; /* copy old settings to new settings */
	newt.c_lflag &= ~(ICANON | ECHO); /* make one change to old settings in new settings */
	tcsetattr(STDIN_FILENO, TCSANOW, &newt); /*apply the new settings immediatly */
	ch = getchar(); /* standard getchar call */
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt); /*reapply the old settings */
	return ch; /*return received char */
}

uint16_t ttyOku(void)
{
FILE *ofsetDosyasi;
char * pch,i,buffer[50];
uint16_t dt;
	ofsetDosyasi  = fopen("/dev/tty", "r+"); // 
	if (ofsetDosyasi == NULL ) 
	{   
	  printf("Error! Could not open file\n"); 
	  exit(-1); // must include stdlib.h 
	} 
	fscanf(ofsetDosyasi, "%d", &dt );
	fclose(ofsetDosyasi);
	return dt;
}


//bu fonksiyonu 800x600 ��z�n�rl�k hatas�n� (�zellikle fb5'in)d�zeltmek i�in yapt�m.
void framebufferAyarla(unsigned char fbNo)
{
	int xres=848;
	int yres=480;
	int nbpp=8;
	struct fb_var_screeninfo screeninfo;
	int fbFd;
	unsigned char *lfb;
	unsigned char bff[50];
			
	sprintf(&bff[0],"/dev/fb%d",fbNo);
	fbFd=open(bff, O_RDWR);
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
	screeninfo.xres_virtual=screeninfo.xres=xres;
	screeninfo.yres_virtual=(screeninfo.yres=yres)*2;
	screeninfo.height=0;
	screeninfo.width=0;
	screeninfo.xoffset=screeninfo.yoffset=0;
	screeninfo.bits_per_pixel=nbpp;
	
	
	
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
	
}

void tumFrameBufferlariYenile(char pNo){
int i,row,col;
long int location = 0;
	printf("Kalibrasyon icin ekranlar yenileniyor...\n");
	for(i=0;i<EKRANADEDI;i++){ 				//resimleri ekranlara basan kisim
		for (row = 0; row != 480; row++) {		//480
			for (col = 0; col != 848; col++) {		//848
				image = (uint8_t *) bmp[2].bitmap;
				size_t z = (row * bmp[2].width + col) * BYTES_PER_PIXEL;		//bmp i�erisinde bpp ne olursa olsun her bir pixel bilgisi 4 byte uzunlugundadir. burada pixel ba�lang�c� hesaplan�yor.
				location = (col+offsetler[i])*2+(row*1696);					//her bir pixel 2 byte oldu�u i�in col*2 yapt�m.
				*((uint16_t*)(fbp[i] + location)) = ((uint16_t)(image[z] << 8) &  0xf800) | ((uint16_t)(image[z+1] << 3) & 0x7E0) |(uint16_t)((image[z+2]>>3) & 0x1f);
			}
		}	
	}
	//kalibre edilecek projektor icerigi yuk
	tekFrameBufferiYenile(pNo);	
	printf("FRAMEBUFFERLAR OK...\n");
}

void tekFrameBufferiYenile(char pNo)
{
int i,row,col;
long int location = 0;

static oncekiPNo;
	//bir �nceki secimi siliyoruz
	i=oncekiPNo;
	for (row = 0; row != 480; row++) {		//480
		for (col = 0; col != 848; col++) {		//848
			image = (uint8_t *) bmp[2].bitmap;
			size_t z = (row * bmp[2].width + col) * BYTES_PER_PIXEL;		//bmp i�erisinde bpp ne olursa olsun her bir pixel bilgisi 4 byte uzunlugundadir. burada pixel ba�lang�c� hesaplan�yor.
			location = (col+offsetler[i])*2+(row*1696);					//her bir pixel 2 byte oldu�u i�in col*2 yapt�m.
			*((uint16_t*)(fbp[i] + location)) = ((uint16_t)(image[z] << 8) &  0xf800) | ((uint16_t)(image[z+1] << 3) & 0x7E0) |(uint16_t)((image[z+2]>>3) & 0x1f);
		}
	}	
	i=pNo;
	for (row = 0; row != 480; row++) {		//480
		for (col = 0; col != 848; col++) {		//848
			image = (uint8_t *) bmp[3].bitmap;
			size_t z = (row * bmp[3].width + col) * BYTES_PER_PIXEL;		//bmp i�erisinde bpp ne olursa olsun her bir pixel bilgisi 4 byte uzunlugundadir. burada pixel ba�lang�c� hesaplan�yor.
			location = (col+offsetler[i])*2+(row*1696);					//her bir pixel 2 byte oldu�u i�in col*2 yapt�m.
			*((uint16_t*)(fbp[i] + location)) = ((uint16_t)(image[z] << 8) &  0xf800) | ((uint16_t)(image[z+1] << 3) & 0x7E0) |(uint16_t)((image[z+2]>>3) & 0x1f);
		}
	}	
	oncekiPNo=pNo;
}
void sendOfsetDataToPc(void)
{
	int cport_nr=22;        /* /dev/ttyAMA0 */
	char i;
	RS232_SendByte(cport_nr,'U');		//start byte
	RS232_SendByte(cport_nr,'U');
	RS232_SendByte(cport_nr,COMMANDVERTICALCALIBRATIONSETPROJECTOR);	//command
	for(i=0;i<EKRANADEDI;i++){
		RS232_SendByte(cport_nr,(unsigned char)(offsetler[i]&0x00ff));
		RS232_SendByte(cport_nr,(unsigned char)((offsetler[i]&0xff00)>>8));
	}
}