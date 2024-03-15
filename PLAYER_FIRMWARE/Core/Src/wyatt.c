#include <fatfs.h_disable>
#include "main.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include "ili9341.h"
#include "ili9341_touch.h"
#include "fonts.h"

#include <FreeRTOS.h>
#include <task.h>

#include "startup_img.h"
#include "wyatt.h"


//internal functions
static void init(void);					// initializes display
static void mountFileSystem(void);		// mount file system
static void getSDstats(void);			// gets space stats
static void showFileNames(void);
static void checkNewSelection(void);
static void showFileContents(void);
static void FSMtest(void);

char disp_buf[50];
uint16_t tick = 0;
uint16_t file = 0;
uint16_t sel = 1;

bool sel_next = false;
bool sel_prev = false;
bool readFile = false;
bool viewDirectory = false;
bool contentsPosted = false;

STATE currentState = viewingDirectory;
STATE nextState;

//some variables for FatFs
FATFS FatFs; 	//Fatfs handle
FIL fil; 		//File handle
FRESULT fres; //Result after operations

// for statistics from the SD card
DWORD free_clusters, free_sectors, total_sectors;
FATFS* getFreeFs;

FILINFO fno;				// file info object
DIR dir;					// directory object
uint16_t nfile; 			// no. of files and directories
char filenames[10][20];		// string array of file names

BYTE readBuf[30];


void init() {
    ILI9341_Unselect();
    ILI9341_TouchUnselect();
    ILI9341_Init();
}

void mountFileSystem(){
	  fres = f_mount(&FatFs, "", 1); //1=mount now
	  if (fres != FR_OK) {
		ILI9341_WriteString(0,0,"f_mount error",Font_11x18,ILI9341_BLACK,ILI9341_WHITE);
		while(1);
	  }
}

void getSDstats(){
	  fres = f_getfree("", &free_clusters, &getFreeFs);
	  if (fres != FR_OK) {
		ILI9341_WriteString(0,0,"f_getfree error",Font_11x18,ILI9341_BLACK,ILI9341_WHITE);
		while(1);
	  }

	  //Formula comes from ChaN's documentation
	  total_sectors = (getFreeFs->n_fatent - 2) * getFreeFs->csize;
	  free_sectors = free_clusters * getFreeFs->csize;

	  uint32_t ts = total_sectors/2;
	  uint32_t fs = free_sectors/2;
	  char ts_str[20];
	  char fs_str[20];
	  sprintf(ts_str, "%u", ts);
	  sprintf(fs_str, "%u", fs);

	  ILI9341_WriteString(0,1*SPACER,"KiB total drive space: ",Font_7x10,MAIN_FONT_COLOR,BG_COLOR);
	  ILI9341_WriteString(160,1*SPACER,ts_str,Font_7x10,MAIN_FONT_COLOR,BG_COLOR);
	  ILI9341_WriteString(0,2*SPACER,"KiB available: ",Font_7x10,MAIN_FONT_COLOR,BG_COLOR);
	  ILI9341_WriteString(160,2*SPACER,fs_str,Font_7x10,MAIN_FONT_COLOR,BG_COLOR);
}

void gatherFileNames(){
	  fres = f_opendir(&dir, "/");	// open the root directory ("\" = root)
	  if (fres != FR_OK){
			ILI9341_WriteString(0,10*SPACER, "open directory error",Font_7x10,MAIN_FONT_COLOR,BG_COLOR);
			while(1);
	  }

	  nfile = 0;
	  for (;;){
		  fres = f_readdir(&dir, &fno);					// read a directory item
		  if (fres != FR_OK || fno.fname[0]==0) break;	// Check for error or end of directory

		  if (fno.fattrib != AM_DIR) {	// ignore directories
			  strcpy(*(filenames + nfile), fno.fname);
			  nfile++;
		  }
	  }

	  f_closedir(&dir); 		// close directory
}

void showFileNames(){
	ILI9341_WriteString(10,2*SPACER, "Directory contents:",Font_7x10,MAIN_FONT_COLOR,BG_COLOR);

	for (file=1; file<nfile; file++){
		if (file == sel & (tick%2==0)){
			// flashing effect for selected file
			ILI9341_WriteString(10,(3+file)*SPACER, filenames[file], Font_7x10,BG_COLOR,MAIN_FONT_COLOR);
		} else {
			ILI9341_WriteString(10,(3+file)*SPACER, filenames[file], Font_7x10,MAIN_FONT_COLOR,BG_COLOR);
		}
	}
}

void checkNewSelection(){
	  if ((sel_next) & (sel < nfile-1) & (currentState == viewingDirectory)){
		  sel_next = false;
		  sel++;
	  }
	  if ((sel_prev) & (sel > 1) & (currentState == viewingDirectory)){
		  sel_prev = false;
		  sel--;
	  }
}

void showFileContents(){
	fres = f_open(&fil, filenames[sel], FA_READ);	// open file
	if (fres != FR_OK) {
		while(1);
	}

	TCHAR* rres = f_gets((TCHAR*)readBuf, 30, &fil);
	if(rres != 0) {
		ILI9341_WriteString(10,4*SPACER,readBuf,Font_11x18,MAIN_FONT_COLOR,BG_COLOR);
	}

	ILI9341_WriteString(10,2*SPACER, "File contents:", Font_7x10,MAIN_FONT_COLOR,BG_COLOR);

	/*
	for (int i=0; i<10; i++){
		unsigned char tmp[2];
		unsigned char bits[2];
		unsigned char mask = 1;
		unsigned char char_num[2];

		// ----------- Display character number ---------------
		char_num[0] = 48 + i;
		char_num[1] = '\0';
		ILI9341_WriteString(0,6*10, "char number:", Font_11x18, ILI9341_BLACK, ILI9341_WHITE);
		ILI9341_WriteString(160,6*10, char_num, Font_11x18, ILI9341_BLACK, ILI9341_WHITE);

		// ---------- Display individual character -----------
		tmp[0] = readBuf[i];
		tmp[1] = '\0';
		ILI9341_WriteString(0,8*10, "character:", Font_11x18, ILI9341_BLACK, ILI9341_WHITE);
		ILI9341_WriteString(160,8*10, tmp, Font_11x18, ILI9341_BLACK, ILI9341_WHITE);

		// ---------- Display binary representation -----------
		ILI9341_WriteString(0, 10*10, "binary:", Font_11x18, ILI9341_BLACK, ILI9341_WHITE);
		for (int j=0; j<8; j++){
			bits[0] = (tmp[0] & (mask<<j)) != 0;
			if (bits[0] == 0){
				bits[0] = 48; // ascii 0
			} else {
				bits[0] = 49; // ascii 1
			}
			// display binary representation
			bits[1] = '\0';
			ILI9341_WriteString(265 - j*15, 10*10, bits, Font_11x18, ILI9341_BLACK, ILI9341_WHITE);
		}
	*/

	f_close(&fil);		// close file
	contentsPosted = true;
}

void FSMtest(){
	  /* experiment code */
	  switch (tick){
	  case 5:
		  sel_next = true;
		  break;
	  case 10:
		  readFile = true;
		  break;
	  case 15:
		  viewDirectory = true;
		  break;
	  case 20:
		  sel_next = true;
		  break;
	  case 25:
		  readFile = true;
		  break;
	  case 30:
		  viewDirectory = true;
		  break;
	  case 35:
		  sel_next = true;
		  break;
	  case 40:
		  readFile = true;
		  break;
	  case 45:
		  viewDirectory = true;
		  break;
	  default:

	  }
}

void wyatt_main(void *ignore) {
	  init();							// initialize TFT display
	  ILI9341_DrawImage(0, 0, 320, 240, (const uint16_t*)startup_img_320x240);
	  osDelay(2000);					// Stall to view intro screen

	  mountFileSystem();				// connect with SD card
	  getSDstats();						// View total space / available space
	  osDelay(1000);					// delay to view stats

	  ILI9341_DrawImage(0, 0, 320, 240, (const uint16_t*)startup_img_320x240);
	  gatherFileNames();				// gather file names
	  //f_mount(NULL, "", 0); 			// de-mount drive

	  nextState = currentState;			// initialize next state


	  /* Infinite loop */
	  for(;;)
	  {
		  /* ---------- NEXT STATE LOGIC ---------- */
		  switch (currentState){
			  case viewingDirectory:
				  if (readFile) {
					  nextState = readingFile;
					  readFile = viewDirectory = contentsPosted = false;
				  }
				  break;
			  case readingFile:
				  if (viewDirectory) {
					  nextState = viewingDirectory;
					  readFile = viewDirectory = false;
				  }
				  break;
		  }

		  //FSMtest();					// used to change states without hardware

		  /* ---------- OUTPUT LOGIC ---------- */
		  if (currentState == viewingDirectory){
			  showFileNames();
		  } else {
			  if (!contentsPosted){		// Prevents display rewrite redundancy
				  showFileContents();	// Displays selected file's contents
			  }
		  }

		  if (currentState != nextState) ILI9341_DrawImage(0, 0, 320, 240, (const uint16_t*)startup_img_320x240); // clear screen for transition

		  /* ---------- STATE REGISTER ---------- */
		  checkNewSelection();			// Adjusts current file selection
		  currentState = nextState;		// transition to next state

		  //sprintf(disp_buf, "ticks: %u", tick);
		  //ILI9341_WriteString(0,0*SPACER, disp_buf,Font_7x10,MAIN_FONT_COLOR, BG_COLOR);
		  tick++;
		  osDelay(500);
	  }
	  vTaskSuspend(xTaskGetCurrentTaskHandle()); //LEAVE AT THE END
	  vTaskDelete(NULL);
}

