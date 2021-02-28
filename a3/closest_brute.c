#include "closest_brute.h"
#include <math.h>

double brute_force(struct Point P[], size_t n) 
{ 
    int i, j;
    double min = INFINITY;
    for (i = 0; i < n; i++) {
        for (j = i+1; j < n; j++) {
            if (dist(P[i], P[j]) < min) {
                min = dist(P[i], P[j]);
            }
        }
    }
    return min; 
} 
