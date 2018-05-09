#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "file_lib.h"
#include "fs_struct.h"

#define HISTSIZE 5 // set to default size

// global variables
int buff_size = 32;
int arg_nums = 0;
char** history; // store history inputs
int count = 0; // keep counting the number of inputs in total
int non_history_count = 0; //keep counting the number of inputs that 
int interrupt = 0;
// current working directory
char cwd[MAX_LENGTH];
char mounted_point[MAX_LENGTH];
// users
struct user reg_user;
struct user spr_user;

/*********************** FUNCTION PROTOCOLS ****************************/
char* read_input();
char** parse_input(char* input);
int exec_cd(char** path_args);
char** get_recent_history_input(int num);
void store_history(char* input);
int execute_history_input(char* history_input);
int print_history();
int exec_history(char** arg);
int exec_args(char** args, char*input, int free_args);
void sig_handler(int sig);
// file system
int mount_disk(char* mounting_point);

// helpers for file system
void construct_user();
int parse_inputpath(char* input_path, char* fs_path);
char** tokenizer(char* filepath);
void freeParse(char** parse_result);
void get_parent_path(char* filepath);
int start_with(char* filepath, char* prefix);
/************************ FUNCTIONS ****************************/
int mount_disk(char* mounting_point) {
  strcpy(mounted_point, mounting_point);
  // needs to do user login, and when call fucntions on files need to check permission
  int ret = f_mount(mounting_point);
  strcpy(cwd, mounting_point);
  return ret;
}

int handle_ls(char* filepath) {
  char fspath[MAX_LENGTH];
  parse_inputpath(filepath, fspath);
  printf("hand_ls:    after parse fs path is %s\n", fspath);
  int dirfd = f_opendir(fspath);
  if(dirfd == FAIL) {
    printf("Open directory %s failed\n", filepath);
    return FALSE;
  }
  struct dirent* temp = f_readdir(dirfd);
  while(temp != NULL) {
    // print filename
    printf("%s\t", temp->filename);
  }
  printf("\n");
  //f_rewind(dirfd);
  f_closedir(dirfd);
  return TRUE;
}



/********************* FS helpers ***********************/

void construct_user() {
  spr_user.uid = SPRUSER;
  spr_user.permission_value = RWE;
  spr_user.home_inode = SPRINODE;
  strcpy(spr_user.username, "superuser");
  strcpy(spr_user.password, "super");

  reg_user.uid = REGUSER;
  reg_user.permission_value = RW;
  reg_user.home_inode = REGINODE;
  strcpy(reg_user.username, "superuser");
  strcpy(reg_user.password, "super");
}

void get_parent_path(char* filepath) {
  printf("get_parent_path:    in getting the parent path of %s\n", filepath);
  char old[MAX_LENGTH];
  strcpy(old, filepath);
  int new_len = strlen(filepath);
  bzero(filepath, new_len);
  char temp = old[new_len-1];
  while(temp != '/') {
    new_len --;
    temp = old[new_len -1];
  }
  new_len --;
  strncpy(filepath, old, new_len);
  filepath[new_len] = '\0';
  printf("get_parent_path: resulting path is %s\n", filepath);
}

int start_with(char* filepath, char* prefix) {
    int len = strlen(prefix);
    if(strlen(filepath) < strlen(prefix)) {
        return FALSE;
    }
    for(int i = 0; i < len; i++) {
        if(filepath[i] != prefix[i]) {
            return FALSE;
        }
    }
    return TRUE;
}

char** tokenizer(char* filepath) {
  char delim[2] = "/";
  int len = 20;
  int size = 20;
  int count = 0;
  char file_path[MAX_LENGTH];
  strcpy(file_path, filepath);
  char** parse_result = (char**)malloc(sizeof(char*) * size);
  bzero(parse_result, size * sizeof(char*));
  char* token;
  printf("parse file path: filepath is %s\n", filepath);
  token = strtok(file_path, delim);
  while(token != NULL) {
    if(count >= size) {
      size += len;
      parse_result = realloc(parse_result, sizeof(char*) * size);
    }
    char* temp = malloc(sizeof(char) * (strlen(token) + 1));
    strcpy(temp, token);
    parse_result[count] = temp;
    printf("parse file path: parse result %d is %s\n", count, parse_result[count]);
    token = strtok(NULL, delim);
    count ++;
  }
  parse_result[count] = NULL;
  return parse_result;
}


void freeParse(char** parse_result) {
  int count = 0;
  char* token = parse_result[count];
  while(token != NULL) {
    free(token);
    count ++;
    token = parse_result[count];
  }
  free(parse_result);
}

// fs path must be max_length
int parse_inputpath(char* input_path, char* fs_path) {
  bzero(fs_path, MAX_LENGTH);
  strcpy(fs_path, "");
  char** tokens = tokenizer(input_path);
  int count = 0;
  char* token = tokens[count];
  while(token != NULL) {
    printf("parse_inputpath: current token is %s and before this interation path is %s\n", token, fs_path);
    if(strcmp(token, ".") == SUCCESS) {
    //   if(count == 0) {
    //     strcat(fs_path, cwd);
    //   } else {
        count++;
        token = tokens[count];
        continue;
      //}
    } else if (strcmp(token, "..") == SUCCESS) {
      get_parent_path(fs_path);
    } else {
      strcat(fs_path, "/");
      strcat(fs_path, token);
    }
    printf("parse_inputpath: after this interation path is %s\n", fs_path);
    count++;
    token = tokens[count];
  } 
    if(start_with(fs_path, cwd)) {
        int prefix = strlen(cwd);
        char temp[MAX_LENGTH];
        strcpy(temp, fs_path);
        bzero(fs_path, MAX_LENGTH);
        char* target = temp + prefix;
        strncpy(fs_path, target, strlen(target));
        fs_path[strlen(target)] = '\0';
    }
  freeParse(tokens);
  return SUCCESS;
}

// execute command line inputs
int exec_args(char** args, char*input, int free_args) {
  int value;
  if(args[0][0] == '!') {
    return execute_history_input(args[0]);
  } else {
    if(strcmp(args[0], "format") == SUCCESS) {
      pid_t pid=fork();
      if (pid==0) { 
          static char *argv[]={"./format",NULL};
          execv("./format",argv);
          free(args);
          exit(0);
      }
      else { /* pid!=0; parent process */
          waitpid(pid,0,0); /* wait for child to exit */
      }
      value = TRUE;
    } else if (strcmp(args[0], "mount") == SUCCESS) {
      value = mount_disk(args[1]);
    } else if(strcmp(args[0], "ls") == SUCCESS) {
      value = 
    }
    
    
    else if(strcmp(args[0], "history") == SUCCESS) {
      value =  exec_history(args);
    }else if(strcmp(args[0], "cd") == SUCCESS) { // if the command is cd
      value = exec_cd(args);
    }else if(strcmp(args[0], "exit") == SUCCESS) { // if the command is exit
      return 0;
    } else {
      pid_t pid;
      int status;
      pid = fork();
      if(pid < 0) {
	      printf("Execute command failed - fork failed! \n");
      } else if (pid == 0) { // children process
        if(execvp(args[0], args) < 0) {
          printf("An error occurred during execution, executing command %s failed!\n", args[0]);
        }
        free(args);
        exit(0); // usage of exit referrence to: https://brennan.io/2015/01/16/write-a-shell-in-c/  
      } else if (pid > 0) {
        pid_t res = wait(&status);
        if(res < 0) {
          printf("Child %d is not waited by the parent\n", pid);
        }
      }
      value = TRUE;
    }
  }
  // when exec_history_input calls exec_args, need to free the array 'args'
   if(free_args) { 
     free(args); 
   } 
    return value;
}

// read in the input
char* read_input() {
  size_t readn;
  char* input = NULL;
  printf("mysh > ");
  if(getline(&input, &readn, stdin) < 0) {
    printf("getline from stdin failed! \n");
    free(input);
    input = NULL;
  }
  return input;
}

// parse the input
char** parse_input(char* input) {
  char** tokens = malloc(buff_size * sizeof(char*));
  char* token = strtok(input, " \t\r\n\a");
  int count = 0;
  while(token) {
    tokens[count] = token;
    count++;
    if(count >= buff_size) {
      buff_size *= 2;
      tokens = (char**) realloc(tokens, buff_size);
    }
    token = strtok(NULL, " \t\r\n\a");
  }
  tokens[count] = NULL;
  arg_nums = count;
  return tokens;
}

// cd is not executable command, function for cd
int exec_cd(char** path_args) {
  if(path_args[1] == NULL) { // when no path is provided
    printf("Please provide a path for changing directory! \n");
  } else if(path_args[2] != NULL) { // when too many arguments are passed into cd
    printf("cd: too many arguments! \n");
  } else {
    if(chdir(path_args[1]) < 0) {
      printf("Change to directory %s failed! \n", path_args[1]);
    }
  }
  return TRUE;
}

// get the address of history input
char** get_recent_history_input(int num) {
  int index = count >= HISTSIZE ? num % HISTSIZE : num;
  index = index == 0 ? HISTSIZE - 1 : index - 1; // index should be one less than the size
  return &history[index];
}

// store inputs
void store_history(char* input) { // incre = 1 when we need to increment count
  count++;
  int index = count % HISTSIZE;
  index = index == 0 ? HISTSIZE - 1 : index - 1; 
  if(count > HISTSIZE) {
    free(history[index]);
  }
  history[index] = malloc((strlen(input)+1)*sizeof(char));
  char* temp = input;
  int tcount = 0;
  while(tcount < strlen(input)) {
    history[index][tcount] = (*temp == '\n') ? ' ' : *temp;
    tcount++; temp++;
  }
  history[index][tcount] = '\0';
}

// execute history input is requested
int execute_history_input(char* history_input) {
  int index = 0;
  int count_backwards = 0; // check if it's counting backwards for history input
  if(strcmp(history_input, "!!") == 0) {
    count_backwards = 1;
    index = 1;
  } else {
    int start = 0;
    if(history_input[1] == '-' ) {
      start = 2;
      count_backwards = 1;
    } else if(isdigit(history_input[1])) {
      start = 1;
    } else {
      printf("Invalid input for executing history input!\n");
      store_history(history_input);
      return TRUE;
    }
    for(int i = start; i < strlen(history_input); i++) {
      if(!isdigit(history_input[i])) {
	printf("History index can be positive numbers only! \n");
	store_history(history_input);
	return TRUE;
      } else {
	index *= 10;
	index += history_input[i] - '0';
      }
    }
  }
  if(index <=0 || index > HISTSIZE || index > count || index > non_history_count) {
    printf("History input index too small or too big! Not enough history inputs to execute this line\n");
    store_history(history_input);
    return 1;
  }
  int minus = (count >= HISTSIZE) ? HISTSIZE : count;
  // get the history input, calculation to ensure the right index
  char* temp = count_backwards ? *get_recent_history_input(count - index + 1) : *get_recent_history_input(count - (minus - index));
  // hard copy since parse_input will change the string
  char temp_input[strlen(temp) + 1];
  for(int i = 0; i < strlen(temp); i++) { temp_input[i] = temp[i];}
  temp_input[strlen(temp)] = '\0';
  store_history(temp_input);
  char** args = parse_input(temp_input);
  if(arg_nums == 0) {
    printf("History input has no command inputs!\n");
    free(args);
    return TRUE;
  }
  if(args[0][0] == '!') {
    free(args);
    return execute_history_input(temp_input);
  } else {
    return exec_args(args, temp_input, 1);
  }
}

// function for print all history input
int print_history(){
  printf("History inputs are: \n");
  int ind = (count >= HISTSIZE) ? HISTSIZE : count;
  for(int i = 0; i < ind; i++) {
    printf("%d. ", i+1);
    printf("%s\n", *get_recent_history_input(count - (ind - i - 1)));
  }
  return TRUE;
}

// function for executing 'history' command
int exec_history(char** arg) {
  if(arg[1] != NULL) {
    printf("'history' does not take any arguments! \n");
  } else {
    print_history();
  }
  return TRUE;
}


void sig_handler(int sig) {
  interrupt = 1;
}

int main(int argc, char** argv) {
  char *input, **args;
  int run;
  history = malloc(HISTSIZE * sizeof(char*));
  signal(SIGINT, sig_handler);
  do {
    input = read_input();
    if(!interrupt) {
      if(input == NULL) {
	printf("Input readin failed!\n");
      } else {
	if(*input != '!') {
	  store_history(input);
	  non_history_count ++;
	} 
	args = parse_input(input);
	if(arg_nums == 0) {
	  printf("No command line inputs!\n");
	} else {
	  run = exec_args(args, input, 0);
	}
	free(input);
	free(args);
      }
    } else {
      store_history(input);
      interrupt = 0;
      run = 1;
      free(input);
    }
  } while (run);
  int end = count >= HISTSIZE ? HISTSIZE : count;
  for(int i = 0; i < end; i++) {
    free(history[i]);
  }
  free(history);
}


