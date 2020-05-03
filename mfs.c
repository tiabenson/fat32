//Tia Deloach Benson
//ID #1001523776

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <stdbool.h>

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

struct __attribute__((__packed__)) DirEntry //structure of image
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
int root;

int LBATToOffset(int32_t sector) //returns offset of sector
{
    return ((sector - 2) * BPB_BytesPerSec) + (BPB_BytesPerSec * BPB_RsvdSecCnt) + (BPB_NumFATS * BPB_FATSz32 * BPB_BytesPerSec);
}

int16_t NextLB(int32_t sector)
{
    uint32_t FATAddress = (BPB_BytesPerSec * BPB_RsvdSecCnt) + (sector * 4);
    int16_t val;
    fseek(file, FATAddress, SEEK_SET);
    fread(&val, 2, 1, file);
    
    return val;
}

bool compare(char *fatName, char *userName) //compares input with filename in FAT
{
    char expanded_name[12];
    memset(expanded_name, ' ', 12 );
    
    char *token = strtok( userName, "." );
    
    strncpy(expanded_name, token, strlen(token));
    
    token = strtok( NULL, "." );
    
    if(token)
    {
        strncpy((char*)(expanded_name+8), token, strlen(token));
    }
    
    expanded_name[11] = '\0';
    
    int i;
    
    for(i = 0; i < 11; i++ )
    {
        expanded_name[i] = toupper(expanded_name[i]);
    }
    
    if(strncmp(expanded_name, fatName, 11) == 0)
    {
        return true;
    }
    
    else
    {
        return false;
    }
}

int main()
{
    char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );
    int count = 0;  //checks if file is open
    int close = 0;  //checks if any commands are entered after a close
    
    int i, a, j = 0;
    int folder = 0;
    int offset;
    
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
            if(token[1] == NULL)
            {
                printf("Error: No image specified.\n");
                continue;
            }
            
            char *img = ".img";
            char *checkIfImage = strstr(token[1], img);
            
            //check if .img file is trying to be opened
            if(checkIfImage)
            {
                if(strlen(token[1]) > 100 && strchr(token[1], ' ') != NULL) //check if .img file is in correct format
                {
                    printf("Error: .img filename not in correct format! Maybe try again.\n");
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
                        printf("Error: File system image not found.\n");
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
                    
                    
                    //get root directory address
                    root = (BPB_NumFATS * BPB_FATSz32 * BPB_BytesPerSec) + (BPB_RsvdSecCnt * BPB_BytesPerSec);
                    
                    fseek(file, root, SEEK_SET);
                    fread(&directory[0], 16, sizeof(struct DirEntry), file);
                    
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
        
        else if(strcmp(token[0], "ls") == 0)   //show files in directory
        {
            if(close == 1)  //check if file isn't open
            {
                printf("Error: File system image must be opened first.\n");
            }
          
            else if(strcmp(token[1], "..") == 0)
            {
                fseek(file, root, SEEK_SET);
                fread(&directory[0], 16, sizeof(struct DirEntry), file);
                
                for(i = 0; i < 16; i++) //print out files/folders in directory
                {
                    if(directory[i].Dir_Attr == 0x01 || directory[i].Dir_Attr == 0x10 || directory[i].Dir_Attr == 0x20 || directory[i].Dir_Attr == 0xe5)
                    {
                        
                        char names[12];     //make files null terminated
                        memset(names, 0, 12);
                        strncpy(names, directory[i].Dir_Name, 11);
                        printf("%s\n", names);
                    }
                }
            }
            
            else
            {
                for(i = 0; i < 16; i++) //print out files/folders in directory
                {
                    if(directory[i].Dir_Attr == 0x01 || directory[i].Dir_Attr == 0x10 || directory[i].Dir_Attr == 0x20 || directory[i].Dir_Attr == 0xe5)
                    {
                        
                        char names[12];     //make files null terminated
                        memset(names, 0, 12);
                        strncpy(names, directory[i].Dir_Name, 11);
                        printf("%s\n", names);
                    }
                }
            }
            
        }
        
        else if(strcmp(token[0], "stat") == 0)  //print stats of file or directory
        {
            if(close == 1)  //check if file isn't open
            {
                printf("Error: File system image must be opened first.\n");
                continue;
            }
            
            bool here = false;
            
            if(token[1] == NULL)
            {
                printf("Error: Didn't state what file or directory to stat.\n");
                continue;
            }
            
            for(i = 0; i < 16; i++)
            {
                char names[12];
                memset(names, 0, 12);
                strncpy(names, token[1], 11);
                
                if(compare(directory[i].Dir_Name, names) == true)   //print stats
                {
                    if(directory[i].Dir_Attr == 0x01 || directory[i].Dir_Attr == 0x10 || directory[i].Dir_Attr == 0x20 || directory[i].Dir_Attr == 0xe5)
                    {
                        here = true;
                        printf("Name: %s\n", names);
                        printf("Attribute: %d\n", directory[i].Dir_Attr);
                        printf("Size: %d\n", directory[i].Dir_FileSize);
                        printf("First cluster low: %d\n", directory[i].Dir_FirstClusterLow);
                    }
            
                }
       
            }
            
            if (!here)
            {
                printf("Error: File not found.\n");
            }
        }
        
        else if(strcmp(token[0], "cd") == 0)
        {
            bool here = false;
            
            
            if(close == 1)  //check if file isn't open
            {
                printf("Error: File system image must be opened first.\n");
                continue;
            }

            else if(token[1] == NULL)    //check if 2nd parameter is used
            {
                printf("Error: Didn't specify directory to cd into\n");
                continue;
            }
      
            else
            {
                for(i = 0; i < 16; i++)
                {
                    if(compare(directory[i].Dir_Name, token[1]) == true && directory[i].Dir_Attr == 0x10)
                    {
                        here = true;
                        
                        offset = LBATToOffset(directory[i].Dir_FirstClusterLow);
                        
                        fseek(file, offset, SEEK_SET);
                        fread(&directory[0], 512, 1, file);
                         folder = 1;
                        
                    }
                    
                }
                
                if (!here)
                {
                    printf("Error: Can't cd into a file.\n");
                }
            }
            
       
        }
        
        else if(strcmp(token[0], "read") == 0)  //read certain bytes of files
        {
            uint8_t value;
            
            if(close == 1)  //check if file isn't open
            {
                printf("Error: File system image must be opened first.\n");
                continue;
            }
            
            else if(token[1] == NULL || token[2] == NULL || token[3] == NULL)
            {
                printf("Error: too few arguments.\n");
                continue;
            }
            
            else
            {
                for(i = 0; i < 16; i++)
                {
                    char names[12];
                    memset(names, 0, 12);
                    strncpy(names, token[1], 11);
                    
                    if(directory[i].Dir_Attr == 0x01 || directory[i].Dir_Attr == 0x10 || directory[i].Dir_Attr == 0x20)
                    {
                        if(compare(directory[i].Dir_Name, names) == true)   //read bytes from certain position
                        {
                            int pos = atoi(token[2]);
                            int bytes = atoi(token[3]);
                            
                            offset = LBATToOffset(directory[i].Dir_FirstClusterLow);
                            fseek(file, offset + pos, SEEK_SET);
                            
                            for(a = 0; a < bytes; a++)
                            {
                                fread(&value, bytes, 1, file);
                                printf("%d ", value);
                            }
                            
                        }
                    }
                    
                }
                
                printf("\n");
            }
            
            
        }
        
        else if(strcmp(token[0], "get") == 0)
        {
            if(close == 1)  //check if file isn't open
            {
                printf("Error: File system image must be opened first.\n");
                continue;
            }
            
            else if(token[1] == NULL)
            {
                printf("Error: Too few parameters");
                continue;
            }
            
            else
            {
                
                bool write = false;
                
                for(i = 0; i < 16; i++)
                {
                    char names[12];
                    memset(names, 0, 12);
                    strncpy(names, token[1], 11);
                    int file_size;
                    FILE *get;
                    
                    if(compare(directory[i].Dir_Name, names) == true)
                    {
                        file_size = directory[i].Dir_FileSize;
                        offset = LBATToOffset(directory[i].Dir_FirstClusterLow);
                        char File[file_size];
                        
                        fseek(file, offset, SEEK_SET);
                        get = fopen(token[1], "w");
                        
                        fread(&File[0], file_size, 1, file);
                        fwrite(&File[0], file_size, 1, get);
                        fclose(get);
                        
                        write = true;
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
