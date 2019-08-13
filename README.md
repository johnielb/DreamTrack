# DreamTrack
This program combines three algorithms: a convolution filter, a circle Hough transform, and a PID algorithm; to facilitate the tracking and following of a red circular Sun in the view of a camera.

## Testing your camera
We have provided a testImage program which allows you to provide 1 frame to our algorithm, ensuring the program can process images provided by your program. Variables in the testImage program that you may adjust to optimise the operation of the tracker are as follows. Be sure to change these fields in the main program once you've determined the values.

### Convolution threshold
```
line 16: double convThreshold = 65.0;
```
This variable determines the threshold at which a convolved pixel is regarded as part of an edge.

### Radius range
```
line 17: int radiusRange = 5;
```
This variable determines how spread out a positive pixel will vote a potential circle centre. A bigger number means it'll vote in more coordinates from the actual circle.

### Degree step
```
line 18: int degStep = 9;
```
This variable determines what angle in degrees to increment as voting takes place around a positive point. A degStep of 9 means that 360/9 = 40 potential votes will result from a positive pixel.

### Vote threshold
```
line 19: int voteThr = 10;
```
This variable stores the amount of votes needed to determine that the vote-maximised coordinates is actually a circle.

## Deploying the tracker
Once you've adjusted the parameters in the main program, you transfer "E101.h" and "main.cpp" to a directory on the live (Linux) system. Ensure to check the x_servo variables that they match the port that the motors are actually plugged into. Compile it using the following command (with the terminal in the correct directory):
```
g++ -Wall -le101 -o main main.cpp
```
Then run it using the command:
```
sudo ./main
```
If the live screen overlaps the terminal window, move the terminal window away so the messages are visible.
