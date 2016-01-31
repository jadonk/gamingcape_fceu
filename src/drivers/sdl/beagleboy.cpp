#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <linux/input.h>
#include "beagleboy.h"

int fd_x = -1;
int fd_y = -1;
int fd_b = -1;

int bb_a = 0;
int bb_b = 0;

int raw_bb_joy_x = -1;
int raw_bb_joy_y = -1;

struct input_event buttons;

// compensated values
int bb_joy_x = -1;
int bb_joy_y = -1;

// calibration values
int min_x, max_x;
int min_y, max_y;
int center_x, center_y;

void bb_init() {
	fd_x = open("/usr/share/gamingcape/AIN0", O_RDONLY);
	fd_y = open("/usr/share/gamingcape/AIN2", O_RDONLY);
	fd_b = open("/usr/share/gamingcape/BUTTONS", O_RDONLY);
	int flags = fcntl(fd_b, F_GETFL, 0);
	fcntl(fd_b, F_SETFL, flags | O_NONBLOCK);

	if (!fd_x) { printf("could not open joy x\n"); }
	if (!fd_y) { printf("could not open joy y\n"); }
	if (!fd_b) { printf("could not open buttons\n"); }
}

void bb_close() {
	close(fd_x);
	close(fd_y);
	close(fd_b);
}


void bb_refresh() {
	char buf_x[100];
	char buf_y[100];

	memset(buf_x, 0, 100);
	memset(buf_y, 0, 100);

	int ret_x = read(fd_x, buf_x, sizeof(buf_x));
	int ret_y = read(fd_y, buf_y, sizeof(buf_y));
	int ret_b = read(fd_b, &buttons, sizeof(buttons));

	if ((ret_x != -1) &&
	    (ret_y != -1)) {

		buf_x[ret_x-1] = '\0';
		buf_y[ret_y-1] = '\0';

		raw_bb_joy_x = atoi(buf_x);
		raw_bb_joy_y = atoi(buf_y);

		// compensate using calibration data
		bb_joy_x = (raw_bb_joy_x - center_x) * 1000;
		bb_joy_y = (raw_bb_joy_y - center_y) * 1000;
		if (bb_joy_x > 0) bb_joy_x /= (max_x - center_x);
		else              bb_joy_x /= (center_x - min_x);
		if (bb_joy_y > 0) bb_joy_y /= (max_y - center_y);
		else              bb_joy_y /= (center_y - min_y);

		lseek(fd_x,0,0);
		lseek(fd_y,0,0);

		// update button status globals
		if ((ret_b > 0) &&
		    (buttons.code == BTN_LEFT) &&
		    (buttons.value == 1)) {
			bb_a = 1;
		} else {
			bb_a = 0;
		}
		if ((ret_b > 0) &&
		    (buttons.code == BTN_RIGHT) &&
		    (buttons.value == 1)) {
			bb_b = 1;
		} else {
			bb_b = 0;
		}
	}

}
void bb_cal() {

	FILE *file;
	file = fopen("/usr/share/gamingcape/cal.txt", "w");
	if (!file) printf("Failed to open cal.txt\n");

	bb_refresh();
	max_x = raw_bb_joy_x; 
	max_y = raw_bb_joy_y; 
	min_x = raw_bb_joy_x; 
	min_y = raw_bb_joy_y; 

	printf("Move all around edge. Press A to start. Press B when finished.\n");
	while (!bb_a) { bb_refresh(); }
	while (!bb_b) {
		bb_refresh();

		if (raw_bb_joy_x > max_x) {max_x = raw_bb_joy_x; printf("X: %d %d Y: %d %d\n", min_x, max_x, min_y, max_y);} 
		if (raw_bb_joy_y > max_y) {max_y = raw_bb_joy_y; printf("X: %d %d Y: %d %d\n", min_x, max_x, min_y, max_y);}
		if (raw_bb_joy_x < min_x) {min_x = raw_bb_joy_x; printf("X: %d %d Y: %d %d\n", min_x, max_x, min_y, max_y);}
		if (raw_bb_joy_y < min_y) {min_y = raw_bb_joy_y; printf("X: %d %d Y: %d %d\n", min_x, max_x, min_y, max_y);}
	}

	printf("Let go of joystick. Press A to start. Press B with finished.\n");
	while (!bb_a) { bb_refresh(); }
	while (!bb_b) {
		bb_refresh();
		center_x = raw_bb_joy_x;
		center_y = raw_bb_joy_y;
		printf("%d %d\n", center_x, center_y);
	}

	printf("Results. Press A to continue..\n");
	printf("Min X: %d\tCenter X: %d\tMax X: %d\n", min_x, center_x, max_x);
	printf("Min Y: %d\tCenter Y: %d\tMax Y: %d\n", min_y, center_y, max_y);
	printf("Range X: %d\tRange Y: %d\n", (max_x-min_y), (max_y-min_y));
	while (!bb_a) { bb_refresh(); }

	fprintf(file, "%d,%d,%d,%d,%d,%d\n", min_x, center_x, max_x, min_y, center_y, max_y);
	fclose(file);
}


void bb_load_cal() {
	FILE *file;
	file = fopen("/usr/share/gamingcape/cal.txt", "r");
	if (!file) printf("Failed to open cal.txt\n");
	fscanf(file, "%i,%i,%i,%i,%i,%i", &min_x, &center_x, &max_x, &min_y, &center_y, &max_y);
	fclose(file);
}

