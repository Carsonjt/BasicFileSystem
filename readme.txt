Instructions:
1. Unpack the file and nagivate into the p3 folder in your terminal
2. Type "make" to generate the 4 programs
3. Type one of the following:
    "./diskinfo <IMAGE FILE>" - Show information on the Superblock and FAT
    "./disklist <IMAGE FILE>" - List the files in the root directory
    "./diskget <IMAGE FILE> <NAME OF NEW FILE> <FILE TO COPY>" - Copy a file from the image file root to your current directory 
    "./diskput <IMAGE FILE> <NAME OF NEW FILE> <FILE TO COPY>" - Copy a file from your current directory to the image file root


Overall Design:
1. diskinfo:
    reads superblock and all FAT entries and outputs general information

2. disklist:
    reads tall root dir entries and outputs information on each file

3. diskget:
    Tries to read through root dir entries until it finds file to copy, then loops through the blocks of data,
    writing that data to a file at new location, closes file when done.

3. diskput:
    Reads given file 512 bytes at a time, finding the next 2 free blocks (current/next blocks). Writes data in current block,
    updating FAT to point to next free block. If last block, sets FAT to 0xFFFFFFFF instead of next block. Then adds the dir
    entry into the root entry at an available location with the key information such as current time, file name, file size and file type.