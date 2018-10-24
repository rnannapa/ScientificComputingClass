#include<stdio.h>
#include<stdlib.h>
#include<math.h>

void run(int *size, float *xy){
  float pi = 3.1415926;
  float dTheta, theta;
  int i;
  float *ptr = xy;
  
  dTheta = 2*pi/ *size;
  theta = 0.0;
  for (i=0;i<=*size;i++){
      *ptr = cos(theta);
      ptr++;
      *ptr = sin(theta);
      ptr++;
      theta+=dTheta;
  }
  
  }

int main(){
  float *xy;
  int size = 100;
  int i;
  xy = malloc((size+1)*2*4);/*Float is 4 bytes in TCL*/
  if (!xy){
    return (1);
  }
  /* Any error in TCL causes the code to terminate*/
  /* in TCL passed by name, C is passed by value*/
  run(&size,xy);
  for (i=0; i<=size; i++){
    printf("xy[%d] = (%f,%f)\n",i,xy[2*i],xy[2*i+1]);
  }
  
  free(xy);
  return (0);
  /*Status "0" is everything is fine. In TCL we use exit*/
}
