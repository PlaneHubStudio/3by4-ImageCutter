#include <assert.h>
#include <stdio.h>
#include "layout.h"
int main(void) {
 int cases[][3]={{1500,1000,2},{2250,1000,3},{1600,1000,2},{2100,1000,3},{1000,1500,2}};
 for(unsigned i=0;i<sizeof(cases)/sizeof(cases[0]);i++){CropLayout l=crop_layout(cases[i][0],cases[i][1]);assert(l.count==cases[i][2]);}
 for(int w=6;w<2000;w+=17)for(int h=4;h<1600;h+=19){
  CropLayout l=crop_layout(w,h);assert(l.count==2||l.count==3);assert(l.x>=0&&l.y>=0);assert(l.x+l.width<=w&&l.y+l.height<=h);assert(3*l.unit*4==l.height*3);
  for(int n=2;n<=3;n++){int u=w/(3*n);if(h/4<u)u=h/4;assert((int64_t)l.width*l.height>=(int64_t)3*n*u*4*u);}
 }
 assert(!crop_layout(5,3).count);puts("PASS: ratio selection, exact 3:4, crop bounds and maximum retained area");return 0;
}
