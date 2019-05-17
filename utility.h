#ifndef UTIL_H
#define UTIL_H

//Utility method I wrote for neatly splitting strings, and using array doubling for efficiency 
//via a list struct
struct list{
    char** array;
    int size;
    int capacity;
};

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

//functions to swap endianess
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


//general FAT utility functions
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

//stats for info and root
uint16_t BPB_BytesPerSec=0;
uint8_t BPB_SecPerClus=0;
uint16_t BPB_RsvdsSecCnt=0;
uint8_t BPB_NumFATs=0;
uint32_t BPB_FATSz32=0;
uint16_t BPB_RootEntCnt=0;
uint32_t root_directory = 0;



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








//general string parsing utilities
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
    while (*p1 != '\0' && (isspace((unsigned char)*p1)))p1++;
    while (*p2 != '\0' && (isspace((unsigned char)*p2))) p2++;
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











#endif /* UTIL_H */

