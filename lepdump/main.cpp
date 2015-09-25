#include <stdint.h>
#include <LEPTON_Types.h>
#include <SPI.h>
#include <Palettes.h>

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

int main(int argc,char* argv[])
{
	if(argc<3){
		usage();
		return -1;
	}

	FILE *fp=NULL;
	std::string filename=argv[2];
	if(filename!="-") fp = fopen(filename.c_str(),"wb");
	//fp!=NULL means stream goes to file, fp=NULL means stream goes to stdout

	char format=*argv[1];
	if(format!='r' && format!='b') {
		fprintf(stderr,"<format> %c not exist\n",format);
		usage();
		return -1;
	}

	if(fp) fprintf(stderr,"<format>=%c\n",format);
	if(fp) fprintf(stderr,"<filename>=%s\n",filename.c_str());

	uint8_t result[PACKET_SIZE*PACKETS_PER_FRAME];
	uint16_t *frameBuffer;

	//open spi port
	SpiOpenPort(0);
	if(fp) fprintf(stderr,"SPI opened\n");

	while(true) {

		if(fp) fprintf(stderr,"<");
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
		if(fp) fprintf(stderr,"1");
		if(resets >= 30) {
			if(fp) fprintf(stderr,"done reading, resets: %d\n",resets);
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
		if(fp) fprintf(stderr,"2");

		float diff = maxValue - minValue;
		float scale = 255/diff;
		//QRgb color;
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
		}
		if(fp) fprintf(stderr,">");

		//lets emit the signal for update
		//emit updateImage(myImage);
	}
	//finally, close SPI port just bcuz
	SpiClosePort(0);
	fclose(fp);
	if(fp) fprintf(stderr,"SPI closed\n");

	if(fp) fprintf(stderr,"program end\n");
	return 0;
}
