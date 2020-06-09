/*
 * WAVResource.cpp
 * WAV parsing code taken from here:
 * http://truelogic.org/wordpress/2015/09/04/parsing-a-wav-file-in-c/
 *      Author: Stefano Ceccherini
 */

#include "WAVResource.h"

#include "Stream.h"

// WAVE file header format
struct wav_header {
	uint8 riff[4];			// RIFF string
	uint32 overall_size;		// overall size of file in bytes
	uint8 wave[4];			// WAVE string
	uint8 fmt_chunk_marker[4];	// fmt string with trailing null char
	uint32 length_of_fmt;		// length of the format data
	uint16 format_type;		// format type. 1-PCM, 3- IEEE float, 6 - 8bit A law, 7 - 8bit mu law
	uint16 channels;		// no.of channels
	uint32 sample_rate;		// sampling rate (blocks per second)
	uint32 byterate;		// SampleRate * NumChannels * BitsPerSample/8
	uint16 block_align;		// NumChannels * BitsPerSample/8
	uint16 bits_per_sample;		// bits per sample, 8- 8bits, 16- 16 bits etc
	uint8 data_chunk_header[4];	// DATA string or FLLR string
	uint32 data_size;		// NumSamples * NumChannels * BitsPerSample/8 - size of the next chunk that will be read
} __attribute__((packed));


/* static */
Resource*
WAVResource::Create(const res_ref& name)
{
	return new WAVResource(name);
}


WAVResource::WAVResource(const res_ref& name)
	:
	Resource(name, RES_WAV)
{
}


WAVResource::~WAVResource()
{
}


/* virtual */
bool
WAVResource::Load(Archive *archive, uint32 key)
{
	printf("WAVResource::Load()\n");
	if (!Resource::Load(archive, key))
		return false;
	return true;	
}


/* virtual */
void
WAVResource::Dump()
{
	printf("WAVResource::Dump()\n");
	unsigned char buffer4[4];
	unsigned char buffer2[2];
	struct wav_header header;
	
	int read = 0;
 	// read header parts
	fData->Read(header.riff, sizeof(header.riff));
	printf("(1-4): %s \n", header.riff); 

	fData->Read(buffer4, sizeof(buffer4));
	printf("%u %u %u %u\n", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);
 
	 // convert little endian to big endian 4 byte int
	header.overall_size  = buffer4[0] | 
					(buffer4[1]<<8) | 
					(buffer4[2]<<16) | 
					(buffer4[3]<<24);

	printf("(5-8) Overall size: bytes:%u, Kb:%u \n", header.overall_size, header.overall_size/1024);

	fData->Read(header.wave, sizeof(header.wave));
	printf("(9-12) Wave marker: %s\n", header.wave);

	fData->Read(header.fmt_chunk_marker, sizeof(header.fmt_chunk_marker));
	printf("(13-16) Fmt marker: %s\n", header.fmt_chunk_marker);

	fData->Read(buffer4, sizeof(buffer4));
	printf("%u %u %u %u\n", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

	 // convert little endian to big endian 4 byte integer
	header.length_of_fmt = buffer4[0] |
						(buffer4[1] << 8) |
						(buffer4[2] << 16) |
						(buffer4[3] << 24);
	printf("(17-20) Length of Fmt header: %u \n", header.length_of_fmt);

	fData->Read(buffer2, sizeof(buffer2));
	printf("%u %u \n", buffer2[0], buffer2[1]);
 
	header.format_type = buffer2[0] | (buffer2[1] << 8);
	char format_name[10] = "";
	if (header.format_type == 1)
		strcpy(format_name,"PCM"); 
	else if (header.format_type == 6)
		strcpy(format_name, "A-law");
	else if (header.format_type == 7)
		strcpy(format_name, "Mu-law");

	printf("(21-22) Format type: %u %s \n", header.format_type, format_name);

	fData->Read(buffer2, sizeof(buffer2));
	printf("%u %u \n", buffer2[0], buffer2[1]);

	header.channels = buffer2[0] | (buffer2[1] << 8);
	printf("(23-24) Channels: %u \n", header.channels);

	fData->Read(buffer4, sizeof(buffer4));
	printf("%u %u %u %u\n", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

	header.sample_rate = buffer4[0] |
						(buffer4[1] << 8) |
						(buffer4[2] << 16) |
						(buffer4[3] << 24);

	printf("(25-28) Sample rate: %u\n", header.sample_rate);

	fData->Read(buffer4, sizeof(buffer4));
	printf("%u %u %u %u\n", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

	header.byterate  = buffer4[0] |
						(buffer4[1] << 8) |
						(buffer4[2] << 16) |
						(buffer4[3] << 24);
	printf("(29-32) Byte Rate: %u , Bit Rate:%u\n", header.byterate, header.byterate*8);

	fData->Read(buffer2, sizeof(buffer2));
	printf("%u %u \n", buffer2[0], buffer2[1]);

	header.block_align = buffer2[0] |
					(buffer2[1] << 8);
	printf("(33-34) Block Alignment: %u \n", header.block_align);

	fData->Read(buffer2, sizeof(buffer2));
	printf("%u %u \n", buffer2[0], buffer2[1]);

	header.bits_per_sample = buffer2[0] |
					(buffer2[1] << 8);
	printf("(35-36) Bits per sample: %u \n", header.bits_per_sample);

	fData->Read(header.data_chunk_header, sizeof(header.data_chunk_header));
	printf("(37-40) Data Marker: %s \n", header.data_chunk_header);

	fData->Read(buffer4, sizeof(buffer4));
	printf("%u %u %u %u\n", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

	header.data_size = buffer4[0] |
				(buffer4[1] << 8) |
				(buffer4[2] << 16) | 
				(buffer4[3] << 24 );
	printf("(41-44) Size of data chunk: %u \n", header.data_size);


 // calculate no.of samples
	long num_samples = (8 * header.data_size) / (header.channels * header.bits_per_sample);
	printf("Number of samples:%lu \n", num_samples);

	long size_of_each_sample = (header.channels * header.bits_per_sample) / 8;
	printf("Size of each sample:%ld bytes\n", size_of_each_sample);

	// calculate duration of file
	float duration_in_seconds = (float) header.overall_size / header.byterate;
	printf("Approx.Duration in seconds=%f\n", duration_in_seconds);
	//printf("Approx.Duration in h:m:s=%s\n", seconds_to_time(duration_in_seconds));

	// read each sample from data chunk if PCM
	if (header.format_type == 1) { // PCM
		int32 i =0;
		char data_buffer[size_of_each_sample];
		bool size_is_correct = true;
		// make sure that the bytes-per-sample is completely divisible by num.of channels
		int32 bytes_in_each_channel = (size_of_each_sample / header.channels);
		if ((bytes_in_each_channel  * header.channels) != size_of_each_sample) {
			printf("Error: %d x %d <> %ld\n", bytes_in_each_channel, header.channels, size_of_each_sample);
			size_is_correct = false;
		}
	
		if (size_is_correct) { 
			// the valid amplitude range for values based on the bits per sample
			int32 low_limit = 0l;
			int32 high_limit = 0l;
	
			switch (header.bits_per_sample) {
				case 8:
					low_limit = -128;
					high_limit = 127;
					break;
				case 16:
					low_limit = -32768;
					high_limit = 32767;
				break;
			case 32:
				low_limit = -2147483648;
				high_limit = 2147483647;
				break;
			}					

			printf("\n\n.Valid range for data values : %d to %d \n", low_limit, high_limit);
			for (i =1; i <= num_samples; i++) {
				printf("==========Sample %d / %ld=============\n", i, num_samples);
				fData->Read(data_buffer, sizeof(data_buffer));
				if (read == 1) {
					// dump the data read
					uint32 xchannels = 0;
					int32 data_in_channel = 0;
					for (xchannels = 0; xchannels < header.channels; xchannels ++ ) {
						printf("Channel#%d : ", (xchannels+1));
						// convert data from little endian to big endian based on bytes in each channel sample
						if (bytes_in_each_channel == 4) {
							data_in_channel =	data_buffer[0] | 
												(data_buffer[1]<<8) | 
												(data_buffer[2]<<16) | 
												(data_buffer[3]<<24);
						}
						else if (bytes_in_each_channel == 2) {
							data_in_channel = data_buffer[0] |
												(data_buffer[1] << 8);
						}
						else if (bytes_in_each_channel == 1) {
							data_in_channel = data_buffer[0];
						}
	
						printf("%d ", data_in_channel);
							// check if value was in range
						if (data_in_channel < low_limit || data_in_channel > high_limit)
							printf("**value out of range\n");	
						printf(" | ");
					}
					printf("\n");
				}
				else {
					printf("Error reading file. %d bytes\n", read);
					break;
				}
			}
		}
	} 
}

