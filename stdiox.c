#include <sys/stat.h>
#include <unistd.h>  //for access() function
#include <errno.h>
#include <linux/limits.h> /* PATH_MAX */
#include <stdlib.h>
#include "stdiox.h"
/* Matt Kearney
I pledge my honor that I have abided by the Stevens Honor System. */

/* Use file system API with open, close, read, write */

/* Objective: 
    write a standard input / output library for integers and strings 
    1.1 - fprintfx
    1.2 - fscanfx
    1.3 - clean   */

#include <string.h>
 
 /* reverse:  reverse string s in place */
void reverse(char s[]){
     int i, j;
     char c;
 
     for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
         c = s[i];
         s[i] = s[j];
         s[j] = c;
     }
}

void itoa(int n, char a[]) {
    int i, s;
 
    if ((s = n) < 0)  /* record sign */
        n = -n;          /* make n positive */
    i = 0;
    do {       /* generate digits in reverse order */
        a[i++] = n % 10 + '0';   /* get next digit */
    } while ((n /= 10) > 0);     /* delete it */
    if (s < 0){
        a[i++] = '-';
    }
    a[i] = '\0';
    reverse(a);
}

int num_digits(int num){
    int i=1;
    if(num < 0){
        num*=-1;
        i++;
    }
    
    while(num>10){
        num/=10;
        i++; 
    }
    return i;
}

/* write_to(int file_desc, void* data, char format)
    = file_desc specifies which file it is writing to
    = data is the memory location to the type of data 
        specified by format 
        
     */
void write_to(int fd, void* data, char format){

    ssize_t size; //for write(), we need the amount of bytes to write

    if(format == 's'){ //the address in data is a char*
        char* s = (char*) data;
        size = strlen(s); //null-term.
        write(fd, s, size); //print the data to the terminal
        write(fd, "\n", 1);
    } 

    if (format == 'd'){ //address is a SINGLE integer
        int* num = (int*) data;
        int num_dig = num_digits(*num);
        // if(*num < 0){ //being handles by num_digits
        //     num_dig++; //this accounts for '-'
        // }
        
        char buf[num_dig]; //make sure that num_dig accounts for '-'
        itoa(*num, buf); //buf now contains the integer as a char*
        size = sizeof(buf);
            
        write(fd, buf, size); 
        write(fd, "\n", 1); 
    } 
}

/*
-If filename is "", print data to terminal 
Check if file exists, 
    -if yes, write the data to the end of the file
    -if not, make a file with permissions rw-r-----
        and write the data to that file. 
        
    -also set errno to EIO and return -1 if adr(data) is null */
    //write(int fd, char *buf, ssize_t num_bytes)
int fprintfx(char* filename, char format, void* data){
    // if base address for data is null
    if(data == NULL){ 
        errno = EIO;
        return -1;
    }

    //format is not in a compatible type
    if (format != 'd' && format != 's'){ 
        errno = EIO;
        return -1;
    }

    int fd;             //used for open()
    size_t size;        //  ^^  ^^   ^^
    //if filename is empty, write the data to the terminal 
    if (filename == ""){ 
        fd = 1;
        write_to(fd, data, format); 
        return 0; 
    }

    int flag = O_CREAT | O_APPEND | O_RDWR; //create if not existent, append, wronly
    mode_t perm = (S_IRUSR | S_IWUSR | S_IRGRP); //perms == 640

    
    char buf[PATH_MAX]; //null-term.
    char *path = realpath(filename, buf);  //store path 

    //check for existence and if it does not, make the file
    if(path == NULL){ //file does not exist
        fd = open(filename, flag , perm); //now it does
        write_to(fd, data, format); 
        return 0;
    }

    if(path!=NULL){ //file exists...
        fd=open(path, O_APPEND | O_RDWR); //necessary
        write_to(fd, data, format);
        return 0;
    }

    return 0;
}

// int atoi(char* num){
//     int res = 0;
//     int n = 0;
//     if(num[0] == '-'){
//         n = 1;
//     }
//     for(int i = 1; num[i] != '\0'; ++i){
//         res = res*10 + num[i] - '0';
//     }
//     res*=-1;
//     return res;
// }


/* 
If filename is empty, receive data from keyboard 
else 
    if the file exists but is not opened, open it and read one line from it
    if the file exists and is opened, read the next line from it
    if the file doesn't exist, sett errno to ENOENT

Process to read from file: 
    Since we don't know how many characters in one line, 
    we will need to make a buffer to store 128 bytes 
    for the data we read in the file (dst)
    
    Does not mention anything about creating a file...
    */

//inode x fstat, look at photos for 
int fscanfx(char* filename, char format, void* dst){

    if( dst == NULL ){ //if dst is null, error
        errno=EIO;
        return -1;
    }

    //format is not in a compatible type, error
    if (format != 'd' && format != 's'){ 
        errno = EIO;
        return -1;
    }

    int fd;
    int i = 0; //fpos
    char buffer[128]; //this is for dst
    char c;

    //for empty filename, read from stdin
    if(filename == ""){ //if empty, receive data from keyboard
        fd = 0;
    }

    while (read(fd, &c, 1) == 1) {
        if (c == '\n') {
            break;
        }
        buffer[i++] = c;
    }
    buffer[i] = '\0';

    if (format == 's'){
        memcpy(dst, buffer, strlen(buffer) + 1);
        read(fd, buffer, *(int*)dst); //0=stdin
        return 0;
    }
    if(format == 'd'){
        memcpy(dst, buffer, sizeof(int));
    }
    return 0;
} 

/* clean() : close all opened files */
int clean(){
    DIR* dp;
    struct dirent *dirp;

    int file_desc;
    dp = opendir("/proc/self/fd");
    
    while((dirp = readdir(dp)) != NULL){
        file_desc = atoi(dirp->d_name);
        if(file_desc > 2){
            close(file_desc);
            if(close(file_desc)){
                closedir(dp);
                errno = EIO;
                return -1;
            }
        }
    }
    closedir(dp);
    return 0;
}


