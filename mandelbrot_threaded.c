#include <stdio.h>
#include <string.h>
#include <unistd.h>    // Unix-like system calls read and write
#include <fcntl.h>     // Unix-like system calls to open and close
#include "myutils.c"
#include "myutils.h"
#include <pthread.h>

#define R 4  // Constant indicating the image divisions RxR
#define N_THREADS R*R

pthread_t thread_ids[R*R];
pthread_mutex_t lock;

enum { width=1024, height=1024 };
//enum { width=8192, height=8192 };

unsigned char pixels[width * height * 3];

struct  thread_data{
  unsigned char pixels[width/4 * height/4 * 3];
  int si;
  int sj;
};

struct thread_data thread_data_array[N_THREADS];

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

void *generate_mandelbrot(void* data) {
  
  struct thread_data *my_data = (struct thread_data *) data;


  unsigned char *p = my_data->pixels;
  int si = (int) my_data->si;
  int sj = (int) my_data->sj;
  
  int i, j;
  for (i = si; i < si * height/R - 1; i++) {
    for (j = sj; j < sj * width/R - 1; j++) {
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
  //printf("0\n");
  int i, si, sj, ti, tj;  
  
  startTimer(0);
  //generate_mandelbrot(pixels, width, height);
  //printf("1\n");
  
  for(i = 0; i < R; i++){
    for(int j = 0; j < R; j++){
      memcpy(thread_data_array[i*R+j].pixels, pixels, sizeof(unsigned char));   
      //printf("2\n"); 
      //thread_data_array[i*R+j].pixels = pixels;
      thread_data_array[i*R+j].si = i;
      thread_data_array[i*R+j].sj = j;
      pthread_create(&thread_ids[i*R+j], NULL, generate_mandelbrot, (void*) &thread_data_array[i*R+j]);
    }
  }

  //printf("3\n");

  for (i = 0; i < N_THREADS; i++){ //Esperamos a los threads
    pthread_join(thread_ids[i], NULL);
  }

  //printf("4\n");


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