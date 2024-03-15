#include <FreeRTOS.h>
#include <task.h>
#include <string.h>
#include "wav.h"
#include "main.h"
#include "cmsis_os.h"
#include "app_fatfs.h"
#include <stdio.h>
#include "fonts.h"
//extern uint8_t audio_buffer[49152]; //Can be recast to a more appropriate type.

#define BUFSIZE 512


#define MIN(a,b) (((a)<(b))? (a):(b))
typedef void (*funcP)(uint8_t channels, uint16_t numSamples, void *pIn, uint16_t *pOutput);

uint8_t flg_dma_done;

static uint8_t fileBuffer[BUFSIZE];
static uint8_t dmaBuffer[2][BUFSIZE];
static uint8_t dmaBank = 0;

static void setSampleRate(uint16_t freq)
{
  uint16_t period = (80000000 / freq) - 1;

  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 0;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = period;
  htim6.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  HAL_TIM_Base_Init(&htim6);
}

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

    // wait for DMA complete
    while(flg_dma_done == 0) {
      __NOP();
    }

    HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_1);
    flg_dma_done = 0;
    HAL_DAC_Start_DMA(&hdac1, DAC_CHANNEL_1, (uint32_t*)dmaBuffer[dmaBank], numSamples, DAC_ALIGN_12B_R);

    dmaBank = !dmaBank;
    bytes_last -= blksize;
  };

  while(flg_dma_done == 0) {
    __NOP();
  }

  HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_1);
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

static void playWavFile(char *filename)
{
  FIL fil;
  FRESULT res;
  UINT count = 0;

  struct Wav_Header header;

  res = f_open(&fil, filename, FA_READ);
  if (res != FR_OK)
    return;

  res = f_read(&fil, &header, sizeof(struct Wav_Header), &count);
  if (res != FR_OK)
    goto done;

  if (!isSupprtedWavFile(&header))
    goto done;

  setSampleRate(header.sampleFreq);
  outputSamples(&fil, &header);

done :
  res = f_close(&fil);
  if (res != FR_OK)
    return;
}


void bryant_main(void *ignore) {


	vTaskSuspend(xTaskGetCurrentTaskHandle()); //LEAVE AT THE END
	vTaskDelete(NULL);

	HAL_TIM_Base_Start(&htim6);

	FATFS FatFs;
	FRESULT res;
	DIR dir;
	FILINFO fno;

	res = f_mount(&FatFs, "", 0);
	if (res != FR_OK) {while(1);}

	res = f_opendir(&dir, "");
	if (res != FR_OK){while(1);}
	while(1) {
	res = f_readdir(&dir, &fno);
	if (res != FR_OK || fno.fname[0] == 0)
	  break;

	char *filename = fno.fname;

	if (strstr(filename, ".WAV") != 0) {
	  playWavFile(filename);
	}

	HAL_Delay(1000);
	}

	res = f_closedir(&dir);
}

