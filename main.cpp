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
    int elevation = 47;
    const int elv_servo = 5;
    int azimuth = 47;
    const int azm_servo = 1;
    int min_tilt = 32;
    int max_tilt = 65;
    double kp = 0.1;
    int x_error, y_error;

public:
    int InitHardware();
    void SetMotors();
    int MeasureSun();
    int FollowSun();
};

int Tracker::InitHardware() {
    int err;
    err = init(0);
    open_screen_stream();
    take_picture();
    update_screen();
    close_screen_stream();
    return 0;
}

void Tracker::SetMotors() {
    set_motors(elv_servo, elevation);
    set_motors(azm_servo, azimuth);
}

int Tracker::MeasureSun() {
    /* CONVOLUTION */
    char edges[CAMERA_HEIGHT][CAMERA_WIDTH]; // array stores edge detected values
    int rCount = 0;
    int diameter = 0;
    for (int row = 0; row<CAMERA_HEIGHT; row++) {
        for (int col = 0; col<CAMERA_WIDTH; col++) {
            int red = get_pixel(row, col, 0);
            int grn = get_pixel(row, col, 1);
            // sun diameter detection
            if ((float)grn/(float)red < 0.4) {
                rCount++;
            } else {
                if (rCount>diameter) diameter=rCount;
                rCount = 0;
            }
            int setting = 0; // convolve redness vals
            if (row>0 && col>0 && row<CAMERA_HEIGHT-2 && col<CAMERA_WIDTH-2) { // convolve using Sobel kernels
                // vertical edge detect
                double sobelX = -get_pixel(row-1,col-1,setting) + get_pixel(row-1,col+1,setting) -
                                2.0*(get_pixel(row, col-1,setting)) + 2.0*(get_pixel(row, col+1,setting)) -
                                get_pixel(row+1,col-1,setting) + get_pixel(row+1,col+1,setting);
                // horizontal edge detect
                double sobelY = -get_pixel(row-1,col-1,setting) - 2.0*get_pixel(row-1,col,setting) - (get_pixel(row-1, col+1,setting)) +
                                (get_pixel(row+1, col-1,setting)) + 2.0*(get_pixel(row+1,col,setting)) + get_pixel(row+1,col+1,setting);
                edges[row][col] = 0;
                if (fabs(sobelX)+fabs(sobelY) > 70.0) edges[row][col] = 1;
            } else {
                edges[row][col] = 0;
            }
        }
    }
    int radius = diameter/2 + 1;
    //printf("radius: %d\n",radius);

    /* ACCUMULATION/VOTING */
    int votes[320][240];
    for (int y=0; y<CAMERA_HEIGHT; y++) {
        for (int x=0; x<CAMERA_WIDTH; x++) {
            char red = get_pixel(y, x, 0);
            char green = get_pixel(y, x, 1);
            if (edges[y][x] == 1 && (float)green/(float)red < 0.4) { // if edge-detected and red THEN vote
                for (int r=radius-3; r<radius+3; r++) {
                    for (int deg=0; deg<360; deg+=10) {
                        int cx = (int) (x - (r * cos(deg*DEG2RAD)));
                        int cy = (int) (y + (r * sin(deg*DEG2RAD)));
                        if (cx >= CAMERA_WIDTH || cx < 0) { // don't look outside camera bounds
                            continue;
                        }
                        if (cy >= CAMERA_HEIGHT || cy < 0) {
                            continue;
                        }
                        votes[cx][cy] += 1;
                    }
                }
            }
        }
    }

    /* TALLY THE VOTES */
    int maxedX = 0;
    int maxedY = 0;
    int maxedVote = 0;
    for (int y=1; y<CAMERA_HEIGHT-1; y++) {
        for (int x = 1; x < CAMERA_WIDTH - 1; x++) {
            if (votes[x][y] > maxedVote) {
                maxedVote = votes[x][y];
                maxedX = x;
                maxedY = y;
            }
        }
    }

    // gets signal for how far to adjust servos
    int xError = kp*(maxedX-CAMERA_WIDTH/2.0);
    int yError = kp*(maxedY-CAMERA_HEIGHT/2.0);
}

int Tracker::FollowSun() {
    MeasureSun();
}

int main() {
    Tracker dt;
    dt.InitHardware();
    while (true) {
        dt.FollowSun();
    }
    return 0;
}