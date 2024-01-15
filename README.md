# Assignment description
The project is an interactive simulator for a drone with two degrees of freedom.
The drone is operated by keys of the keyboard: 8 directions, plus keys for stopping, resetting, and quitting.
The drone dynamics is a 2 degrees of freedom dot with mass (inertia) and viscous resistance. Any key pressed increases (decreases if reversed) in steps a force pushing the drone in the appropriate direction.
The sides of the operation window are obstacles as well. They simulate the geo-fences used in drone operations.

# Assignment 2
For the 2nd assignment, we created the full system that inclues Server, Window, Keyboard, Drone, Watchdog, Targets, Obstacles (processes S,W,K,D,T,O). The server is using pipes to communicate with other processes. The Window and Keyboard are implemented using ncurses. This assignment represents the project for Advanced and Robot Programming course on UniGe in the winter semester 2023/2024.
The work has been performed by a team of two: Iris Laanearu(s6350192) and Tomoha Neki(s6344955).


## Installation & Usage
For the project's configuration, we have used `Makefile`.

To build executables, run this :
```
make
```
in the project directory.

Then move to `Build`:
```
cd /Build
```
To start the game, run this:
```
./master
```

To remove the executables and logs, run this:
```
make clean
```

```
make clean logs
```





###  Operational instructions, controls ###
To operate the drone use the following keybindings
- `w`: move left up
- `s`: move left
- `x`: move left down
- `e`: move up
- `c`: move down
- `r`: move right up
- `f`: move right
- `v`: move right down
- `d`: brake
- `k`: restart
- `esc` : exit



## Overview 

![Assignment2_architecture](https://github.com/TNunige/ARP_Assignment_Mandarins/assets/145358917/5e6c4e8d-c62b-4f3d-82c7-36e8c1d05f69)



The components are 8 processes:
- Master
- Blackboard
- Window (User interface)
- Watchdog
- Drone
- Keyboard (User interface)
- Targets
- Obstacles

All of the above mentioned components were created.

### Master
Master process is the father of all processes. It creates child processes by using fork(). It runs keyboard and window inside a wrapper program Konsole.
After creating children, the process waits termination of all processes and prints the termination status.
After spawning the children master process enters an infinite loop and checks if any of the processes that use konsole(window and keyboard) are still active. If one or both of the processes have died then it either kills the other process and then waits for the terminations for all the other processes. After exiting from the infinite loop it prints out the termination statuses of all child processes.	

### Server
Server communicates with the other processes through pipes and logs the information it receives. The process recieves data from processes drone, obstacles and targets. It sends data to processes drone and window. The server checks the data exchanged via pipes and updates the contents to a logfile. Additionally, it periodically sends a signal to the watchdog after a certain number of iterations. Upon exiting the loop, it closes the used file descriptors and terminates.


### Watchdog
The watchdog process's job is to monitor the activities of all the other processes (except the master). Watchdog monitors window, drone, keyboard, server, obstacles and targets processes. At the beginning of the process, it writes its PID to a temporary file for the other processes to read its PID and reads the PIDs of other monitored child processes from temporary files. We have chosen to implement a watchdog that only receives signals from monitored processes. It receives signal SIGUSR1 from other child processes, indicating that child processes are active during the monitored period. In the infinite loop, it monitors the elapsed time since the last data reception from each child process. When the elapsed time exceeds the timeout, the watchdog sends SIGKILL signals to all monitored processes and terminates all child processes.

### Window
The window process creates the game window and prints the drone, the obstacles and the targets positions using ncurses library. The process sends and receives the appropriate data from other processes via pipes. The process dynamically updates the position of the drone based on the calculations made in the drone process. Obstacle positions are updated based on the positions randomly calculated in the obstacles process. Targets are also base on the positions sent by the targets process but also updated whenever the drone has reached a target. Upon reaching the target, the process increments the score and deletes the reached target from the window. Aditionally, it periodically sends a signal to the watchdog process to inform its activity.


### Drone
The drone process models the drone character movement by dynamically calculating the force impacting the drone based on the user key input (direction), command, and repulsive force(border and obstacles). The repulsive force is activated when the drone is near the game window borders or obstacles. The process calculates the forces based on the received user key from the keyboard process. It utilizes the dynamic motion equation to determine the new position of the drone considering the sum of input forces and repulsive forces. Subsequently, the drone process sends the drone's new position and the user key input to the server via pipes. Additionally, it periodically sends a signal to the watchdog process to inform its activity.



### Keyboard 
The keyboard handles the user key inputs and displays messages on the inspection window. It scans the key pressed by the user by the getch() command in the ncurses library and sends the values of the pressed key to the drone process through a FIFO (named pipe). Additionally, it periodically sends a signal to the watchdog process to inform its activity.

### Targets
The target process generates random target positions and sends them to the server process via pipes. The process deletes a random target after a certain time interval. Instead the process generates a new obstacle position. Aditionallty, it periodically sends a signal to the watchdog process to inform its activity.

### Obstacles
The obstacle process generates random obstacle positions and sends them to the server process via pipes. The process deletes a random obstacle after a certain time interval and generates a new obstacle position. Additionally, it periodically sends a signal to the watchdog process to inform its activity.

### Constants.h ###
All the necessary constants and structures are defined here.

### Improvements ###
The first and the third improvement from the first assignment have found a solution.
- Currently the watchdog does not check for the escape key inserted by user but it could be an improvement to exit the game sooner when user has initiated it.


   





