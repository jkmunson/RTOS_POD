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

extern uint8_t audio_buffer[49152]; //Can be recast to a more appropriate type.
extern DMA_HandleTypeDef hdma_dac1_ch1;
#define AUD_GREEN_L_DMA hdma_dac1_ch1;


#define BUFSIZE 512


#define MIN(a,b) (((a)<(b))? (a):(b))
typedef void (*funcP)(uint8_t channels, uint16_t numSamples, void *pIn, uint16_t *pOutput);


uint8_t flg_dma_done;

static uint8_t fileBuffer[BUFSIZE];
static uint8_t dmaBuffer[2][BUFSIZE];
static uint8_t dmaBank = 0;



//uint16_t audio_convert(int16_t old_val, uint16_t volume) {
//	int32_t expanded = ((int32_t)old_val)*(int32_t)VOLUME_ADC_RANGE;
//	int32_t attenuated = expanded/volume;
//	uint32_t shifted = (uint32_t)(attenuated + 32768);
//	uint16_t truncated = shifted;
//	return truncated;
//}

static inline uint16_t val2Dac8(int32_t v)
{
  uint16_t out = v << 3;
  return out;
}

static inline uint16_t val2Dac16(int32_t v)
{
  v >>= 4;
  v += 2047;
  return v & 0xfff;
}

static void prepareDACBuffer_8Bit(uint8_t channels, uint16_t numSamples, void *pIn, uint16_t *pOutput)
{
  uint8_t *pInput = (uint8_t *)pIn;

  for (int i=0; i<numSamples; i++) {
    int32_t val = 0;

    for(int j=0; j<channels; j++) {
      val += *pInput++;
    }
    val /= channels;
    *pOutput++ = val2Dac8(val);
  }
}

static void prepareDACBuffer_16Bit(uint8_t channels, uint16_t numSamples, void *pIn, uint16_t *pOutput)
{
  int16_t *pInput = (int16_t *)pIn;

  for (int i=0; i<numSamples; i++) {
    int32_t val = 0;

    for(int j=0; j<channels; j++) {
      val += *pInput++;
    }
    val /= channels;
    *pOutput++ = val2Dac16(val);
  }
}

static void outputSamples(FIL *fil, struct Wav_Header *header)
{
  const uint8_t channels = header->channels;
  const uint8_t bytesPerSample = header->bitsPerSample / 8;

  funcP prepareData = (header->bitsPerSample == 8)? prepareDACBuffer_8Bit : prepareDACBuffer_16Bit;

  flg_dma_done = 1;
  dmaBank = 0;

  uint32_t bytes_last = header->dataChunkLength;

  while(0 < bytes_last) {

    int blksize = (header->bitsPerSample == 8)? MIN(bytes_last, BUFSIZE / 2) : MIN(bytes_last, BUFSIZE);

    UINT bytes_read;
    FRESULT res;

    res = f_read(fil, fileBuffer, blksize, &bytes_read);
    if (res != FR_OK || bytes_read == 0)
      break;

    uint16_t numSamples = bytes_read / bytesPerSample / channels;
    int16_t     *pInput = (int16_t *)fileBuffer;
    uint16_t   *pOutput = (uint16_t *)dmaBuffer[dmaBank];

    prepareData(channels, numSamples, pInput, pOutput);


    while(flg_dma_done == 0) {
      __NOP();
    }

	HAL_DAC_Stop_DMA(&AUD_GREEN_DAC, DAC_CHANNEL_1);
	HAL_DAC_Stop_DMA(&AUD_GREEN_DAC, DAC_CHANNEL_2);

    flg_dma_done = 0;
	HAL_DAC_Start(&AUD_GREEN_DAC, DAC_CHANNEL_1);
	HAL_DAC_Start(&AUD_GREEN_DAC, DAC_CHANNEL_2);

	HAL_DAC_Start_DMA(&AUD_GREEN_DAC, DAC_CHANNEL_1, (uint32_t*)dmaBuffer[dmaBank], (numSamples>>2)-1, DAC_ALIGN_12B_R);
	HAL_DAC_Start_DMA(&AUD_GREEN_DAC, DAC_CHANNEL_2, (uint32_t*)(dmaBuffer[dmaBank]+2), (numSamples>>2)-1, DAC_ALIGN_12B_R);

    dmaBank = !dmaBank;
    bytes_last -= blksize;
  };


//  bool pause;			// not used yet


  while(flg_dma_done == 0) {
	  if(stop_playing)
		  break;
    __NOP();
  }

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

done :
  res = f_close(fil);
  if (res != FR_OK)
    return;
}

void bryant_main(void *ignore __attribute__((unused))) {

	FIL *filename = audio_file_handle;

	if(file_ready) {
		playWavFile(filename);
	}
	vTaskDelay(1000);
	vTaskSuspend(NULL); //LEAVE AT THE END
	vTaskDelete(NULL);
}
