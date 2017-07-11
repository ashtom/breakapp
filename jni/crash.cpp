#include "crash.h"
#include <string.h>
#include <vector>
#include <algorithm>

void swap(int*& left, int*& right) {
    int*tmp = left;
    left    = right;
    right   = tmp;
}

void bubble(int** first, int** last) {
    if (first < last) {
        bubble(&first[1], last);
        while (first < last) {
            if (*first[0] > *first[1]) {
                swap(first[0], first[1]);
            }
            ++first;
        }
    }
}

void CrashWithStack() {
    const int count = 10;
    const int last = count - 1;
    int values[count] = { 12, 34, 5, 7, 218, 923, -1, 0, -1, 5};
    int* refs[count];
    memset(&refs, 0, sizeof(refs));
    // let's introduce an error here and skip the base index (0)
    for (int i = 1; i < count; ++i) {
        refs[i] = &values[i];
    }
    // now let's do a bubble sort
    bubble(&refs[0], &refs[last]);
}

bool compare(int* left, int* right) {
    return *left < *right;
}

void CrashWithHeap() {
    const int count = 10;
    const int last = count - 1;
    int values[count] = { 12, 34, 5, 7, 218, 923, -1, 0, -1, 5};
    std::vector<int*> refs(count);
    // let's introduce an error here and skip the base index (0)
    for (int i = 1; i < count; ++i) {
        refs[i] = &values[i];
    }
    // now let's do a sort with a custom comparator
    std::sort(refs.begin(), refs.end(), compare);
}
