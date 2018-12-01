from __future__ import absolute_import, division, print_function

import numpy
import argparse
import subprocess

def BuildOptions():
    # build options
    print('==> Build Options')
    parser = argparse.ArgumentParser()
    parser.add_argument('--input-bits', action='store', type=int, default=4,
            help='the number of bits for input')
    parser.add_argument('--input-size', action='store', type=int, default=5,
            help='input size')
    parser.add_argument('--weight-bits', action='store', type=int, default=2,
            help='the number of bits for weight')
    parser.add_argument('--input-channels', action='store', type=int, default=2,
            help='the number of input channels')
    parser.add_argument('--output-channels', action='store', type=int, default=2,
            help='the number of output channels')
    parser.add_argument('--kernel-size', action='store', type=int, default=3,
            help='kernel size')
    parser.add_argument('--padding', action='store', type=int, default=1,
            help='padding')
    args = parser.parse_args()
    for arg in vars(args):
        print('----> {:15s} = {}'.format(arg, getattr(args, arg)))
    return args

def Quantize(data, bits):
    data *= (2**bits)
    data -= (2**(bits - 1))
    data = numpy.floor(data)
    data = data.astype(int)
    return data

def BuildInput(input_bits, input_size, input_channels):
    print('==> Build Inputs')
    input_v = numpy.random.rand(input_channels, input_size, input_size)
    input_v = Quantize(input_v, input_bits)
    return input_v

def BuildKernel(weight_bits, kernel_size, input_channels, output_channels):
    print('==> Build Kernels')
    kernel_v = numpy.random.rand(output_channels, input_channels,
            kernel_size, kernel_size)
    kernel_v = Quantize(kernel_v, weight_bits)
    return kernel_v

def DefaultConv(input_v, kernel_v, padding, step, zero_bias, input_bits):
    output_size = input_v.shape[1] + padding * 2 - kernel_v.shape[2] + 1
    output_channels = kernel_v.shape[0]
    output_v = numpy.zeros([output_channels, output_size, output_size]).astype(int)
    # add padding
    input_v = numpy.pad(input_v, ((0,0), (1,1), (1,1)), 'constant')
    kernel_v = kernel_v.reshape(kernel_v.shape[0], -1)
    for i in range(output_v.shape[1]):
        for j in range(output_v.shape[2]):
            tmp_input_window = input_v[:, i:i+3, j:j+3]
            tmp_input_window = tmp_input_window.reshape(-1, 1)
            tmp_output = numpy.matmul(kernel_v, tmp_input_window)
            output_v[:,i,j] = tmp_output.flatten()
    output_v = (output_v - zero_bias) / step
    output_v = output_v.astype(int)
    upper_bound = 2 ** (input_bits - 1) - 1
    lower_bound = - 2 ** (input_bits - 1)
    output_v[output_v > upper_bound] = upper_bound
    output_v[output_v < lower_bound] = lower_bound
    return output_v

if __name__=='__main__':
    args = BuildOptions()
    input_v = BuildInput(args.input_bits, args.input_size, args.input_channels)
    kernel_v = BuildKernel(args.weight_bits, args.kernel_size,
            args.input_channels, args.output_channels)
    shift = numpy.floor(numpy.log(kernel_v[0].size * args.weight_bits * 0.1)
            / numpy.log(2.0))
    step = 2 ** shift + 1 # some random step :)
    zero_bias = int(numpy.random.rand() * (2.0 ** args.input_bits) - \
            (2.0 ** (args.input_bits - 1)))
    output_v = DefaultConv(input_v, kernel_v, args.padding,
            step, zero_bias, args.input_bits)

    # run command

    subprocess.call('make', shell=True)
    subprocess.call('cp test_perf.bin /media/jiecaoyu/NODE_F411RE/', shell=True)
