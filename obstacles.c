#include <fcntl.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <ncurses.h>
#include <semaphore.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "Include/constants.h"

// Function to log a message to the logfile_obstacles.txt
void log_message(FILE *fp, const char *message, enum DataType type,
                 void *data) {
  // Get the current time
  time_t now = time(NULL);
  struct tm *timenow;
  time(&now);
  timenow = gmtime(&now);

  // Format time as a string
  char time_str[26];
  strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timenow);

  // Open the logfile
  fp = fopen("../Log/logfile_obstacles.txt", "a");
  if (fp == NULL) {  // Check for errors
    perror("Error opening logfile");
    return;
  }

  // Print time and message to the file
  fprintf(fp, "[%s] %s ", time_str, message);

  // Print data based on data type
  switch (type) {
    case CHAR_TYPE:
      fprintf(fp, "%c\n", *((char *)type));
      break;
    case INT_TYPE:
      fprintf(fp, "%d\n", *((int *)data));
      break;
    case DOUBLE_TYPE:
      fprintf(fp, "%f\n", *((double *)data));
      break;
    default:
      fprintf(fp, "Unknown data type\n");
  }

  // Close the file
  fclose(fp);
}

int main(int argc, char *argv[]) {
  // Logfile
  FILE *fp;
  // Open the logfile to either create or delete the content
  fp = fopen("../Log/logfile_obstacles.txt", "w");
  if (fp == NULL) {  // Check for error
    perror("Error opening logfile");
    return -1;
  }
  fclose(fp);  // Close the file

  // Initialize a counter for watchdog signal sending
  int counter = 0;
  // Initialize a counter for deleting a obstacle
  int counter_obs = 0;
  int rand_obs = 0;

  /* PIPES */
  // Creating the file descriptors
  int obstacles_server[2];
  int server_obstacles[2];
  // Scanning and closing unecessary fds
  sscanf(argv[1], "%d %d %d %d", &obstacles_server[0], &obstacles_server[1],
         &server_obstacles[0], &server_obstacles[1]);
  close(obstacles_server[0]);
  close(server_obstacles[1]);

  /* WATCHDOG */
  pid_t watchdog_pid;              // For watchdog PID
  pid_t obstacles_pid = getpid();  // Recieve the process PID

  // Get the file locations
  char *fnames[NUM_PROCESSES] = PID_FILE_SP;

  // Open the obstacles process tmp file to write its PID
  FILE *pid_fp = fopen(fnames[OBSTACLES_SYM], "w");
  if (pid_fp == NULL) {  // Check for errors
    perror("Error opening Obstacles tmp file");
    return -1;
  }
  fprintf(pid_fp, "%d", obstacles_pid);
  fclose(pid_fp);  // Close file

  // Read watchdog PID
  FILE *watchdog_fp = NULL;
  struct stat sbuf;

  // Check if the file size is bigger than 0
  if (stat(PID_FILE_PW, &sbuf) == -1) {
    perror("error-stat");
    return -1;
  }
  // Wait until the file has data
  while (sbuf.st_size <= 0) {
    if (stat(PID_FILE_PW, &sbuf) == -1) {
      perror("error-stat");
      return -1;
    }
    usleep(50000);
  }

  // Open the watchdog tmp file
  watchdog_fp = fopen(PID_FILE_PW, "r");
  if (watchdog_fp == NULL) {  // Check for errors
    perror("Error opening watchdog PID file");
    return -1;
  }

  // Read the watchdog PID from the file
  if (fscanf(watchdog_fp, "%d", &watchdog_pid) != 1) {
    perror("Error reading Watchdog PID from file.\n");
    fclose(watchdog_fp);
    return -1;
  }

  // Close the file
  fclose(watchdog_fp);

  /* GENERATE OBSTACLES */
  // Seed the random number generator with the current time
  srand(time(NULL));
  // Define an array of coordinates of an old and new obstacle
  struct shared_data data;
  // Define an array of coordinates to store obstacles
  struct shared_data obstacles[NUM_OBSTACLES];
  // Generate random coordinates for each obstacle
  for (int i = 0; i < NUM_OBSTACLES; ++i) {
    obstacles[i].real_y =
        rand() % (91) + 5;  // Random y coordinate between 5 and 95
    obstacles[i].real_x =
        rand() % (91) + 5;  // Random x coordinate between 5 and 95
  }
  // Send the array to the server
  if (write(obstacles_server[1], obstacles, sizeof(obstacles)) == -1) {
    perror("Writing to server failed");
  }

  for (int i = 0; i < NUM_OBSTACLES; ++i) {
    log_message(fp, "Wrote to server coordinate y", DOUBLE_TYPE,
                &obstacles[i].real_y);
    log_message(fp, "Wrote to server coordinate x", DOUBLE_TYPE,
                &obstacles[i].real_x);
  }

  // For the use of select
  fd_set rdfds;
  int retval;
  // Set the timeout to 0
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 0;

  // Set the max fd
  int max_fd = server_obstacles[0];
  // Key pressed by the user
  int ch = 0;

  while (1) {
    // Check if the server has send the ESC key
    FD_ZERO(&rdfds);  // Reset the reading set
    FD_SET(server_obstacles[0],
           &rdfds);  // Put the server_obstacles[0] to the set
    retval = select(max_fd + 1, &rdfds, NULL, NULL, &tv);
    // Check for errors
    if (retval == -1) {
      perror("Obstacles: select failed");
      return -1;
    }
    // Data is available
    else if (retval) {
      //  Read from the server
      if ((read(server_obstacles[0], &data, sizeof(struct shared_data))) ==
          -1) {
        perror("Obstacles: reading from server_obstacles failed ");
        return -1;
      }
      // Save the key recieved
      ch = data.ch;
    }

    // Check if it the ESC key was pressed
    if (ch == ESCAPE) {
      // Log the exit
      log_message(fp, "Exit key pressed ", INT_TYPE, &ch);
      // Break the loop
      break;
    }

    // Check if it is time to delete an obstacle
    if (counter_obs == DEL_TIME_OBS) {
      // Save the type as obstacles
      data.type = OBSTACLES_SYM;
      // Generate a random obstacle to be deleted
      rand_obs = rand() % NUM_OBSTACLES;
      data.num = rand_obs;
      // Generate a new obstacle position
      obstacles[rand_obs].real_y = rand() % (91) + 5;
      obstacles[rand_obs].real_x = rand() % (91) + 5;
      data.co_y = obstacles[rand_obs].real_y;
      data.co_x = obstacles[rand_obs].real_x;

      // Send the number of an old and coordinates of a new to the server
      if (write(obstacles_server[1], &data, sizeof(struct shared_data)) == -1) {
        perror("Obstacles: writing to server failed");
      }

      // Open the logfile to create space
      fp = fopen("../Log/logfile_obstacles.txt", "a");
      if (fp == NULL) {  // Check for errors
        perror("Error opening logfile");
        return -1;
      }
      fprintf(fp, "\n");  // Start from the new row
      fclose(fp);         // Close the file

      // Log all current obstacles
      log_message(fp, "Number of the obstacle to be deleted", INT_TYPE,
                  &rand_obs);
      for (int i = 0; i < NUM_OBSTACLES; ++i) {
        log_message(fp, "Obstacle coordinate y", DOUBLE_TYPE,
                    &obstacles[i].real_y);
        log_message(fp, "Obstacle coordinate x", DOUBLE_TYPE,
                    &obstacles[i].real_x);
      }

      // Reset the counter
      counter_obs = 0;
    } else {
      // Increment the counter
      counter_obs++;
    }

    // Send a signal after certain iterations have passed
    if (counter == PROCESS_SIGNAL_INTERVAL) {
      // Send signal to watchdog every process_signal_interval
      if (kill(watchdog_pid, SIGUSR1) < 0) {
        perror("kill");
      }
      // Set counter to zero (start over)
      counter = 0;
    } else {
      // Increment the counter
      counter++;
    }

    // Wait for a certain time
    usleep(WAIT_TIME);
  }
  // Clean up
  close(server_obstacles[0]);
  close(obstacles_server[1]);

  // End the obstacles program
  return 0;
}