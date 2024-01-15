all: master server window drone keyboard watchdog obstacles targets

clean:
	rm -f Build/master Build/server Build/window Build/drone Build/keyboard Build/watchdog Build/obstacles Build/targets

clean-logs:
	rm Log/*

master: master.c
	gcc master.c -o Build/master

server: server.c
	gcc server.c -o Build/server

window: window.c
	gcc window.c -o Build/window -lncurses -lm

drone: drone.c
	gcc drone.c -o Build/drone -lm

keyboard: keyboard.c
	gcc keyboard.c -o Build/keyboard -lncurses

watchdog: watchdog.c
	gcc watchdog.c -o Build/watchdog

obstacles: obstacles.c
	gcc obstacles.c -o Build/obstacles

targets: targets.c
	gcc targets.c -o Build/targets	