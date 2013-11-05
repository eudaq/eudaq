/*

Test programme to read Fortis data from file.
and write it to an output file one frame at a time.

D.Cussans, 27th April 09

 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#define WORDS_IN_HEADER 2

int main ( int argc,
	   const char *argv[]) {

  /*  int pixels_per_row = 128; */
  /* int rows_per_frame = 512; */

  if (argc != 5) { 
    printf("Usage: %s INPUT OUTPUT TRIGGERPROB FRAMEINTERVAL\n" , argv[0] );
    exit(1);
 }

  int pixels_per_row = 64; 
  int rows_per_frame = 64;

  int words_per_row = pixels_per_row + WORDS_IN_HEADER;
  int words_per_frame = words_per_row * rows_per_frame;

  FILE *inputFilePtr;
  FILE *outputFilePtr;

  unsigned short *frame_buffer = malloc ( sizeof(short)*words_per_frame ) ; /* Pointer to the frame buffer */

  unsigned short frame;

  int word_counter;

  int row_counter = 0;
  int frame_counter = 0;
 
  int n;

  int nitems;

  int triggers_in_frame ;

  unsigned int random_trigger_cut;

  char *endp;
  double trigger_cut_float = strtod(argv[3], &endp);

  long int frame_interval = strtol(argv[4], &endp , 0 );

  int short_size = sizeof(short);

  random_trigger_cut = RAND_MAX * trigger_cut_float / rows_per_frame ;

  printf("Input file = %s\n" , argv[1] );
  printf("Output file = %s\n" , argv[2] );
  printf("Rows, columns = %i  %i \n" , rows_per_frame , pixels_per_row );
  printf("Frame interval = %i mico-seconds \n" , frame_interval );
  printf("trigger cut ( string , float , int )= %s %f %i \n" , argv[3] , trigger_cut_float , random_trigger_cut );

  printf("Words per frame = %i\n", words_per_frame);
  printf("Size of short = %i\n", short_size);

  if ( frame_buffer == 0 ) {
    printf("Error allocating memory\n");
    return 2;
  }
  
  inputFilePtr = fopen(argv[1], "rb+"); /* need extra b+ under MinGW ....*/
  if (inputFilePtr == NULL )             /* Could not open file */
    {
      printf("Error opening input file %s: code = (%u)\n", argv[1], errno); 
      return 1;
    }

  outputFilePtr = fopen(argv[2], "w");
  if (outputFilePtr == NULL )             /* Could not open file */
    {
      printf("Error opening output file %s: code = (%u)\n", argv[2], errno); 
      return 1;
    }

  while ( ! feof(inputFilePtr) ) {
    

    printf("Frame = %d\n", frame_counter++);

    n =  fread(frame_buffer, sizeof(short), words_per_frame , inputFilePtr) ;

    if ( ferror( inputFilePtr )) {
      printf("Error reading: %s (%u)\n", strerror(errno), errno);
      return 1;
    }

    printf("Read %d words\n",n);

    frame = frame_buffer[0];
    printf("Frame counter from data = %d\n\n\n", frame);

    triggers_in_frame = 0;

    for (row_counter=0; row_counter < rows_per_frame; row_counter++) {

#ifdef DEBUG
      printf("Row = %d\n\n", row_counter);
#endif

      for (word_counter =0; word_counter < words_per_row; word_counter++) {
#ifdef DEBUG
	printf( "%04x\t",frame_buffer[word_counter + (words_per_row)*row_counter ]);
#endif


      }

      if (rand() < random_trigger_cut ) {
	frame_buffer[(words_per_row)*row_counter +1 ] = 1;
	printf("Added a trigger to row %i\n", row_counter);
	triggers_in_frame++;
      }


#ifdef DEBUG
      printf("\n\n\n");
#endif      

    }


    printf("Number of triggers in frame %i is %i \n", frame , triggers_in_frame );

    nitems = fwrite( frame_buffer ,  sizeof(short), words_per_frame , outputFilePtr );

    printf("Wrote %i shorts\n",nitems);

    usleep( frame_interval);

  }
    fclose(inputFilePtr);
    free(frame_buffer);
    return;
}
