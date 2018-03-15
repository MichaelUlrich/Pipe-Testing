#include<fcntl.h>
#include<dirent.h>
#include<stdlib.h>
#include<stdio.h>
#include<stdbool.h>
#include<sys/syscall.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<string.h>
#include<unistd.h>
#include<errno.h>
#define MAX_FILES       50000
#define MAX_FILENAME    5000
#define READ		0
#define WRITE		1
typedef struct {
	char *fileName;    
        struct stat status; 
} statStruct;

//Global Variables
char * fileNames[MAX_FILES]; 		//Array of file names
int totalFilesSize = 0;      		//Track total size of all files
statStruct stats[MAX_FILES]; 		//Information from stat()

//Function Declerations
void monitorFile(char *, int[]);
void processDirectory(char *, int[]);

void runPipe(int currPos, int myPipe[]) { 	//Writes data to pipe
	printf("passing data through pipe\n");
	close(myPipe[READ]);			//Close child's read-end of pipe
	write(myPipe[WRITE], &stats[currPos], sizeof(statStruct));
//	close(myPipe[1]);

}
void processDirectory(char * dirName, int myPipe[]) {
        int fd, charsRead;
        struct dirent dirEntry;
        char fileName[MAX_FILENAME];

        fd = open(dirName, O_RDONLY); 								//Open for reading
        if(fd == -1) {
                perror("Can't process directory"); exit(1);
        }
        while(true) {
                charsRead = syscall(SYS_getdents64, fd, &dirEntry, sizeof(struct dirent));
                if(charsRead == -1) { perror("Error reading chars"); exit(1); }
                if(charsRead == 0)  { break; } 							//EOF
                if(strcmp(dirEntry.d_name, ".") != 0 && strcmp(dirEntry.d_name, "..") != 0) {
                        sprintf(fileName, "%s/%s", dirName, dirEntry.d_name);
                        monitorFile(fileName, myPipe); 						//Recursively calls
                }
                lseek(fd, dirEntry.d_off, SEEK_SET); 						//Next Entry
        }
        close(fd);
} 
void monitorFile(char * fileName, int myPipe[]) {
        struct stat statBuffer;
        mode_t mode;
	int result;
			
        result = stat(fileName, &statBuffer);
        if(result == -1) {
                fprintf(stderr, "Can not stat %s ###\n", fileName);
                return;   
	}
	mode = statBuffer.st_mode; 		//Mode of file
        if(S_ISDIR(mode)) { 			//Directory
                processDirectory(fileName, myPipe);	
        } else if(S_ISREG(mode)) {		//Regular file;
	//	printf("%li\t%s\t\t%i\n", statBuffer.st_size, fileName, totalFilesSize);
		
		stats[totalFilesSize].fileName = malloc(MAX_FILENAME);
		strcpy(stats[totalFilesSize].fileName, fileName);
               	stats[totalFilesSize].status = statBuffer;
	
		runPipe(totalFilesSize, myPipe); //Function to pass data between processes
		
                totalFilesSize += 1;
		free(stats[totalFilesSize].fileName);	
		wait(NULL);
	}	
}
int main(int argc, char * argv[]) {

        char * rootPath = argv[1], TEMP[4]; 
	int myPipe[2], status, readFlag = 0, i = 0;
	pid_t pid;
	statStruct stat[MAX_FILES];
	status = pipe(myPipe); 

	if(status == -1) {
		perror("pipe err"); exit(1);
	}
	switch(fork()) {
		case 0: 			//Child Case - Writes to pipe
			printf("child process\n");
			monitorFile(rootPath, myPipe);
			exit(EXIT_SUCCESS);
		case -1:			//Error Case
			perror("fork err");
		default:			//Parent Case- Reads from pipe
			printf("parent process\n");
			wait(NULL);
			close(myPipe[WRITE]); 	//Close write-end of pipe
			while(readFlag != 0) {
					//readFlag = read(myPipe[READ], &stat[i], sizeof(struct statStruct));
					printf("Receiving data\n");
					printf("%li\n", read(myPipe[READ], &stat[i], sizeof(statStruct)));
					i+=1;
					printf("%s", stat[i].fileName);
			}
		//	printf("%s", *stat[i].fileName);
			printf("total files: %i \n", totalFilesSize);
		//	wait(NULL);
		//	close(myPipe[0]);
		//	exit(EXIT_SUCCESS);
	}
	return 0;
}
