#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <time.h>

// Superblock
struct __attribute__((__packed__)) superblock_t {
    uint8_t   fs_id [8];
    uint16_t block_size;
    uint32_t file_system_block_count;
    uint32_t fat_start_block;
    uint32_t fat_block_count;
    uint32_t root_dir_start_block;
    uint32_t root_dir_block_count;
};
// Time/Date entry
struct __attribute__((__packed__)) dir_entry_timedate_t {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

// Directory Entry
struct __attribute__((__packed__ )) dir_entry_t {
    uint8_t status;
    uint32_t starting_block;
    uint32_t block_count;
    uint32_t size;
    struct dir_entry_timedate_t ct;
    struct dir_entry_timedate_t mt;
    uint8_t filename[31];
    uint8_t unused[6];
};

int main(int argc, char *argv[]) {
    
    if(argc < 4) {
        printf("%s\n", "Not enough arguments. USAGE: ./diskput <image file> <file name> <file to copy>");
        return;
    }

    if(argv[3][0] == '/') {
            argv[3] = argv[3]+1;
    }

    if(argv[2][0] == '/') {
            argv[2] = argv[2]+1;
    }

    int fd = open(argv[1], O_RDWR);
    struct stat buffer;
    int status = fstat(fd, &buffer);

    void* address=mmap(NULL, buffer.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    struct superblock_t* sb = (struct superblock_t*) address;

    int root_start = ntohl(sb->root_dir_start_block);
    int root_blocks = ntohl(sb->root_dir_block_count);

    struct dir_entry_t* dir;
    uint32_t cur_block;
    uint32_t tmp_block;
    char* fname;
    int found = 0;

    for(int i = root_start; i < root_blocks + root_start; i++) {
        for(int o = 0; o < 512; o=o+64) {
            int offset = i*512+o;

            dir = (struct dir_entry_t*) (address+offset);
            if(dir->status == 3) {
                if(strcmp(dir->filename, argv[3]) == 0) {
                    cur_block = dir->starting_block;
                    found = 1;
                    break;
                }
            }

        }
    }
    if(found == 1){ 
        FILE *output = fopen(argv[2], "w+");
        //uint8_t data[513];
        while(1) {
            uint8_t data[513] = {'/0'};
            memcpy(data, address+512*ntohl(cur_block), 512);
            fputs(data, output);
            memcpy(&tmp_block, address + 512*ntohl(sb->fat_start_block) + ntohl(cur_block)*4, 4);
            memcpy(&cur_block, &tmp_block, 4);
            if(ntohl(cur_block) == -1)
                break;
        }
        printf("%s\n", "File successfully copied to current directory!");
        fclose(output);
    } else {
        printf("%s\n", "ERROR! File to copy not found.");
    }
    munmap(address,buffer.st_size);
    close(fd);
}