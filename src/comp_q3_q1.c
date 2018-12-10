#include "arm_math.h"
#include "stdio.h"
#include "util.h"
#include <math.h>
#include <stdlib.h>

void Conv2d_q3_q1(
        const q7_t * pSrcI, const float step_i, const float minv_i,
        const q7_t * pSrcK, const float * step_k, const float * minv_k,
        q7_t * pSrcO, const float step_o, const float minv_o,
        const size_t input_channels,
        const size_t input_size,
        const size_t padding,
        const size_t output_channels,
        const size_t kernel_size,
        const float * step_k_K
        ) {
    // (step_i * I + minv_i) * (step_k * K + minv_k)
    // = step_i * step_i * I * K + minv_i * step_k * K
    //      + step_i * minv_k * A + minv_i * minv_k
    const int cache_size =
        ceil(((float)(input_channels * kernel_size * kernel_size)) / 4.0) * 4;
    const int quantized_input_channels =
        ceil(((float)(input_channels)) / 2.0) * 2;
    const int quantized_output_channels =
        ceil(((float)(output_channels)) / 2.0) * 2;
    q7_t* cache = (q7_t*) malloc (cache_size * sizeof(q7_t));

    // set zero
    cache[cache_size - 4] = 0;
    cache[cache_size - 3] = 0;
    cache[cache_size - 2] = 0;
    cache[cache_size - 1] = 0;
    
    const size_t output_size = input_size + 2 * padding - kernel_size + 1;

    const q7_t padding_constant = 8. + ((q7_t)(-(minv_i + step_i * 8.)/step_i));

    // step_i * A
    int* input_sum = (int*) malloc (input_size * input_size * sizeof(int));
    float* output_sum = (float*) malloc (output_size * output_size * sizeof(float));
    memset(output_sum, 0, output_size * output_size * sizeof(int));
    for (size_t x = 0; x < input_size; ++x) {
        for (size_t y = 0; y < input_size; ++y) {
            int sum_tmp = 0;
            int pSrcI_index = (x * input_size + y) * quantized_input_channels;
            for (size_t c = 0; c < input_channels; ++c) {
                int pSrcI_index_q7 = pSrcI_index / 2;
                int pSrcI_shift_q7 = 4 - (pSrcI_index % 2) * 4;
                pSrcI_index++;
                sum_tmp +=
                    (pSrcI[pSrcI_index_q7] >> pSrcI_shift_q7) & 0b1111;
            }
            input_sum[x * input_size + y] = sum_tmp;
        }
    }
    for (size_t x = 0; x < output_size; ++x) {
        for (size_t y = 0; y < output_size; ++y) {
            int sum_tmp = 0;
            for (size_t s = 0; s < kernel_size; ++s) {
                const int x_i = x + s - padding;
                if ((x_i < 0) || (x_i >= (int)input_size)) {
                    sum_tmp += padding_constant * kernel_size * input_channels;
                }
                else {
                    for (size_t t = 0; t < kernel_size; ++t) {
                        const int y_i = y + t - padding;
                        if ((y_i < 0) || (y_i >= (int)input_size)) {
                            sum_tmp += padding_constant * input_channels;
                        }
                        else {
                            sum_tmp += input_sum[x_i * input_size + y_i];
                        }
                    }
                }
            }
            output_sum[x * output_size + y] = sum_tmp * step_i;
        }
    }
    free(input_sum);

    for (size_t x = 0; x < output_size; ++x) {
        for (size_t y = 0; y < output_size; ++y) {
            // write cache
            q7_t * cache_ptr = cache;
            for (size_t s = 0; s < kernel_size; ++s) {
                const int x_i = x + s - padding;
                if ((x_i < 0) || (x_i >= (int)input_size)) {
                    for (size_t t = 0; t < kernel_size * input_channels; ++t) {
                        *(cache_ptr++) = padding_constant;
                    }
                }
                else {
                    for (size_t t = 0; t < kernel_size; ++t) {
                        const int y_i = y + t - padding;
                        if ((y_i < 0) || (y_i >= (int)input_size)) {
                            for (size_t c = 0; c < input_channels; ++c) {
                                *(cache_ptr++) = padding_constant;
                            }
                        }
                        else {
                            int pSrcI_index =
                                (x_i * input_size + y_i) * quantized_input_channels;
                            for (size_t c = 0; c < input_channels; ++c) {
                                int pSrcI_index_q7 = pSrcI_index / 2;
                                int pSrcI_shift_q7 = 4 - (pSrcI_index % 2) * 4;
                                pSrcI_index++;
                                *(cache_ptr++) =
                                    (pSrcI[pSrcI_index_q7] >> pSrcI_shift_q7) & 0b1111;
                            }
                        }
                    }
                }
            }
            int pSrcO_index = (x * output_size + y) * quantized_output_channels;
            for (size_t a = 0; a < output_channels; ++a) {
                int k = cache_size >> 2;
                uint32_t* cache_ptr_uint32 = (uint32_t*) cache;
                char* kernel_ptr_char = (char*) (pSrcK + (cache_size >> 2) * a);
                uint32_t acc = 0;
                while(k) {
                    uint32_t input = *(cache_ptr_uint32++);
                    char kernel_index = *(kernel_ptr_char++);
                    uint32_t kernel = klist2[(int)kernel_index];
                    uint32_t tmp_result = (input * kernel) >> 24;
                    acc += tmp_result;
                    k--;
                }
                float result = ((float)acc) * (step_i * step_k[a]);
                result += step_k_K[a] * minv_i;
                result += minv_k[a] * output_sum[x * output_size + y];
                result += minv_k[a] * minv_i * input_channels * kernel_size * kernel_size;
                result = (result - minv_o) / step_o;
                char result_q7 = (result > 0b1111) ? 0b1111 : result;
                int pSrcO_index_q7 = pSrcO_index / 2;
                int pSrcO_shift_q7 = 4 - (pSrcO_index % 2) * 4;
                pSrcO[pSrcO_index_q7] &= (0b11110000 << pSrcO_shift_q7);
                pSrcO[pSrcO_index_q7] |= (result_q7 << pSrcO_shift_q7);
                pSrcO_index++;
            }
        }
    }
    free(cache);
    free(output_sum);
    return;
}

void Conv2d_q3_q1_opt(
        const q7_t * pSrcI, const float step_i, const float minv_i,
        const q7_t * pSrcK, const float * step_k, const float * minv_k,
        q7_t * pSrcO, const float step_o, const float minv_o,
        const size_t input_channels,
        const size_t input_size,
        const size_t padding,
        const size_t output_channels,
        const size_t kernel_size,
        const float * step_k_K
        ) {
    // (step_i * I + minv_i) * (step_k * K + minv_k)
    // = step_i * step_i * I * K + minv_i * step_k * K
    //      + step_i * minv_k * A + minv_i * minv_k
    const int cache_size =
        ceil(((float)(input_channels * kernel_size * kernel_size)) / 4.0) * 4;
    const int quantized_input_channels =
        ceil(((float)(input_channels)) / 2.0) * 2;
    const int quantized_output_channels =
        ceil(((float)(output_channels)) / 2.0) * 2;
    q7_t* cache_1 = (q7_t*) malloc (cache_size * sizeof(q7_t));
    q7_t* cache_2 = (q7_t*) malloc (cache_size * sizeof(q7_t));
    uint32_t klist2_sram[256];

    for (int i = 0; i < 256 / 4; i += 1) {
        klist2_sram[4 * i + 0] = klist2[4 * i + 0];
        klist2_sram[4 * i + 1] = klist2[4 * i + 1];
        klist2_sram[4 * i + 2] = klist2[4 * i + 2];
        klist2_sram[4 * i + 3] = klist2[4 * i + 3];
    }

    // set zero
    memset(cache_1 + cache_size - 4, 0, 4 * sizeof(q7_t));
    memset(cache_2 + cache_size - 4, 0, 4 * sizeof(q7_t));
    
    const size_t output_size = input_size + 2 * padding - kernel_size + 1;

    const q7_t padding_constant = 8. + ((q7_t)(-(minv_i + step_i * 8.)/step_i));

    // step_i * A
    int* input_sum = (int*) malloc (input_size * input_size * sizeof(int));
    float* output_sum = (float*) malloc (output_size * output_size * sizeof(float));
    memset(output_sum, 0, output_size * output_size * sizeof(int));
    for (size_t x = 0; x < input_size; ++x) {
        for (size_t y = 0; y < input_size; ++y) {
            int sum_tmp = 0;
            int pSrcI_index = (x * input_size + y) * quantized_input_channels;
            for (size_t c = 0; c < input_channels; ++c) {
                int pSrcI_index_q7 = pSrcI_index / 2;
                int pSrcI_shift_q7 = 4 - (pSrcI_index % 2) * 4;
                pSrcI_index++;
                sum_tmp +=
                    (pSrcI[pSrcI_index_q7] >> pSrcI_shift_q7) & 0b1111;
            }
            input_sum[x * input_size + y] = sum_tmp;
        }
    }
    const float output_sum_constant = minv_i * input_channels * kernel_size * kernel_size;
    for (size_t x = 0; x < output_size; ++x) {
        for (size_t y = 0; y < output_size; ++y) {
            int sum_tmp = 0;
            for (size_t s = 0; s < kernel_size; ++s) {
                const int x_i = x + s - padding;
                if ((x_i < 0) || (x_i >= (int)input_size)) {
                    sum_tmp += padding_constant * kernel_size * input_channels;
                }
                else {
                    for (size_t t = 0; t < kernel_size; ++t) {
                        const int y_i = y + t - padding;
                        if ((y_i < 0) || (y_i >= (int)input_size)) {
                            sum_tmp += padding_constant * input_channels;
                        }
                        else {
                            sum_tmp += input_sum[x_i * input_size + y_i];
                        }
                    }
                }
            }
            output_sum[x * output_size + y] = sum_tmp * step_i + output_sum_constant;
        }
    }
    free(input_sum);

    size_t x = 0;
    for (; x < output_size - 1; x += 2) {
        for (size_t y = 0; y < output_size; ++y) {
            // write cache
            for (int cache_index = 0; cache_index < 2; ++cache_index) {
                q7_t * cache_ptr = cache_1;
                if (cache_index == 1) {
                    cache_ptr = cache_2;
                }
                if (y == 0) {
                    for (size_t s = 0; s < kernel_size; ++s) {
                        const int x_i = x + s - padding + cache_index;
                        if ((x_i < 0) || (x_i >= (int)input_size)) {
                            for (size_t t = 0; t < kernel_size * input_channels; ++t) {
                                *(cache_ptr++) = padding_constant;
                            }
                        }
                        else {
                            for (size_t t = 0; t < kernel_size; ++t) {
                                const int y_i = y + t - padding;
                                if ((y_i < 0) || (y_i >= (int)input_size)) {
                                    for (size_t c = 0; c < input_channels; ++c) {
                                        *(cache_ptr++) = padding_constant;
                                    }
                                }
                                else {
                                    int pSrcI_index =
                                        (x_i * input_size + y_i)
                                        * quantized_input_channels;
                                    size_t c = 0;
                                    for (; c < input_channels - 1; c += 2) {
                                        int pSrcI_index_q7 = pSrcI_index / 2;
                                        *(cache_ptr++) =
                                            (pSrcI[pSrcI_index_q7] >> 4) & 0b1111;
                                        *(cache_ptr++) =
                                            (pSrcI[pSrcI_index_q7]) & 0b1111;
                                        pSrcI_index += 2;
                                    }
                                    for (; c < input_channels; ++c) {
                                        int pSrcI_index_q7 = pSrcI_index / 2;
                                        int pSrcI_shift_q7 = 4 - (pSrcI_index & 0b1) * 4;
                                        pSrcI_index++;
                                        *(cache_ptr++) =
                                            (pSrcI[pSrcI_index_q7] >> pSrcI_shift_q7)
                                            & 0b1111;
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
                        int* cpy_dst_int_ptr = (int*)(cache_ptr + cpy_index);
                        int* cpy_src_int_ptr =
                            (int*)(cache_ptr + cpy_index + input_channels);
                        for (; j < (kernel_size - 1) * input_channels - 3; j += 4) {
                            *(cpy_dst_int_ptr++) = *(cpy_src_int_ptr++);
                        }
                        cpy_index = i * kernel_size * input_channels + j;
                        for (; j < (kernel_size - 1) * input_channels; ++j) {
                            *(cache_ptr + cpy_index) =
                                *(cache_ptr + cpy_index + input_channels);
                            cpy_index++;
                        }
                    }
                    for (size_t s = 0; s < kernel_size; ++s) {
                        const int x_i = x + s - padding + cache_index;
                        cache_ptr += (kernel_size - 1) * input_channels;
                        if ((x_i < 0) || (x_i >= (int)input_size)) {
                            for (size_t t = 0; t < input_channels; ++t) {
                                *(cache_ptr++) = padding_constant;
                            }
                        }
                        else {
                            const int y_i = y + kernel_size - 1 - padding;
                            if ((y_i < 0) || (y_i >= (int)input_size)) {
                                for (size_t c = 0; c < input_channels; ++c) {
                                    *(cache_ptr++) = padding_constant;
                                }
                            }
                            else {
                                int pSrcI_index =
                                    (x_i * input_size + y_i)
                                    * quantized_input_channels;
                                size_t c = 0;
                                for (; c < input_channels - 1; c += 2) {
                                    int pSrcI_index_q7 = pSrcI_index / 2;
                                    *(cache_ptr++) =
                                        (pSrcI[pSrcI_index_q7] >> 4) & 0b1111;
                                    *(cache_ptr++) =
                                        (pSrcI[pSrcI_index_q7]) & 0b1111;
                                    pSrcI_index += 2;
                                }
                                for (; c < input_channels; ++c) {
                                    int pSrcI_index_q7 = pSrcI_index / 2;
                                    int pSrcI_shift_q7 = 4 - (pSrcI_index & 0b1) * 4;
                                    pSrcI_index++;
                                    *(cache_ptr++) =
                                        (pSrcI[pSrcI_index_q7] >> pSrcI_shift_q7)
                                        & 0b1111;
                                }
                            }
                        }
                    }
                }
            }
            int pSrcO_index_1 = (x * output_size + y) * quantized_output_channels;
            int pSrcO_index_2 = ((x + 1) * output_size + y) * quantized_output_channels;
            size_t a = 0;
            for (; a < output_channels - 1; a += 2) {
                int k = cache_size >> 2;
                uint32_t* cache_ptr_uint32_1 = (uint32_t*) cache_1;
                uint32_t* cache_ptr_uint32_2 = (uint32_t*) cache_2;
                char* kernel_ptr_char_1 = (char*) (pSrcK + (cache_size >> 2) * a);
                char* kernel_ptr_char_2 =
                    (char*) (pSrcK + (cache_size >> 2) * (a + 1));
                uint32_t acc_1 = 0;
                uint32_t acc_2 = 0;
                uint32_t acc_3 = 0;
                uint32_t acc_4 = 0;
                while(k) {
                    uint32_t input_1 = *(cache_ptr_uint32_1++);
                    uint32_t input_2 = *(cache_ptr_uint32_2++);
                    char kernel_index_1 = *(kernel_ptr_char_1++);
                    char kernel_index_2 = *(kernel_ptr_char_2++);
                    uint32_t kernel_1 = klist2_sram[(int)kernel_index_1];
                    uint32_t kernel_2 = klist2_sram[(int)kernel_index_2];
                    uint32_t tmp_result_1 = (input_1 * kernel_1) >> 24;
                    uint32_t tmp_result_2 = (input_1 * kernel_2) >> 24;
                    uint32_t tmp_result_3 = (input_2 * kernel_1) >> 24;
                    uint32_t tmp_result_4 = (input_2 * kernel_2) >> 24;
                    acc_1 += tmp_result_1;
                    acc_2 += tmp_result_2;
                    acc_3 += tmp_result_3;
                    acc_4 += tmp_result_4;
                    k--;
                }
                float result_1 = ((float)acc_1) * (step_i * step_k[a]);
                float result_2 = ((float)acc_2) * (step_i * step_k[a + 1]);
                result_1 += step_k_K[a] * minv_i;
                result_1 += minv_k[a] * output_sum[x * output_size + y];
                result_1 = (result_1 - minv_o) / step_o;
                result_2 += step_k_K[a + 1] * minv_i;
                result_2 += minv_k[a + 1] * output_sum[x * output_size + y];
                result_2 = (result_2 - minv_o) / step_o;
                char result_q7_1 = (result_1 > 0b1111) ? 0b1111 : result_1;
                char result_q7_2 = (result_2 > 0b1111) ? 0b1111 : result_2;
                int pSrcO_index_q7_1 = pSrcO_index_1 >> 1;
                pSrcO[pSrcO_index_q7_1] = (result_q7_2);
                pSrcO[pSrcO_index_q7_1] |= (result_q7_1 << 4);

                float result_3 = ((float)acc_3) * (step_i * step_k[a]);
                float result_4 = ((float)acc_4) * (step_i * step_k[a + 1]);
                result_3 += step_k_K[a] * minv_i;
                result_3 += minv_k[a] * output_sum[(x + 1) * output_size + y];
                result_3 = (result_3 - minv_o) / step_o;
                result_4 += step_k_K[a + 1] * minv_i;
                result_4 += minv_k[a + 1] * output_sum[(x + 1) * output_size + y];
                result_4 = (result_4 - minv_o) / step_o;
                char result_q7_3 = (result_3 > 0b1111) ? 0b1111 : result_3;
                char result_q7_4 = (result_4 > 0b1111) ? 0b1111 : result_4;
                int pSrcO_index_q7_2 = pSrcO_index_2 >> 1;
                pSrcO[pSrcO_index_q7_2] = (result_q7_4);
                pSrcO[pSrcO_index_q7_2] |= (result_q7_3 << 4);
                pSrcO_index_1 += 2;
                pSrcO_index_2 += 2;
            }
            for (; a < output_channels; ++a) {
                int k = cache_size >> 2;
                uint32_t* cache_ptr_uint32_1 = (uint32_t*) cache_1;
                uint32_t* cache_ptr_uint32_2 = (uint32_t*) cache_2;
                char* kernel_ptr_char = (char*) (pSrcK + (cache_size >> 2) * a);
                uint32_t acc_1 = 0;
                uint32_t acc_2 = 0;
                while(k) {
                    uint32_t input_1 = *(cache_ptr_uint32_1++);
                    uint32_t input_2 = *(cache_ptr_uint32_2++);
                    char kernel_index = *(kernel_ptr_char++);
                    uint32_t kernel = klist2_sram[(int)kernel_index];
                    uint32_t tmp_result_1 = (input_1 * kernel) >> 24;
                    uint32_t tmp_result_2 = (input_2 * kernel) >> 24;
                    acc_1 += tmp_result_1;
                    acc_2 += tmp_result_2;
                    k--;
                }
                float result = ((float)acc_1) * (step_i * step_k[a]);
                result += step_k_K[a] * minv_i;
                result += minv_k[a] * output_sum[x * output_size + y];
                result = (result - minv_o) / step_o;
                char result_q7 = (result > 0b1111) ? 0b1111 : result;
                int pSrcO_index_q7 = pSrcO_index_1 >> 1;
                int pSrcO_shift_q7 = 4 - (pSrcO_index_1 & 0b1) * 4;
                pSrcO[pSrcO_index_q7] &= (0b11110000 << pSrcO_shift_q7);
                pSrcO[pSrcO_index_q7] |= (result_q7 << pSrcO_shift_q7);
                pSrcO_index_1++;

                result = ((float)acc_2) * (step_i * step_k[a]);
                result += step_k_K[a] * minv_i;
                result += minv_k[a] * output_sum[(x + 1) * output_size + y];
                result = (result - minv_o) / step_o;
                result_q7 = (result > 0b1111) ? 0b1111 : result;
                pSrcO_index_q7 = pSrcO_index_2 >> 1;
                pSrcO_shift_q7 = 4 - (pSrcO_index_2 & 0b1) * 4;
                pSrcO[pSrcO_index_q7] &= (0b11110000 << pSrcO_shift_q7);
                pSrcO[pSrcO_index_q7] |= (result_q7 << pSrcO_shift_q7);
                pSrcO_index_2++;
            }
        }
    }

    for (; x < output_size; ++x) {
        for (size_t y = 0; y < output_size; ++y) {
            // write cache
            q7_t * cache_ptr_1 = cache_1;
            for (size_t s = 0; s < kernel_size; ++s) {
                const int x_i = x + s - padding;
                if ((x_i < 0) || (x_i >= (int)input_size)) {
                    for (size_t t = 0; t < kernel_size * input_channels; ++t) {
                        *(cache_ptr_1++) = padding_constant;
                    }
                }
                else {
                    for (size_t t = 0; t < kernel_size; ++t) {
                        const int y_i = y + t - padding;
                        if ((y_i < 0) || (y_i >= (int)input_size)) {
                            for (size_t c = 0; c < input_channels; ++c) {
                                *(cache_ptr_1++) = padding_constant;
                            }
                        }
                        else {
                            int pSrcI_index =
                                (x_i * input_size + y_i) * quantized_input_channels;
                            for (size_t c = 0; c < input_channels; ++c) {
                                int pSrcI_index_q7 = pSrcI_index / 2;
                                int pSrcI_shift_q7 = 4 - (pSrcI_index % 2) * 4;
                                pSrcI_index++;
                                *(cache_ptr_1++) =
                                    (pSrcI[pSrcI_index_q7] >> pSrcI_shift_q7) & 0b1111;
                            }
                        }
                    }
                }
            }
            int pSrcO_index = (x * output_size + y) * quantized_output_channels;
            size_t a = 0;
            for (; a < output_channels - 1; a += 2) {
                int k = cache_size >> 2;
                uint32_t* cache_ptr_uint32_1 = (uint32_t*) cache_1;
                char* kernel_ptr_char_1 = (char*) (pSrcK + (cache_size >> 2) * a);
                char* kernel_ptr_char_2 =
                    (char*) (pSrcK + (cache_size >> 2) * (a + 1));
                uint32_t acc_1 = 0;
                uint32_t acc_2 = 0;
                while(k) {
                    uint32_t input = *(cache_ptr_uint32_1++);
                    char kernel_index_1 = *(kernel_ptr_char_1++);
                    char kernel_index_2 = *(kernel_ptr_char_2++);
                    uint32_t kernel_1 = klist2_sram[(int)kernel_index_1];
                    uint32_t kernel_2 = klist2_sram[(int)kernel_index_2];
                    uint32_t tmp_result_1 = (input * kernel_1) >> 24;
                    uint32_t tmp_result_2 = (input * kernel_2) >> 24;
                    acc_1 += tmp_result_1;
                    acc_2 += tmp_result_2;
                    k--;
                }
                float result_1 = ((float)acc_1) * (step_i * step_k[a]);
                float result_2 = ((float)acc_2) * (step_i * step_k[a + 1]);
                result_1 += step_k_K[a] * minv_i;
                result_1 += minv_k[a] * output_sum[x * output_size + y];
                result_1 = (result_1 - minv_o) / step_o;
                result_2 += step_k_K[a + 1] * minv_i;
                result_2 += minv_k[a + 1] * output_sum[x * output_size + y];
                result_2 = (result_2 - minv_o) / step_o;
                char result_q7_1 = (result_1 > 0b1111) ? 0b1111 : result_1;
                char result_q7_2 = (result_2 > 0b1111) ? 0b1111 : result_2;
                int pSrcO_index_q7_1 = pSrcO_index >> 1;
                pSrcO[pSrcO_index_q7_1] = (result_q7_2);
                pSrcO[pSrcO_index_q7_1] |= (result_q7_1 << 4);
                pSrcO_index += 2;
            }
            for (; a < output_channels; ++a) {
                int k = cache_size >> 2;
                uint32_t* cache_ptr_uint32_1 = (uint32_t*) cache_1;
                char* kernel_ptr_char = (char*) (pSrcK + (cache_size >> 2) * a);
                uint32_t acc = 0;
                while(k) {
                    uint32_t input = *(cache_ptr_uint32_1++);
                    char kernel_index = *(kernel_ptr_char++);
                    uint32_t kernel = klist2_sram[(int)kernel_index];
                    uint32_t tmp_result = (input * kernel) >> 24;
                    acc += tmp_result;
                    k--;
                }
                float result = ((float)acc) * (step_i * step_k[a]);
                result += step_k_K[a] * minv_i;
                result += minv_k[a] * output_sum[x * output_size + y];
                result = (result - minv_o) / step_o;
                char result_q7 = (result > 0b1111) ? 0b1111 : result;
                int pSrcO_index_q7 = pSrcO_index >> 1;
                int pSrcO_shift_q7 = 4 - (pSrcO_index & 0b1) * 4;
                pSrcO[pSrcO_index_q7] &= (0b11110000 << pSrcO_shift_q7);
                pSrcO[pSrcO_index_q7] |= (result_q7 << pSrcO_shift_q7);
                pSrcO_index++;
            }
        }
    }
    free(cache_1);
    free(output_sum);
    return;
}
