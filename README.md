# Eileen_Feng_File_System

- A file system that allows creating a disk, mounting a disk, creating, removing, editing files and directories. File editing including overwriting (open an existing file to write will truncate the file), as well as appending (appending new things to the end of the file). File seeking is also available. 

- File system functions implementation are available in 'file_lib.h' and 'file_lib.c'

- Design Doc & Implementation
  - An updated version of design doc is included in this repo, which is not that different from the design doc that I submitted, just some minor changes for the structs for the convenience of implementing. 
  - Allocated 15 blocks in total for inodes, 17408 free blocks, with blocksize 512 bytes

- How to run:
  - Type in 'make' to compile, and the library is compiled under the name 'libfilesys.so'. The library functions are included in 'file_lib.c'. 
  - The 'format' program will format a disk with 3 default directories: /home, /home/rusr (regular user), and /home/susr (super user)

- What works & does not work
  - most library functions work. However 'f_read' and 'f_write' have some issues for large files that reaches 'i3block' level.
  - Does not support permissions: When user created a file or directory, all file and directory created will have the default permission. When user try to access files, file permissions are not fully checked. However 'f_open' an existing file will truncate that file. 
  
  - shell commands:
    - redirection: supports redirection with 'cat' and redirection with system build in commands, however does not support redirecting the shell commands implemented by this project. For instance: 'ps aux > haha.txt' will direct the output to haha.txt, however 'ls > haha.txt' does not work, will possibly cause the shell to terminate or other undefined behaviors. Requires proper spacing. 
    - ls: roughly supports '-F' and '-l' flags
    - mkdir: supports 'mkdir <dir_name>', does not support entering permission of the directory created
    - rmdir: works when entering the corret path;
    - chmod: mostly working, however changing the modes does not affect how users access files. Require the second argument to be the new mode. Requires proper spacing. 
    - cd: mostly works
    - pwd: mostly works
    - cat: mostly works with decent size files (large files that reaches 'i3block' level might have issues because of 'f_read' and 'f_write' with large files. Support 'cat >> <filename>' for appending. Requires proper spacing.
      - When use redirection (for instance 'yes > y') to create new files, allow some time for the file to be created and updated on disk. 'ls' right after creating a new large file might not show the newly created file because the program needs some time to update the disk. 
    - more: Not implemented... Do not have enough time
    - mount: supports mounting at a specific position. For instance, when a regular user logged in and typed in 'mount eileen', the current working directory of the user, right after mounting, will be at the mounting point, aka 'home/rusr/eileen'. Does not support nested mounting!
    - unmount: mostly works
    
- Tests
  - 'test.c', executable under name 'testinit', tests for opening files, reopen the file, read, and write 'i2block' level files. Needs to reformat the disk to run sometimes in order to obtain enough space and free inodes. 
  - 'testmkdir.c' tests for making directory, creating file in the newly created directory, and remove directory.
  - 'test_dirs.c' tests making around 40 directories. 
  
- Limitations:
  - Does not fully support nested mounting: after mounting one disk, cannot mount another disk under the mounted disk, will cause issue with current working directory and undefined behaviors.
  - Have Issues with 'f_read' and 'f_write' with large files, especially when reaches 'i3block' level
  - Small number of free blocks is lost after removing files. This is a corner cases that failed to be handled: When a file is removed, the corresponding 'dirent' is removed from the parent directory. At this point, if the parent directory becomes empty, the data block that it used to store the dirent for deleted files will be lost. Same cases applys when the parent directory is extremely large and reachs hight levels: 'indirect', 'i2block', 'i3block'.
  - Does not support permission checking for files. 'chmod' is implemented, however when files are accessed, file's permissions are not fully checked. 
  - Memory Leaks: a lot... Did not have enough time to handle.... :( (really sad face)
  - Error handling is not fully implemented due to time issue. Under most cases no segmentation faults would occur, but rarely it happened, most usually due to incomplete error handling. 
