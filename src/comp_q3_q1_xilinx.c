#include "arm_math.h"
#include "stdio.h"
#include "util.h"
#include <math.h>
#include <stdlib.h>

void Conv2d_q3_q1_xilinx(
        const q7_t * pSrcA, const float step_a, const float minv_a,
        const q7_t * pSrcB, const float * step_b, const float * minv_b,
        q7_t * pSrcC, const float step_c, const float minv_c,
        const size_t input_channels,
        const size_t input_size,
        const size_t padding,
        const size_t output_channels,
        const size_t kernel_size,
        const float * step_k_K
        ) {
    const size_t cache_size =
        input_channels * kernel_size * kernel_size;
    const int quantized_input_channels =
        ceil(((float)(input_channels)) / 2.0) * 2;
    const int quantized_output_channels =
        ceil(((float)(output_channels)) / 2.0) * 2;
    const uint32_t quantized_output_channels_4 =
        ceil(((float)(output_channels)) / 4.0) * 4;
    uint32_t* cache_0 = (uint32_t*) malloc(cache_size * sizeof(uint32_t));
    uint32_t* cache_1 = (uint32_t*) malloc(cache_size * sizeof(uint32_t));

    uint32_t klist2_sram[256];

    for (int i = 0; i < 256 / 4; i += 1) {
        klist2_sram[4 * i + 0] = klist2[4 * i + 0];
        klist2_sram[4 * i + 1] = klist2[4 * i + 1];
        klist2_sram[4 * i + 2] = klist2[4 * i + 2];
        klist2_sram[4 * i + 3] = klist2[4 * i + 3];
    }

    const size_t output_size = input_size + 2 * padding - kernel_size + 1;
    const uint32_t padding_constant = 8. + ((q7_t)(-(minv_a + step_a * 8.)/step_a));

    // step_i * A
    int* input_sum = (int*) malloc (input_size * input_size * sizeof(int));
    float* output_sum = (float*) malloc (output_size * output_size * sizeof(float));
    memset(output_sum, 0, output_size * output_size * sizeof(int));
    for (size_t x = 0; x < input_size; ++x) {
        for (size_t y = 0; y < input_size; ++y) {
            int sum_tmp = 0;
            int pSrcA_index = (x * input_size + y) * quantized_input_channels;
            for (size_t c = 0; c < input_channels; ++c) {
                int pSrcA_index_q7 = pSrcA_index / 2;
                int pSrcA_shift_q7 = 4 - (pSrcA_index % 2) * 4;
                pSrcA_index++;
                sum_tmp +=
                    (pSrcA[pSrcA_index_q7] >> pSrcA_shift_q7) & 0b1111;
            }
            input_sum[x * input_size + y] = sum_tmp;
        }
    }
    for (size_t x = 0; x < output_size; ++x) {
        for (size_t y = 0; y < output_size; ++y) {
            int sum_tmp = 0;
            for (size_t s = 0; s < kernel_size; ++s) {
                const int x_a = x + s - padding;
                if ((x_a < 0) || (x_a >= (int)input_size)) {
                    sum_tmp += padding_constant * kernel_size * input_channels;
                }
                else {
                    for (size_t t = 0; t < kernel_size; ++t) {
                        const int y_a = y + t - padding;
                        if ((y_a < 0) || (y_a >= (int)input_size)) {
                            sum_tmp += padding_constant * input_channels;
                        }
                        else {
                            sum_tmp += input_sum[x_a * input_size + y_a];
                        }
                    }
                }
            }
            output_sum[x * output_size + y] = sum_tmp * step_a;
        }
    }
    free(input_sum);

    size_t x = 0;
    for (; x < output_size - 1; x += 2) {
        for (size_t y = 0; y < output_size; ++y) {
            // write cache
            uint32_t* cache_ptr_0 = cache_0;
            if (y == 0 || kernel_size == 1) {
                for (size_t s = 0; s < kernel_size; ++s) {
                    const int x_a = x + s - padding;
                    if ((x_a < 0) || (x_a >= (int)input_size)) {
                        for (size_t t = 0; t < kernel_size * input_channels; ++t) {
                            *(cache_ptr_0++) = padding_constant;
                        }
                    }
                    else {
                        for (size_t t = 0; t < kernel_size; ++t) {
                            const int y_a = y + t - padding;
                            if ((y_a < 0) || (y_a >= (int)input_size)) {
                                for (size_t c = 0; c < input_channels; ++c) {
                                    *(cache_ptr_0++) = padding_constant;
                                }
                            }
                            else {
                                int pSrcA_index =
                                    (x_a * input_size + y_a) * quantized_input_channels;
                                for (size_t c = 0; c < input_channels; ++c) {
                                    int pSrcA_index_q7 = pSrcA_index / 2;
                                    int pSrcA_shift_q7 = 4 - (pSrcA_index % 2) * 4;
                                    pSrcA_index++;
                                    *(cache_ptr_0++) =
                                        (pSrcA[pSrcA_index_q7] >> pSrcA_shift_q7) & 0b1111;
                                }
                            }
                        }
                    }
                }
            }
            else {
                // copy from the previous cache
                for (size_t i = 0; i < kernel_size; ++i) {
                    int cpy_index = i * kernel_size * input_channels;
                    size_t j = 0;
                    for (; j < (kernel_size - 1) * input_channels; ++j) {
                        cache_ptr_0[cpy_index] =
                            cache_ptr_0[cpy_index + input_channels];
                        cpy_index++;
                    }
                }
                for (size_t s = 0; s < kernel_size; ++s) {
                    const int x_a = x + s - padding;
                    cache_ptr_0 += (kernel_size - 1) * input_channels;
                    if ((x_a < 0) || (x_a >= (int)input_size)) {
                        for (size_t t = 0; t < input_channels; ++t) {
                            *(cache_ptr_0++) = padding_constant;
                        }
                    }
                    else {
                        const int y_a = y + kernel_size - 1 - padding;
                        if ((y_a < 0) || (y_a >= (int)input_size)) {
                            for (size_t c = 0; c < input_channels; ++c) {
                                *(cache_ptr_0++) = padding_constant;
                            }
                        }
                        else {
                            int pSrcA_index =
                                (x_a * input_size + y_a)
                                * quantized_input_channels;
                            size_t c = 0;
                            for (; c < input_channels; ++c) {
                                int pSrcA_index_q7 = pSrcA_index / 2;
                                int pSrcA_shift_q7 = 4 - (pSrcA_index & 0b1) * 4;
                                pSrcA_index++;
                                *(cache_ptr_0++) =
                                    (pSrcA[pSrcA_index_q7] >> pSrcA_shift_q7)
                                    & 0b1111;
                            }
                        }
                    }
                }
            }
            uint32_t* cache_ptr_1 = cache_1;
            if (y == 0 || kernel_size == 1) {
                for (size_t s = 0; s < kernel_size; ++s) {
                    const int x_a = x + s - padding + 1;
                    if ((x_a < 0) || (x_a >= (int)input_size)) {
                        for (size_t t = 0; t < kernel_size * input_channels; ++t) {
                            *(cache_ptr_1++) = padding_constant;
                        }
                    }
                    else {
                        for (size_t t = 0; t < kernel_size; ++t) {
                            const int y_a = y + t - padding;
                            if ((y_a < 0) || (y_a >= (int)input_size)) {
                                for (size_t c = 0; c < input_channels; ++c) {
                                    *(cache_ptr_1++) = padding_constant;
                                }
                            }
                            else {
                                int pSrcA_index =
                                    (x_a * input_size + y_a) * quantized_input_channels;
                                for (size_t c = 0; c < input_channels; ++c) {
                                    int pSrcA_index_q7 = pSrcA_index / 2;
                                    int pSrcA_shift_q7 = 4 - (pSrcA_index % 2) * 4;
                                    pSrcA_index++;
                                    *(cache_ptr_1++) =
                                        (pSrcA[pSrcA_index_q7] >> pSrcA_shift_q7) & 0b1111;
                                }
                            }
                        }
                    }
                }
            }
            else {
                // copy from the previous cache
                for (size_t i = 0; i < kernel_size; ++i) {
                    int cpy_index = i * kernel_size * input_channels;
                    size_t j = 0;
                    for (; j < (kernel_size - 1) * input_channels; ++j) {
                        cache_ptr_1[cpy_index] =
                            cache_ptr_1[cpy_index + input_channels];
                        cpy_index++;
                    }
                }
                for (size_t s = 0; s < kernel_size; ++s) {
                    const int x_a = x + s - padding + 1;
                    cache_ptr_1 += (kernel_size - 1) * input_channels;
                    if ((x_a < 0) || (x_a >= (int)input_size)) {
                        for (size_t t = 0; t < input_channels; ++t) {
                            *(cache_ptr_1++) = padding_constant;
                        }
                    }
                    else {
                        const int y_a = y + kernel_size - 1 - padding;
                        if ((y_a < 0) || (y_a >= (int)input_size)) {
                            for (size_t c = 0; c < input_channels; ++c) {
                                *(cache_ptr_1++) = padding_constant;
                            }
                        }
                        else {
                            int pSrcA_index =
                                (x_a * input_size + y_a)
                                * quantized_input_channels;
                            size_t c = 0;
                            for (; c < input_channels; ++c) {
                                int pSrcA_index_q7 = pSrcA_index / 2;
                                int pSrcA_shift_q7 = 4 - (pSrcA_index & 0b1) * 4;
                                pSrcA_index++;
                                *(cache_ptr_1++) =
                                    (pSrcA[pSrcA_index_q7] >> pSrcA_shift_q7)
                                    & 0b1111;
                            }
                        }
                    }
                }
            }

            int pSrcC_index_0 = (x * output_size + y) * quantized_output_channels;
            int pSrcC_index_1 = ((x + 1) * output_size + y) * quantized_output_channels;
            for (size_t a = 0; a < quantized_output_channels_4; a += 4) {
                uint32_t tmp_output_0[4] = {0,0,0,0};
                uint32_t tmp_output_1[4] = {0,0,0,0};
                char* kernel_ptr_char = (char*) (pSrcB + (a >> 2) * cache_size);
                uint32_t tmp_acc_0 = 0;
                uint32_t tmp_acc_1 = 0;
                size_t k = 0;
                char kernel_index = 0;
                uint32_t kernel = 0;
                for (; k < cache_size - 4; k += 5) {
                    tmp_acc_0 = 0;
                    tmp_acc_1 = 0;

                    kernel_index = *(kernel_ptr_char++);
                    kernel = klist2_sram[(int)kernel_index];
                    tmp_acc_0 += kernel * cache_0[k + 0];
                    tmp_acc_1 += kernel * cache_1[k + 0];

                    kernel_index = *(kernel_ptr_char++);
                    kernel = klist2_sram[(int)kernel_index];
                    tmp_acc_0 += kernel * cache_0[k + 1];
                    tmp_acc_1 += kernel * cache_1[k + 1];

                    kernel_index = *(kernel_ptr_char++);
                    kernel = klist2_sram[(int)kernel_index];
                    tmp_acc_0 += kernel * cache_0[k + 2];
                    tmp_acc_1 += kernel * cache_1[k + 2];

                    kernel_index = *(kernel_ptr_char++);
                    kernel = klist2_sram[(int)kernel_index];
                    tmp_acc_0 += kernel * cache_0[k + 3];
                    tmp_acc_1 += kernel * cache_1[k + 3];

                    kernel_index = *(kernel_ptr_char++);
                    kernel = klist2_sram[(int)kernel_index];
                    tmp_acc_0 += kernel * cache_0[k + 4];
                    tmp_acc_1 += kernel * cache_1[k + 4];

                    tmp_output_0[0] += (tmp_acc_0 >> 24);
                    tmp_output_0[1] += (tmp_acc_0 >> 16) & 0b11111111;
                    tmp_output_0[2] += (tmp_acc_0 >>  8) & 0b11111111;
                    tmp_output_0[3] += (tmp_acc_0      ) & 0b11111111;
                    tmp_output_1[0] += (tmp_acc_1 >> 24);
                    tmp_output_1[1] += (tmp_acc_1 >> 16) & 0b11111111;
                    tmp_output_1[2] += (tmp_acc_1 >>  8) & 0b11111111;
                    tmp_output_1[3] += (tmp_acc_1      ) & 0b11111111;
                }
                tmp_acc_0 = 0;
                tmp_acc_1 = 0;
                for (; k < cache_size; k += 1) {
                    kernel_index = *(kernel_ptr_char++);
                    kernel = klist2_sram[(int)kernel_index];
                    tmp_acc_0 += kernel * cache_0[k];
                    tmp_acc_1 += kernel * cache_1[k];
                }
                tmp_output_0[0] += (tmp_acc_0 >> 24);
                tmp_output_0[1] += (tmp_acc_0 >> 16) & 0b11111111;
                tmp_output_0[2] += (tmp_acc_0 >>  8) & 0b11111111;
                tmp_output_0[3] += (tmp_acc_0      ) & 0b11111111;
                tmp_output_1[0] += (tmp_acc_1 >> 24);
                tmp_output_1[1] += (tmp_acc_1 >> 16) & 0b11111111;
                tmp_output_1[2] += (tmp_acc_1 >>  8) & 0b11111111;
                tmp_output_1[3] += (tmp_acc_1      ) & 0b11111111;
                for (size_t i = 0;
                        i < ((output_channels - a) > 4 ? 4 :
                            (output_channels - a)); ++i) {
                    float result_0 = ((float)tmp_output_0[i]) * (step_a * step_b[a + i]);
                    float result_1 = ((float)tmp_output_1[i]) * (step_a * step_b[a + i]);
                    result_0 += step_k_K[a + i] * minv_a;
                    result_1 += step_k_K[a + i] * minv_a;
                    result_0 += minv_b[a + i] * output_sum[x * output_size + y];
                    result_1 += minv_b[a + i] * output_sum[(x + 1) * output_size + y];
                    result_0 +=
                        minv_b[a + i] * minv_a * input_channels *
                        kernel_size * kernel_size;
                    result_1 +=
                        minv_b[a + i] * minv_a * input_channels *
                        kernel_size * kernel_size;
                    result_0 = (result_0 - minv_c) / step_c;
                    result_1 = (result_1 - minv_c) / step_c;
                    char result_q7_0 = (result_0 > 0b1111) ? 0b1111 : result_0;
                    char result_q7_1 = (result_1 > 0b1111) ? 0b1111 : result_1;
                    int pSrcC_index_q7_0 = pSrcC_index_0 / 2;
                    int pSrcC_index_q7_1 = pSrcC_index_1 / 2;
                    int pSrcC_shift_q7_0 = 4 - (pSrcC_index_0 % 2) * 4;
                    int pSrcC_shift_q7_1 = 4 - (pSrcC_index_1 % 2) * 4;
                    pSrcC[pSrcC_index_q7_0] &= (0b11110000 << pSrcC_shift_q7_0);
                    pSrcC[pSrcC_index_q7_1] &= (0b11110000 << pSrcC_shift_q7_1);
                    pSrcC[pSrcC_index_q7_0] |= (result_q7_0 << pSrcC_shift_q7_0);
                    pSrcC[pSrcC_index_q7_1] |= (result_q7_1 << pSrcC_shift_q7_1);
                    pSrcC_index_0++;
                    pSrcC_index_1++;
                }
            }
        }
    }
    for (; x < output_size; ++x) {
        for (size_t y = 0; y < output_size; ++y) {
            // write cache
            uint32_t* cache_ptr_0 = cache_0;
            if (y == 0 || kernel_size == 1) {
                for (size_t s = 0; s < kernel_size; ++s) {
                    const int x_a = x + s - padding;
                    if ((x_a < 0) || (x_a >= (int)input_size)) {
                        for (size_t t = 0; t < kernel_size * input_channels; ++t) {
                            *(cache_ptr_0++) = padding_constant;
                        }
                    }
                    else {
                        for (size_t t = 0; t < kernel_size; ++t) {
                            const int y_a = y + t - padding;
                            if ((y_a < 0) || (y_a >= (int)input_size)) {
                                for (size_t c = 0; c < input_channels; ++c) {
                                    *(cache_ptr_0++) = padding_constant;
                                }
                            }
                            else {
                                int pSrcA_index =
                                    (x_a * input_size + y_a) * quantized_input_channels;
                                for (size_t c = 0; c < input_channels; ++c) {
                                    int pSrcA_index_q7 = pSrcA_index / 2;
                                    int pSrcA_shift_q7 = 4 - (pSrcA_index % 2) * 4;
                                    pSrcA_index++;
                                    *(cache_ptr_0++) =
                                        (pSrcA[pSrcA_index_q7] >> pSrcA_shift_q7) & 0b1111;
                                }
                            }
                        }
                    }
                }
            }
            else {
                // copy from the previous cache
                for (size_t i = 0; i < kernel_size; ++i) {
                    int cpy_index = i * kernel_size * input_channels;
                    size_t j = 0;
                    for (; j < (kernel_size - 1) * input_channels; ++j) {
                        cache_ptr_0[cpy_index] =
                            cache_ptr_0[cpy_index + input_channels];
                        cpy_index++;
                    }
                }
                for (size_t s = 0; s < kernel_size; ++s) {
                    const int x_a = x + s - padding;
                    cache_ptr_0 += (kernel_size - 1) * input_channels;
                    if ((x_a < 0) || (x_a >= (int)input_size)) {
                        for (size_t t = 0; t < input_channels; ++t) {
                            *(cache_ptr_0++) = padding_constant;
                        }
                    }
                    else {
                        const int y_a = y + kernel_size - 1 - padding;
                        if ((y_a < 0) || (y_a >= (int)input_size)) {
                            for (size_t c = 0; c < input_channels; ++c) {
                                *(cache_ptr_0++) = padding_constant;
                            }
                        }
                        else {
                            int pSrcA_index =
                                (x_a * input_size + y_a)
                                * quantized_input_channels;
                            size_t c = 0;
                            for (; c < input_channels; ++c) {
                                int pSrcA_index_q7 = pSrcA_index / 2;
                                int pSrcA_shift_q7 = 4 - (pSrcA_index & 0b1) * 4;
                                pSrcA_index++;
                                *(cache_ptr_0++) =
                                    (pSrcA[pSrcA_index_q7] >> pSrcA_shift_q7)
                                    & 0b1111;
                            }
                        }
                    }
                }
            }

            int pSrcC_index = (x * output_size + y) * quantized_output_channels;
            for (size_t a = 0; a < quantized_output_channels_4; a += 4) {
                uint32_t tmp_output[4] = {0,0,0,0};
                char* kernel_ptr_char = (char*) (pSrcB + (a >> 2) * cache_size);
                uint32_t tmp_acc = 0;
                size_t k = 0;
                char kernel_index = 0;
                uint32_t kernel = 0;
                for (; k < cache_size - 4; k += 5) {
                    tmp_acc = 0;

                    kernel_index = *(kernel_ptr_char++);
                    kernel = klist2_sram[(int)kernel_index];
                    tmp_acc += kernel * cache_0[k + 0];

                    kernel_index = *(kernel_ptr_char++);
                    kernel = klist2_sram[(int)kernel_index];
                    tmp_acc += kernel * cache_0[k + 1];

                    kernel_index = *(kernel_ptr_char++);
                    kernel = klist2_sram[(int)kernel_index];
                    tmp_acc += kernel * cache_0[k + 2];

                    kernel_index = *(kernel_ptr_char++);
                    kernel = klist2_sram[(int)kernel_index];
                    tmp_acc += kernel * cache_0[k + 3];

                    kernel_index = *(kernel_ptr_char++);
                    kernel = klist2_sram[(int)kernel_index];
                    tmp_acc += kernel * cache_0[k + 4];

                    tmp_output[0] += (tmp_acc >> 24);
                    tmp_output[1] += (tmp_acc >> 16) & 0b11111111;
                    tmp_output[2] += (tmp_acc >>  8) & 0b11111111;
                    tmp_output[3] += (tmp_acc      ) & 0b11111111;
                }
                tmp_acc = 0;
                for (; k < cache_size; k += 1) {
                    kernel_index = *(kernel_ptr_char++);
                    kernel = klist2_sram[(int)kernel_index];
                    tmp_acc += kernel * cache_0[k];
                }
                tmp_output[0] += (tmp_acc >> 24);
                tmp_output[1] += (tmp_acc >> 16) & 0b11111111;
                tmp_output[2] += (tmp_acc >>  8) & 0b11111111;
                tmp_output[3] += (tmp_acc      ) & 0b11111111;
                for (size_t i = 0;
                        i < ((output_channels - a) > 4 ? 4 :
                            (output_channels - a)); ++i) {
                    float result = ((float)tmp_output[i]) * (step_a * step_b[a + i]);
                    result += step_k_K[a + i] * minv_a;
                    result += minv_b[a + i] * output_sum[x * output_size + y];
                    result +=
                        minv_b[a + i] * minv_a * input_channels *
                        kernel_size * kernel_size;
                    result = (result - minv_c) / step_c;
                    char result_q7 = (result > 0b1111) ? 0b1111 : result;
                    int pSrcC_index_q7 = pSrcC_index / 2;
                    int pSrcC_shift_q7 = 4 - (pSrcC_index % 2) * 4;
                    pSrcC[pSrcC_index_q7] &= (0b11110000 << pSrcC_shift_q7);
                    pSrcC[pSrcC_index_q7] |= (result_q7 << pSrcC_shift_q7);
                    pSrcC_index++;
                }
            }
        }
    }

    free(cache_0);
    free(output_sum);
    return;
}
