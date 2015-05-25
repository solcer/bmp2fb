

//selim ?l?er 14/10/2014 bmp to framebuffer
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
#include "constants.h"


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



/*****************GPIO AYARLARI***************************/
#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

#define BCM2708_PERI_BASE        0x20000000
#define GPIO_BASE                (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */
 
int  mem_fd;
void *gpio_map;
 
// I/O access
volatile unsigned *gpio;
  
// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))
 
#define GPIO_SET *(gpio+7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR *(gpio+10) // clears bits which are 1 ignores bits which are 0
 
#define GET_GPIO(g) (*(gpio+13)&(1<<g)) // 0 if LOW, (1<<g) if HIGH
 
#define GPIO_PULL *(gpio+37) // Pull up/pull down
#define GPIO_PULLCLK0 *(gpio+38) // Pull up/pull down clock
/*************************************************************/

void setup_io();				//GPIO ayarlari
unsigned char *load_file(const char *path, size_t *data_size);
void warning(const char *context, bmp_result code);
void *bitmap_create(int width, int height, unsigned int state);
unsigned char *bitmap_get_buffer(void *bitmap);
size_t bitmap_get_bpp(void *bitmap);
void bitmap_destroy(void *bitmap);
void cleanUp(void);

void frameBufferTemizle(void);						//tum framebufferlar? temizler
void framebufferAyarla(unsigned char );				//??z?n?rl??? bozuk framebufferi tespit edip 848x480 yapar
void tumFrameBufferlariYenile(char pNo);			//Dikey kalibrasyon ekran?nda t?m framebufferlara bmp[2]'yi y?kler.
void tekFrameBufferiYenile(char pNo);				//Dikey kalibrasyon ekran?nda secilen framebuffer'a farkl? resim y?kler.
void slitDoldur(uint8_t projektorNo,uint16_t slit,uint8_t clear,uint8_t resim);		//istenilen pixele slit olu?turur
void bufferTemizle(unsigned char bufferNo);
void ttyleriHazirla(void);
uint16_t ttyOku(void);
int getch(void);

void dataYaz(int * ,unsigned char *);
void dataOku(int * ,unsigned char *);
uint8_t loadImage (char bufNo,char *imageName);			//bmp registerine istenilen resimi y?kler

void sendOfsetDataToPc(void);
void sendProjektorSlitNolariToPc(void);


unsigned char SLITSIZE =	13,firstPersonKaydirma=0,secondPersonKaydirma=0,firstPersonImageNumber=0,secondPersonImageNumber=0;

unsigned char IKIGOZMESAFESI = 29;

signed int dikeyOfsetler[EKRANADEDI];//={0,18,50,48,36,32,45,44,55,54,44,20,59,47,51,30,53};			//3. ayar
//pixelData.dat :48,36,37,33,29,30,22,16,22,25,1,3,13,16,6,0,7,
//300,290,286,287,283,286,272,271,278,281,255,259,273,274,255,254,256,
//46,36,32,33,29,32,18,17,24,27,1,5,19,20,1,0,2,
//45,33,30,30,26,28,18,14,22,25,0,3,16,7,0,1,


bmp_bitmap_callback_vt bitmap_callbacks = {
	bitmap_create,
	bitmap_destroy,
	bitmap_get_buffer,
	bitmap_get_bpp
};
bmp_result code;

bmp_image bmp[RESIMSAYISI+2];			//resim sayýsý + iki tane kalibrasyon resimleri


int calismaModu;
int uzaktty;
int fbfd[EKRANADEDI] , tty[EKRANADEDI] ;

struct fb_var_screeninfo vinfo[EKRANADEDI];

struct fb_fix_screeninfo finfo[EKRANADEDI];

struct fb_con2fbmap c2m[EKRANADEDI];			//tty leri framebuffer a atamak i?in kullan?l?yor

long int screensize[EKRANADEDI] ;

char *fbp[EKRANADEDI] ;

uint8_t *image;

unsigned char *data[EKRANADEDI];				//resim datas?n? i?eren de?i?ken
unsigned int projektorSlitNolari[EKRANADEDI];  //=296,284,285,281,277,278,270,264,270,273,249,251,261,264,254,248,255,

uint8_t buffer[100];


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
	
	uint8_t outputBuffer[100];
	
	int cport_nr=22;        /* /dev/ttyAMA0 */
	int bdrate=115200;       /* 9600 baud */

	//unsigned char uartBuf[4096];
	char mode[]={'8','N','1',0};
	uint16_t firstPersonHeadPositionX,secondPersonHeadPositionX;
	uint16_t firstPersonHeadPosY,secondPersonHeadPosY;
	uint16_t firstPersonHeadPosZ;
	
	uint8_t pNo;
	uint16_t sltNo;
	uint16_t oncekiSltNo[2][EKRANADEDI];			//iki kullanýcý icin degiskeni iki boyutlu yaptým.
	uint8_t kullaniciNo;
	

	setup_io();			//io lari hazirla
	INP_GPIO(CTSCONTROL);		//ikinci pini cikis yapmak icin once giris yapmak gerekli
	OUT_GPIO(CTSCONTROL);		//pini cikis yap
	
	
	dataOku(&projektorSlitNolari[0],"./pixelData.dat");	
	dataOku(&dikeyOfsetler[0],"./ofsetler.dat");
	
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
	for(i=0;i<EKRANADEDI;i++)			//pointerlar?n ba?lang?? degeri
	{
	  fbp[i]=0;
	  fbfd[i]=0;
	  tty[i]=0;
	  screensize[i]=0;
	  oncekiSltNo[0][i]=0;
	  oncekiSltNo[1][i]=0;
	}
	
	if ((getuid ()) != 0) {

		printf ("You are not root, run this as root or sudo\n");
		exit(1);

    }

	ttyleriHazirla();
	frameBufferTemizle();
	for(i=0;i<RESIMSAYISI;i++){			//tum resimleri ram'a yukle
		sprintf(&buffer[0],"/home/pi/selim/bmp2fb/AllContent/bmps/image%d.bmp",i);
		loadImage(i,buffer);
		printf("%d\n",bmp[i].width);
	}
	
	
	loadImage(RESIMSAYISI,"/home/pi/selim/bmp2fb/calibrationPictureRed.bmp");		//36. resim
	loadImage(RESIMSAYISI+1,"/home/pi/selim/bmp2fb/calibrationPictureGreen.bmp");

	
//	for(i=0;i<EKRANADEDI;i++){
//		slitDoldur(i,projektorSlitNolari[i],1,SOLGOZRESMI);
//		slitDoldur(i,projektorSlitNolari[i]+IKIGOZMESAFESI,1,SAGGOZRESMI);
//	} 
	RS232_flush(cport_nr);			//portta bekleyen datalar? temizle
	
	while(1){
	
		//RS232_PollComport(cport_nr,&buffer[0],15);
		RS232_PollComport(cport_nr,&buffer[0],8);
		if(buffer[0] =='U' && buffer[1]=='U' )
		{
			GPIO_CLR = 1<<CTSCONTROL;	
			printf("buffer[]= ");
			for(i=0;i<15;i++){
				printf("%d,",buffer[i]);
			}
			 printf("\n");	
			switch(buffer[2]){
				case COMMANDMANUALCONTROL:
					printf("manual control\r");
					firstPersonHeadPositionX=(buffer[4]<<8)&0xff00 | buffer[3];
					if(firstPersonHeadPositionX>390) firstPersonHeadPositionX=390;
					IKIGOZMESAFESI=(buffer[6]<<8)&0xff00 | buffer[5];
					SLITSIZE=(buffer[8]<<8)&0xff00 | buffer[7];
					printf("Head Pos: %d   onceki Slit Pos:%d , yeni sltNo:%d   \r",firstPersonHeadPositionX,oncekiSltNo[0][0], projektorSlitNolari[0]+firstPersonHeadPositionX);
					for(i=0;i<EKRANADEDI;i++){
						slitDoldur(i,oncekiSltNo[0][i],0,SOLGOZRESMI);	//1. resimi sil
						slitDoldur(i,oncekiSltNo[0][i]+IKIGOZMESAFESI,0,SAGGOZRESMI);	//2. resimi sil
						
						oncekiSltNo[0][i]=projektorSlitNolari[i]+firstPersonHeadPositionX;
						slitDoldur(i,oncekiSltNo[0][i],1,SOLGOZRESMI);
						slitDoldur(i,oncekiSltNo[0][i]+IKIGOZMESAFESI,1,SAGGOZRESMI);		//2.slitler
					}
					break;
				case COMMANDMULTIUSERPOSITION:							//coklu kullanici bilgisi
					//printf("COMMANDPOSITION\n");
  					secondPersonHeadPositionX=(buffer[4]<<8)&0xff00 | buffer[3];
					if(secondPersonHeadPositionX>390) secondPersonHeadPositionX=390;
					kullaniciNo=0;					//pcden gelen kullanici numarasi
					secondPersonImageNumber=(buffer[6]<<8)&0xff00 | buffer[5];			//elle cevrilen görüntü numarasý
					secondPersonKaydirma=secondPersonHeadPositionX/20;					//yataydaki görüntü kaydirma
					if(secondPersonKaydirma>=20) secondPersonKaydirma=20;
					printf("Head Pos: %d   onceki Slit Pos:%d , yeni sltNo:%d  ,secondPersonImageNumber:%d \n",secondPersonHeadPositionX,oncekiSltNo[kullaniciNo][0], projektorSlitNolari[0]+secondPersonHeadPositionX,secondPersonImageNumber);
					if(secondPersonImageNumber==0) secondPersonImageNumber= RESIMSAYISI-2;			
					
					for(i=0;i<EKRANADEDI;i++){
						
						slitDoldur(i,oncekiSltNo[1][i],0,secondPersonImageNumber);	//1. resimi sil
						slitDoldur(i,oncekiSltNo[1][i]+IKIGOZMESAFESI,0,secondPersonImageNumber+1);	//2. resimi sil
						
						oncekiSltNo[1][i]=projektorSlitNolari[i]+secondPersonHeadPositionX;
						slitDoldur(i,oncekiSltNo[1][i],1,secondPersonImageNumber);
						slitDoldur(i,oncekiSltNo[1][i]+IKIGOZMESAFESI,1,secondPersonImageNumber+1);		//2.slitler
						
					}
					
					break;
				case COMMANDPOSITION:							//pc'den kafa posizyonlar?n? al?r
					//printf("COMMANDPOSITION\n");
  					firstPersonHeadPositionX=(buffer[4]<<8)&0xff00 | buffer[3];
					if(firstPersonHeadPositionX>390) firstPersonHeadPositionX=390;
					kullaniciNo=buffer[7];					//pcden gelen kullanici numarasi
					firstPersonHeadPosY=(buffer[6]<<8)&0xff00 | buffer[5];
					firstPersonKaydirma=firstPersonHeadPositionX/20;						//görüntüyü kaydýrma  
					if(firstPersonKaydirma>=20) firstPersonKaydirma=20;
					firstPersonImageNumber=firstPersonHeadPosY;
					printf("Head Pos: %d   onceki Slit Pos:%d , yeni sltNo:%d  ,firstPersonImageNumber:%d \n",firstPersonHeadPositionX,oncekiSltNo[kullaniciNo][0], projektorSlitNolari[0]+firstPersonHeadPositionX,firstPersonImageNumber);
					
					if(firstPersonImageNumber==0) firstPersonImageNumber= RESIMSAYISI-2;
					
					for(i=0;i<EKRANADEDI;i++){
						slitDoldur(i,oncekiSltNo[kullaniciNo][i],0,firstPersonImageNumber);	//1. resimi sil
						slitDoldur(i,oncekiSltNo[kullaniciNo][i]+IKIGOZMESAFESI,0,firstPersonImageNumber+1);	//2. resimi sil
						
						/*******************************************
						//sol ve sag tarafan silinen slitler
						slitDoldur(i,oncekiSltNo[i]-SLITSIZE,0,SOLGOZRESMI);	//1. resimi sil
						slitDoldur(i,oncekiSltNo[i]+IKIGOZMESAFESI-SLITSIZE,0,SAGGOZRESMI);	//2. resimi sil
						slitDoldur(i,oncekiSltNo[i]+SLITSIZE,0,SOLGOZRESMI);	//1. resimi sil
						slitDoldur(i,oncekiSltNo[i]+IKIGOZMESAFESI+SLITSIZE,0,SAGGOZRESMI);	//2. resimi sil
						/*******************************************/
						
						oncekiSltNo[kullaniciNo][i]=projektorSlitNolari[i]+firstPersonHeadPositionX;
						slitDoldur(i,oncekiSltNo[kullaniciNo][i],1,firstPersonImageNumber);
						slitDoldur(i,oncekiSltNo[kullaniciNo][i]+IKIGOZMESAFESI,1,firstPersonImageNumber+1);		//2.slitler
						
						/*******************************************
						//sol ve sag tarafa eklenen slitler
						slitDoldur(i,oncekiSltNo[i]-SLITSIZE,1,firstPersonImageNumber);
						slitDoldur(i,oncekiSltNo[i]+IKIGOZMESAFESI-SLITSIZE,1,firstPersonImageNumber+1);		//2.slitler
						slitDoldur(i,oncekiSltNo[i]+SLITSIZE,1,firstPersonImageNumber);
						slitDoldur(i,oncekiSltNo[i]+IKIGOZMESAFESI+SLITSIZE,1,firstPersonImageNumber+1);		//2.slitler
						/*******************************************/
						
					}
					
					break;
				case COMMANDVERTICALCALIBRATIONSETPROJECTOR:			//projekt?rlerin yeni ofset degerini al
					switch(buffer[4]){			//yukar? veya asagi komutu geldi mi kontrol et
						case 1:
							//yukar? kaydir komutu geldiyse 
							//gelen ofset degeri kadar offseti kaydir
							//printf("yukari:%d");
							dikeyOfsetler[buffer[3]]=((buffer[6]<<8) + buffer[5]);
							break;
						case 2:
							//asagi komutu
							//printf("asagi");
							dikeyOfsetler[buffer[3]]=((buffer[6]<<8) + buffer[5]);
							break;
						default:
							break;
					}
					//printf("gelen ofset:%d \n",dikeyOfsetler[buffer[3]]);
					//sendOfsetDataToPc();
					tekFrameBufferiYenile(buffer[3]);					// secilen projektorun rengini degistir
					break;
				case COMMANDVERTICALCALIBRATION:
					sendOfsetDataToPc();
					tumFrameBufferlariYenile(buffer[3]);
					//kalibre edilecek projektor icerigi yuk
					tekFrameBufferiYenile(buffer[3]);	
					break;
				case COMMANDSLITSIZE:
					//printf("COMMANDSLITSIZE\n");
					SLITSIZE=buffer[3];
					break;
				case COMMANDSLITDISTANCE:
					IKIGOZMESAFESI=buffer[3];
					printf("COMMANDSLITDISTANCE: %d\n",IKIGOZMESAFESI);
					break;
				case COMMANDCLEARSCREEN:
					frameBufferTemizle();
					break;
				case COMMANDSLITAYARLA:
					sltNo=((buffer[5]<<8)+ buffer[4]);			//pc'den gelen slit numarasini hesapla
					pNo=buffer[3];								//hangi projektor?
					//slitDoldur(pNo,sltNo -1,0,SOLGOZRESMI);
					//slitDoldur(pNo,sltNo +1,0,SOLGOZRESMI);
					bufferTemizle(pNo);
					slitDoldur(pNo,sltNo,1,SOLGOZRESMI);
					projektorSlitNolari[pNo]=sltNo;
					break;
				case COMMANDSLITAYARLASAVE:
					//int a=5000;							//baslangic degeri
					sltNo=5000;
					printf("degerler: ");
					for(i=0;i<EKRANADEDI-1;i++)			//projektorSlitNolari[] icerisindeki en kucuk degeri bul
					{
						//a=projektorSlitNolari[i];
						if(sltNo>projektorSlitNolari[i]){
							sltNo=projektorSlitNolari[i];
							printf("%d,",projektorSlitNolari[i]);
						}
					}
					printf("\n");
					for(i=0;i<EKRANADEDI;i++)			// projektorSlitNolari[] 'dan en kucuk degeri c?kar
					{
						projektorSlitNolari[i]-=sltNo;
						printf("%d,",projektorSlitNolari[i]);
					}
					dataYaz(&projektorSlitNolari[0],"./pixelData.dat");			//hesaplanan degeri dosyaya yaz
					//dataYaz(&projektorSlitNolari[0],"./pixelData.dat");	
					break;
				case COMMANDVERTICALCALIBRATIONSAVE:
					dataYaz(&dikeyOfsetler[0],"./ofsetler.dat");
					break;
				case COMMANDSLITNOLARINIGONDER:			//bu k?s?m iptal 
					sendProjektorSlitNolariToPc();
					break;
				default:
					printf("Default geldi.\n");
				break;
			}//switch(buffer[2]){
		}//if(buffer[0] =='U' && buffer[1]=='U')
		RS232_flush(cport_nr);			//portta bekleyen datalar? temizle
		for(i=0;i<15;i++){
			buffer[i]=0;				//buffer i temizle
		}
		GPIO_SET = 1<<CTSCONTROL;					//Pc'ye hazir bilgisi gönder
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

//tek bir resimi istenilen buffer'a yukler
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

void frameBufferTemizle(void){
int i,row,col;
long int location = 0;
	for(i=0;i<EKRANADEDI;i++){ 				//resimleri ekranlara basan kisim
		for (row = 0; row != 480; row++) {		//480
			for (col = 0; col != 848; col++) {		//848
				location = (col+dikeyOfsetler[i])*2+(row*1696);					//her bir pixel 2 byte oldu?u i?in col*2 yapt?m.
				*((uint16_t*)(fbp[i] + location)) = 0;//((uint16_t)(image[z] << 8) &  0xf800) | ((uint16_t)(image[z+1] << 3) & 0x7E0) |(uint16_t)((image[z+2]>>3) & 0x1f);
				
			}
		}
	}
}
//tek bir bufferi temizler
void bufferTemizle(unsigned char bufferNo)
{
int row,col;
long int location = 0;
	for (row = 0; row != 480; row++) {		//480
		for (col = 0; col != 848; col++) {		//848
			location = (col+dikeyOfsetler[bufferNo])*2+(row*1696);					//her bir pixel 2 byte oldu?u i?in col*2 yapt?m.
			*((uint16_t*)(fbp[bufferNo] + location)) = 0;//((uint16_t)(image[z] << 8) &  0xf800) | ((uint16_t)(image[z+1] << 3) & 0x7E0) |(uint16_t)((image[z+2]>>3) & 0x1f);
			
		}
	}
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
	for(i=0;i<EKRANADEDI;i++)				//ekranlar? tty lere atay?p ram'de pointer a i?aretliyor
	{
		//printf("i:%d\n",i);		

		c2m[i].console = (uint32_t) 10+i;			//framebuffer a atanacak consol numaras?
		c2m[i].framebuffer = (uint32_t) i;		//framebuffer numaras?

		sprintf(&buffer[0],"/dev/fb%d",i); 
		fbfd[i] = open(buffer, O_RDWR);	//framebuffer ? a??yoru		
		if (fbfd[i] == -1) 			//hata geldi mi?
		{
			printf("acilamayan device no:%d",i);
			perror("Error: cannot open framebuffer device :" );
			exit(1);
		}
		//printf("The framebuffer %d  was opened successfully.\n",i);

		if (ioctl(fbfd[i], FBIOPUT_CON2FBMAP, &c2m[i])) 			//framebuffer? tty lere assign et
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
tekrarOlc:			//ekran ??z?n?rl??? tutmuyorsa ayarlay?p tekrar buraya gelecek
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
void slitDoldur(uint8_t projektorNo,uint16_t slit,uint8_t clear,uint8_t resim)
{
unsigned int row,col;
unsigned char i=projektorNo;
long int location = 0,slitBaslangici,temp1,temp2;
static unsigned char oncekiSlit=0;

//slitDoldur(i,oncekiSltNo[i],1,SOLGOZRESMI);
//slitDoldur(i,oncekiSltNo[i]+IKIGOZMESAFESI,1,SAGGOZRESMI);		//2.slitler
if((slit+SLITSIZE)<=480){ 
	image = (uint8_t *) bmp[resim].bitmap;	
	slitBaslangici=(projektorNo+firstPersonKaydirma)*SLITSIZE*848 ;			//bitmapler içerisindeki slitlerin yerini hesaplýyor
		if(clear==1){
			for (row = slit; row < slit + SLITSIZE; row++) {
				//alttaki d?ng?y? hizlandirmak icin temp1 ve temp2 icerigini buraya aldim.
				//1696 sayisini fb5'in surekli farkli cozunurlukte gelmesinden oturu sabit olarak giriyorum. Bu sekilde fb5'de goruntuyu normal verebiliyor.
				temp1=(row*1696);									
				temp2=slitBaslangici +  ((row-slit) * bmp[resim].width);
				//buradaki deger 848'e kadard?. ?stteki bosluklar? almak icin 700'e kadar yazdiriyorum.					
				for (col = 300; col != 550; col++) {		
					size_t z = (temp2 +  col) * BYTES_PER_PIXEL;
					//her bir pixel 2 byte oldu?u i?in (col+dikeyOfsetler[i])*2 yapt?m.
					location = (col+dikeyOfsetler[i])*2+temp1;								
					*((uint16_t*)(fbp[i] + location)) = ((uint16_t)(image[z] << 8) &  0xf800) | ((uint16_t)(image[z+1] << 3) & 0x7E0) |(uint16_t)((image[z+2]>>3) & 0x1f);				
				}
			}
		}else{		//sliti sil
			for (row = slit; row < slit + SLITSIZE; row++) {
				temp1=(row*1696);
				for (col = 300; col != 550; col++) {
					location = (col+dikeyOfsetler[i])*2+temp1;			//her bir pixel 2 byte oldu?u i?in col*2 yapt?m.
					*((uint16_t*)(fbp[i] + location)) = 0;//((uint16_t)(image[z] << 8) &  0xf800) | ((uint16_t)(image[z+1] << 3) & 0x7E0) |(uint16_t)((image[z+2]>>3) & 0x1f);
				}
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


//bu fonksiyonu 800x600 ??z?n?rl?k hatas?n? (?zellikle fb5'in)d?zeltmek i?in yapt?m.
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

void tumFrameBufferlariYenile(char resimNo){
int i,row,col;
long int location = 0;
	printf("Kalibrasyon icin ekranlar yenileniyor...\n");
	for(i=0;i<EKRANADEDI;i++){ 				//resimleri ekranlara basan kisim
		for (row = 0; row != 480; row++) {		//480
			for (col = 0; col != 848; col++) {		//848
				image = (uint8_t *) bmp[RESIMSAYISI].bitmap;
				size_t z = (row * bmp[RESIMSAYISI].width + col) * BYTES_PER_PIXEL;		//bmp i?erisinde bpp ne olursa olsun her bir pixel bilgisi 4 byte uzunlugundadir. burada pixel ba?lang?c? hesaplan?yor.
				location = (col+dikeyOfsetler[i])*2+(row*1696);					//her bir pixel 2 byte oldu?u i?in col*2 yapt?m.
				*((uint16_t*)(fbp[i] + location)) = ((uint16_t)(image[z] << 8) &  0xf800) | ((uint16_t)(image[z+1] << 3) & 0x7E0) |(uint16_t)((image[z+2]>>3) & 0x1f);
			}
		}	
	}
	printf("FRAMEBUFFERLAR OK...\n");
}

void tekFrameBufferiYenile(char pNo)
{
int i,row,col;
long int location = 0;

static oncekiPNo;
	//bir ?nceki secimi siliyoruz
	i=oncekiPNo;
	for (row = 0; row != 480; row++) {		//480
		for (col = 0; col != 848; col++) {		//848
			image = (uint8_t *) bmp[RESIMSAYISI].bitmap;
			size_t z = (row * bmp[RESIMSAYISI].width + col) * BYTES_PER_PIXEL;		//bmp i?erisinde bpp ne olursa olsun her bir pixel bilgisi 4 byte uzunlugundadir. burada pixel ba?lang?c? hesaplan?yor.
			location = (col+dikeyOfsetler[i])*2+(row*1696);					//her bir pixel 2 byte oldu?u i?in col*2 yapt?m.
			*((uint16_t*)(fbp[i] + location)) = ((uint16_t)(image[z] << 8) &  0xf800) | ((uint16_t)(image[z+1] << 3) & 0x7E0) |(uint16_t)((image[z+2]>>3) & 0x1f);
		}
	}	
	i=pNo;
	for (row = 0; row != 480; row++) {		//480
		for (col = 0; col != 848; col++) {		//848
			image = (uint8_t *) bmp[RESIMSAYISI+1].bitmap;
			size_t z = (row * bmp[RESIMSAYISI+1].width + col) * BYTES_PER_PIXEL;		//bmp i?erisinde bpp ne olursa olsun her bir pixel bilgisi 4 byte uzunlugundadir. burada pixel ba?lang?c? hesaplan?yor.
			location = (col+dikeyOfsetler[i])*2+(row*1696);					//her bir pixel 2 byte oldu?u i?in col*2 yapt?m.
			*((uint16_t*)(fbp[i] + location)) = ((uint16_t)(image[z] << 8) &  0xf800) | ((uint16_t)(image[z+1] << 3) & 0x7E0) |(uint16_t)((image[z+2]>>3) & 0x1f);
		}
	}	
	oncekiPNo=pNo;
}
void sendProjektorSlitNolariToPc(void)
{
	int cport_nr=22;        /* /dev/ttyAMA0 */
	char i;
	RS232_SendByte(cport_nr,'U');		//start byte
	RS232_SendByte(cport_nr,'U');
	RS232_SendByte(cport_nr,COMMANDSLITNOLARINIGONDER);	//command
	for(i=0;i<EKRANADEDI;i++){
		RS232_SendByte(cport_nr,(unsigned char)(projektorSlitNolari[i]&0x00ff));
		RS232_SendByte(cport_nr,(unsigned char)((projektorSlitNolari[i]&0xff00)>>8));
	}
}
void sendOfsetDataToPc(void)
{
	int cport_nr=22;        /* /dev/ttyAMA0 */
	char i;
	RS232_SendByte(cport_nr,'U');		//start byte
	RS232_SendByte(cport_nr,'U');
	RS232_SendByte(cport_nr,COMMANDVERTICALCALIBRATIONSETPROJECTOR);	//command
	for(i=0;i<EKRANADEDI;i++){
		RS232_SendByte(cport_nr,(unsigned char)(dikeyOfsetler[i]&0x00ff));
		RS232_SendByte(cport_nr,(unsigned char)((dikeyOfsetler[i]&0xff00)>>8));
	}
}

void setup_io()
{
   /* open /dev/mem */
   if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
      printf("can't open /dev/mem \n");
      exit(-1);
   }
 
   /* mmap GPIO */
   gpio_map = mmap(
      NULL,             //Any adddress in our space will do
      BLOCK_SIZE,       //Map length
      PROT_READ|PROT_WRITE,// Enable reading & writting to mapped memory
      MAP_SHARED,       //Shared with other processes
      mem_fd,           //File to map
      GPIO_BASE         //Offset to GPIO peripheral
   );
 
   close(mem_fd); //No need to keep mem_fd open after mmap
 
   if (gpio_map == MAP_FAILED) {
      printf("mmap error %d\n", (int)gpio_map);//errno also set!
      exit(-1);
   }
 
   // Always use volatile pointer!
   gpio = (volatile unsigned *)gpio_map;
 
 
} // setup_io