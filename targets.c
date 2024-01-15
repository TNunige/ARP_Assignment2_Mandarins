#include <fcntl.h>
#include <math.h>
#include <semaphore.h>
#include <signal.h>
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

// Function to log a message to the logfile_targets.txt
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
  fp = fopen("../Log/logfile_targets.txt", "a");
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
  //  Logfile
  FILE *fp;
  // Open the logfile to either create or delete the content
  fp = fopen("../Log/logfile_targets.txt", "w");
  if (fp == NULL) {  // Check for error
    perror("Error opening logfile");
    return -1;
  }
  // Close the file
  fclose(fp);
  // Initialize the counter for signal sending
  int counter = 0;
  // Initialize a counter for deleting a target
  int counter_targ = 0;
  int rand_tar = 0;

  /* PIPES */
  // Creating the file descriptors
  int targets_server[2];
  int server_targets[2];
  // Scanning and closing unecessary fds
  sscanf(argv[1], "%d %d %d %d", &targets_server[0], &targets_server[1],
         &server_targets[0], &server_targets[1]);
  close(targets_server[0]);
  close(server_targets[1]);

  // For the use of select
  fd_set rdfds;
  int retval;
  // Set the timeout to zero
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  // Set the max fd
  int max_fd = server_targets[0];
  // Key pressed by the user
  int ch = 0;

  /* WATCHDOG */
  pid_t watchdog_pid;            // For watchdog PID
  pid_t targets_pid = getpid();  // Recieve the process PID

  // Get the file locations
  char *fnames[NUM_PROCESSES] = PID_FILE_SP;

  // Open the targets process tmp file to write its PID
  FILE *pid_fp = fopen(fnames[TARGETS_SYM], "w");
  if (pid_fp == NULL) {  // Check for errors
    perror("Error opening Targets tmp file");
    return -1;
  }
  fprintf(pid_fp, "%d", targets_pid);
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

  /* GENERATE TARGETS */
  // Seed the random number generator with the process PID
  srand(targets_pid);
  struct shared_data data;
  // Define an array of coordinates to store targets
  struct shared_data targets[NUM_TARGETS];
  // Generate random target prositions
  for (int i = 0; i < NUM_TARGETS; ++i) {
    targets[i].real_y =
        rand() % (91) + 5;  // Random y coordinate between 5 and 95
    targets[i].real_x =
        rand() % (91) + 5;  // Random x coordinate between 5 and 95
  }

  // Send the data to server and check for errors
  if (write(targets_server[1], targets, sizeof(targets)) == -1) {
    perror("Targets: error writing to pipe target_server");
  }

  // Update the log
  for (int i = 0; i < NUM_TARGETS; ++i) {
    log_message(fp, "Sent the target position y", DOUBLE_TYPE,
                &targets[i].real_y);
    log_message(fp, "Sent the target position x", DOUBLE_TYPE,
                &targets[i].real_x);
  }

  while (1) {
    // Check if the server has send the ESC key
    FD_ZERO(&rdfds);                    // Reset the reading set
    FD_SET(server_targets[0], &rdfds);  // Put the server_targets[0] to the set
    retval = select(max_fd + 1, &rdfds, NULL, NULL, &tv);
    // Check for errors
    if (retval == -1) {
      perror("Targets: select failed");
      return -1;
    }
    // Data is available
    else if (retval) {
      //  Read from the server
      if ((read(server_targets[0], &data, sizeof(struct shared_data))) == -1) {
        perror("Targets: reading from server_obstacles failed ");
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

    // Check if it is time to delete a target
    if (counter_targ == DEL_TIME_TAR) {
      // Save the type as obstacles
      data.type = TARGETS_SYM;
      // Generate a random obstacle to be deleted
      rand_tar = rand() % NUM_TARGETS;
      data.num = rand_tar;
      // Generate a new obstacle location
      targets[rand_tar].real_y = rand() % (91) + 5;
      targets[rand_tar].real_x = rand() % (91) + 5;
      data.co_y = targets[rand_tar].real_y;
      data.co_x = targets[rand_tar].real_x;

      // Send the number of an old and coordinates of a new target to the server
      if (write(targets_server[1], &data, sizeof(struct shared_data)) == -1) {
        perror("Targets: writing to pipe targets_server failed");
      }

      // Open the logfile to create space
      fp = fopen("../Log/logfile_targets.txt", "a");
      if (fp == NULL) {  // Check for errors
        perror("Error opening logfile");
        return -1;
      }
      fprintf(fp, "\n");  // Start from the new row
      fclose(fp);         // Close the file

      // Log all current targets
      log_message(fp, "Number of the target to be deleted", INT_TYPE,
                  &rand_tar);
      for (int i = 0; i < NUM_TARGETS; ++i) {
        log_message(fp, "Target coordinate y", DOUBLE_TYPE, &targets[i].real_y);
        log_message(fp, "Target coordinate x", DOUBLE_TYPE, &targets[i].real_x);
      }

      // Reset the counter
      counter_targ = 0;
    } else {
      // Increment the counter
      counter_targ++;
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
  close(targets_server[1]);
  close(server_targets[0]);

  // End the target program
  return 0;
}