#define _GNU_SOURCE

#include <err.h>
#include <errno.h>
#include <linux/limits.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <wait.h>

#include "change_root.h"

#define CONTAINER_ID_MAX 16
#define CHILD_STACK_SIZE 4096 * 10

typedef struct container {
  char id[CONTAINER_ID_MAX];
  
  // TODO: Add fields
  char cwd[PATH_MAX];
  char img[PATH_MAX];
  char cmd[PATH_MAX*10];

} container_t;

/**
 * `usage` prints the usage of `client` and exists the program with
 * `EXIT_FAILURE`
 */
void usage(char* cmd) {
  printf("Usage: %s [ID] [IMAGE] [CMD]...\n", cmd);
  exit(EXIT_FAILURE);
}

/**
 * `container_exec` is an entry point of a child process and responsible for
 * creating an overlay filesystem, calling `change_root` and executing the
 * command given as arguments.
 */
int container_exec(void* arg) {
  container_t* container = (container_t*)arg;
  // this line is required on some systems
  if (mount("/", "/", "none", MS_PRIVATE | MS_REC, NULL) < 0) {
    err(1, "mount / private");
  }

  // TODO: Create a overlay filesystem
  // `lowerdir`  should be the image directory: ${cwd}/images/${image}
  // `upperdir`  should be `/tmp/container/{id}/upper`
  // `workdir`   should be `/tmp/container/{id}/work`
  // `merged`    should be `/tmp/container/{id}/merged`
  // ensure all directories exist (create if not exists) and
  // call `mount("overlay", merged, "overlay", MS_RELATIME,
  //    lowerdir={lowerdir},upperdir={upperdir},workdir={workdir})`

  // First check if the directories exist, if not then use mkdir to create it, arg should be the name of the container
  struct stat s;
  //Checking or creating lowerdir
  char lowerdir[PATH_MAX*2];
  strcat(lowerdir, container->cwd);
  strcat(lowerdir, "/images/$");
  strcat(lowerdir, container->img);
  //printf("Lower: %s\n", lowerdir);
  if (stat(lowerdir,&s) == -1){
    if (mkdir(lowerdir, 0700) < 0 && errno != EEXIST){
      //return -1;
      //printf("Here 1?");
      err(1, "Failed to create a directory to store container file systems");
    }
  }
  //Creating dir to hold the upper, work, and merged
  char overdir[PATH_MAX];
  strcat(overdir, "/tmp/container/");
  strcat(overdir, container->id);
  //printf("%s\n", overdir);
  if(stat(overdir, &s) == -1){
    if (mkdir(overdir, 0700) < 0 && errno != EEXIST){
      //return -1;
      err(1, "Failed to create a directory to store container file systems");
    }

  }

  //Checking or creating upperdir
  char upperdir[PATH_MAX*2];
  strcat(upperdir, "/tmp/container/");
  strcat(upperdir, container->id);
  strcat(upperdir, "/upper");
  //printf("Upper: %s\n", upperdir);
  if (stat(upperdir,&s) == -1){
    if(mkdir(upperdir, 0700) < 0 && errno != EEXIST){
      //return -1;
      //printf("Here 2?\n");
      err(1, "Failed to create a directory to store container file systems");
    }
  }
  //Checking or creating workdir
  char workdir[PATH_MAX*2];
  strcat(workdir, "/tmp/container/");
  strcat(workdir, container->id);
  strcat(workdir, "/work");
  //printf("Work: %s\n", workdir);
  if (stat(workdir,&s) == -1){
    if (mkdir(workdir, 0700) < 0 && errno != EEXIST){
      //return -1; 
      //printf("Here 3?\n");
      err(1, "Failed to create a directory to store container file systems");
    }
  }
  //Checking or creating merged
  char merged[PATH_MAX*2];
  strcat(merged, "/tmp/container/");
  strcat(merged, container->id);
  strcat(merged, "/merged");
  //printf("Merged: %s\n", merged);
  if (stat(merged,&s) == -1){
    if (mkdir(merged, 0700)  < 0 && errno != EEXIST){
      //return -1;
      //printf("Here 4?\n");
      err(1, "Failed to create a directory to store container file systems");
    }
  }

  // Calling mount




  // TODO: Call `change_root` with the `merged` directory
  // change_root(merged)

  // TODO: use `execvp` to run the given command and return its return value

  return 0;
}

int main(int argc, char** argv) {
  if (argc < 4) {
    usage(argv[0]);
  }

  /* Create tmpfs and mount it to `/tmp/container` so overlayfs can be used
   * inside Docker containers */
  if (mkdir("/tmp/container", 0700) < 0 && errno != EEXIST) {
    err(1, "Failed to create a directory to store container file systems");
  }
  if (errno != EEXIST) {
    if (mount("tmpfs", "/tmp/container", "tmpfs", 0, "") < 0) {
      err(1, "Failed to mount tmpfs on /tmp/container");
    }
  }

  /* cwd contains the absolute path to the current working directory which can
   * be useful constructing path for image */
  char cwd[PATH_MAX];
  getcwd(cwd, PATH_MAX);

  container_t container;
  strncpy(container.id, argv[1], CONTAINER_ID_MAX);

  // TODO: store all necessary information to `container`

  strncpy(container.cwd, cwd, PATH_MAX);
  strncpy(container.img, argv[2], PATH_MAX);
  strncpy(container.cmd, argv[3], PATH_MAX);

  /* Use `clone` to create a child process */
  char child_stack[CHILD_STACK_SIZE];  // statically allocate stack for child
  int clone_flags = SIGCHLD | CLONE_NEWNS | CLONE_NEWPID;
  int pid = clone(container_exec, &child_stack, clone_flags, &container);
  if (pid < 0) {
    err(1, "Failed to clone");
  }

  waitpid(pid, NULL, 0);
  return EXIT_SUCCESS;
}
