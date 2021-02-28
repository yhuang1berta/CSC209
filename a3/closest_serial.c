#include "closest_serial.h"
#include <stdlib.h>
#include <string.h>

double combine_lr(struct Point P[], size_t n, struct Point mid_point, double d)
{
    struct Point *point_arr = malloc(sizeof(struct Point) * n);
    int i, j;
    int arr_count = 0;
    struct Point p_i;
    // step i).
    for (i = 0; i < n; i++) {
        p_i = P[i];
        if (abs(p_i.x - mid_point.x) < d) {
            point_arr[arr_count].x = p_i.x;
            point_arr[arr_count].y = p_i.y;
            arr_count++;
        }
    }
    // step ii).
    qsort(point_arr, arr_count, sizeof(struct Point), compare_y);
    // step iii).
    double curr_d;
    for (i = 0; i < arr_count; i++) {
        for (j = i + 1; j < arr_count; j++) {
            if (abs((point_arr[i]).y - (point_arr[j]).y) < d) {
                curr_d = dist(point_arr[i], point_arr[j]);
                if (curr_d < d) {
                    d = curr_d;
                }
            }
        }
    }
    free(point_arr);
    return d;
}

double _closest_serial(struct Point P[], size_t n)
{
    // step 1). find mid-point
    struct Point p_mid = P[n/2];
    double dl, dr, d;
    //step 2 && 3). find closest distance in left and right
    // closes distance on left half
    if (n/2 <= 3) {
        dl = brute_force(P, n/2);
    } else {
        dl = _closest_serial(P, n/2);
    }
    // closes distance on right half
    if ((n - n/2) <= 3) {
        dr = brute_force(&(P[n/2]), n - n/2);
    } else {
        dr = _closest_serial(&(P[n/2]), n - n/2);
    }
    // step 4). find minimum of the 2 halves
    if (dl > dr) {
        d = dr;
    } else {
        d = dl;
    }
    // step 5)
    return combine_lr(P, n, p_mid, d);
}

double closest_serial(struct Point P[], size_t n)
{
    qsort(P, n, sizeof(struct Point), compare_x);
    return _closest_serial(P, n);
}
