#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <time.h>


struct __attribute__((__packed__)) superblock_t {
    uint8_t   fs_id [8];
    uint16_t block_size;
    uint32_t file_system_block_count;
    uint32_t fat_start_block;
    uint32_t fat_block_count;
    uint32_t root_dir_start_block;
    uint32_t root_dir_block_count;
};


int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("%s\n", "Not enough arguments. USAGE: ./diskput <image file>");
        return;
    }
    int fd = open(argv[1], O_RDWR);
    struct stat buffer;
    int status = fstat(fd, &buffer);

    void* address=mmap(NULL, buffer.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    struct superblock_t* sb = (struct superblock_t*) address;


    printf("%s\n", "Super block information:");
    printf("Block size: %d\n", ntohs(sb->block_size));
    printf("Block count: %d\n", ntohl(sb->file_system_block_count));
    printf("FAT starts: %d\n", ntohl(sb->fat_start_block));
    printf("FAT blocks: %d\n", ntohl(sb->fat_block_count));
    printf("Root directory start: %d\n", ntohl(sb->root_dir_start_block));
    printf("Root directory blocks: %d\n", ntohl(sb->root_dir_block_count));

    int free = 0;
    int reserved = 0;
    int allocated = 0;
    for(int i = ntohl(sb->fat_start_block); i < ntohl(sb->fat_block_count) + ntohl(sb->fat_start_block); i++) {
            for(int o = 0; o < 512; o=o+4) {
                int offset = i*512+o;

                uint32_t number;
                memcpy(&number, address+offset, 4);

                if(ntohl(number) == 0) free++;
                else if(ntohl(number) == 1) reserved++;
                else allocated++;
            }
    }
    printf("\n%s\n", "FAT information:");
    printf("Free Blocks: %d\n", free);
    printf("Reserved Blocks: %d\n", reserved);
    printf("Allocated Blocks: %d\n", allocated);
    munmap(address,buffer.st_size);
    close(fd);
}