// DreamTrack
// by the Tuff Dreamerz

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cmath>
#include "E101.h"
#define CAMERA_WIDTH 320 //Control Resolution from Camera
#define CAMERA_HEIGHT 240 //Control Resolution from Camera
double DEG2RAD = M_PI/180.0;

class Tracker {
private:
    int elevation = 57;
    const int elv_servo = 5;
    int azimuth = 53;
    const int azm_servo = 3;
    int min_tilt = 32;
    int max_tilt = 65;
    int xError, yError;
    bool isSunUp;

    // thresholds to play around with:
    double convThreshold = 65.0;
    int radiusRange = 5;
    int degStep = 10;
    double voteThreshold = 0.7;
    double kp = 0.05;

public:
    int InitHardware();
    void SetMotors();
    int MeasureSun();
    void FollowSun();
};

int Tracker::InitHardware() {
    int err;
    err = init(0);
    open_screen_stream();
    SetMotors();
    take_picture();
    update_screen();
    return 0;
}

void Tracker::SetMotors() {
    set_motors(elv_servo, elevation);
    set_motors(azm_servo, azimuth);
    hardware_exchange();
}

int Tracker::MeasureSun() {
    take_picture();
    update_screen();
    /* CONVOLUTION */
    char edges[CAMERA_HEIGHT][CAMERA_WIDTH]; // array stores edge detected values
    int diamCount = 0;
    double diameter = 0;
    for (int row = 0; row<CAMERA_HEIGHT; row++) {
        diamCount = 0;
        for (int col = 0; col<CAMERA_WIDTH; col++) {
            int red = get_pixel(row, col, 0);
            int grn = get_pixel(row, col, 1);
            // sun diameter detection
            if ((float)grn/(float)red < 0.4) {
                diamCount++;
            } else {
                if (diamCount > diameter) diameter=diamCount;
                diamCount = 0;
            }
            int setting = 2; // convolve blueness vals
            if (row>0 && col>0 && row<CAMERA_HEIGHT-2 && col<CAMERA_WIDTH-2) { // convolve using Sobel kernels
                // vertical edge detect
                double sobelX = -get_pixel(row-1,col-1,setting) + get_pixel(row-1,col+1,setting) -
                                2.0*(get_pixel(row, col-1,setting)) + 2.0*(get_pixel(row, col+1,setting)) -
                                get_pixel(row+1,col-1,setting) + get_pixel(row+1,col+1,setting);
                // horizontal edge detect
                double sobelY = -get_pixel(row-1,col-1,setting) - 2.0*get_pixel(row-1,col,setting) - (get_pixel(row-1, col+1,setting)) +
                                (get_pixel(row+1, col-1,setting)) + 2.0*(get_pixel(row+1,col,setting)) + get_pixel(row+1,col+1,setting);
                edges[row][col] = 0;
                if (fabs(sobelX)+fabs(sobelY) > convThreshold) edges[row][col] = 1;
            } else {
                edges[row][col] = 0;
            }
        }
    }
    int radius = diameter*0.51;
    if (radius > 50) radiusRange = 8;
    else radiusRange = 5;
    printf("radius: %d\n",radius);

    /* ACCUMULATION/VOTING */
    int votes[320][240];
    for (int y=0; y<CAMERA_HEIGHT; y++) { // clear array
        for (int x=0; x<CAMERA_WIDTH; x++) {
            votes[x][y] = 0;
            // fill in gaps where we're confident there's an edge
            if ((edges[y-1][x] == 1 && edges[y+1][x] == 1) || (edges [y][x-1] == 1 && edges [y][x+1] == 1)) {
                edges[y][x] = 1;
            }
        }
    }
    for (int y=0; y<CAMERA_HEIGHT; y++) {
        for (int x=0; x<CAMERA_WIDTH; x++) {
            unsigned char red = get_pixel(y, x, 0);
            unsigned char green = get_pixel(y, x, 1);
            if (edges[y][x] == 1 && (float)green/(float)red < 0.4) { // if edge-detected and red THEN vote
                for (int r=radius-radiusRange; r<radius+radiusRange; r++) {
                    for (int deg=0; deg<360; deg+=degStep) {
                        int cx = (int) (x - (r * cos(deg*DEG2RAD)));
                        int cy = (int) (y + (r * sin(deg*DEG2RAD)));
                        if (cx >= CAMERA_WIDTH || cx < 0 || cy >= CAMERA_HEIGHT || cy < 0) {
                            continue; // don't look outside camera bounds
                        }
                        votes[cx][cy] += 1;
                    }
                }
            }
        }
    }
    update_screen();

    /* TALLY THE VOTES */
    int maxedX = 0;
    int maxedY = 0;
    int maxedVote = 0;
    for (int y=1; y<CAMERA_HEIGHT-1; y++) {
        for (int x = 1; x < CAMERA_WIDTH - 1; x++) {
			bool isLeftCorner = false;
			bool isRightCorner = false;

            int squareX = x-radius+3; // ignore shapes with a top left square corner
            int squareY = y-radius+3;
            if (squareX > 0 && squareY > 0) {
				int red = get_pixel(squareY, squareX, 0);
				int grn = get_pixel(squareY, squareX, 1);
				if ((float)grn/(float)red < 0.4 && edges[squareY][squareX] == 1) isLeftCorner = true;
			}
            
            squareX = x+radius-3; // move to bottom right corner
            squareY = y+radius-3;
            if (squareX < 240 && squareY < 240) {
				int red = get_pixel(squareY, squareX, 0);
				int grn = get_pixel(squareY, squareX, 1);
				if ((float)grn/(float)red < 0.4 && edges[squareY][squareX] == 1) isRightCorner = true;
			}
            if (isLeftCorner && isRightCorner) continue;

            if (votes[x][y] > maxedVote) {
                maxedVote = votes[x][y];
                maxedX = x;
                maxedY = y;
            }
        }
    }
    printf("x: %d y: %d votes: %d\n", maxedX, maxedY, maxedVote);

    // count how many red pixels in middle
    diamCount = 0;
    diameter = 0;
    for (int r=0; r<CAMERA_HEIGHT; r++) {
        int red = get_pixel(r, maxedX, 0);
        int grn = get_pixel(r, maxedX, 1);
        if ((float) grn / (float) red < 0.4) {
            diamCount++;
        } else {
            if (diamCount > diameter) diameter=diamCount;
            diamCount = 0;
        }
    }

    // set convolutional result only after getting pixel vals
    for (int y=0; y<CAMERA_HEIGHT; y++) {
        for (int x=0; x<CAMERA_WIDTH; x++) {
            if (edges[y][x] == 1) set_pixel(y,x,255,255,255);
            else set_pixel(y,x,0,0,0);
        }
    }
    // mark voted centre
    for (int i = -2; i<2; i++) {
        for (int j = -2; j<2; j++) {
            set_pixel(maxedY+i, maxedX+j, 255,0,0);
        }
    }
    xError = 0;
    yError = 0;
    
    update_screen();
    if (edges[maxedY][maxedX] == 1) {
		printf("Half circle\n");
		return 0;
	} else if (maxedY>CAMERA_HEIGHT-radius/2 || maxedY<radius/2) {
		printf("Out of bounds\n");
		return 0;
	} else if (maxedVote<10) {
		printf("No more votes\n");
		return 0;
	} else if (abs(diameter/2-radius) > 5) {
		printf("No middle red line\n");
		return 0;
	} 

    update_screen();

    // gets signal for how far to adjust servos
    xError = kp*(maxedX-CAMERA_WIDTH/2.0);
    yError = kp*(maxedY-CAMERA_HEIGHT/2.0);
    printf("xError: %d yError: %d\n", xError, yError);
    return 1;
}

void Tracker::FollowSun() {
    int isSunUp = MeasureSun();
    if (isSunUp) {
        elevation += yError;
        if (elevation > max_tilt) elevation = max_tilt;
        if (elevation < min_tilt) elevation = min_tilt;
        azimuth += xError;
        if (azimuth > max_tilt) azimuth = max_tilt;
        if (azimuth < min_tilt) azimuth = min_tilt;
    } else {
        printf("No sun detected, resetting\n");
        elevation = 51;
        azimuth = 55;
    }
    double degrees = ((double)(elevation-min_tilt)/(max_tilt-min_tilt))*180.0-90.0;
    printf("E: %d A: %d Deg: %1.2f\n", elevation, azimuth,degrees);
    SetMotors();
}

int main() {
    Tracker dt;
    dt.InitHardware();
    while (true) {
        dt.FollowSun();
    }
    return 0;
}
