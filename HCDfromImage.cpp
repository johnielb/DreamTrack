/*Program to emulate image processing part of
 * E101 library on PC
 * Either Linux or mingw
 * Use at your own risk.
 * Feel free to modify */

#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cmath>

#define CAMERA_WIDTH 320 //Control Resolution from Camera
#define CAMERA_HEIGHT 240 //Control Resolution from Camera
unsigned char pixels_buf[CAMERA_WIDTH*CAMERA_HEIGHT*4];
double DEG2RAD = M_PI/180.0;

// returns color component (color==0 -red,color==1-green,color==2-blue
// color == 3 - luminocity
// for pixel located at (row,column)
unsigned char get_pixel( int row,int col, int color) {
    // calculate address in 1D array of pixels
    int address = CAMERA_WIDTH*row*3 + col*3;
    if ((row < 0 ) || (row > CAMERA_HEIGHT) )
    {
        printf("row is out of range\n");
        return -1;
    }
    if ( (col< 0) || (col > CAMERA_WIDTH))
    {
        printf("column is out of range\n");
        return -1;
    }

    if (color==0)
    {
        return (pixels_buf[address]);
    }
    if (color==1)
    {
        return (pixels_buf[address + 1]);
    }
    if (color==2)
    {
        return (pixels_buf[address + 2]);
    }
    if (color==3)
    {
        unsigned char y = (pixels_buf[address] + pixels_buf[address+1] +pixels_buf[address+2])/3;
        return y;
    }
    printf("Color encoding wrong: 0-red, 1-green,2-blue,3 - luminosity\n");
    return -2; //error
}

int set_pixel(int row, int col, char red, char green,char blue) {
    int address = CAMERA_WIDTH*row*3 + col*3;
    if ((address < 0) || (address>CAMERA_WIDTH*CAMERA_HEIGHT*4))
    {
        printf("SetPixel(): wrong x,y coordinates\n");
        return -1;
    }
    pixels_buf[address] = red;
    pixels_buf[address+1] = green;
    pixels_buf[address+2] = blue;
    return 0;
}

int ReadPPM(const char *filename) {
    //char buff[16];
    FILE *fp=fopen(filename, "rb");
    if (!fp) {
        printf("Unable to open file '%s'\n", filename);
        return -1;
    }
    // read the header
    char ch;
    if ( fscanf(fp,"P%c\n",&ch) != 1 || ch != '6')
    {
        printf("file is wrong format\n");
        return -2;
    }
    // skip comments
    ch = getc(fp);
    while(ch == '#')
    {
        do {
            ch = getc(fp);
        } while (ch != '\n');
        ch = getc(fp);
    }

    if (!isdigit(ch))  printf("Wrong header\n");
    ungetc(ch,fp);
    //read width,height and max color value
    int width, height, maxval;
    int res = fscanf(fp,"%d%d%d\n",&width,&height,&maxval);
    printf("Open file: width=%d height=%d\n",width,height);


    int size = width*height*3;

    int num =fread((void*) pixels_buf, 1,size,fp);
    if (num!=size) {
        printf("can not allocate image data memory: file=%s num=%d size=%d\n",
               filename,num,size);
        return -3;
    }
    fclose(fp);
    return 0;

}

int SavePPM(char fn[5]) {
    //save image into ppm file
    FILE *fp;
    char fname[9];
    sprintf(fname,"%s",fn);
    fp = fopen(fname,"wb");
    if ( !fp){
        printf("Unable to open the file\n");
        return -1;
    }
    // write file header
    fprintf(fp,"P6\n %d %d %d\n",CAMERA_WIDTH , CAMERA_HEIGHT,255);
    int ind = 0;
    int row = 0;
    int col = 0;
    char red;
    char green;
    char blue;
    for ( row = 0 ; row < CAMERA_HEIGHT; row++){
        for ( col = 0 ; col < CAMERA_WIDTH; col++){
            red =  pixels_buf[ind];
            green =  pixels_buf[ind+1];
            blue =  pixels_buf[ind+2];
            fprintf(fp,"%c%c%c",red,green,blue);
            ind = ind + 3;
        }
    }
    fflush(fp);
    fclose(fp);
    return 0;
}

int main()
{
    // enter image file name
    char file_name[7];
    printf(" Enter input image file name(with extension:\n");
    scanf("%s",file_name);
    printf(" You enter:%s\n",file_name);
    // read image file
    if (ReadPPM(file_name) != 0){
        printf(" Can not open file\n");
        return -1;
    };
    char edges[CAMERA_HEIGHT][CAMERA_WIDTH];
    // do your processing here
    for (int row = 0; row<CAMERA_HEIGHT; row++) {
        for (int col = 0; col<CAMERA_WIDTH; col++) {
            if (row>0 && col>0 && row<CAMERA_HEIGHT && col<CAMERA_WIDTH) {
                double rx = -get_pixel(row-1,col-1,0) + get_pixel(row-1,col+1,0) -
                            2.0*(get_pixel(row, col-1,0)) + 2.0*(get_pixel(row, col+1,0)) -
                            get_pixel(row+1,col-1,0) + get_pixel(row+1,col+1,0);
                double ry = -get_pixel(row-1,col-1,0) - 2.0*get_pixel(row-1,col,0) - (get_pixel(row-1, col+1,0)) +
                            (get_pixel(row+1, col-1,0)) + 2.0*(get_pixel(row+1,col,0)) + get_pixel(row+1,col+1,0);
                /*double gx = -get_pixel(row-1,col-1,1) + get_pixel(row-1,col+1,1) -
                            2.0*(get_pixel(row, col-1,1)) + 2.0*(get_pixel(row, col+1,1)) -
                            get_pixel(row+1,col-1,1) + get_pixel(row+1,col+1,1);
                double gy = -get_pixel(row-1,col-1,1) - 2.0*get_pixel(row-1,col,1) - (get_pixel(row-1, col+1,1)) +
                            (get_pixel(row+1, col-1,1)) + 2.0*(get_pixel(row+1,col,1)) + get_pixel(row+1,col+1,1);*/
                /*double bx = -get_pixel(row-1,col-1,2) + get_pixel(row-1,col+1,2) -
                            2.0*(get_pixel(row, col-1,2)) + 2.0*(get_pixel(row, col+1,2)) -
                            get_pixel(row+1,col-1,2) + get_pixel(row+1,col+1,2);
                double by = -get_pixel(row-1,col-1,2) - 2.0*get_pixel(row-1,col,2) - (get_pixel(row-1, col+1,2)) +
                            (get_pixel(row+1, col-1,2)) + 2.0*(get_pixel(row+1,col,1)) + get_pixel(row+1,col+1,2);*/
                edges[row][col] = 0;
                if (fabs(rx)+fabs(ry) > 70.0) edges[row][col] = 1;
            } else {
                edges[row][col] = 0;
            }
        }
    }
    printf("convolution done\n");
    int minR = 100;
    int maxR = 101;
    printf("Start accumulating\n");
    int accum[320][240];
    for (int y=0; y<CAMERA_HEIGHT; y++) {
        for (int x=0; x<CAMERA_WIDTH; x++) {
            for (int r=minR; r<maxR; r++) {
                for (int deg=0; deg<360; deg++) {
                    int a = (int) (x - (r * cos(deg*DEG2RAD)));
                    int b = (int) (y - (r * sin(deg*DEG2RAD)));
                    if (a >= CAMERA_WIDTH || a < 0) {
                        continue;
                    }
                    if (b >= CAMERA_HEIGHT || b < 0) {
                        continue;
                    }
                    /*if (y>127) {
                        printf("x: %d y: %d a: %d b: %d\n", x, y, a, b);
                    }*/
                    //printf("x: %d y: %d r: %d deg: %d\n", x, y, r, deg);
                    //int i = (y*CAMERA_HEIGHT)+(r-minR)/10;
                    if (edges[b][a] == 1) {
                        accum[x][y] += 1;
                    }
                }
            }
        }
        printf("y: %d\n",y);
    }
    printf("Finished accumulating\n");
    int maxedX = 0;
    int maxedY = 0;
    int maxedVote = 0;
    for (int y=0; y<CAMERA_HEIGHT; y++) {
        for (int x=0; x<CAMERA_WIDTH; x++) {
            //printf("x: %d y: %d votes: %d\n", x, y, accum[x][y]);
            if (accum[x][y]>maxedVote) {
                maxedVote = accum[x][y];
                maxedX = x;
                maxedY = y;
            }
        }
        printf("y: %d\n",y);
    }
    printf("x: %d y: %d votes: %d\n",maxedX,maxedY,maxedVote);

    // save convolved image to file
    for (int row = 0; row<CAMERA_HEIGHT; row++) {
        for (int col = 0; col<CAMERA_WIDTH; col++) {
            set_pixel(row,col,0,0,0);
            if (edges[row][col]==1) set_pixel(row,col,255,255,255);
        }
    }

    /*for ( int r = 10; r<30;r++){
       set_pixel(r,10,0,255,0); // draws vertical green line
    }*/

    printf(" Enter output image file name(with extension:\n");
    scanf("%s",file_name);
    //printf(" You enter:%s\n",file_name);
    if (SavePPM(file_name) != 0){
        printf(" Can not save file\n");
        return -1;
    };

    return 0;
}
 
