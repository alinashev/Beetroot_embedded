#ifndef BEETROOT_EMBEDDED_POINTERS_H
#define BEETROOT_EMBEDDED_POINTERS_H

void b1_task_run(void);
void b2_task_run(void);
void b3_task_run(void);
void b4_task_run(void);
void b5_task_run(void);
void b6_task_run(void);
void b7_task_run(void);
void b8_task_run(void);

void fill(int *buf, int n, int value);
void get_min_max(const int *arr, int n, int *min, int *max);

#endif