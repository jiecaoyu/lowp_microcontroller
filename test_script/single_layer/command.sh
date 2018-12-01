#!/usr/bin/env bash
    weight_bits=16
     input_bits=16
        padding=1
     input_size=16
output_channels=32
    kernel_size=3
 input_channels=32

 python run_test.py \
	 --weight-bits     $weight_bits \
	 --input-bits      $input_bits \
	 --padding         $padding \
	 --input-size      $input_size \
	 --output-channels $output_channels \
	 --kernel-size     $kernel_size \
	 --input-channels  $input_channels
