#include "comp_q3.h"
#include "comp_q1.h"

void Conv2d_q3_q1(
        const q7_t * pSrcA, const float step_a, const float minv_a,
        const q7_t * pSrcB, const float * step_b, const float * minv_b,
        q7_t * pSrcC, const float step_c, const float minv_c,
        const size_t input_channels,
        const size_t input_size,
        const size_t padding,
        const size_t output_channels,
        const size_t kernel_size,
        const float * step_k_K
        );

void Conv2d_q3_q1_opt(
        const q7_t * pSrcA, const float step_a, const float minv_a,
        const q7_t * pSrcB, const float * step_b, const float * minv_b,
        q7_t * pSrcC, const float step_c, const float minv_c,
        const size_t input_channels,
        const size_t input_size,
        const size_t padding,
        const size_t output_channels,
        const size_t kernel_size,
        const float * step_k_K
        );

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
        );
