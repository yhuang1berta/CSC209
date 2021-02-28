#include "closest_helpers.h"
#include <math.h>
#include <stdlib.h>

int verbose = 0;

int compare_x(const void* a, const void* b) 
{
    int a_x = ((struct Point*)a)->x;
    int b_x = ((struct Point*)b)->x;
    return a_x - b_x;
} 

int compare_y(const void* a, const void* b) 
{ 
    int a_y = ((struct Point*)a)->y;
    int b_y = ((struct Point*)b)->y;
    return a_y - b_y;
} 

double dist(struct Point p1, struct Point p2) 
{
    int x_diff = abs(p1.x - p2.x);
    int y_diff = abs(p1.y - p2.y);
    return sqrt(pow(x_diff, 2) + pow(y_diff, 2));
} 
