#include <stdint.h>
#include <LEPTON_Types.h>
#include <SPI.h>
#include <Palettes.h>
#include <bmpfile.h>

#define PACKET_SIZE 164
#define PACKET_SIZE_UINT16 (PACKET_SIZE/2)
#define PACKETS_PER_FRAME 60
#define FRAME_SIZE_UINT16 (PACKET_SIZE_UINT16*PACKETS_PER_FRAME)

void usage()
{
	fprintf(stderr,"usage: lepdump <format> <filename>\n");
	fprintf(stderr,"\t<format> r:raw data(RGB),b:BMP format\n");
	fprintf(stderr,"\t<filename> output file name, if set as '-', the stream goes to stdout\n");
}

void dump(bmpfile_t *bmp,char format,FILE *fp)
{
	if(format=='r') //raw
		bmp_write_pixels(bmp,fp);
	else if(format=='b') //bmp
		bmp_save_to(bmp,fp);
}

int main(int argc,char* argv[])
{
	if(argc<3){
		usage();
		return -1;
	}

	bool bStdout = true;
	std::string filename=argv[2];
	if(filename!="-") bStdout=false;

	char format=*argv[1];
	if(format!='r' && format!='b') {
		fprintf(stderr,"<format> %c not exist\n",format);
		usage();
		return -1;
	}

	if(!bStdout) fprintf(stderr,"<format>=%c\n",format);
	if(!bStdout) fprintf(stderr,"<filename>=%s\n",filename.c_str());

	uint8_t result[PACKET_SIZE*PACKETS_PER_FRAME];
	uint16_t *frameBuffer;

	//open spi port
	SpiOpenPort(0);
	if(!bStdout) fprintf(stderr,"SPI opened\n");

	int loop=0;
	while(true) {

		if(!bStdout) fprintf(stderr,"<");
		//read data packets from lepton over SPI
		int resets = 0;
		for(int j=0;j<PACKETS_PER_FRAME;j++) {
			//if it's a drop packet, reset j to 0, set to -1 so he'll be at 0 again loop
			read(spi_cs0_fd, result+sizeof(uint8_t)*PACKET_SIZE*j, sizeof(uint8_t)*PACKET_SIZE);
			int packetNumber = result[j*PACKET_SIZE+1];
			if(packetNumber != j) {
				j = -1;
				resets += 1;
				usleep(1000);
				//Note: we've selected 750 resets as an arbitrary limit, since there should never be 750 "null" packets between two valid transmissions at the current poll rate
				//By polling faster, developers may easily exceed this count, and the down period between frames may then be flagged as a loss of sync
				if(resets == 750) {
					SpiClosePort(0);
					usleep(750000);
					SpiOpenPort(0);
				}
			}
		}
		if(!bStdout) fprintf(stderr,"1");
		if(resets >= 30) {
			if(!bStdout) fprintf(stderr,"done reading, resets: %d\n",resets);
		}

		frameBuffer = (uint16_t *)result;
		int row, column;
		uint16_t value;
		uint16_t minValue = 65535;
		uint16_t maxValue = 0;

		
		for(int i=0;i<FRAME_SIZE_UINT16;i++) {
			//skip the first 2 uint16_t's of every packet, they're 4 header bytes
			if(i % PACKET_SIZE_UINT16 < 2) {
				continue;
			}
			
			//flip the MSB and LSB at the last second
			int temp = result[i*2];
			result[i*2] = result[i*2+1];
			result[i*2+1] = temp;
			
			value = frameBuffer[i];
			if(value > maxValue) {
				maxValue = value;
			}
			if(value < minValue) {
				minValue = value;
			}
			column = i % PACKET_SIZE_UINT16 - 2;
			row = i / PACKET_SIZE_UINT16 ;
		}
		if(!bStdout) fprintf(stderr,"2");

		float diff = maxValue - minValue;
		float scale = 255/diff;
		//QRgb color;

		bmpfile_t *bmp = NULL;
		bmp = bmp_create(80,60,24);
		rgb_pixel_t pixel = {0,0,0,0};

		for(int i=0;i<FRAME_SIZE_UINT16;i++) {
			if(i % PACKET_SIZE_UINT16 < 2) {
				continue;
			}
			value = (frameBuffer[i] - minValue) * scale;
			const int *colormap = colormap_ironblack;
			//color = qRgb(colormap[3*value], colormap[3*value+1], colormap[3*value+2]);
			column = (i % PACKET_SIZE_UINT16 ) - 2;
			row = i / PACKET_SIZE_UINT16;
			//myImage.setPixel(column, row, color);

			pixel.red = colormap[3*value+0];
			pixel.green = colormap[3*value+1];
			pixel.blue = colormap[3*value+2];
			bmp_set_pixel(bmp,column,row,pixel);
		}
		if(bStdout){
			dump(bmp,format,stdout);
		}else{
			char bmpFile[256];
			sprintf(bmpFile,"%s-%d.%s",filename.c_str(),loop,(format=='r')?"raw":"bmp");
			FILE *fp = fopen(bmpFile,"wb");
			dump(bmp,format,fp);
			fclose(fp);
		}
		bmp_destroy(bmp);
		if(!bStdout) fprintf(stderr,">");

		//lets emit the signal for update
		//emit updateImage(myImage);
		loop++;
	}
	//finally, close SPI port just bcuz
	SpiClosePort(0);
	if(!bStdout) fprintf(stderr,"SPI closed\n");

	if(!bStdout) fprintf(stderr,"program end\n");
	return 0;
}
