#include <stdio.h>
#include <iostream>
#include <assert.h>
#include <math.h>

#include "mbed.h"
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#include "arm_math.h"

#include <time.h>

extern "C" {
#include "lowp_micro.h"
#include "profile.h"
#include "data.h"
}

int main() {
    PerfProbe *perfref = CreatePerfProbe("ref");
    EnableProfiling();

    printf("\n\r==========================================\n\r");
    printf("==> Testing computation for a single layer\n\r");
    printf("----> weight_bits     = %d\n\r", weight_bits    );
    printf("----> input_bits      = %d\n\r", input_bits     );
    printf("----> padding         = %d\n\r", padding        );
    printf("----> input_size      = %d\n\r", input_size     );
    printf("----> output_channels = %d\n\r", output_channels);
    printf("----> kernel_size     = %d\n\r", kernel_size    );
    printf("----> input_channels  = %d\n\r", input_channels );

    // prepare the input values
    const int byte_size = 8;
    const int output_size = input_size + 2 * padding - kernel_size + 1;
    printf("----> output_size     = %d\n\r", output_size );

    const int input_sram_size =
        ceil((float)(input_bits * input_channels) / (float)byte_size)
        * input_size * input_size;
    q7_t* input_sram = (q7_t*) malloc(input_sram_size * sizeof(q7_t));

    const int output_sram_size =
        ceil((float)(input_bits * output_channels) / (float)byte_size)
        * output_size * output_size;
    q7_t* output_sram = (q7_t*) malloc(output_sram_size * sizeof(q7_t));
    // remember to clear the output_sram in case the key is randomly matched
    memset(output_sram, 7, output_sram_size * sizeof(q7_t));

    q7_t* copy_ptr = (q7_t*) input_v;
    for (int i = 0; i < input_sram_size; ++i) {
        input_sram[i] = copy_ptr[i];
    }

#ifdef Q3_Q1
    int key = 0;
    // PrintInputFM_q3(input_sram, input_channels, input_size);
    // PrintKernel_q1((q7_t*)kernel_v, input_channels, output_channels, kernel_size);
    ResetPerfData(perfref);
    Conv2d_q3_q1_opt(
            (q7_t*)input_sram, step_i, minv_i,
            (q7_t*)kernel_v, step_k, minv_k,
            (q7_t*) output_sram, step_o, minv_o,
            input_channels, input_size, padding, output_channels, kernel_size,
            step_k_K
            );
    OnePerfData(perfref);
    printf("==> Conv2d 4I x 2W: %lu\n\r", perfref->cyccnt);
    // PrintInputFM_q3(output_sram, output_channels, output_size);
    const int quantized_output_channels = ceil(output_channels / 2.0) * 2;
    int count = 0;
    for (size_t i = 0; i < output_channels; ++i) {
        for (size_t x = 0; x < output_size; ++x) {
            for (size_t y = 0; y < output_size; ++y) {
                const int index = (x * output_size + y) * quantized_output_channels + i;
                const int index_q7 = index / 2;
                const int shift_q7 = 4 - (index % 2) * 4;
                key ^= count + ((output_sram[index_q7] >> shift_q7) & 0b1111);
                count++;
            }
        }
    }
    printf("==> key = %d\n\r", key);
    if (key == key_ref) {
        printf("==> Test Passed.\n\r");
    }
    else {
        printf("==> Test Failed.\n\r");
        return 1;
    }
#endif

    return 0;
}
