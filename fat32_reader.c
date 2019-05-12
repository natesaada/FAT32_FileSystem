/***********************************************************
 * Name of program: fat32_reader
 * Authors: Natanel Saada and Yehuda Goldfeder
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

//stats for info
uint16_t BPB_BytesPerSec=0;
uint8_t BPB_SecPerClus=0;
uint16_t BPB_RsvdsSecCnt=0;
uint8_t BPB_NumFATs=0;
uint32_t BPB_FATSz32=0;
uint16_t BPB_RootEntCnt=0;
uint32_t root_directory = 0;


uint16_t BytesPerSec(int fd);
uint8_t SecPerClus(int fd);
uint16_t RsvdsSecCnt(int fd);
uint8_t NumFATs (int fd);
uint32_t FATSz32(int fd);
uint16_t RootEntCnt(int fd);

uint32_t RootDirSectors();
uint32_t FirstDataSector();
uint32_t FirstSectorofCluster(uint32_t n);
uint32_t RootDir();

//helper functions... taken from Microsoft Specs

uint32_t  swapEndian32(uint32_t num);
uint16_t  swapEndian16(uint16_t num);



/* Want to calculate the root directory in order to get the volume name */




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
	
	/* Getting information on BPB */
  	BPB_BytesPerSec = BytesPerSec(fd);	
  	BPB_SecPerClus = SecPerClus(fd);
  	BPB_RsvdsSecCnt = RsvdsSecCnt(fd);
  	BPB_NumFATs = NumFATs(fd);
  	BPB_FATSz32= FATSz32(fd);
  	BPB_RootEntCnt = RootEntCnt(fd);
  	root_directory = RootDir();
  	
  	
  	/* Swap bytes if the host is big endian */
    if(little_endian==0){
        //flip endianness
        BPB_BytesPerSec = swapEndian16(BPB_BytesPerSec);
        BPB_RsvdsSecCnt = swapEndian16(BPB_RsvdsSecCnt);
        BPB_FATSz32 = swapEndian32(BPB_FATSz32);
    }
    

	while(True) {
		bzero(cmd_line, MAX_CMD);
		printf("/]");
		fgets(cmd_line,MAX_CMD,stdin);

		/* Start comparing input */
		
		/* info prints out the information about BPB retrieved above */
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

		else if(strncmp(cmd_line,"volume",6)==0) {
			
			//need to change this to malloc so it can work for any volume name
			char volumeID[9];
            lseek(fd, root_directory, SEEK_SET);
			read(fd,&volumeID,8);
			printf("%s\n",volumeID);
			
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







/* get the amount of bytes per sector */
uint16_t BytesPerSec(int fd){

	uint16_t value;
	int result;
	result = lseek(fd, 11, 0);
	if(result == -1){
    	perror("lseek");
    	close(fd);
    	return -1;
  	}
  	
  	result = read(fd,&value,(sizeof(value)));
  	
  	if(result== -1) {
    	perror("read");
    	close(fd);
    	return -1;
  	}
  	return value;
}

/* Get sectors per clusters */
uint8_t SecPerClus(int fd){

	uint8_t value;
	int result;
	result = lseek(fd, 13, 0);
	if(result == -1){
    	perror("lseek");
    	close(fd);
    	return -1;
  	}
  	
  	result = read(fd,&value,(sizeof(value)));
	if(result== -1) {
    	perror("read");
    	close(fd);
    	return -1;
  	}
	return value;

}

/* Get the count of reserved sectors */
uint16_t RsvdsSecCnt(int fd){

	uint16_t value;
	int result;
	result = lseek(fd, 14, 0);
	if(result == -1){
    	perror("lseek");
    	close(fd);
    	return -1;
  	}
  	
  	result = read(fd,&value,(sizeof(value)));
  	
  	if(result== -1) {
    	perror("read");
    	close(fd);
    	return -1;
  	}
  	return value;
}

/* Get the count FAT data structures on the volume */
uint8_t NumFATs(int fd){

	uint8_t value;
	int result;
	result = lseek(fd, 16, 0);
	if(result == -1){
    	perror("lseek");
    	close(fd);
    	return -1;
  	}
  	
  	result = read(fd,&value,(sizeof(value)));
	if(result== -1) {
    	perror("read");
    	close(fd);
    	return -1;
  	}
	return value;
}

/* Get the FAT32 32-bit count of sectors occupied by ONE FAT */
uint32_t FATSz32( int fd){
	uint32_t value;
	int result;
	result = lseek(fd, 36, 0);
	if(result == -1){
    	perror("lseek");
    	close(fd);
    	return -1;
  	}
  	
  	result = read(fd,&value,(sizeof(value)));
  	
  	if(result== -1) {
    	perror("read");
    	close(fd);
    	return -1;
  	}

  	return value;
}

uint16_t RootEntCnt(int fd){

	uint16_t value;
	int result;
	result = lseek(fd, 17, 0);
	if(result == -1){
    	perror("lseek");
    	close(fd);
    	return -1;
  	}
  	
  	result = read(fd,&value,(sizeof(value)));
  	
  	if(result== -1) {
    	perror("read");
    	close(fd);
    	return -1;
  	}
  	return value;
}





/* Swap the bytes for 16 bit num */
uint16_t  swapEndian16(uint16_t num){
    uint16_t b0,b1;
    uint16_t res;

    b0 = (num & 0x00ff) << 8u;
    b1 = (num & 0xff00) >> 8u;


    res = b0 | b1;
    return res;
}

/* Swap the bytes for 32 bit num */
uint32_t  swapEndian32(uint32_t num){
    uint32_t b0,b1,b2,b3;
    uint32_t res;

    b0 = (num & 0x000000ff) << 24u;
    b1 = (num & 0x0000ff00) << 8u;
    b2 = (num & 0x00ff0000) >> 8u;
    b3 = (num & 0xff000000) >> 24u;

    res = b0 | b1 | b2 | b3;
    return res;
}


//returns an array of strings, including . and ..
char** getls(char* dirName){
    // @TODO add . and ..
    //   also use malloc and realloc for resizable arrays
}

//print out ls
void printLs(char* dirName){
    char** list = getls(dirName);
    int i = 0;
    while(list[i]!=NULL){
        printf("%s\n",list[i]);
        i++;
    }
}

void printStat(char* fileName){
    //get the info
    int size, clusterNum;
    char* attributes;
    //size
    printf("%d",size);
    //attributes
    printf("%s",attributes);
    //first cluster number
    printf("%d",clusterNum);
}


//print out the size of the file name
void size(char* fileName){

	//read filename
	//seek it till the end
	// save the value
	//reset the seek
	
	//corner case: check to see if the file is even valid

}



//for our purposes its always zero
uint32_t RootDirSectors(){
	uint32_t root_dir_sectors;
	root_dir_sectors = ((BPB_RootEntCnt * 32) + (BPB_BytesPerSec - 1)) / BPB_BytesPerSec;
	return root_dir_sectors;
	
	/* convert endian-ness */
}

//gives us the first data sector for root
uint32_t FirstDataSector(){
	uint32_t first_data_sector;
	first_data_sector = BPB_RsvdsSecCnt + (BPB_NumFATs * BPB_FATSz32) + RootDirSectors();
	return first_data_sector;
}


//gives me the first sector of a given cluster
uint32_t FirstSectorofCluster(uint32_t n){
	uint32_t first_sector_of_cluster;
 	first_sector_of_cluster = ((n - 2) * BPB_SecPerClus) + FirstDataSector();
 	return first_sector_of_cluster;
}


//Returns the address of the root directory 
uint32_t RootDir(){
	uint32_t root_Address = (FirstDataSector() * BPB_BytesPerSec);
	return root_Address;
}






