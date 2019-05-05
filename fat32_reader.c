/***********************************************************
 * Name of program:
 * Authors:
 * Description:
 **********************************************************/

/* These are the included libraries.  You may need to add more. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>


/* Put any symbolic constants (defines) here */
#define True 1  /* C has no booleans! */
#define False 0
#define MAX_CMD 80

uint16_t BPB_BytesPerSec=0;
uint8_t BPB_SecPerClus=0;
uint16_t BPB_RsvdsSecCnt=0;
uint8_t BPB_NumFATs=0;
uint32_t BPB_FATSz32=0;

uint16_t BytesPerSec(int fd);
uint8_t SecPerClus(int fd);
uint16_t RsvdsSecCnt(int fd);
uint8_t NumFATs (int fd);
uint32_t FATSz32(int fd);




/* This is the main function of your project, and it will be run
 * first before all other functions.
 */
int main(int argc, char *argv[])
{
	char cmd_line[MAX_CMD];
	int little_endian = 5;
	uint16_t convert = 0;

	//testing to see what machine architecture we have.
	if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__){
		little_endian=1; //host is little endian
	} else {
		little_endian = 0; // host is big endian
	}
	
	
	/* Parse args and open our image file */
	int fd;
	fd = open(argv[1],O_RDWR);
	
	/* Error checking for write. I/O functions tend to return
	   -1 if something is wrong. */
	if(fd == -1) {
	  perror(argv[1]);
	  return -1;
	}
	
  	BPB_BytesPerSec = BytesPerSec(fd);	
  	BPB_SecPerClus = SecPerClus(fd);
  	BPB_RsvdsSecCnt = RsvdsSecCnt(fd);
  	BPB_NumFATs = NumFATs(fd);
  	BPB_FATSz32= FATSz32(fd);
  	
	
	/* Parse boot sector and get information */

	/* Get root directory address */
	//printf("Root addr is 0x%x\n", root_addr);


	/* Main loop.  You probably want to create a helper function
       for each command besides quit. */

	while(True) {
		bzero(cmd_line, MAX_CMD);
		printf("/]");
		fgets(cmd_line,MAX_CMD,stdin);

		/* Start comparing input */
		if(strncmp(cmd_line,"info",4)==0) {
			printf("Going to display info.\n");
			printf("BPB_BytesPerSec is 0x%x, decimal: %i\n", BPB_BytesPerSec, BPB_BytesPerSec);
			printf("BPB_SecPerClus is 0x%x, decimal: %i\n", BPB_SecPerClus, BPB_SecPerClus);
			printf("BPB_RsvdsSecCnt is 0x%x, decimal: %i\n", BPB_RsvdsSecCnt, BPB_RsvdsSecCnt);
			printf("BPB_NumFATs is 0x%x, decimal: %i\n", BPB_NumFATs, BPB_NumFATs);
			printf("BPB_FATSz32 is 0x%x, decimal: %i\n", BPB_FATSz32, BPB_FATSz32);
			

		}

		else if(strncmp(cmd_line,"open",4)==0) {
			printf("Going to open!\n");
		}

		else if(strncmp(cmd_line,"close",5)==0) {
			printf("Going to close!\n");
		}
		
		else if(strncmp(cmd_line,"size",4)==0) {
			printf("Going to size!\n");
		}

		else if(strncmp(cmd_line,"cd",2)==0) {
			printf("Going to cd!\n");
		}

		else if(strncmp(cmd_line,"ls",2)==0) {
			printf("Going to ls.\n");
		}

		else if(strncmp(cmd_line,"read",4)==0) {
			printf("Going to read!\n");
		}
		
		else if(strncmp(cmd_line,"quit",4)==0) {
			printf("Quitting.\n");
			
			break;
		}
		else
			printf("Unrecognized command.\n");

	}
	/* Close the file */

	return 0; /* Success */
}


uint16_t BytesPerSec(int fd){

	uint16_t read_num;
	int result;
	result = lseek(fd, 11, 0);
	if(result == -1){
    	perror("lseek");
    	close(fd);
    	return -1;
  	}
  	
  	result = read(fd,&read_num,(sizeof(read_num)));
  	
  	if(result== -1) {
    	perror("read");
    	close(fd);
    	return -1;
  	}
  	
  	 // if(little_endian == 0){
//   		convert = (read_num>>8) | (read_num<<8);
// 		BPB_BytesPerSec = convert;
//   	}
  	
  	return read_num;
}

uint8_t SecPerClus(int fd){

	uint8_t read_num;
	int result;
	result = lseek(fd, 13, 0);
	if(result == -1){
    	perror("lseek");
    	close(fd);
    	return -1;
  	}
  	
  	result = read(fd,&read_num,(sizeof(read_num)));
	if(result== -1) {
    	perror("read");
    	close(fd);
    	return -1;
  	}
	return read_num;

}

uint16_t RsvdsSecCnt(int fd){

	uint16_t read_num;
	int result;
	result = lseek(fd, 14, 0);
	if(result == -1){
    	perror("lseek");
    	close(fd);
    	return -1;
  	}
  	
  	result = read(fd,&read_num,(sizeof(read_num)));
  	
  	if(result== -1) {
    	perror("read");
    	close(fd);
    	return -1;
  	}
  	
  	 // if(little_endian == 0){
//   		convert = (read_num>>8) | (read_num<<8);
// 		BPB_BytesPerSec = convert;
//   	}
  	
  	return read_num;
}

uint8_t NumFATs(int fd){

	uint8_t read_num;
	int result;
	result = lseek(fd, 16, 0);
	if(result == -1){
    	perror("lseek");
    	close(fd);
    	return -1;
  	}
  	
  	result = read(fd,&read_num,(sizeof(read_num)));
	if(result== -1) {
    	perror("read");
    	close(fd);
    	return -1;
  	}
	return read_num;
}

uint32_t FATSz32( int fd){
	uint32_t read_num;
	int result;
	result = lseek(fd, 36, 0);
	if(result == -1){
    	perror("lseek");
    	close(fd);
    	return -1;
  	}
  	
  	result = read(fd,&read_num,(sizeof(read_num)));
  	
  	if(result== -1) {
    	perror("read");
    	close(fd);
    	return -1;
  	}
  	
  	 // if(little_endian == 0){
//   		convert = (read_num>>8) | (read_num<<8);
// 		BPB_BytesPerSec = convert;
//   	}
  	
  	return read_num;
	

}

