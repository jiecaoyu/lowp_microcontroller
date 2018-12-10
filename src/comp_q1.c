#include "arm_math.h"
#include "stdio.h"
#include <math.h>

void PrintKernel_q1(
        q7_t* kernel,
        const size_t input_channels,
        const size_t output_channels,
        const size_t kernel_size
        ) {
    const int quantized_window_size =
        ceil(input_channels * kernel_size * kernel_size / 4.0) * 4;
    for (size_t o = 0; o < output_channels; ++o) {
        for (size_t x = 0; x < kernel_size; ++x) {
            for (size_t i = 0; i < input_channels; ++i) {
                for (size_t y = 0; y < kernel_size; ++y) {
                    const int index = o * quantized_window_size +
                        (x * kernel_size + y) * input_channels + i;
                    const int index_q7 = index / 4;
                    const int shift_q7 = 6 - (index % 4) * 2;
                    printf("%+1d ", (kernel[index_q7] >> shift_q7) & 0b11);
                }
            printf(" | ");
            }
        printf("\n\r");
        }
    printf("------------\n\r");
    }
    printf("############################\n\r");
    return;
}
