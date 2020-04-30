//Tia Deloach Benson
//ID #1001523776

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define WHITESPACE " \t\n"      //Whitespace will be delimiters for tokens
#define MAX_COMMAND_SIZE 255    // The maximum command-line size
#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports five arguments

//infrormation for image
uint16_t BPB_BytesPerSec;
uint8_t BPB_SecPerClus;
uint16_t BPB_RsvdSecCnt;
uint8_t BPB_NumFATS;
uint16_t BPB_RootEntCnt;
uint32_t BPB_FATSz32;

struct __attribute__((__packed__)) DirEntry
{
    char Dir_Name[11];
    uint8_t Dir_Attr;
    uint8_t Unused1[8];
    uint16_t Dir_FirstClusterHigh;
    uint16_t Dir_FirstClusterLow;
    uint8_t Unused[4];
    uint32_t Dir_FileSize;
};

struct DirEntry directory[16];
FILE *file;

int main()
{
    char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );
    int count = 0;  //checks if file is open
    int close = 0;  //checks if any commands are entered after a close
    int root = 0;
    
    while(1)
    {
        // Print out the mfs prompt
        printf ("mfs> ");
        
        while(!fgets (cmd_str, MAX_COMMAND_SIZE, stdin));   //read input from the terminal
        
        //Account for and ignore the new line character before parsing the command string
        if(strcmp(cmd_str, "\n") == 0)
        {
            continue;
        }
        
        // Parse input
        char *token[MAX_NUM_ARGUMENTS];
        
        int token_count = 0;    //keeps track of number of arguments user has typed
        
        // Pointer to point to the token
        // parsed by strsep
        char *arg_ptr;
        
        char *working_str  = strdup( cmd_str );
        
        // move the working_str pointer to
        // keep track of its original value
        char *working_root = working_str;
        
        // Tokenize the input stringswith whitespace used as the delimiter
        while (((arg_ptr = strsep(&working_str, WHITESPACE )) != NULL) &&
               (token_count<MAX_NUM_ARGUMENTS))
        {
            token[token_count] = strndup(arg_ptr, MAX_COMMAND_SIZE);
            
            if(strlen(token[token_count]) == 0)
            {
                token[token_count] = NULL;
            }
            
            token_count++;
        }
        
        if((strcmp(token[0], "exit") == 0) || (strcmp(token[0], "quit") == 0))
        {
            exit(0);
        }
        
        else if(strcmp(token[0], "open") == 0)
        {
            char *img = ".img";
            char *checkIfImage = strstr(token[1], img);
            
            //check if .img file is trying to be opened
            if(checkIfImage)
            {
                if(strlen(token[1]) > 100 && strchr(token[1], ' ') != NULL) //check if .img file is in correct format
                {
                    printf("Error: .img filename not in correct format! Maybe try again.");
                    continue;
                }
                
                if(count == 1)  //if file has already been opened without being closed
                {
                    printf("Error: File system image already open.\n");
                }
                
                else    //after checking possible errors, open file
                {
                    file = fopen(token[1], "r");
                    count = 1;
                    close = 0;  //file hasn't been closed
                    
                    if(!file)   //check if file is ok
                    {
                        printf("Error: File system image not found.");
                    }
                    
                    //retrieve the bytes per sec, sector per cluster, reserved sector count, etc. from image
                    fseek(file, 11, SEEK_SET);
                    fread(&BPB_BytesPerSec, 2, 1, file);
                    
                    fseek(file, 13, SEEK_SET);
                    fread(&BPB_SecPerClus, 1, 1, file);
                    
                    fseek(file, 14, SEEK_SET);
                    fread(&BPB_RsvdSecCnt, 2, 1, file);
                    
                    fseek(file, 16, SEEK_SET);
                    fread(&BPB_NumFATS, 1, 1, file);
                    
                    fseek(file, 17, SEEK_SET);
                    fread(&BPB_RootEntCnt, 2, 1, file);
                    
                    fseek(file, 36, SEEK_SET);
                    fread(&BPB_FATSz32, 4, 1, file);
                    
                    root = (BPB_NumFATS * BPB_FATSz32 * BPB_BytesPerSec) + (BPB_RsvdSecCnt * BPB_BytesPerSec);
                    
                }
            }
            
            else
            {
                printf("Error: File system image not found.\n");
            }
            
        }
        
        else if(strcmp(token[0], "close") == 0) //close the file
        {
            if(count == 0)  //if file isn't open
            {
                printf("Error: File system not open.\n");
            }
            
            else
            {
                fclose(file);
                count = 0;
                close = 1;  //no commands can execute after file has been closed
            }
        }
        
        else if(strcmp(token[0], "info") == 0)  //print info
        {
            if(close == 1)  //check if file isn't open
            {
                printf("Error: File system image must be opened first.\n");
                continue;
            }
            
            printf("BytesPerSec (base 10): %d\n", BPB_BytesPerSec);
            printf("BytesPerSec (hex): %x\n", BPB_BytesPerSec);
            
            printf("SecPerClus (base 10): %d\n", BPB_SecPerClus);
            printf("SecPerClus (hex): %x\n", BPB_SecPerClus);
            
            printf("RsvdSecCnt (base 10): %d\n", BPB_RsvdSecCnt);
            printf("RsvdSecCnt (hex): %x\n", BPB_RsvdSecCnt);
            
            printf("NumFATS (base 10): %d\n", BPB_NumFATS);
            printf("NumFATS (hex): %x\n", BPB_NumFATS);
            
            printf("FATSz32 (base 10): %d\n", BPB_FATSz32);
            printf("FATSz32 (hex): %x\n", BPB_FATSz32);
 
        }
        
        else if (strcmp(token[0], "ls") == 0)   //show files in directory
        {
            if(close == 1)  //check if file isn't open
            {
                printf("Error: File system image must be opened first.\n");
            }
            
            else
            {
                fseek(file, root, SEEK_SET);
                fread(&directory[0], 16, sizeof(struct DirEntry), file);
                
                int i;
                
                for(i = 0; i < 16; i++)
                {
                    if(directory[i].Dir_Attr == 0x01 || directory[i].Dir_Attr == 0x10 || directory[i].Dir_Attr == 0x20 || directory[i].Dir_Attr == 0xe5)
                    {
                        printf("%s\n", directory[i].Dir_Name);
                    }
                }
            }
            
        }
        
        else
        {
            printf("Command not found.\n");
        }
        
        free(working_root);
        
    }
    return 0;
}
