#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "Include/constants.h"

// Function to spawn all the processes with using execvp()
int spawn(const char *program, char **arg_list) {
  execvp(program, arg_list);
  perror("Process execution failed");  // Check for errors
  return -1;
}

// Main function starts
int main() {
  // To check the konsole status
  int status_konsole;
  int result_id3, result_id5;
  // To recieve the exit statuses of the child processes
  int exit_status[NUM_PROCESSES + 1];  // With Watchdog
  int status;

  // Clear all file contents of watchdog file
  FILE *file_pw = fopen(PID_FILE_PW, "w");
  if (file_pw == NULL) {  // Check for errors
    perror("Error opening PID_FILE_PW");
    return -1;
  }
  fclose(file_pw);  // Close file

  // Clear all file contents of other process files
  char *fnames[NUM_PROCESSES] = PID_FILE_SP;
  for (int i = 0; i < NUM_PROCESSES; i++) {
    FILE *file_sp = fopen(fnames[i], "w");
    if (file_sp == NULL) {  // Check for errors
      perror("Error opening a file in fnames");
      return 1;
    }
    fclose(file_sp);  // Close file
  }

  /* CREATING 4 COUPLES OF PIPES*/
  // Creating FDs
  // Window
  int window_server[2];
  int server_window[2];
  // Drone
  int drone_server[2];
  int server_drone[2];
  // Keyboard
  int keyboard_server[2];
  int server_keyboard[2];
  // Obstacles
  int obstacles_server[2];
  int server_obstacles[2];
  // Targets
  int targets_server[2];
  int server_targets[2];

  /* CREATING PIPES */
  // Window to server
  if (pipe(window_server) == -1) {
    perror("Error creating window_server");
    return -1;
  }
  // Server to window
  if (pipe(server_window) == -1) {
    perror("Error creating server_window");
    return -1;
  }

  // Drone to server
  if (pipe(drone_server) == -1) {
    perror("Error creating drone_server");
    return -1;
  }
  // Server to drone
  if (pipe(server_drone) == -1) {
    perror("Error creating server_drone");
    return -1;
  }

  // Obstacles to server
  if (pipe(obstacles_server) == -1) {
    perror("Error creating obstacles_server");
    return -1;
  }
  // Server to obstacles
  if (pipe(server_obstacles) == -1) {
    perror("Error creating server_obstacles");
    return -1;
  }

  // Targets to server
  if (pipe(targets_server) == -1) {
    perror("Error creating targets_server");
    return -1;
  }
  // Server to targets
  if (pipe(server_targets) == -1) {
    perror("Error creating server_targets");
    return -1;
  }

  // Initialize data arrays shared between processes
  char window_arg[MAX_LEN];
  char drone_arg[MAX_LEN];
  char obstacles_arg[MAX_LEN];
  char targets_arg[MAX_LEN];

  // Format fds to arrays
  sprintf(window_arg, "%d %d %d %d", window_server[0], window_server[1],
          server_window[0], server_window[1]);
  sprintf(drone_arg, "%d %d %d %d", drone_server[0], drone_server[1],
          server_drone[0], server_drone[1]);
  sprintf(obstacles_arg, "%d %d %d %d", obstacles_server[0],
          obstacles_server[1], server_obstacles[0], server_obstacles[1]);
  sprintf(targets_arg, "%d %d %d %d", targets_server[0], targets_server[1],
          server_targets[0], server_targets[1]);

  /* SPAWNING CHILDREN*/
  // Argument lists to pass to the command
  char *arg_list1[] = {"./watchdog", NULL};
  char *arg_list2[] = {"./server",    window_arg,  drone_arg,
                       obstacles_arg, targets_arg, NULL};
  char *arg_list3[] = {"konsole", "-e", "./window", window_arg, NULL};
  char *arg_list4[] = {"./drone", "-lm", drone_arg, NULL};
  char *arg_list5[] = {"konsole", "-e", "./keyboard", NULL};
  char *arg_list6[] = {"./obstacles", obstacles_arg, NULL};
  char *arg_list7[] = {"./targets", targets_arg, NULL};

  // Spawning watchdog
  pid_t id1 = fork();
  // Error check
  if (id1 < 0) {
    perror("Error using id1 fork");
    return -1;
  }
  // Child process
  else if (id1 == 0) {
    printf("Spawning watchdog \n");
    spawn(arg_list1[0], arg_list1);
  }

  // Spawning server
  pid_t id2 = fork();
  // Error check
  if (id2 < 0) {
    perror("Error using id2 fork");
    return -1;
  }
  // Child process
  else if (id2 == 0) {
    printf("Spawning server\n");
    spawn(arg_list2[0], arg_list2);
  }

  // Spawning window
  pid_t id3 = fork();
  // Error check
  if (id3 < 0) {
    perror("Error using id3 fork");
    return -1;
  }
  // Child process
  else if (id3 == 0) {
    printf("Spawning window\n");
    spawn(arg_list3[0], arg_list3);
  }

  // Spawning drone
  pid_t id4 = fork();
  // Error check
  if (id4 < 0) {
    perror("Error using id4 fork");
    return -1;
  }
  // Child process
  else if (id4 == 0) {
    printf("Spawning drone\n");
    spawn(arg_list4[0], arg_list4);
  }

  // Spawning keyboard
  pid_t id5 = fork();
  // Error check
  if (id5 < 0) {
    perror("Error using id5 fork");
    return -1;
  }
  // Child process
  else if (id5 == 0) {
    printf("Spawning keyboard\n");
    spawn(arg_list5[0], arg_list5);
  }

  // Spawning obstacles
  pid_t id6 = fork();
  // Error check
  if (id6 < 0) {
    perror("Error using id6 fork");
    return -1;
  }
  // Child process
  else if (id6 == 0) {
    printf("Spawning obstacles\n");
    spawn(arg_list6[0], arg_list6);
  }

  // Spawning targets
  pid_t id7 = fork();
  // Error check
  if (id7 < 0) {
    perror("Error using id7 fork");
    return -1;
  }
  // Child process
  else if (id7 == 0) {
    printf("Spawning targets\n");
    spawn(arg_list7[0], arg_list7);
  }

  // Loop to monitor the status of the konsoles
  while (1) {
    // Check the statuses
    result_id3 = waitpid(id3, &status_konsole, WNOHANG);
    if (result_id3 == -1) {
      perror("Master: error checking process id3 status");
      return -1;
    }
    result_id5 = waitpid(id5, &status_konsole, WNOHANG);
    if (result_id5 == -1) {
      perror("Master: error checking process id5 status");
      return -1;
    }

    // Check if any of the two konsole processes have terminated
    if (result_id3 > 0 || result_id5 > 0) {
      if (result_id3 > 0) {
        // Terminate the id5 process also
        if (kill(id5, SIGTERM) == -1) {
          perror("Master: id5 kill failed");
          exit(EXIT_FAILURE);
        }
      } else if (result_id5 > 0) {
        // Terminate the id3 process also
        if (kill(id3, SIGTERM) == -1) {
          perror("Master: id3 kill failed");
          exit(EXIT_FAILURE);
        }
      }

      // Wait for all children (+ watchdog) to terminate
      for (int i = 0; i < (NUM_PROCESSES + 1); i++) {
        wait(&status);
        exit_status[i] = WEXITSTATUS(status);
        wait(NULL);
      }

      // Break the infinite loop
      break;
    }

    // Wait for a certain time
    usleep(WAIT_TIME);
  }

  // Print all the childrens termination statuses
  for (int i = 0; i < (NUM_PROCESSES + 1); i++) {
    printf("Process %d terminated with status: %d\n", i + 1, exit_status[i]);
  }

  // End the master process
  return 0;
}