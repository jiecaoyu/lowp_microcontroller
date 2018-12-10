#include "arm_math.h"
#include "stdio.h"
#include <math.h>

void PrintInputFM_q3(
        q7_t* input,
        const size_t input_channels,
        const size_t input_size
        ) {
    const int quantized_input_channels = ceil(input_channels / 2.0) * 2;
    for (size_t x = 0; x < input_size; ++x) {
        for (size_t i = 0; i < input_channels; ++i) {
            for (size_t y = 0; y < input_size; ++y) {
                const int index = (x * input_size + y) * quantized_input_channels + i;
                const int index_q7 = index / 2;
                const int shift_q7 = 4 - (index % 2) * 4;
                printf("%+4d ", (input[index_q7] >> shift_q7) & 0b1111);
            }
            printf(" | ");
        }
        printf("\n\r");
    }
    printf("############################\n\r");
    return;
}
