#include <stdio.h>
#include <string.h>
#include <unistd.h>    // Unix-like system calls read and write
#include <fcntl.h>     // Unix-like system calls to open and close
#include "myutils.c"
#include "myutils.h"

#define R 4  // Constant indicating the image divisions RxR

enum { width=1024, height=1024 };
//enum { width=8192, height=8192 };

int tga_write_header(int fd, int width, int height) {
  static unsigned char tga[18];
  int nbytes;
  tga[2] = 2;
  tga[12] = 255 & width;
  tga[13] = 255 & (width >> 8);
  tga[14] = 255 & height;
  tga[15] = 255 & (height >> 8);
  tga[16] = 24;
  tga[17] = 32;
  nbytes = write(fd, tga, sizeof(tga));
  return nbytes == sizeof(tga);
}


void write_tga(char* fname, unsigned char *pixels, int width, int height)
{
  int fd = open(fname,  O_CREAT | O_RDWR, 0640);
  tga_write_header(fd, width, height);
  printf("Created file %s: Writing pixels size %d bytes\n", fname, 3*width*height);
  write(fd, pixels,3*width*height);
  close(fd);
}



void tga_read_header(int fd, int* width, int* height) {
  static unsigned char tga[18];
  read(fd, tga, 12);
  read(fd, width, 2);
  read(fd, height, 2);
  read(fd, &tga[16], 2);
}


int compute_iter(int i, int j, int width, int height) {
  int itermax = 255/2;  
  int iter;
  double x,xx,y,cx,cy;
  cx = (((float)i)/((float)width)-0.5)/1.3*3.0-0.7;
  cy = (((float)j)/((float)height)-0.5)/1.3*3.0;
  x = 0.0; y = 0.0;
  for (iter=1;iter<itermax && x*x+y*y<itermax;iter++)  {
    xx = x*x-y*y+cx;
    y = 2.0*x*y+cy;
    x = xx;
  }
  return iter;
}

void generate_mandelbrot(unsigned char *p, int width, int height) {
  int i, j;
  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
      *p++ = 255 * ((float)j / height);
      *p++ = 255 * ((float)i / width);
      *p++ = 2*compute_iter(i,j,width,height);
    }
  }
}


void interchange(int si, int sj, int ti, int tj, unsigned char *p, int width, int height) {
  int k;
  int n = width / R;
  unsigned char* square = malloc(n*n*3);
  memset(square, 0, n*n*3);

  for (k=0;k<n;k++) {
    int t_index = ti*n*3*width + tj*3*n + k*3*width;
    memcpy(&square[k*3*n], &p[t_index], n*3);
  }
  for (k=0;k<n;k++) {
    int s_index = si*n*3*width + sj*3*n + k*3*width;
    int t_index = ti*n*3*width + tj*3*n + k*3*width;
    memcpy(&p[t_index], &p[s_index], n*3);
  }
  for (k=0;k<n;k++) {
    int s_index = si*n*3*width + sj*3*n + k*3*width;
    memcpy(&p[s_index], &square[k*3*n], n*3);
  }

  free(square);
}


int main(void) {
  int i, si, sj, ti, tj;
  unsigned char pixels[width * height * 3];

  startTimer(0);
  generate_mandelbrot(pixels, width, height);
  printf("Time spent generate_mandelbrot: %ldms\n", endTimer(0));
  write_tga("image.tga", pixels, width, height);

  startTimer(0);
  for(i=0;i<1000;i++) {
    si = rand()%R;
    sj = rand()%R;
    ti = rand()%R;
    tj = rand()%R;
    interchange(si,sj,ti,tj, pixels,width,height);
  }
  printf("Time spent during interchange: %ldms\n", endTimer(0));
  write_tga("image_scrambled.tga", pixels, width, height);

  return 0;
}