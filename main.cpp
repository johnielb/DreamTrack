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
    int azimuth = 47;
    const int azm_servo = 3;
    int min_tilt = 32;
    int max_tilt = 65;
    int xError, yError;
    bool isSunUp;

    // thresholds to play around with:
    double convThreshold = 65.0;
    int radiusRange = 5;
    int degStep = 9;
    double voteThreshold = 0.7;
    double kp = 0.08;

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
    int rCount = 0;
    double diameter = 0;
    for (int row = 0; row<CAMERA_HEIGHT; row++) {
        rCount = 0;
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
                if (fabs(sobelX)+fabs(sobelY) > convThreshold) edges[row][col] = 1;
            } else {
                edges[row][col] = 0;
            }
        }
    }
    int radius = diameter/2.0;
    if (radius > 50) radiusRange = 8;
    else radiusRange = 5;
    printf("radius: %d\n",radius);

    /* ACCUMULATION/VOTING */
    int votes[320][240];
    for (int y=0; y<CAMERA_HEIGHT; y++) { // clear array
        for (int x=0; x<CAMERA_WIDTH; x++) {
            votes[x][y] = 0;
            // fill in gaps where we're confident there's an edge
            if (edges[y-1][x] == 1 && edges[y+1][x] == 1 || edges [y][x-1] == 1 && edges [y][x+1] == 1) {
                edges[y][x] = 1;
            }
        }
    }
    for (int y=0; y<CAMERA_HEIGHT; y++) {
        for (int x=0; x<CAMERA_WIDTH; x++) {
            unsigned char red = get_pixel(y, x, 0);
            unsigned char green = get_pixel(y, x, 1);
            if (edges[y][x] == 1) set_pixel(y,x,255,255,255);
            else set_pixel(y,x,0,0,0);
            if (edges[y][x] == 1 && (float)green/(float)red < 0.4) { // if edge-detected and red THEN vote
                for (int r=radius-radiusRange; r<radius+radiusRange; r++) {
                    for (int deg=0; deg<360; deg+=degStep) {
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
    update_screen();

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
    printf("x: %d y: %d votes: %d\n", maxedX, maxedY, maxedVote);
    // don't recalculate
    if (edges[maxedY][maxedX] == 1 || maxedY>CAMERA_HEIGHT-radius/2 || maxedY<radius/2 || maxedVote<20) return 0;

    // mark red square
    for (int i = -1; i<1; i++) {
        for (int j = -1; j<1; j++) {
            set_pixel(maxedY+i, maxedX+j, 255,0,0);
        }
    }
    update_screen();

    double scaledVotes = (double)maxedVote/(double)radius;
    voteThreshold = 42.0/(double)radius;
    // gets signal for how far to adjust servos
    xError = kp*(maxedX-CAMERA_WIDTH/2.0);
    yError = kp*(maxedY-CAMERA_HEIGHT/2.0);
    printf("xError: %d yError: %d Scaled votes: %1.2f\n", xError, yError, scaledVotes);
    if (scaledVotes > voteThreshold && scaledVotes < 2.5) return 1;
    else return 0;
}

void Tracker::FollowSun() {
    int isSunUp = MeasureSun();
    if (isSunUp == 1) {
        elevation += yError;
        if (elevation > max_tilt) elevation = max_tilt;
        if (elevation < min_tilt) elevation = min_tilt;
        azimuth += xError;
        if (azimuth > max_tilt) azimuth = max_tilt;
        if (azimuth < min_tilt) azimuth = min_tilt;
    } else {
        printf("No sun detected, resetting\n");
        //elevation = 47;
        azimuth = 47;
    }
    printf("E: %d A: %d\n", elevation, azimuth);
    //elevation = 60;
    //double degrees = ((double)(elevation-min_tilt)/(max_tilt-min_tilt))*180.0-90.0;
    //sleep1(50);
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
