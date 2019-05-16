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
#include <ctype.h>
#include "utility.h"
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
/* Put any symbolic constants (defines) here */
#define True 1  /* C has no booleans! */
#define False 0
#define MAX_CMD 80




//variables to keep track of the current path
char** path = NULL;
int pathNum= 0;
//file pointer to disk
int fd=0;
//pwd cluster and location
uint32_t pwd = 0;
int  pwdClustNum = 2;

//read helper functions
uint16_t readFromDisk16(int offset);
uint32_t readFromDisk32(int offset);
uint32_t getFatEntry(int n);
uint8_t* readFromDisk(int offset,int byteNum);


//other functions
void ls(int dirName);
void cd(char* dir);
void readFile(char* FILE_NAME, int POSITION, int NUM_BYTES);
void size(char* file);
void statf(char* c);


/* This is the main function of your project, and it will be run
 * first before all other functions.
 */
int main(int argc, char *argv[])
{
    //set up path and pwd
    path= malloc(sizeof(char*)*20);
	
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

    pwd = root_directory;


  	/* Swap bytes if the host is big endian */
    if(little_endian==0){
        //flip endianness
        BPB_BytesPerSec = swapEndian16(BPB_BytesPerSec);
        BPB_RsvdsSecCnt = swapEndian16(BPB_RsvdsSecCnt);
        BPB_FATSz32 = swapEndian32(BPB_FATSz32);
        BPB_RootEntCnt = swapEndian16(BPB_RootEntCnt);
    }
    


	while(True) {
		bzero(cmd_line, MAX_CMD);
        
        //print out path and update path info
        if(pathNum>0)
            {if(!strcmp_ign_ws(path[pathNum-1],".."))pathNum-=2;}
        if(pathNum<0)pathNum=0;
        printf("/");
        for(int i =0;i<pathNum;i++)printf("%s/",trimwhitespace(path[i]));
        printf("]");
        if(pathNum<0)pathNum=0;

        //read in prompt
        fgets(cmd_line,MAX_CMD,stdin);
        //replace . if not beginning
        if(cmd_line[strlen(cmd_line)-5]=='.') cmd_line[strlen(cmd_line)-5]='$';
		
        
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

        //prints out name of volume
		else if(strncmp(cmd_line,"volume",6)==0) {
			//need to change this to malloc so it can work for any volume name
			char volumeID[9];
            lseek(fd, root_directory, SEEK_SET);
			read(fd,&volumeID,8);
			printf("%s\n",volumeID);

		}
		
        //print out stats about file
        else if (strncmp(cmd_line,"stat",4)==0){
            //split string
            struct list* l = split(cmd_line," ");
            if(l->size ==2)
                statf(l->array[1]);
            else
                printf("wrong args\n");
		}

        //print out size of file
		else if(strncmp(cmd_line,"size",4)==0) {
            //split
            struct list* l = split(cmd_line," ");
            if(l->size ==2)
                size(l->array[1]);
            else
            printf("wrong args\n");
		}

        //change directory
		else if(strncmp(cmd_line,"cd",2)==0) {
            //split
            struct list* l = split(cmd_line," ");
            if(l->size ==2)
                cd(l->array[1]);
            else
                printf("wrong args\n");
            
            //special case when a .. is done into the root, change clust from 0 to 2
            if(pwdClustNum==0){
                pwdClustNum=2;
                pwd=root_directory;
            }
		}

        //list files in directory
		else if(strncmp(cmd_line,"ls",2)==0) {
			ls(pwd);
		}

        //read from a file
		else if(strncmp(cmd_line,"read",4)==0) {
            //split
            struct list* l = split(cmd_line," ");
            if(l->size ==4){        
                //replace . from string processing
                if(l->array[1][strlen(l->array[1])-4]=='.') l->array[1][strlen(l->array[1])-4]='$';
                readFile(l->array[1],atoi(l->array[2]),atoi(l->array[3]));
            }else
                printf("wrong args\n");
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


void size(char* file){
  //find file
  char* name = malloc(sizeof(char)*12);
  name[11] = 0;

      uint8_t* entry;
      for(int i = 0;i<BPB_BytesPerSec/32 ;i++){
          entry = readFromDisk(pwd+(i*32),32);
          char att = entry[11]&63;
          if((att!=0x10)&&
              (att!=0x20)&&
              (att!=0x20)&&
              (att!=0x20))continue;
          if( entry[0] == 0xE5) continue;
          if( entry[0] == 0x00) {break;}
          memcpy(name,entry,11);
          if(strcmp_ign_ws(file,name)==0){
              //read attribute
              if(att==0x10){
                  printf("Error: not a file\n");return;
              }
              printf("%i\n",*(int*)(entry+28));
              return;
          }
      }

}

//returns an array of strings, including . and ..
void ls(int dir){
    char* name = malloc(sizeof(char)*9);
    char* ext = malloc(sizeof(char)*5);
    ext[0]= '.';
    ext+=1;
    name[8] = '\0';
    ext[3]= '\0';
        uint8_t* entry;
        for(int i = 0;i<BPB_BytesPerSec*BPB_SecPerClus/32 ;i++){
            entry = readFromDisk(pwd+(i*32),32);
	    char att = entry[11]&63;
	    if((att!=0x10)&&
	    (att!=0x20)&&
	    (att!=0x20)&&
	    (att!=0x20))continue;
	    if( entry[0] == 0xE5) continue;
            if( entry[0] == 0x00) {break;}
            memcpy(name,entry,8);
            memcpy(ext,entry+8,3);
            if(ext[0]==' ')ext++;
	    printf("%s%s\n",trimwhitespace(name),ext-1);
	}
  int clustNum = pwdClustNum;
  while(getFatEntry(clustNum)!=0){
      clustNum = getFatEntry(clustNum);
      int loc = root_directory+ (clustNum-2)*BPB_BytesPerSec*BPB_SecPerClus;
      uint8_t* file = readFromDisk(loc,BPB_BytesPerSec*BPB_SecPerClus);
      for(int i = 0;i<BPB_BytesPerSec*BPB_SecPerClus/32 ;i++){
          entry = file+(i*32);
    char att = entry[11]&63;
    if((att!=0x10)&&
    (att!=0x20)&&
    (att!=0x20)&&
    (att!=0x20))continue;
    if( entry[0] == 0xE5) continue;
          if( entry[0] == 0x00) {break;}
          memcpy(name,entry,8);
          memcpy(ext,entry+8,3);
            if(ext[0]==' ')ext++;
          printf("%s%s\n",trimwhitespace(name),ext-1);
}

  }

}

void cd(char* dir){
    //search pwd
    char* name = malloc(sizeof(char)*12);
    name[11] = 0;

        uint8_t* entry;
        for(int i = 0;i<BPB_BytesPerSec/32 ;i++){
            entry = readFromDisk(pwd+(i*32),32);
	    char att = entry[11]&63;
	    if((att!=0x10)&&
	    (att!=0x20)&&
	    (att!=0x20)&&
	    (att!=0x20))continue;
	    if( entry[0] == 0xE5) continue;
            if( entry[0] == 0x00) {break;}
            memcpy(name,entry,11);

            if(strcmp_ign_ws(dir,name)==0){
                 //read attribute
            if(att!=0x10){
                printf("Error: not a directory\n");return;
            }
		int clustNum = entry[26]| (entry[20]<<16);
        pwd = root_directory+ (clustNum-2)*BPB_BytesPerSec*BPB_SecPerClus;
        pwdClustNum=clustNum;
        path[pathNum++] = name;
        return;
        }
	}

  int clustNum = pwdClustNum;
  while(getFatEntry(clustNum)!=0){
      clustNum = getFatEntry(clustNum);
      int loc = root_directory+ (clustNum-2)*BPB_BytesPerSec*BPB_SecPerClus;
      uint8_t* file = readFromDisk(loc,BPB_BytesPerSec*BPB_SecPerClus);
      for(int i = 0;i<BPB_BytesPerSec*BPB_SecPerClus/32 ;i++){
          entry = file+(i*32);
    char att = entry[11]&63;
    if((att!=0x10)&&
    (att!=0x20)&&
    (att!=0x20)&&
    (att!=0x20))continue;
    if( entry[0] == 0xE5) continue;
          if( entry[0] == 0x00) {break;}
          memcpy(name,entry,11);


          if(strcmp_ign_ws(dir,name)==0){
               //read attribute
          if(att!=0x10){
              printf("Error: not a directory\n");return;
          }
      int clustNum = entry[26]| (entry[20]<<16);
      path[pathNum++] = name;
      pwd = root_directory+ (clustNum-2)*BPB_BytesPerSec*BPB_SecPerClus;
      pwdClustNum=clustNum;
      return;
      }

}

  }


	printf("Error: does not exist\n");
}

void readFile(char* FILE_NAME, int POSITION, int NUM_BYTES){
    //find file
    //allocate space to store name
    char* name = malloc(sizeof(char)*12);
    name[11] = 0;
    uint8_t* entry;
        for(int i = 0;i<BPB_BytesPerSec/32 ;i++){
            entry = readFromDisk(pwd+(i*32),32);
            char att = entry[11]&63;
            if((att!=0x10)&&
                (att!=0x20)&&
                (att!=0x20)&&
                (att!=0x20))continue;
            if( entry[0] == 0xE5) continue;
            if( entry[0] == 0x00) {break;}
            memcpy(name,entry,11);
            if(strcmp_ign_ws(FILE_NAME,name)==0){
                //read attribute
                if(att==0x10){
                    printf("Error: not a file\n");return;
                }
                int clustNum = entry[26]| (entry[20]<<16);
                int loc = root_directory+ (clustNum-2)*BPB_BytesPerSec*BPB_SecPerClus;
                uint8_t* file = readFromDisk(loc +POSITION,MIN(NUM_BYTES,BPB_BytesPerSec*BPB_SecPerClus-POSITION));
                printf("%s",file);
                NUM_BYTES -= BPB_BytesPerSec*BPB_SecPerClus-POSITION;
                while(getFatEntry(clustNum)!=0){
                    if(NUM_BYTES<=0) break;
                    clustNum = getFatEntry(clustNum);
                    loc = root_directory+ (clustNum-2)*BPB_BytesPerSec*BPB_SecPerClus;
                    file = readFromDisk(loc,MIN(NUM_BYTES,BPB_BytesPerSec*BPB_SecPerClus));
                    NUM_BYTES -= BPB_BytesPerSec*BPB_SecPerClus;

                    printf("%s",file);

                }

                return;
            }
        }

        	printf("Error: does not exist\n");
}

//utility method for use in statf
void printEntry(uint8_t* entry){
    printf("size: %i\n",*(int*)(entry+28));
    printf("attr: 0x%x\n",entry[11]);
    printf("clust: 0x%x\n",entry[26]| (entry[20]<<16));
}

//finds the specified entry, and passes it to print entry to do the requisite IO
void statf(char* fileName){
    //search pwd
    char* name = malloc(sizeof(char)*12);
    name[11] = '\0';

        uint8_t* entry;
        for(int i = 0;i<BPB_BytesPerSec/32 ;i++){
            entry = readFromDisk(pwd+(i*32),32);
	    char att = entry[11]&63;
	    if((att!=0x10)&&
	    (att!=0x20)&&
	    (att!=0x20)&&
	    (att!=0x20))continue;
	    if( entry[0] == 0xE5) continue;
            if( entry[0] == 0x00) {break;}
            memcpy(name,entry,11);

            if(strcmp_ign_ws(fileName,name)==0){
                 //read attribute
            printEntry(entry);
        return;
        }
	}

  int clustNum = pwdClustNum;
  while(getFatEntry(clustNum)!=0){
      clustNum = getFatEntry(clustNum);
      int loc = root_directory+ (clustNum-2)*BPB_BytesPerSec*BPB_SecPerClus;
      uint8_t* file = readFromDisk(loc,BPB_BytesPerSec*BPB_SecPerClus);
      for(int i = 0;i<BPB_BytesPerSec*BPB_SecPerClus/32 ;i++){
          entry = file+(i*32);
    char att = entry[11]&63;
    if((att!=0x10)&&
    (att!=0x20)&&
    (att!=0x20)&&
    (att!=0x20))continue;
    if( entry[0] == 0xE5) continue;
          if( entry[0] == 0x00) {break;}
          memcpy(name,entry,11);


          if(strcmp_ign_ws(fileName,name)==0){
               //read attribute
          printEntry(entry);
      return;
      }

}

  }


	printf("Error: does not exist\n");




}





uint32_t getFatEntry(int n){
    //assume for now this is FAT32
    int offset = n*4;
    int secNum = BPB_RsvdsSecCnt + (offset/BPB_BytesPerSec);
    offset = offset % BPB_BytesPerSec;

    //read from disk
    //we didnt really need sector computatoin above since this isnt really a disk, but it just felt right in the spirit of the project
    return readFromDisk32(secNum*BPB_BytesPerSec+offset)& 0x0FFFFFFF;
}

uint32_t readFromDisk32(int offset){
    uint32_t* value = malloc(4);
	int result;

	result = lseek(fd, offset, SEEK_SET);
	if(result == -1){
    	perror("lseek");
    	close(fd);
    	return -1;
  	}

  	result = read(fd,value,4);

  	if(result== -1) {
    	perror("read");
    	close(fd);
    	return -1;
  	}
  	return *value;
}

uint16_t readFromDisk16(int offset){
    uint16_t* value= malloc(2);
	int result;
	result = lseek(fd, offset, SEEK_SET);
	if(result == -1){
    	perror("lseek");
    	close(fd);
    	return -1;
  	}

  	result = read(fd,value,2);

  	if(result== -1) {
    	perror("read");
    	close(fd);
    	return -1;
  	}

  	return *value;
}

//read specified amount from disk, with error checking
uint8_t* readFromDisk(int offset,int byteNum){
  uint8_t* value= calloc(byteNum+1,1);
	int result;
	result = lseek(fd, offset, SEEK_SET);
	if(result == -1){
    	perror("lseek");
    	close(fd);
    	return NULL;
  	}

  	result = read(fd,value,byteNum);

  	if(result== -1) {
    	perror("read");
    	close(fd);
    	return NULL;
  	}
    value[byteNum]='\0';
  	return value;
}
