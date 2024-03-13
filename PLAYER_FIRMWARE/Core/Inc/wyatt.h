#ifndef WYATT_H_
#define WYATT_H_

#include <stdint.h>
#include <stdbool.h>

// defines
#define SPACER		12				// vertical spacer - used to tidy display
#define MAIN_FONT_COLOR	ILI9341_BLACK
#define BG_COLOR		ILI9341_WHITE

// external variables
typedef enum {
	  viewingDirectory, readingFile
} STATE;

extern STATE currentState;
extern STATE nextState;

extern char disp_buf[50];		// display buffer - used to print to display
extern uint16_t sel;			// Used to select specific files
extern uint16_t tick;			// Handles blinking effect
extern uint16_t i;				// handles blinking effect

extern bool sel_next;
extern bool sel_prev;
extern bool readFile;
extern bool viewDirectory;
extern bool contentsPosted;

extern FATFS FatFs; 			//Fatfs handle
extern FIL fil; 				//File handle
extern FRESULT fres; 			//Result after operations

extern DWORD free_clusters, free_sectors, total_sectors;	// for SD card stats
extern FATFS* getFreeFs;									// 		"	"

extern FILINFO fno;				// file info
extern DIR dir;					// directory
extern uint16_t nfile; 			// no. of files and directories
extern char filenames[10][20];	// string array of file names

extern BYTE readBuf[30];

void wyatt_main(void *);


#endif // WYATT_H
