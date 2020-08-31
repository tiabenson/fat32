//Tia Deloach Benson

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <stdbool.h>

#define WHITESPACE " \t\n"      //command line will be delimted by whitespace

#define MAX_COMMAND_SIZE 255

#define MAX_NUM_ARGUMENTS 5

FILE *file; //Will open our image file

int root = 0; // Root address

struct __attribute__((__packed__)) DirEntry //structure for image file
{
    char DIR_Name[11];
    uint8_t DIR_Attr;
    uint8_t Unused1[8];
    uint16_t DIR_FirstClusterHigh;
    uint8_t Unused2[4];
    uint16_t DIR_FirstClusterLow;
    uint32_t DIR_FileSize;
};

struct DirEntry directory[16];

//information we need for fat32 image file
int16_t BPB_BytesPerSec;
int8_t BPB_SecPerClus;
int16_t BPB_RsvdSecCnt;
int8_t BPB_NumFATs;
int16_t BPB_RootEntCnt;
int32_t BPB_FATSz32;

bool compare(char* Fatname, char* input)    //compares fat name with user imput name
{
    char expanded_name[12];
    memset(expanded_name, ' ', 12);
    
    char *token = strtok( input, "." );
    
    strncpy(expanded_name, token, strlen(token));
    
    token = strtok( NULL, "." );
    
    if(token)
    {
        strncpy((char*)(expanded_name + 8), token, strlen(token));
    }
    
    expanded_name[11] = '\0';
    
    int i;
    
    for(i = 0; i < 11; i++)
    {
        expanded_name[i] = toupper(expanded_name[i] );
    }
    
    if(strncmp(expanded_name, Fatname, 11) == 0)
    {
        return true;
    }
    
    else
    {
        return false;
    }
}

// Finds offset of a sector
int LBATToOffset(int32_t sector)
{
    return (( sector - 2 ) * BPB_BytesPerSec) + (BPB_BytesPerSec * BPB_RsvdSecCnt) + (BPB_NumFATs * BPB_FATSz32 * BPB_BytesPerSec);
}

int16_t NextLB(uint32_t sector)
{
    uint32_t FATAddress = (BPB_BytesPerSec * BPB_RsvdSecCnt) + (sector * 4);
    int16_t val;
    fseek(file, FATAddress, SEEK_SET);
    fread(&val, 2, 1, file);
    return val;
}

int main()
{
    char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );
    
    int count;  //checks if file is open
    int close;  //checks if any commands are entered after a close
    
    int i, a, j = 0;
    int offset; //will hold offset of cluster
    
    while(1)
    {
        // Print out the mfs prompt
        printf ("mfs> ");
        
        // Read the command from the commandline.
        while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );
        
        //Account for and ignore the new line character before parsing the command string
        if(strcmp(cmd_str, "\n") == 0)
        {
            continue;
        }
        
        /* Parse input */
        char *token[MAX_NUM_ARGUMENTS];
        
        int token_count = 0;
        
        char *arg_ptr;
        
        char *working_str  = strdup( cmd_str );
        
        // we are going to move the working_str pointer so
        // keep track of its original value so we can deallocate
        // the correct amount at the end
        char *working_root = working_str;
        
        // Tokenize the input stringswith whitespace used as the delimiter
        while (((arg_ptr = strsep(&working_str, WHITESPACE )) != NULL) &&
               (token_count<MAX_NUM_ARGUMENTS))
        {
            token[token_count] = strndup(arg_ptr, MAX_COMMAND_SIZE);
            
            if(strlen( token[token_count]) == 0 )
            {
                token[token_count] = NULL;
            }
            
            token_count++;
        }
        
        if(strcmp(token[0], "exit") == 0 || strcmp(token[0], "quit") == 0)
        {
            exit(0);
        }
        
        else if(strcmp(token[0], "open") == 0)  //open image file
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
                }
                
                else if(count == 1)  //if file has already been opened without being closed
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
                        continue;
                    }
                    
                    //retrieve the bytes per sec, sector per cluster, reserved sector count, etc. from image
                    fseek(file, 11, SEEK_SET);
                    fread(&BPB_BytesPerSec, 2, 1, file);
                    
                    fseek(file, 13, SEEK_SET);
                    fread(&BPB_SecPerClus, 1, 1, file);
                    
                    fseek(file, 14, SEEK_SET);
                    fread(&BPB_RsvdSecCnt, 2, 1, file);
                    
                    fseek(file, 16, SEEK_SET);
                    fread(&BPB_NumFATs, 1, 1, file);
                    
                    fseek(file, 17, SEEK_SET);
                    fread(&BPB_RootEntCnt, 2, 1, file);
                    
                    fseek(file, 36, SEEK_SET);
                    fread(&BPB_FATSz32, 4, 1, file);
                    
                    //get root directory address
                    root = (BPB_NumFATs * BPB_FATSz32 * BPB_BytesPerSec) + (BPB_RsvdSecCnt * BPB_BytesPerSec);
                    
                    //seek to that address
                    fseek(file, root, SEEK_SET);
                    fread(&directory[0], 16, sizeof(struct DirEntry), file);
                    
                }
            }
            
            else
            {
                printf("Error: File system image not found.\n");
            }
            
        }
        
        else if(strcmp(token[0], "close") == 0)    //close image file
        {
            if(count == 0)  //if file isn't open
            {
                printf("Error: File system not open.\n");
                continue;
            }
            
            count = 0;
            close = 1;  //no commands can execute after file has been closed
            fclose(file);
            
        }
        
        else if(strcmp(token[0], "info") == 0)  //print info
        {
            if(close == 1)  //check if file isn't open
            {
                printf("Error: File system image must be opened first.\n");
            }
            
            else
            {
                printf("BytesPerSec (base 10): %d\n", BPB_BytesPerSec);
                printf("BytesPerSec (hex): %x\n", BPB_BytesPerSec);
                
                printf("SecPerClus (base 10): %d\n", BPB_SecPerClus);
                printf("SecPerClus (hex): %x\n", BPB_SecPerClus);
                
                printf("RsvdSecCnt (base 10): %d\n", BPB_RsvdSecCnt);
                printf("RsvdSecCnt (hex): %x\n", BPB_RsvdSecCnt);
                
                printf("NumFATS (base 10): %d\n", BPB_NumFATs);
                printf("NumFATS (hex): %x\n", BPB_NumFATs);
                
                printf("FATSz32 (base 10): %d\n", BPB_FATSz32);
                printf("FATSz32 (hex): %x\n", BPB_FATSz32);
            }
        }
        
        else if(strcmp(token[0], "ls") == 0)   //show what's in directory
        {
            if(close == 1)  //check if file isn't open
            {
                printf("Error: File system image must be opened first.\n");
                continue;
            }
            
            for (i = 0; i < 16; i++)
            {
                //don't show deleted files (0xe5), so check 1st character of the filename
                if((directory[i].DIR_Attr == 0x01 || directory[i].DIR_Attr == 0x10 || directory[i].DIR_Attr == 0x20) && (directory[i].DIR_Name[0] != (char) 0xe5))
                {
                    char name[12];  //Show files as null terminated
                    memset(&name, 0, 12);
                    strncpy(name, directory[i].DIR_Name, 11);
                    printf("%s\n", name);
                }
            }
        }
        
        else if(strcmp(token[0], "stat") == 0)  //show stats of files and directories
        {
            if(close == 1)  //check if file isn't open
            {
                printf("Error: File system image must be opened first.\n");
                continue;
            }
            
            bool here = false;
            
            for (i = 0; i < 16; i++)
            {
                char names[12];     //make names null terminated
                memset(&names, 0, 12);
                strncpy(names, token[1], 11);
                
                //if file is found, print out stats
                if (compare(directory[i].DIR_Name, names) == true)
                {
                    if(directory[i].DIR_Attr == 0x01 || directory[i].DIR_Attr == 0x10 || directory[i].DIR_Attr == 0x20)
                    {
                        printf("Name: %s\n", names);
                        printf("Attribute: %d\n", directory[i].DIR_Attr);
                        printf("Size: %d\n", directory[i].DIR_FileSize);
                        printf("First cluster low: %d\n", directory[i].DIR_FirstClusterLow);
                        here = true;
                    }
                }
            }
            
            if(!here)
            {
                printf("Error: File not found.\n");
            }
            
        }
        
        else if(strcmp(token[0], "cd") == 0)   //change into different directory
        {
            bool here = false;  //checks if directory was found
            int offset = 0;
            char *dir = strtok(token[1], "/");  //delimit by /
            
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
            
            else    //cd into directory
            {
                if(strcmp("..", token[1]) != 0)  //if .. wasn't typed, treat as a folder
                {
                    for(i = 0; i < 16; i++)
                    {
                        //check if a subdirectory
                        if(directory[i].DIR_Attr == 0x10 && compare(directory[i].DIR_Name, dir) == true)
                        {
                            offset = LBATToOffset(directory[i].DIR_FirstClusterLow);
                            fseek(file, offset, SEEK_SET);
                            fread(&directory[0], 16, sizeof(struct DirEntry), file);
                            here = true;
                        }
                        
                    }
                }
                
                else    //if .. is found
                {
                    for (i = 0; i < 16; i++)
                    {
                        if(strstr("..", directory[i].DIR_Name) != NULL)
                        {
                            // Check if .. was read as 0, change to 2 if necessary
                            if (directory[i].DIR_FirstClusterLow == 0)
                            {
                                offset = LBATToOffset(2);
                                fseek(file, offset, SEEK_SET);
                                fread(&directory[0], 16, sizeof(struct DirEntry), file);
                            }
                            
                            else
                            {
                                offset = LBATToOffset(directory[i].DIR_FirstClusterLow);
                                fseek(file, offset, SEEK_SET);
                                fread(&directory[0], 16, sizeof(struct DirEntry), file);
                            }
                            
                            
                            here = true;
                        }
                    }
                }
                
                dir = strtok(NULL, "/");
                
                while(dir)    //continue cd until there's no more slashes
                {
                    if(strcmp("..", dir) != 0)  //if .. wasn't typed, treat as a folder
                    {
                        for(i = 0; i < 16; i++)
                        {
                            //check if a subdirectory
                            if(directory[i].DIR_Attr == 0x10 && compare(directory[i].DIR_Name, dir) == true)
                            {
                                offset = LBATToOffset(directory[i].DIR_FirstClusterLow);
                                fseek(file, offset, SEEK_SET);
                                fread(&directory[0], 16, sizeof(struct DirEntry), file);
                                here = true;
                            }
                            
                        }
                    }
                    
                    else    //if .. is found
                    {
                        for (i = 0; i < 16; i++)
                        {
                            if (strstr("..", directory[i].DIR_Name) != NULL)
                            {
                                // Check if .. was read as 0, change to 2 if necessary
                                if (directory[i].DIR_FirstClusterLow == 0)
                                {
                                    offset = LBATToOffset(2);
                                    fseek(file, offset, SEEK_SET);
                                    fread(&directory[0], 16, sizeof(struct DirEntry), file);
                                }
                                
                                else
                                {
                                    offset = LBATToOffset(directory[i].DIR_FirstClusterLow);
                                    fseek(file, offset, SEEK_SET);
                                    fread(&directory[0], 16, sizeof(struct DirEntry), file);
                                }
                                
                                here = true;
                            }
                        }
                    }
                    
                    dir = strtok(NULL, "/");    //keep delimitinig by /
                }
                
                if (!here)
                {
                    printf("Error: Can't cd here.\n");
                }
            }
            
        }
        
        else if(strcmp(token[0], "read") == 0)  //read certain bytes of a file
        {
            bool here = false;
            
            if(close == 1)  //check if file isn't open
            {
                printf("Error: File system image must be opened first.\n");
            }
            
            else if(token[1] == NULL || token[2] == NULL || token[3] == NULL)   //check if user typed enough arguments
            {
                printf("Error: too few arguments.\n");
            }
            
            else
            {
                for(i = 0; i < 16; i++)
                {
                    char names[12];
                    memset(names, 0, 12);
                    strncpy(names, token[1], 11);
                    int offset = 0;
                    int data = 0;
                    
                    //check if a acceptable file
                    if(directory[i].DIR_Attr == 0x01 || directory[i].DIR_Attr == 0x10 || directory[i].DIR_Attr == 0x20)
                    {
                        if(compare(directory[i].DIR_Name, names) == true)   //read bytes from certain position
                        {
                            int bytes = atoi(token[3]);
                            int length = bytes;
                            int pos = atoi(token[2]);
                            int values[length]; //array will hold all data
                        
                            offset = LBATToOffset(directory[i].DIR_FirstClusterLow);
                            fseek(file, offset, SEEK_SET);  //get offset, then seek to given position and stay there
                            fseek(file, pos, SEEK_CUR);
                            
                            for(j = 0; j < length; j++) //initialize value array
                            {
                                values[j] = 0;
                            }
                            
                            for(a = 0; a < length; a++) //save all data in array
                            {
                                fread(&data, 1, 1, file);
                                values[a] = data;
                            }
                            
                            for(j = 0; j < length; j++) //print hex values
                            {
                                printf("%x ", values[j]);
                            }
                            
                            here = true;
                            
                        }
                    }
                    
                }
                
                if(!here)   //if error occurred
                {
                    printf("Error with read function. Try again.");
                }
                
                printf("\n");
            }
        }
        
        else if(strcmp(token[0], "get") == 0)   //copy file in FAT to your directory
        {
            FILE *get;  //used to fwrite
            bool write = false; //check is write was successful
            
            if(close == 1)  //check if file isn't open
            {
                printf("Error: File system image must be opened first.\n");
                continue;
            }
            
            else if(token[1] == NULL)   //if not enough parameters
            {
                printf("Error: Too few parameters");
                continue;
            }
            
            else
            {
                for(i = 0; i < 16; i++)
                {
                    char names[12];
                    memset(names, 0, 12);
                    strncpy(names, token[1], 11);
                    int file_size = 0;
                    
                    if(compare(directory[i].DIR_Name, names) == true)
                    {
                        offset = LBATToOffset(directory[i].DIR_FirstClusterLow);
                        file_size = directory[i].DIR_FileSize;  //get file size and offset
                        
                        char File[file_size];   //read data to this array
                        
                        fseek(file, offset, SEEK_SET);
                        fread(&File[0], file_size, 1, file);
                        
                        get = fopen(token[1], "w"); //create new file and write to it
                        fwrite(&File[0], file_size, 1, get);
                        
                        write = true;
                    }
                }
                
            }
            
            if(write)   //close file if written to successfully
            {
                fclose(get);
            }
            
            else
            {
                printf("Error: File not found.\n");
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

