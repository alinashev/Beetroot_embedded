#include <stdio.h>
#include "pointers.h"

void b1_task_run(void) {
    int x = 42;
    int *p = &x;

    printf("x  = %d\n", x);
    printf("&x = %p\n", (void *)&x);
    printf("p  = %p\n", (void *)p);
    printf("*p = %d\n", *p);

    *p = 100;

    printf("x після *p = 100: %d\n", x);
}

void b2_task_run(void) {
    /*
    Before swap:
    a = 5
    b = 9
    After swap:
    a = 9
    b = 5
     */
    int a = 5;
    int b = 9;

    int *pa = &a;
    int *pb = &b;

    printf("Before swap:\n");
    printf("a = %d\n", *pa);
    printf("b = %d\n", *pb);

    int tmp = *pa;
    *pa = *pb;
    *pb = tmp;

    printf("After swap:\n");
    printf("a = %d\n", *pa);
    printf("b = %d\n", *pb);
}

void b3_task_run(void) {
    int arr[5] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        printf("arr[%d] = %d; &arr[0] = %p\n", i, arr[i], &arr[i]);
    }
}

void b4_task_run(void) {
    int arr[5] = {10, 20, 30, 40, 50};
    printf("arr       = %p\n", (void *)arr);
    printf("&arr[0]   = %p\n", (void *)&arr[0]);

    printf("\n");

    printf("*(arr + 2) = %d\n", *(arr + 2));
    printf("arr[2]     = %d\n", arr[2]);

    printf("\n");

    printf("(arr + 4) = %p\n", (void *)(arr + 4));
    printf("(arr + 1) = %p\n", (void *)(arr + 1));
    printf("(arr + 4) - (arr + 1) = %ld\n", (arr + 4) - (arr + 1));

    printf("\nThis difference is in elements, not bytes.\n");
}

void b5_task_run(void) {
    int arr[5] = {10, 20, 30, 40, 50};

    int sum_by_index = 0;
    int sum_by_pointer = 0;

    for (int i = 0; i < 5; i++) {
        sum_by_index += arr[i];
    }

    for (int *q = arr; q < arr + 5; q++) {
        sum_by_pointer += *q;
    }

    printf("Sum by index   = %d\n", sum_by_index);
    printf("Sum by pointer = %d\n", sum_by_pointer);

    if (sum_by_index == sum_by_pointer) {
        printf("Results are equal.\n");
    } else {
        printf("Results are different.\n");
    }
}

void b6_task_run(void) {
    char s[] = "ESP32-S3";
    int length = 0;
    for (char *p = s; *p != '\0'; p++) {
        printf("%c\n", *p);
        length++;
    }


    printf("s      = %s\n", s);
    printf("&s[0]  = %s\n", &s[0]);

    printf("address of s     = %p\n", (void *)s);
    printf("address of &s[0] = %p\n", (void *)&s[0]);

    printf("length = %d\n", length);
}

void fill(int *buf, int n, int value) {
    for (int i = 0; i < n; i++) {
        *(buf + i) = value;
    }
}

void b7_task_run(void) {
    int arr[5] = {10, 20, 30, 40, 50};
    printf("Before fill:\n");
    for (int i = 0; i < 5; i++) {
        printf("arr[%d] = %d\n", i, arr[i]);
    }
    fill(arr, 5, 10);
    printf("After fill:\n");
    for (int i = 0; i < 5; i++) {
        printf("arr[%d] = %d\n", i, arr[i]);
    }
}

void get_min_max(const int *arr, int n, int *min, int *max) {
    *min = *arr;
    *max = *arr;

    for (int i = 1; i < n; i++) {
        if (*(arr + i) < *min) {
            *min = *(arr + i);
        }

        if (*(arr + i) > *max) {
            *max = *(arr + i);
        }
    }
}

void b8_task_run(void) {
    int arr[5] = {10, 20, 30, 40, 50};

    int min_value = 0;
    int max_value = 0;

    get_min_max(arr, 5, &min_value, &max_value);

    printf("Array:\n");

    for (int i = 0; i < 5; i++) {
        printf("arr[%d] = %d\n", i, arr[i]);
    }

    printf("\nmin = %d\n", min_value);
    printf("max = %d\n", max_value);
}