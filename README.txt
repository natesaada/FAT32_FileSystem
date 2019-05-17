Program Name: fat32_reader.c
Authors: Natanel Saada and Yehuda Goldfeder

In order to compile, you just need to be within the appropriate directory and type: 
1. "make clean" to remove any previously made executables 
2. "make" to recompile the c files and create executable 
3. "./fat32_reader fat32.img" to run the program


Info Command:
Type info in order to get information about the BPB, specifically the fields: 
o BPB_BytesPerSec
o BPB_SecPerClus 
o BPB_RsvdSecCnt 
o BPB_NumFATS
o BPB_FATSz32

Within the main function, before entering the while loop, we convert the bytes to the appropriate endian-ness.

Ls Command:
this function displays the files that are in the specified directory.

Cd command:
Changes the present working directory to the specified directory 

Stat command:
This function returns the sizeof the fileor directory name, the attributes of the file or directory name, and the first cluster number of the file or directory name if it is in the present working directory.

Size command:
prints the size of file FILE_NAME in the present working directory.

Read command:
reads from a file, starting at a specified position, and prints a specified number of bytes.

Volume command:
Prints the volume name of the file system image.

Quit command:
Quits the utility.   

Challenges faced:
Some of the challenges we faced so far was being on top of our game with endian conversion. But once we got that down things were looking good for us. It was also a bit frustrating testing in a VM but we'll make do :)
