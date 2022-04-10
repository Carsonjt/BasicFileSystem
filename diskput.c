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

    if(argc < 4) {
        printf("%s\n", "Not enough arguments. USAGE: ./diskput <image file> <new filename> <file to copy>");
        return;
    }
    if(argv[3][0] == '/') {
            argv[3] = argv[3]+1;
    }

    if(argv[2][0] == '/') {
            argv[2] = argv[2]+1;
    }


    FILE* input;
    input = fopen(argv[3], "r");
    if(NULL == input) {
        printf("%s\n", "Error! File to copy not found.");
        return;
    }
    char str[512];

    int prev=ftell(input);
    fseek(input, 0L, SEEK_END);
    int sz=ftell(input);
    fseek(input,prev,SEEK_SET);
    int block_count = (sz+511) /512 ;

    int last = -1;


    int new_block_start;

    for(int u = 0; u < block_count; u++) {
        int cur_block = -1;
        int cur_offset = -1;
        int next_block = -1;
        int next_offset = -1;
        int offset = 0;

        int count = 0;
        for(int i = ntohl(sb->fat_start_block); i < ntohl(sb->fat_block_count) + ntohl(sb->fat_start_block); i++) {
            for(int o = 0; o < 512; o=o+4) {
                if(count > ntohl(sb->root_dir_start_block)) {
                    offset = i*512+o;
                    uint32_t number;
                    memcpy(&number, address+offset, 4);
                    if(ntohl(number) == 0) { //IS FREE BLOCK
                        if(cur_block == -1) {
                            if(u == 0)
                                new_block_start = count;
                            //printf("%s %d\n", "SET CUR", offset);

                            cur_block = count;
                            cur_offset = offset;
                        } else if(next_block == -1) {
                            //printf("%s %d\n", "SET NEXT", offset);
                            next_block = count;
                            next_offset = offset;
                            //printf("%d\n", offset);
                            break;
                        }
                    }
                }
                count++;
            }
            if(next_block != -1)
                break;
        }
        //HAS CUR/NEXT BLOCK
        //set block data
        //if not last, set fat entry to next
        //if last, set fat entry to -1

        uint8_t data[513] = {'/0'};
        fread(data,512, 1, input);
        //printf("%s\n------\n", str);
        //printf("%d\n", cur_block*512);
        memcpy(address+cur_block*512,&data,512);
        if(u == block_count-1)
            memcpy(address+cur_offset, &last, 4);
        else
            memcpy(address+cur_offset, &next_block, 4);

        uint32_t number;
        memcpy(&number, address+cur_offset, 4);
        uint32_t converted = htonl(number);
        memcpy(address+cur_offset, &converted, 4);

        
    }


    //TODO: ADD DIRECTORY ENTRY
    cur_block = sb->root_dir_start_block;
    tmp_block = 0;
    while(1) { 

        for(int o = 0; o < 512; o=o+64) {
            int offset = ntohl(cur_block)*512+o;

            struct dir_entry_t* dir = (struct dir_entry_t*) (address+offset);
            if((dir->status == 0) || (strcmp(dir->filename, argv[2]) == 0)) {
                dir->status = 3;
                dir->starting_block = ntohl(new_block_start);
                dir->block_count = block_count;

                sz = ntohl(sz);
                memcpy(&(dir->size), &sz, 4);

                char* fn = argv[2];
                memcpy((dir->filename), fn, 31);

                time_t t = time(NULL);
                struct tm tm = *localtime(&t);

                dir->ct.second = tm.tm_sec;
                dir->ct.minute = tm.tm_min;
                dir->ct.hour = tm.tm_hour;
                dir->ct.day = tm.tm_mday;
                dir->ct.month = tm.tm_mon + 1;
                dir->ct.year = ntohs(tm.tm_year + 1900);

                munmap(address,buffer.st_size);
                close(fd);
                printf("%s\n", "File successfully copied to current directory!");
                return;
            }
        }

        if(ntohl(cur_block) == -1) {
            //ADD NEW BLOCK
            printf("%s\n", "No room to add file");
        }

        memcpy(&tmp_block, address + 512*ntohl(sb->fat_start_block) + ntohl(cur_block)*4, 4);
        memcpy(&cur_block, &tmp_block, 4);
    }
    munmap(address,buffer.st_size);
    close(fd);
}