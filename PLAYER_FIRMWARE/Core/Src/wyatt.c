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
#include "buttons.h"
#include "braeden.h"
#include "bryant.h"
#include "wav_header.h"


//internal functions
static void init(void);					// initializes display
static void mountFileSystem(void);		// mount file system
static void getSDstats(void);			// gets space stats
static void showFileNames(void);
static void checkNewSelection(void);
static void openSelectedFile(void);
static void closeSelectedFile(void);
static void handleButtonPress(void);
static void FSMtest(void);
static void setAllFalse(void);
static void showRecordSlots(void);
static void handleButtonPress(void);
static void postRecordingPrompt(void);
static void passRecordFile(void);
static void closeRecordFile(void);
static void setAllFalse(void);



char disp_buf[50];
uint16_t tick = 0;
uint16_t file = 0;
uint16_t sel = 1;

bool sel_next = false;
bool sel_prev = false;
bool readFile = false;
bool viewDirectory = false;
bool viewRecordSlots = false;
bool startRecording = false;

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

FIL *audio_file_handle;
bool file_ready;
bool stop_playing;	// not used yet
bool pause;			// not used yet
bool song_complete;	// not used yet

FIL *write_file_handle;

uint16_t long_press_count = 0;
bool long_press_detected;
bool start_recording;
bool stop_recording;
char recording_names[4][20] = {"", "/Recordings/recording1.wav", "/Recordings/ecording2.wav", "/Recordings/recording3.wav"};
unsigned int a;


void init() {
	HAL_GPIO_WritePin(TFT_LED_LEVEL_GPIO_Port, TFT_LED_LEVEL_Pin, 1);
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

void showRecordSlots(void){
	ILI9341_WriteString(10,2*SPACER, "Hold right to record to selected file:",Font_7x10,MAIN_FONT_COLOR,BG_COLOR);

	for (int i=1; i<4; i++){
		if ((i == sel) & (tick%6>2)){
			ILI9341_WriteString(10,(3+i)*SPACER, recording_names[i], Font_7x10,BG_COLOR,MAIN_FONT_COLOR);
		} else {
			ILI9341_WriteString(10,(3+i)*SPACER, recording_names[i], Font_7x10,MAIN_FONT_COLOR,BG_COLOR);
		}
	}

}

void showFileNames(){
	ILI9341_WriteString(10,2*SPACER, "Make a selection:",Font_7x10,MAIN_FONT_COLOR,BG_COLOR);

	for (file=1; file<nfile; file++){
		if ((file == sel) & (tick%6>2)){
			// flashing effect for selected file
			ILI9341_WriteString(10,(3+file)*SPACER, filenames[file], Font_7x10,BG_COLOR,MAIN_FONT_COLOR);
		} else {
			ILI9341_WriteString(10,(3+file)*SPACER, filenames[file], Font_7x10,MAIN_FONT_COLOR,BG_COLOR);
		}
	}

	if (sel == nfile){
		if (tick%6>2){
			ILI9341_WriteString(10,(3+nfile)*SPACER, "RECORD", Font_7x10,BG_COLOR,MAIN_FONT_COLOR);
		} else {
			ILI9341_WriteString(10,(3+nfile)*SPACER, "RECORD", Font_7x10,MAIN_FONT_COLOR,BG_COLOR);
		}
	} else {
		ILI9341_WriteString(10,(3+nfile)*SPACER, "RECORD", Font_7x10,MAIN_FONT_COLOR,BG_COLOR);
	}

}

void checkNewSelection(){
	// Select limits for choosing files
	if ((sel_next) & (sel < nfile) & (currentState == viewingDirectory)){
		sel_next = false;
		sel++;
	}
	if ((sel_prev) & (sel > 1) & (currentState == viewingDirectory)){
		sel_prev = false;
		sel--;
	}

	// select limits for choosing recordings
	if ((sel_next) & (sel < 3) & (currentState == viewingRecordSlots)){
		sel_next = false;
		sel++;
	}
	if ((sel_prev) & (sel > 1) & (currentState == viewingRecordSlots)){
		sel_prev = false;
		sel--;
	}

	sel_next = sel_prev = false;

}

void openSelectedFile(){
	fres = f_open(&fil, filenames[sel], FA_READ);	// open file
	if (fres != FR_OK) {
		while(1);
	}

	audio_file_handle = &fil;	// added 3/19
	file_ready = true;

	contentsPosted = true;
}

void closeSelectedFile(){
	file_ready = false;
	f_close(audio_file_handle);		// close file
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
		break;
	}
}

void handleButtonPress(void){
	if (up_pressed){
		up_pressed = false;
		sel_prev = true;
	}
	if (down_pressed){
		down_pressed = false;
		sel_next = true;
	}

	if (left_pressed){
		left_pressed = false;
		viewDirectory = true;
	}

	if (right_pressed){
		right_pressed = false;
		if (currentState == viewingDirectory){
			if (sel < nfile){
				readFile = true;
			} else {
				viewRecordSlots = true;
			}
		}
	}


	if (currentState == viewingRecordSlots){
		if (right_held){
			long_press_count++;
			if (long_press_count > 10){
				startRecording = true;
			}
		} else {
			long_press_count = 0;
		}
	}

	if (!right_held & (currentState == recording)){
		viewRecordSlots = true;
	}



}

void postRecordingPrompt(void){
	ILI9341_WriteString(10,2*SPACER, "ACTIVELY RECORDING", Font_7x10,MAIN_FONT_COLOR,BG_COLOR);
	sprintf(disp_buf, "Writing to file %u", sel);
	ILI9341_WriteString(10,3*SPACER, disp_buf, Font_7x10,MAIN_FONT_COLOR,BG_COLOR);
	contentsPosted = true;
}

void passRecordFile(void){
	fres = f_open(&fil, recording_names[sel], FA_WRITE);	// open file
	if (fres != FR_OK) {
		ILI9341_WriteString(10,6*SPACER, "Error opening write file", Font_7x10,MAIN_FONT_COLOR,BG_COLOR);
		while(1){
			vTaskSuspend(NULL);
		}
	}

	write_file_handle = &fil;
	f_write(write_file_handle, wav_header_bin, wav_header_bin_len, &a);

	start_recording = true;
}

void closeRecordFile(void){
	stop_recording = true;
	while (!recording_complete){
		ILI9341_WriteString(10,3*SPACER, "Waiting for recording complete", Font_7x10,MAIN_FONT_COLOR,BG_COLOR);
		vTaskDelay(1);
	};
	f_close(write_file_handle);		// close file
}

void setAllFalse(void){
	readFile = viewDirectory = viewRecordSlots = startRecording = contentsPosted = false;
}

void wyatt_main(void *ignore __attribute__((unused))) {
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
					setAllFalse();
					}
				if (viewRecordSlots) {
					nextState = viewingRecordSlots;
					setAllFalse();
				}
				break;
			case readingFile:
				if (viewDirectory) {
					nextState = viewingDirectory;
					setAllFalse();
					closeSelectedFile();
				}
				break;
			case viewingRecordSlots:
				if (viewDirectory){
					nextState = viewingDirectory;
					setAllFalse();
				}
				if (startRecording){
					nextState = recording;
					setAllFalse();
					passRecordFile();
				}
				break;
			case recording:
				if (viewRecordSlots){
					nextState = viewingRecordSlots;
					setAllFalse();
					closeRecordFile();
				}
				break;


		}


		handleButtonPress();

		//FSMtest();					// used to change states without hardware

		/* ---------- OUTPUT LOGIC ---------- */
		if (currentState == viewingDirectory){
			showFileNames();
		} else if (currentState == readingFile) {
			if (!contentsPosted){		// Prevents display rewrite redundancy
				openSelectedFile();		// Displays selected file's contents
			}
		} else if (currentState == viewingRecordSlots){ // viewing record slots
			showRecordSlots();
		} else {
			if (!contentsPosted){
				postRecordingPrompt();
			}
		}

		if (currentState != nextState) {
			ILI9341_DrawImage(0, 0, 320, 240, (const uint16_t*)startup_img_320x240); // clear screen for transition
			if (nextState != recording){
				sel = 1;	// reset sel except for when passing it to record.
			}
		}

		/* ---------- STATE REGISTER ---------- */
		checkNewSelection();			// Adjusts current file selection
		currentState = nextState;		// transition to next state

		//sprintf(disp_buf, "ticks: %u", tick);
		//ILI9341_WriteString(0,0*SPACER, disp_buf,Font_7x10,MAIN_FONT_COLOR, BG_COLOR);
		tick++;
   		vTaskDelay(100);
	}
	vTaskSuspend(xTaskGetCurrentTaskHandle()); //LEAVE AT THE END
	vTaskDelete(NULL);
}

