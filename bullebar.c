#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <jack/jack.h>
#include <jack/midiport.h>
#include "wave.h"

unsigned char buffer4[4];
unsigned char buffer2[2];

FILE *ptr;

struct HEADER kick1;
struct HEADER kick2;
struct HEADER snare1;
struct HEADER snare2;

int standardSeq[12][24] = 	{{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
							{1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0},
							{1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0},
							{1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0},
							{1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0},
							{0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0},
							{0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0},
							{1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0},
							{1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0},
							{1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0},
							{1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0},
							{1,0,0,1,0,0,1,0,0,1,0,0,1,0,0,1,0,0,1,0,0,1,0,0}};

int note;
int tenuto = 0;
int stepper = 0;
int loopSize = 0;
int pas = 0;
int lastPas = 0;
int tempo = 87;

jack_port_t* input_port;
jack_port_t* output_port_left;
jack_port_t* output_port_right;

int init(struct HEADER *header);
int process(jack_nframes_t nframes, void*arg);

int main(int argc, char **argv){

 // get file path (current working directory)
 char cwd[1024];
 if (getcwd(cwd, sizeof(cwd)) != NULL) {
	strcat(cwd, "/");

	// get filename from command line
	if (argc < 2) {
	  printf("aide\n");
	  return(1);
	}else if(argc > 1){
		kick1.filename = (char*) malloc(sizeof(char) * 1024);
		if (kick1.filename == NULL) {
			printf("Error in malloc\n");
			exit(1);
			}
		strcpy(kick1.filename,cwd);
		strcat(kick1.filename, argv[1]);
		}
	if(argc > 2){
		kick2.filename = (char*) malloc(sizeof(char) * 1024);
		if (kick2.filename == NULL) {
			printf("Error in malloc\n");
			exit(1);
			}
		strcpy(kick2.filename,cwd);
		strcat(kick2.filename, argv[2]);
		}
	if(argc > 3){
		snare1.filename = (char*) malloc(sizeof(char) * 1024);
		if (snare1.filename == NULL) {
			printf("Error in malloc\n");
			exit(1);
			}
		strcpy(snare1.filename,cwd);
		strcat(snare1.filename, argv[3]);
		}
	if(argc > 4){
		snare2.filename = (char*) malloc(sizeof(char) * 1024);
		if (snare2.filename == NULL) {
			printf("Error in malloc\n");
			exit(1);
			}
		strcpy(snare2.filename,cwd);
		strcat(snare2.filename, argv[4]);
		}
	}


init(&kick1);
init(&snare1);
init(&kick2);
init(&snare2);
//init(hihat);

    jack_options_t options = JackNullOption;
    jack_status_t status;

    /* Ouvrir le client JACK */
    jack_client_t* client = jack_client_open("bullebar", options, &status);

    /* Ouvrir les ports en entrée et en sortie */
    output_port_left = jack_port_register(client, "output_l", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
   // output_port_right = jack_port_register(client, "output_r", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
	input_port = jack_port_register (client, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
	
    /* Enregister le traitement qui sera fait à chaque cycle */
    jack_set_process_callback(client, process, NULL);

    jack_activate(client);

    /* Fonctionne jusqu'à ce que Ctrl+C soit utilisé */
    printf("Utiliser 'q' pour quitter l'application...\n");

    char c;

    while ((c = getchar()) != 'q') {
        sleep(1);
    }

    jack_deactivate(client);
    jack_client_close(client);
    
    return 0;
}


int init(struct HEADER *header){
	
	printf("Opening  file..\n");
		ptr = fopen(header->filename, "rb");
	 if (ptr == NULL) {
		printf("Error opening file\n");
		exit(1);
	 }
	 
	 int read = 0;
	 
	 read = fread(header->riff, sizeof(header->riff), 1, ptr);	
	 read = fread(buffer4, sizeof(buffer4), 1, ptr);	 
	 // convert little endian to big endian 4 byte int
	 header->overall_size  = buffer4[0] | 
							(buffer4[1]<<8) | 
							(buffer4[2]<<16) | 
							(buffer4[3]<<24);
	 read = fread(header->wave, sizeof(header->wave), 1, ptr);	
	 read = fread(header->fmt_chunk_marker, sizeof(header->fmt_chunk_marker), 1, ptr);
	 read = fread(buffer4, sizeof(buffer4), 1, ptr);
	 // convert little endian to big endian 4 byte integer
	 header->length_of_fmt = buffer4[0] |
								(buffer4[1] << 8) |
								(buffer4[2] << 16) |
								(buffer4[3] << 24);
	 read = fread(buffer2, sizeof(buffer2), 1, ptr);
	 
	 header->format_type = buffer2[0] | (buffer2[1] << 8);
	 char format_name[10] = "";
	 if (header->format_type == 1)
	   strcpy(format_name,"PCM"); 
	 else if (header->format_type == 6)
	  strcpy(format_name, "A-law");
	 else if (header->format_type == 7)
	  strcpy(format_name, "Mu-law");
	
	 read = fread(buffer2, sizeof(buffer2), 1, ptr);	
	 header->channels = buffer2[0] | (buffer2[1] << 8);	
	 read = fread(buffer4, sizeof(buffer4), 1, ptr);	
	 header->sample_rate = buffer4[0] |
							(buffer4[1] << 8) |
							(buffer4[2] << 16) |
							(buffer4[3] << 24);	
	 read = fread(buffer4, sizeof(buffer4), 1, ptr);	
	 header->byterate  = buffer4[0] |
							(buffer4[1] << 8) |
							(buffer4[2] << 16) |
							(buffer4[3] << 24);	
	 read = fread(buffer2, sizeof(buffer2), 1, ptr);	
	 header->block_align = buffer2[0] |
						(buffer2[1] << 8);	
	 read = fread(buffer2, sizeof(buffer2), 1, ptr);	
	 header->bits_per_sample = buffer2[0] |
						(buffer2[1] << 8);
	 read = fread(header->data_chunk_header, sizeof(header->data_chunk_header), 1, ptr);	
	 read = fread(buffer4, sizeof(buffer4), 1, ptr);
	 header->data_size = buffer4[0] |
					(buffer4[1] << 8) |
					(buffer4[2] << 16) | 
					(buffer4[3] << 24 );
	
	 // calculate no.of samples
	 long num_samples = (8 * header->data_size) / (header->channels * header->bits_per_sample);
	header->size = num_samples;
	
	 long size_of_each_sample = (header->channels * header->bits_per_sample) / 8;	 
	
	 // read each sample from data chunk if PCM
	 if (header->format_type == 1) { // PCM 
			long i =0;
			char data_buffer[size_of_each_sample];
			int  size_is_correct = 1;
	
			// make sure that the bytes-per-sample is completely divisible by num.of channels
			long bytes_in_each_channel = (size_of_each_sample / header->channels);
			if ((bytes_in_each_channel  * header->channels) != size_of_each_sample) {
				printf("Error: %ld x %ud <> %ld\n", bytes_in_each_channel, header->channels, size_of_each_sample);
				size_is_correct = 0;
			}
	 
			if (size_is_correct) { 
						// the valid amplitude range for values based on the bits per sample
				long low_limit = 0l;
				long high_limit = 0l;
	
				switch (header->bits_per_sample) {
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
	
				printf("\n\n.Valid range for data values : %ld to %ld \n", low_limit, high_limit);
				for (i =1; i <= num_samples; i++) {
					read = fread(data_buffer, sizeof(data_buffer), 1, ptr);
					if (read == 1) {
					
						// dump the data read
						unsigned int  xchannels = 0;
						int data_in_channel = 0;
	
						for (xchannels = 0; xchannels < header->channels; xchannels ++ ) {
							// convert data from little endian to big endian based on bytes in each channel sample
							if (bytes_in_each_channel == 4) {
								data_in_channel =	data_buffer[0+(4*xchannels)] | 
													(data_buffer[1+(4*xchannels)]<<8) | 
													(data_buffer[2+(4*xchannels)]<<16) | 
													(data_buffer[3+(4*xchannels)]<<24);					
							}
							else if (bytes_in_each_channel == 2) {
								data_in_channel = data_buffer[0+(2*xchannels)] &255|
													(data_buffer[1+(2*xchannels)] << 8);					
							}
							else if (bytes_in_each_channel == 1) {
								data_in_channel = data_buffer[0+(1*xchannels)];
							}
							header->sample[i] = data_in_channel / (-1.0*low_limit);
							// check if value was in range
							if (data_in_channel < low_limit || data_in_channel > high_limit)
								printf("**value out of range\n");
						}
					}
					else {
						printf("Error reading file. %d bytes\n", read);
						break;
					}
	
				} // 	for (i =1; i <= num_samples; i++) {
	
			} // 	if (size_is_correct) {  
	 } //  if (header->format_type == 1) { 
	
	 printf("Closing file..\n");
	 fclose(ptr);
	
	  // cleanup before quitting
	 free(header->filename);
	 return 0;

}

int process(jack_nframes_t nframes, void*arg)
{    
	void* port_buf = jack_port_get_buffer(input_port, nframes);
	float* out1 = jack_port_get_buffer(output_port_left, nframes);
	//float* out2 = jack_port_get_buffer(output_port_right, nframes);

	int i;
	
	jack_default_audio_sample_t *outl = (jack_default_audio_sample_t *) jack_port_get_buffer (output_port_left, nframes);
	//jack_default_audio_sample_t *outr = (jack_default_audio_sample_t *) jack_port_get_buffer (output_port_right, nframes);
	
	jack_midi_event_t in_event;
	jack_nframes_t event_index = 0;
	jack_nframes_t event_count = jack_midi_get_event_count(port_buf);

	
		for(i=0; i<nframes; i++){
			int k;
			for (k=0;k<event_count+1;k++){
				jack_midi_event_get(&in_event, port_buf, k);
		
				if((in_event.time == i) && (event_index < event_count)){
					if( ((*(in_event.buffer) & 0xf0)) == 0x90 ){
				/* note on */
						note = *(in_event.buffer + 1);
						if(note<53){
							note = note%12;
							kick1.motif[note]= 1;
							}
						else if(note<60){
							note = note%12;
							kick2.motif[note]= 1;
							}
						else if(note<65){
							note = note%12;
							snare1.motif[note]= 1;
							}
						else {
							note = note%12;
							snare2.motif[note]= 1;
							}
						}
					
					else if( ((*(in_event.buffer)) & 0xf0) == 0x80 ){
				/* note off */
						note = *(in_event.buffer + 1);
						if(note<53){
							note = note%12;
							kick1.motif[note]= 0;
							}
						else if(note<60){
							note = note%12;
							kick2.motif[note]= 0;
							}
						else if(note<65){
							note = note%12;
							snare1.motif[note]= 0;
							}
						else {
							note = note%12;
							snare2.motif[note]= 0;
							}
						}
						
					if( ((*(in_event.buffer) & 0xf0)) == 0xC0 ){
				/* note on */
						tempo = *(in_event.buffer + 1);
						printf("%i bpm\n",tempo+53);
						}
					}
				}
			    /* Boucle de traitement sur les échantillons. */
			out1[i] =	kick1.sample[kick1.teteDeLecture]*(kick1.teteDeLecture!=-1) + kick2.sample[kick2.teteDeLecture]*(kick2.teteDeLecture!=-1) +
						snare1.sample[snare1.teteDeLecture]*(snare1.teteDeLecture!=-1) + snare2.sample[snare2.teteDeLecture]*(snare2.teteDeLecture!=-1);
			
			loopSize = 2*44100/((tempo+53)/60.0);
			stepper = (stepper + 1)%loopSize;
			
			pas = (int)(stepper*24/(float)loopSize);
			
			if((kick1.sequence[pas])&&(pas != lastPas)){
				kick1.teteDeLecture = 0;
				}
			if((kick2.sequence[pas])&&(pas != lastPas)){
				kick2.teteDeLecture = 0;
				}
			if((snare1.sequence[pas])&&(pas != lastPas)){
				snare1.teteDeLecture = 0;
				}
			if((snare2.sequence[pas])&&(pas != lastPas)){
				snare2.teteDeLecture = 0;
				}
			
			lastPas = pas;
			
			if ((kick1.teteDeLecture>=0)&&(kick1.teteDeLecture<kick1.size)){
				kick1.teteDeLecture = kick1.teteDeLecture + 1;
				}
			else if (kick1.teteDeLecture==kick1.size){
				kick1.teteDeLecture = -1;
				}
			if ((kick2.teteDeLecture>=0)&&(kick2.teteDeLecture<kick2.size)){
				kick2.teteDeLecture = kick2.teteDeLecture + 1;
				}
			else if (kick2.teteDeLecture==kick2.size){
				kick2.teteDeLecture = -1;
				}
			if ((snare1.teteDeLecture>=0)&&(snare1.teteDeLecture<snare1.size)){
				snare1.teteDeLecture = snare1.teteDeLecture + 1;
				}
			else if (snare1.teteDeLecture==snare1.size){
				snare1.teteDeLecture = -1;
				}
			if ((snare2.teteDeLecture>=0)&&(snare2.teteDeLecture<snare2.size)){
				snare2.teteDeLecture = snare2.teteDeLecture + 1;
				}
			else if (snare2.teteDeLecture==snare2.size){
				snare2.teteDeLecture = -1;
				}
				
		//	out2[i] =	0;
			for(k=0;k<24;k++){
				kick1.sequence[k]=0;
				kick2.sequence[k]=0;
				snare1.sequence[k]=0;
				snare2.sequence[k]=0;					
				}
			for(k=0;k<12;k++){
				if(kick1.motif[k]){
					for(int j=0;j<24;j++){
						kick1.sequence[j] = (kick1.sequence[j]+standardSeq[k][j])%2;
						}
					}
				if(kick2.motif[k]){
					for(int j=0;j<24;j++){
						kick2.sequence[j] = (kick2.sequence[j]+standardSeq[k][j])%2;
						}
					}
				if(snare1.motif[k]){
					for(int j=0;j<24;j++){
						snare1.sequence[j] = (snare1.sequence[j]+standardSeq[k][j])%2;
						}
					}
				if(snare2.motif[k]){
					for(int j=0;j<24;j++){
						snare2.sequence[j] = (snare2.sequence[j]+standardSeq[k][j])%2;
						}
					}
				}
			}
    return 0;      
}
