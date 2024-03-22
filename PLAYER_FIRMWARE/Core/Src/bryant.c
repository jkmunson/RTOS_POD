#include <string.h>
#include "wav.h"
#include "main.h"
#include "app_fatfs.h"
#include <stdio.h>
#include "fonts.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_dac.h"
#include "stm32g4xx_hal_dma.h"
#include "stm32g4xx_hal_tim.h"
#include "console.h"
#include "buttons.h"
#include "wyatt.h"
#include "bryant.h"
#include "console.h"

#define AUDIO_BUF_SIZE 49152

extern uint8_t audio_buffer[AUDIO_BUF_SIZE]; //Can be recast to a more appropriate type.
extern DMA_HandleTypeDef hdma_dac1_ch1;

#define AUD_GREEN_L_DMA hdma_dac1_ch1

#define BUFSIZE 512

static size_t buf_iter = 0;
const size_t BUF_HALF_IDX = AUDIO_BUF_SIZE/8; // /2 for 16 bit, /2 for 2 channels /2 half of the buffer

#define MIN(a,b) (((a)<(b))? (a):(b))
typedef void (*funcP)(uint8_t channels, uint16_t numSamples, void *pIn, uint16_t *pOutput);

uint16_t * const left_buf = (uint16_t *)audio_buffer;
uint16_t * const right_buf = (uint16_t *)(audio_buffer+(AUDIO_BUF_SIZE/4));

static uint8_t fileBuffer[BUFSIZE];

static bool buf_filled, half_filled, fill_to_half;
static bool dma_half_done, dma_done;

uint16_t audio_convert(int16_t old_val) {
	int32_t expanded = (int32_t)old_val;
	int32_t attenuated = expanded;
	uint32_t shifted = (uint32_t)(attenuated + 32768);
	uint16_t truncated = shifted;
	return truncated;
}

void AUDIO_DMA_half(struct __DMA_HandleTypeDef *hdma __attribute__((unused))){
	dma_half_done = true;
}

void AUDIO_DMA_full(struct __DMA_HandleTypeDef *hdma __attribute__((unused))){
	dma_done = true;
}

static void prepareDACBuffer_16Bit(uint8_t channels __attribute__((unused)), uint16_t numSamples, uint16_t *pIn, uint16_t *pOutput)
{
	
	const uint16_t *left_buf = pOutput;
	const uint16_t *right_buf = pOutput+(AUDIO_BUF_SIZE/4);
	
	for(int i = 0; i < numSamples*2;) {
		buf_iter = (buf_iter+1);
		if(buf_iter >= AUDIO_BUF_SIZE/4) {
			buf_iter = 0;
			buf_filled = 1;
		}
		if(buf_iter > AUDIO_BUF_SIZE/8) half_filled = true;
	}
}

static void outputSamples(FIL *fil, struct Wav_Header *header)
{
	const uint8_t channels = header->channels;
	const uint8_t bytesPerSample = header->bitsPerSample / 8;
	


	uint32_t bytes_left = header->dataChunkLength;
	
	static dac_started = false;
	
	while(bytes_left > 0) {
		
		int blksize = MIN(bytes_left, BUFSIZE);
		
		while(!buf_filled && !(half_filled && fill_to_half)){
			uint32_t bytes_read;
			FRESULT res;
			res = f_read(fil, fileBuffer, blksize, &bytes_read);
			
			bytes_left -= bytes_read;
			if (res != FR_OK || bytes_read == 0)
				break;

			uint16_t numSamples = bytes_read / bytesPerSample / channels;

			prepareDACBuffer_16Bit(channels, numSamples, (void *)fileBuffer, (uint16_t *)audio_buffer);
		}
		
		if(!dac_started) {
			HAL_DAC_Start_DMA(&AUD_GREEN_DAC, DAC_CHANNEL_1, left_buf, AUDIO_BUF_SIZE/4, DAC_ALIGN_12B_L);
			HAL_DAC_Start_DMA(&AUD_GREEN_DAC, DAC_CHANNEL_2, right_buf, AUDIO_BUF_SIZE/4, DAC_ALIGN_12B_L);
			dac_started = 1;
		}
		
		while(!dma_done && !dma_half_done) vTaskDelay(15);
		
		if(dma_half_done) fill_to_half = true, dma_half_done = false, half_filled=false, buf_filled = false;
		if(dma_done) fill_to_half = false, dma_done = false, half_filled=false, buf_filled = false;
	};

	HAL_DAC_Stop_DMA(&AUD_GREEN_DAC, DAC_CHANNEL_1);
	HAL_DAC_Stop_DMA(&AUD_GREEN_DAC, DAC_CHANNEL_2);

	song_complete = true;
}

static uint8_t isSupprtedWavFile(const struct Wav_Header *header)
{
	if (strncmp(header->riff, "RIFF", 4 ) != 0)
	return 0;

	if (header->vfmt != 1)
	return 0;

	if (strncmp(header->dataChunkHeader, "data", 4 ) != 0)
	return 0;

	return 1;
}

static void playWavFile(FIL *fil)
{

	FRESULT res;
	UINT count = 0;

	struct Wav_Header header;

	res = f_read(fil, &header, sizeof(struct Wav_Header), &count);
	if (res != FR_OK)
	goto done;

	if (!isSupprtedWavFile(&header))
	goto done;

	outputSamples(fil, &header);

done:
	song_complete = true;
	file_ready = false;
}

void bryant_main(void *ignore __attribute__((unused))) {
	//register callbacks
	AUD_GREEN_L_DMA.XferHalfCpltCallback = AUDIO_DMA_half;
	AUD_GREEN_L_DMA.XferCpltCallback = AUDIO_DMA_full;
	
	while(1){
		while (!file_ready) vTaskDelay(200);
		playWavFile(audio_file_handle);
	}

	vTaskSuspend(NULL); //LEAVE AT THE END
	vTaskDelete(NULL);
}
