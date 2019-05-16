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
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
//path
char** path = NULL;
int pathNum= 0;
//dynamic string list functionality via these utility methods
struct list{
    char** array;
    int size;
    int capacity;
};
//taken from stack overflow https://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing-whitespace-in-a-standard-way
char *trimwhitespace(char *str)
{
  char *end;

  // Trim leading space
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator character
  end[1] = '\0';

  return str;
}


//taken from stack overflow https://stackoverflow.com/questions/20016953/c-how-to-compare-strings-with-pointers-ignoring-whitespaces
int strcmp_ign_ws(const char *s1, const char *s2) {
  const char *p1 = s1, *p2 = s2;

  while (1) {
    while (*p1 != '\0' && (isspace((unsigned char)*p1))||(*p1)=='$') p1++;
    while (*p2 != '\0' && (isspace((unsigned char)*p2))||(*p2)=='$') p2++;
    if (*p1 == '\0' || *p2 == '\0') {
      return (*p2 == '\0') - (*p1 == '\0');
    }
    if (*p1 != *p2) {
      return (unsigned char)*p2 - (unsigned char)*p1;
    }
    p1++;
    p2++;
  }
}


void doubleList(struct list* l){
    l->array = realloc(l->array,l->capacity*2*sizeof(char*));
    l->capacity *=2;
    if(l==NULL){printf("ERROR: memory can't be allocated");exit(1);}
}

struct list* newList(){
    struct list* l = (struct list*)malloc(sizeof(struct list));
    l->array = (char**) malloc(2*sizeof(char*));
    l->size=0;
    l->capacity=2;
    return l;
}

void add(struct list* l,char* string){
    if(l->size==l->capacity){
        doubleList(l);
    }
    l->array[l->size++] = string;
}

char* get(struct list* l,int i){
    return l->array[i];
}

void freeList(struct list* l){
    free(l->array);
    free(l);
}

char* cpyString(char* string){
    return strcpy(malloc(sizeof(char)*(strlen(string)+1)),string);
}

struct list* split(char* string,char* delim){
    struct list* l = newList();
    char* token;
    token =  strtok (string,delim);
    while (token != NULL){
        add(l,cpyString(token));
        token = strtok(NULL,delim);
    }
    return l;
}



/* Put any symbolic constants (defines) here */
#define True 1  /* C has no booleans! */
#define False 0
#define MAX_CMD 80

//file pointer to disk
int fd=0;

//stats for info and root
uint16_t BPB_BytesPerSec=0;
uint8_t BPB_SecPerClus=0;
uint16_t BPB_RsvdsSecCnt=0;
uint8_t BPB_NumFATs=0;
uint32_t BPB_FATSz32=0;
uint16_t BPB_RootEntCnt=0;
uint32_t root_directory = 0;
uint32_t pwd = 0;
int  pwdClustNum = 2;
//declaring info functions here
uint16_t BytesPerSec(int fd);
uint8_t SecPerClus(int fd);
uint16_t RsvdsSecCnt(int fd);
uint8_t NumFATs (int fd);
uint32_t FATSz32(int fd);
uint16_t RootEntCnt(int fd);

//declaring root info functions here
uint32_t RootDirSectors();
uint32_t FirstDataSector();
uint32_t FirstSectorofCluster(uint32_t n);
uint32_t RootDir();

//swap endian functions
uint32_t  swapEndian32(uint32_t num);
uint16_t  swapEndian16(uint16_t num);

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


  	/* Swap bytes if the host is big endian */
    if(little_endian==0){
        //flip endianness
        BPB_BytesPerSec = swapEndian16(BPB_BytesPerSec);
        BPB_RsvdsSecCnt = swapEndian16(BPB_RsvdsSecCnt);
        BPB_FATSz32 = swapEndian32(BPB_FATSz32);
        BPB_RootEntCnt = swapEndian16(BPB_RootEntCnt);
    }
    	pwd = root_directory;

	while(True) {
		bzero(cmd_line, MAX_CMD);
    if(pathNum>0)
    {if(!strcmp_ign_ws(path[pathNum-1],".."))pathNum-=2;}
    if(pathNum<0)pathNum=0;

		printf("/");
    for(int i =0;i<pathNum;i++)printf("%s/",trimwhitespace(path[i]));
    printf("]");
    if(pathNum<0)pathNum=0;

		fgets(cmd_line,MAX_CMD,stdin);
    //replace . if not begingnig
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


		else if(strncmp(cmd_line,"volume",6)==0) {

			//need to change this to malloc so it can work for any volume name
			char volumeID[9];
            lseek(fd, root_directory, SEEK_SET);
			read(fd,&volumeID,8);
			printf("%s\n",volumeID);

		}
		else if (strncmp(cmd_line,"stat",4)==0){
      struct list* l = split(cmd_line," ");
      if(l->size ==2)
       statf(l->array[1]);
      else
       printf("wrong args\n");
		}

		else if(strncmp(cmd_line,"size",4)==0) {
      struct list* l = split(cmd_line," ");
      if(l->size ==2)
       size(l->array[1]);
      else
       printf("wrong args\n");
		}

		else if(strncmp(cmd_line,"cd",2)==0) {
      struct list* l = split(cmd_line," ");
      if(l->size ==2)
       cd(l->array[1]);
      else
       printf("wrong args\n");
       if(pwdClustNum==0){
         pwdClustNum=2;
         pwd=root_directory;
       }
		}

		else if(strncmp(cmd_line,"ls",2)==0) {
			ls(pwd);
		}

		else if(strncmp(cmd_line,"read",4)==0) {

			     struct list* l = split(cmd_line," ");
           if(l->size ==4)
            readFile(l->array[1],atoi(l->array[2]),atoi(l->array[3]));
           else
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

// this field contains the count of 32 byte directory entries in the root directory.
// For FAT32 volumes, this field must be set to 0.
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

void printEntry(uint8_t* entry){
//size
printf("size:%i\n",*(int*)(entry+28));
printf("attr:%x\n",entry[11]);
printf("clust:%x\n",entry[26]| (entry[20]<<16));
}

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

	/* convert endian-ness */
}


//gives me the first sector of a given cluster
uint32_t FirstSectorofCluster(uint32_t n){
	uint32_t first_sector_of_cluster;
 	first_sector_of_cluster = ((n - 2) * BPB_SecPerClus) + FirstDataSector();
 	return first_sector_of_cluster;

 	/* convert endian-ness */
}


//Returns the address of the root directory
uint32_t RootDir(){
	uint32_t root_Address = (FirstDataSector() * BPB_BytesPerSec);
	return root_Address;

	/* convert endian-ness */
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
