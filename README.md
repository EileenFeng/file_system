# file_system
##Eileen Feng

- Design Doc & Implementation
  - An updated version of design doc is included in this repo, which is not that different from the design doc that I submitted, just some minor changes for the structs for the convenience of implementing. 


- How to run:
  - Type in 'make' to compile, and the library is compiled under the name 'libfilesys.so'. The library functions are included in 'file_lib.c'. 

- What works & does not work
  - most library functions work. However 'f_read' and 'f_write' have some issues for large files that reaches 'i3block' level.
  - Does not support permissions: When user created a file or directory, all file and directory created will have the default permission.
  
  - shell commands:
    - ls: supports '-F' and '-l' flags
    - mkdir: supports 'mkdir <dir_name>', does not support entering permission of the directory created
    - rmdir: works when entering the corret path;
    - cd: mostly works
    - pwd: mostly works
    - cat: mostly works with decent size files (large files that reaches 'i3block' level might have issues because of 'f_read' and 'f_write'
    - more: Not implemented... Do not have enough time
    - mount: supports mounting at a specific position. For instance, when a regular user logged in and typed in 'mount eileen', the current working directory of the user, right after mounting, will be at the mounting point, aka 'home/rusr/eileen'. Does not support nested mounting!
    - unmount: mostly works
    
  
- Limitations:
  - Does not fully support nested mounting: after mounting one disk, cannot mount another disk under the mounted disk, will cause issue with current working directory and undefined behaviors.
  - Have Issues with 'f_read' and 'f_write' with large files, especially when reaches 'i3block' level
  - Small number of free blocks is lost after removing files. This is a corner cases that failed to be handled: When a file is removed, the corresponding 'dirent' is removed from the parent directory. At this point, if the parent directory becomes empty, the data block that it used to store the dirent for deleted files will be lost. Same cases applys when the parent directory is extremely large and reachs hight levels: 'indirect', 'i2block', 'i3block'.
  - Does not support changing permissions
  