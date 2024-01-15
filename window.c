#include <fcntl.h>
#include <math.h>
#include <ncurses.h>
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

// Function to create a new ncurses window
WINDOW *create_newwin(int height, int width, int starty, int startx) {
  // Initialize a new window
  WINDOW *local_win;

  // Create the window
  local_win = newwin(height, width, starty, startx);
  // Draw a box around the window
  box(local_win, 0, 0);  // Give default characters (lines) as the box
  // Refresh window to show the box
  wrefresh(local_win);

  return local_win;
}

// Function to print the character at a desired location
void print_character(WINDOW *win, int y, int x, char *character, int pair) {
  // Print the character at the desired location in blue
  wattron(win, COLOR_PAIR(pair));
  mvwprintw(win, y, x, "%c", *character);
  wattroff(win, COLOR_PAIR(pair));

  // Update box of the window
  box(win, 0, 0);
  // Refresh the window
  wrefresh(win);
}

// Function to log a message to the logfile_window.txt
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
  fp = fopen("../Log/logfile_window.txt", "a");
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

// Function to update the positions of obstacles or targets
void update_object(WINDOW *win, struct shared_data *objects, int num, int type,
                   double co_y, double co_x, double *old_y, double *old_x,
                   int color_pair, double scale_y, double scale_x) {
  int main_y, main_x;
  int new_main_y, new_main_x;

  // Coordinates from real to main window
  main_y = (int)(objects[num].real_y / scale_y);
  main_x = (int)(objects[num].real_x / scale_x);
  // Delete the object at the current position
  print_character(win, main_y, main_x, " ", color_pair);

  // Print the new object and save it in the array
  objects[num].real_y = co_y;
  objects[num].real_x = co_x;
  // Coordinates from real to main window
  new_main_y = (int)(co_y / scale_y);
  new_main_x = (int)(co_x / scale_x);

  // Print the obstacle character
  if (type == 1) {
    print_character(win, new_main_y, new_main_x, "O", color_pair);
  }
  // Print the target character
  else {
    print_character(win, new_main_y, new_main_x, "Y", color_pair);
  }

  // Update old coordinates
  *old_y = co_y;
  *old_x = co_x;
}

// Function to print out the initial objects positions
void initial_objects(WINDOW *win, FILE *fp, struct shared_data objects[],
                     int num_objects, double scale_y, double scale_x,
                     int color_pair) {
  // Print the objects
  for (int i = 0; i < num_objects; i++) {
    // Update the log
    log_message(fp, "Object number ", INT_TYPE, &i);
    log_message(fp, "Received object coordinate y (real) ", DOUBLE_TYPE,
                &objects[i].real_y);
    log_message(fp, "Received object coordinate x (real) ", DOUBLE_TYPE,
                &objects[i].real_x);

    // Convert to window size
    int main_y = (int)(objects[i].real_y / scale_y);
    int main_x = (int)(objects[i].real_x / scale_x);

    // Print the obstacles on the window
    if (color_pair == 2) {
      print_character(win, main_y, main_x, "O", color_pair);
    }
    // Print the targets on the window
    else {
      print_character(win, main_y, main_x, "Y", color_pair);
    }

    // Update the log
    log_message(fp, "Printed object coordinate y (main) ", INT_TYPE, &main_y);
    log_message(fp, "Printed object coordinate x (main) ", INT_TYPE, &main_x);
  }
}

// Function to check if the drone has reached a target
void reached_target(FILE *fp, WINDOW *win, double real_y, double real_x,
                    struct shared_data targets[], double scale_y,
                    double scale_x, int *score) {
  double euclidean_distance = 0.0;
  double main_y = 0.0, main_x = 0.0;
  double rand_tar = 0.0;

  // Check if the drone is close to the target
  for (int i = 0; i < NUM_TARGETS; i++) {
    // Calculate the euclidean_distance between drone and target
    euclidean_distance = sqrt(pow(real_y - targets[i].real_y, 2) +
                              pow(real_x - targets[i].real_x, 2));

    // Check if euclidean distance is smaller than the threshold
    if (euclidean_distance <= FORCE_FIELD) {
      // Delete the target
      main_y = (int)(targets[i].real_y / scale_y);
      main_x = (int)(targets[i].real_x / scale_x);
      print_character(win, main_y, main_x, " ", 3);

      // Update the log
      log_message(fp, "Deleted a target at coordinate y ", DOUBLE_TYPE,
                  &targets[i].real_y);
      log_message(fp, "Deleted a target at coordinate x ", DOUBLE_TYPE,
                  &targets[i].real_x);

      // Calculate a new target coordinate and save it to the array targets
      targets[i].real_y = rand() % (91) + 5;
      targets[i].real_x = rand() % (91) + 5;

      // Calculate the main coordinates and print the new target
      main_y = (int)(targets[i].real_y / scale_y);
      main_x = (int)(targets[i].real_x / scale_x);
      print_character(win, main_y, main_x, "Y", 3);

      (*score)++;

      // Update the log
      for (int i = 0; i < NUM_TARGETS; ++i) {
        log_message(fp, "Target coordinate y", DOUBLE_TYPE, &targets[i].real_y);
        log_message(fp, "Target coordinate x", DOUBLE_TYPE, &targets[i].real_x);
      }
      log_message(fp, "SCORE ", INT_TYPE, score);
    }
  }
}

// Main function starts
int main(int argc, char *argv[]) {
  /* VARIABLES */
  // Values for creating the winow
  WINDOW *main_win, *inspection_win;
  int height1, width1;
  int startx1, starty1, titlex1;
  int height2, width2, startx2, starty2, titlex2;
  // Initialize the struct
  struct shared_data data = {.real_y = 0.0,
                             .real_x = 0.0,
                             .ch = 0,
                             .num = 0,
                             .type = 0,
                             .co_y = 0.0,
                             .co_x = 0.0};
  // Logfile
  FILE *fp;
  // For coordinates of the drone, obstacles and targets
  int main_x = 0, main_y = 0;
  int ch, num, type;
  double co_y = 0.0, co_x = 0.0;
  double old_obs_y = 0.0, old_obs_x = 0.0;
  double old_tar_y = 0.0, old_tar_x = 0.0;
  // Initialize the counter for signal sending
  int counter = 0;
  // SCORE counter
  int score = 0;

  // Open the logfile to either create or delete the content
  fp = fopen("../Log/logfile_window.txt", "w");
  if (fp == NULL) {  // Check for error
    perror("Error opening logfile");
    return -1;
  }
  // Close the file
  fclose(fp);

  /* PIPES */
  // For use of select
  fd_set rdfds;
  int retval;
  // Set the timeout to 0 milliseconds
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  // Creating file descriptors
  int window_server[2];
  int server_window[2];

  // Scanning and close unnecessary fds
  sscanf(argv[1], "%d %d %d %d", &window_server[0], &window_server[1],
         &server_window[0], &server_window[1]);
  close(window_server[0]);
  close(window_server[1]);
  close(server_window[1]);

  // Max fd
  int max_fd = server_window[0];

  /* Start ncurses mode */
  initscr();
  cbreak();
  noecho();

  // Enable color and create color pairs
  start_color();
  init_pair(1, COLOR_BLUE, COLOR_BLACK);  // Color pair for blue -> drone
  init_pair(2, COLOR_MAGENTA,
            COLOR_BLACK);                  // Color pair for purple -> obstacles
  init_pair(3, COLOR_GREEN, COLOR_BLACK);  // Color pair for green -> targets

  // Print out a welcome message on the screen
  printw("Welcome to the Drone simulator\n");
  refresh();

  /* CREATING WINDOWS */
  // Create the main game window
  height1 = LINES * 0.8;
  width1 = COLS;
  starty1 = (LINES - height1) / 2;
  startx1 = 0;
  main_win = create_newwin(height1, width1, starty1, startx1);
  // Print the title of the window
  titlex1 = (width1 - strlen("Drone game")) / 2;
  mvprintw(starty1 - 1, startx1 + titlex1, "Drone game");
  wrefresh(main_win);  // Refresh window

  // Update the log
  log_message(fp, "Height of the main window ", INT_TYPE, &height1);
  log_message(fp, "Lenght of the main window ", INT_TYPE, &width1);

  // Print "SCORE" under the main window
  mvprintw(starty1 + height1, startx1, "SCORE:");
  refresh();

  /* PRINTING THE DRONE */
  // Real life to ncurses window scaling factors
  double scale_y = (double)DRONE_AREA / height1;
  double scale_x = (double)DRONE_AREA / width1;
  // Setting the real coordinates to middle of the geofence
  double real_y = DRONE_AREA / 2;
  double real_x = DRONE_AREA / 2;
  // Convert real coordinates for ncurses window
  main_y = (int)(real_y / scale_y);
  main_x = (int)(real_x / scale_x);
  // Add the character in the middle of the window
  print_character(main_win, main_y, main_x, "X", 1);

  // Update character coordinates to the logfile
  log_message(fp, "Printed character coordinate y (main) ", INT_TYPE, &main_y);
  log_message(fp, "Printed character coordinate x (main) ", INT_TYPE, &main_x);

  /* WATCHDOG */
  pid_t Watchdog_pid;           // For watchdog PID
  pid_t window_pid = getpid();  // Recieve the process PID

  // Randomness
  srand(window_pid);

  // Get the file locations
  char *fnames[NUM_PROCESSES] = PID_FILE_SP;

  // Open the window process tmp file to write its PID
  FILE *pid_fp = fopen(fnames[WINDOW_SYM], "w");
  if (pid_fp == NULL) {  // Check for errors
    perror("Error opening Window tmp file");
    return -1;
  }
  fprintf(pid_fp, "%d", window_pid);
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
  if (watchdog_fp == NULL) {
    perror("Error opening watchdog PID file");
    return -1;
  }

  // Read the watchdog PID from the file
  if (fscanf(watchdog_fp, "%d", &Watchdog_pid) != 1) {
    printf("Error reading Watchdog PID from file.\n");
    fclose(watchdog_fp);
    return -1;
  }

  // Close the file
  fclose(watchdog_fp);

  /* Recieve initial obstacles and targets */
  // Read data from the pipe
  struct shared_data obstacles[NUM_OBSTACLES];
  struct shared_data targets[NUM_TARGETS];
  // Recieve initial obstacles
  if (read(server_window[0], obstacles, sizeof(obstacles)) == -1) {
    perror("Read from the server failed");
    exit(EXIT_FAILURE);
  }
  // Print the obstacles on the window
  initial_objects(main_win, fp, obstacles, NUM_OBSTACLES, scale_y, scale_x, 2);

  // Recieve initial targets
  if (read(server_window[0], targets, sizeof(targets)) == -1) {
    perror("Window: read from the server failed 2");
    exit(EXIT_FAILURE);
  }
  // Print the targets on the window
  initial_objects(main_win, fp, targets, NUM_TARGETS, scale_y, scale_x, 3);

  // Open the logfile to create a space
  fp = fopen("../Log/logfile_window.txt", "a");
  if (fp == NULL) {  // Check for errors
    perror("Error opening logfile");
    return -1;
  }
  fprintf(fp, "\n");  // Start from the new row
  fclose(fp);         // Close the file

  // Start the loop
  while (1) {
    // Recieve key pressed and new coordinates from the server (pipes)
    FD_ZERO(&rdfds);         // Reset the reading set
    FD_SET(max_fd, &rdfds);  // Add the fd to the set
    retval = select(max_fd + 1, &rdfds, NULL, NULL, &tv);

    // Check for errors
    if (retval == -1) {
      perror("Window: select error");
      break;
    }
    // Data is available
    else if (retval > 0 && FD_ISSET(max_fd, &rdfds)) {
      // Read from server and check for errors
      if (read(server_window[0], &data, sizeof(struct shared_data)) == -1) {
        perror("Window: error reading from server_window");
      }
      // Save the recieved data
      ch = data.ch;      // Get the key pressed by the user
      num = data.num;    // Get the number of the obstacle to delete
      co_y = data.co_y;  // Get the new obstacle y coordinate
      co_x = data.co_x;  // Get the new obstacle x coordinate
      type = data.type;  // Get the type of the object

      // If the data is about the drone
      if (type == 3) {
        real_y = data.real_y;  // Get the new y coordinate (real life)
        real_x = data.real_x;  // Get the new x coordinate (real life)

        // Update the log
        log_message(fp, "Recieved from drone coordinate y (real) ", DOUBLE_TYPE,
                    &real_y);
        log_message(fp, "Recieved from drone coordinate x (real) ", DOUBLE_TYPE,
                    &real_x);

        // Check if the restart key has been pressed
        if (ch == RESTART) {
          // If the score is not zero
          if (score > 0) {
            // Print the current score next to "SCORE"
            mvprintw(starty1 + height1, startx1 + strlen("SCORE:") + 1, "%d",
                     score);
            refresh();

            // Reset the score to zero
            score = 0;
          }
        }
      }
      /* Update the positions of the obstacles */
      else if (type == 4) {
        // Check if it is new obstacle coordinates
        if (co_y != old_obs_y && co_x != old_obs_x) {
          // Update the obstacle coordinates
          update_object(main_win, obstacles, num, type, co_y, co_x, &old_obs_y,
                        &old_obs_x, 2, scale_y, scale_x);

          // Update the log
          log_message(fp, "Delete an object in position ", INT_TYPE, &num);
          log_message(fp, "Deleted an obstacle at coordinate y (real) ",
                      DOUBLE_TYPE, &obstacles[num].real_y);
          log_message(fp, "Deleted an obstacle at coordinate x (real) ",
                      DOUBLE_TYPE, &obstacles[num].real_x);
          log_message(fp, "Created an obstacle at coordinate y (real) ",
                      DOUBLE_TYPE, &co_y);
          log_message(fp, "Created an obstacle at coordinate x (real) ",
                      DOUBLE_TYPE, &co_x);
        }
      }
      /* Update the positions of the targets */
      else if (type == 5) {
        // Check if it is new target coordinates
        if (co_y != old_tar_y && co_x != old_tar_x) {
          // Update the target coordinates
          update_object(main_win, targets, num, type, co_y, co_x, &old_tar_y,
                        &old_tar_x, 3, scale_y, scale_x);
          // Update the log
          log_message(fp, "Delete a target in position ", INT_TYPE, &num);
          log_message(fp, "Deleted a target at coordinate y (real) ",
                      DOUBLE_TYPE, &targets[num].real_y);
          log_message(fp, "Deleted a target at coordinate x (real) ",
                      DOUBLE_TYPE, &targets[num].real_x);
          log_message(fp, "Created a target at coordinate y (real) ",
                      DOUBLE_TYPE, &co_y);
          log_message(fp, "Created a target at coordinate x (real) ",
                      DOUBLE_TYPE, &co_x);
        }
      }
    }

    // Check if the esc button has been pressed
    if (ch == ESCAPE) {
      // Log the exit of the program
      log_message(fp, "Recieved ESCAPE key ", INT_TYPE, &ch);
      break;  // Leave the loop
    }

    // Delete drone at the current position
    print_character(main_win, main_y, main_x, " ", 1);

    // Convert the real coordinates for the ncurses window using scales
    main_y = (int)(real_y / scale_y);
    main_x = (int)(real_x / scale_x);

    // Add drone to the desired position
    print_character(main_win, main_y, main_x, "X", 1);

    // Update character coordinates to the logfile
    log_message(fp, "Printed character coordinate y (main) ", INT_TYPE,
                &main_y);
    log_message(fp, "Printed character coordinate x (main) ", INT_TYPE,
                &main_x);

    // Check if the drone has reached a target
    reached_target(fp, main_win, real_y, real_x, targets, scale_y, scale_x,
                   &score);

    // Send a signal o watchdog after certain iterations have passed
    if (counter == PROCESS_SIGNAL_INTERVAL) {
      // send signal to watchdog every process signal interval
      if (kill(Watchdog_pid, SIGUSR1) < 0) {
        perror("kill");
      }
      // Set counter to zero (start over)
      counter = 0;
    } else {
      // Increment the counter
      counter++;
    }

    // Open the logfile to create a space
    fp = fopen("../Log/logfile_window.txt", "a");
    if (fp == NULL) {  // Check for errors
      perror("Error opening logfile");
      return -1;
    }
    fprintf(fp, "\n");  // Start from the new row
    fclose(fp);         // Close the file

    // Wait for a certain time
    usleep(WAIT_TIME);
  }

  // End ncurses mode
  endwin();

  // Clean up
  close(server_window[0]);

  // End the window program
  return 0;
}
