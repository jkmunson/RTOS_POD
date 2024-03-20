#include <stdio.h>
#include <stdint.h>

#define VOLUME_ADC_RANGE 4095

struct __attribute__((__packed__)) wav_header {
    // RIFF Header
    char riff_header[4]; // Contains "RIFF"
    uint32_t wav_size; // Size of the wav portion of the file, which follows the first 8 bytes. File size - 8
    char wave_header[4]; // Contains "WAVE"

    // Format Header
    char fmt_header[4]; // Contains "fmt " (includes trailing space)
    uint32_t fmt_chunk_size; // Should be 16 for PCM
    uint16_t audio_format; // Should be 1 for PCM. 3 for IEEE Float
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate; // Number of bytes per second. sample_rate * num_channels * Bytes Per Sample
    uint16_t sample_alignment; // num_channels * Bytes Per Sample
    uint16_t bit_depth; // Number of bits per sample

    // Data
    char data_header[4]; // Contains "data"
    uint32_t data_bytes; // Number of bytes in data. Number of samples * num_channels * sample byte size
    // uint8_t bytes[]; // Remainder of wave file is bytes
};

uint16_t audio_convert(int16_t old_val, uint16_t volume) {
	int32_t expanded = ((int32_t)old_val)*(int32_t)VOLUME_ADC_RANGE;
	int32_t attenuated = expanded/volume;
	uint32_t shifted = (uint32_t)(attenuated + 32768);
	uint16_t truncated = shifted;
	return truncated;
}

int main() {
	char head[sizeof(struct wav_header)];
	unsigned char in[2];
	int innt[2];
	for(int i = 0; i < sizeof(struct wav_header); i++){
		if( (head[i] = getchar()) == EOF) {
			fprintf(stderr, "Not valid header\n");
			for(int j = 0; j < i; j++) {
				return 1;
			}
		}
		putchar(head[i]); //Just output the header unmodified
	}

	while( ((innt[0]=getchar()) != EOF) && ((innt[1]=getchar()) != EOF) ) {
		in[0] = innt[0];
		in[1] = innt[1];
		int16_t p = in[0] | in[1]<<8;
		uint16_t q = audio_convert(p, 2048);
		//		 ^ q is the value for the DAC array.
		//Convert back so I can listen to it. The rest of this is NOT for use on the MCU
		uint32_t ex = q;
		int32_t sign = ((int32_t)q) - 32768;
		union {
				int16_t trunc;
				char bytes[2];
		} t;
		t.trunc = sign;

		putchar(t.bytes[0]);
		putchar(t.bytes[1]);

	}

}
