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
#include "data.h"
}

int main() {
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
    const int input_sram_size =
        ceil((float)(input_bits * input_channels) / (float)byte_size)
        * input_size * input_size;
    q7_t* input_sram = (q7_t*) malloc(input_sram_size * sizeof(q7_t));

    q7_t* copy_ptr = (q7_t*) input_v;
    for (int i = 0; i < input_sram_size; ++i) {
        input_sram[i] = copy_ptr[i];
    }

#ifdef Q15_Q15
#endif
    printf("==> Test Finished.\n\r");
    return 0;
}
