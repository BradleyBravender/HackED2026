#include <stdbool.h>
#include <stdio.h>

#define NUM_ANCHORS 8
#define BUFF_SIZE 5 // The number of ranges in the buffer at a time
#define STABILITY_THRESHOLD 5 // The total variance range measurements can have

struct RangeBuffer {
    int buffer[BUFF_SIZE];
};


int test_data[][NUM_ANCHORS] = {
    {0,0,0,309,0,0,0,0},
    {0,0,0,308,0,0,0,0},
    {0,0,0,307,0,0,0,0},
    {0,0,0,305,0,0,0,0},
    {0,0,0,302,0,0,0,0},
    {0,0,0,318,0,0,0,0},
    {0,0,0,314,0,0,0,0},
    {0,0,0,318,0,0,0,0},
    {0,0,0,325,0,0,0,0},
    {0,0,0,341,0,0,0,0},
    {0,0,0,353,0,0,0,0},
    {0,0,0,355,0,0,0,0},
    {0,0,0,356,0,0,0,0},
    {0,0,0,357,0,0,0,0},
    {0,0,0,362,0,0,0,0},
    {0,0,0,366,0,0,0,0},
    {0,0,0,369,0,0,0,0},
    {0,0,0,371,0,0,0,0},
    {0,0,0,373,0,0,0,0},
    {0,0,0,374,0,0,0,0},
    {0,0,0,375,0,0,0,0},
    {0,0,0,376,0,0,0,0},
    {0,0,0,375,0,0,0,0},
};

int num_samples = sizeof(test_data)/sizeof(test_data[0]);

int sample_index = 0;

void get_measurement(int ranges[NUM_ANCHORS]) {
    if (sample_index < num_samples) {
        for (int i = 0; i < NUM_ANCHORS; i++) {
            ranges[i] = test_data[sample_index][i];
        }
        sample_index++;
    } else {
        // Repeat last sample to simulate stabilized measurements
        for (int i = 0; i < NUM_ANCHORS; i++) {
            ranges[i] = test_data[num_samples - 1][i];
        }
    }
}

void get_ranges(int ranges[NUM_ANCHORS]) {
    struct RangeBuffer buff_array[NUM_ANCHORS];
    int index = 0;
    bool filled = false;

    while (true) {
        // Returns a list of distances
        get_measurement(ranges);

        for (int i = 0; i < NUM_ANCHORS; i++) 
            buff_array[i].buffer[index] = ranges[i];

        index = (index + 1) % BUFF_SIZE;

        // At this point, we know we've gone through the whole buffer
        if (index == 0)
            filled = true;

        if (!filled)
            continue;

        bool can_return = true;

        // Reset max, min, and sum after every buffer update
        for (int i = 0; i < NUM_ANCHORS; i++) {
            int max = buff_array[i].buffer[0];
            int min = buff_array[i].buffer[0];
            int sum = buff_array[i].buffer[0];

            for (int j = 1; j < BUFF_SIZE; j++) {
                int v = buff_array[i].buffer[j];
                
                if (v > max) max = v;
                if (v < min) min = v;
                sum += v;
            }

            if (max - min > STABILITY_THRESHOLD) {
                can_return = false;
                break;
            }

            // Calculate the mean distance to the ith anchor
            ranges[i] = sum / BUFF_SIZE;

        }

        // can_return is true when all distances have converged
        if (can_return) return;

    }
}

int main() {
    int ranges[NUM_ANCHORS];
    get_ranges(ranges);
    for (int i = 0; i < NUM_ANCHORS; i++) {
        printf("Anchor %i distance: %d\n", i, ranges[i]);
    }
    return 0;
}