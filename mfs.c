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

uint16_t BPB_BytesPerSec;
uint8_t BPB_SecPerClus;
uint16_t BPB_RsvdSecCnt;
uint8_t BPB_NumFATS;
uint32_t BPB_FATSz32;

struct __attribute__((__packed__)) DirEntry
{
    char Dir_Name[11];
    uint8_t Dir_Attr;
    uint8_t Unused1[0];
    uint16_t Dir_FirstClusterHigh;
    uint16_t Dir_FirstClusterLow;
    uint8_t Unused[4];
    uint32_t Dir_FileSize;
};

FILE *file; //opens .img file


int main()
{
    char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );
    int count = 0;  //checks if file is open
    int close = 0;  //checks if any commands are entered after a close
    
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
                    printf("Error: File system image already open.");
                }
                
                else    //after checking possible errors, open file
                {
                    file = fopen(token[1], "r");
                    count = 1;
                    
                    if(!file)   //check if file is ok
                    {
                        printf("Error: File system image not found.");
                    }
                    
                    //retrieve the bytes per sec, sector per cluster, and reserved sector count
                    fseek(file, 11, SEEK_SET);
                    fread(&BPB_BytesPerSec, 2, 1, file);
                    
                    fseek(file, 13, SEEK_SET);
                    fread(&BPB_SecPerClus, 1, 1, file);
                    
                    fseek(file, 14, SEEK_SET);
                    fread(&BPB_RsvdSecCnt, 2, 1, file);
                    
                    printf("Bytes per sec is: %d\n", BPB_BytesPerSec);
                    printf("Sector per clus is: %d\n", BPB_SecPerClus);
                    printf("Reserved sec count is : %d\n", BPB_RsvdSecCnt);
                }
                
                
            }
            
            else
            {
                printf("Error: Can't open a non-image file!\n");
            }
            
        }
        
        else if (strcmp(token[0], "close") == 0)
        {
            if(count == 0)  //if file isn't open
            {
                printf("Error: File system not open.");
            }
            
            else
            {
                fclose(file);
                count = 0;
                close = 1;
            }
        }
        
        free(working_root);
        
    }
    return 0;
}
