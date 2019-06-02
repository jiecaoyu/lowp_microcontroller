from __future__ import absolute_import, division, print_function

import numpy
import argparse
import subprocess
import write

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
    input_v = Quantize(input_v, min(input_bits, 8))
    return input_v

def BuildKernel(weight_bits, kernel_size, input_channels, output_channels):
    print('==> Build Kernels')
    kernel_v = numpy.random.rand(output_channels, input_channels,
            kernel_size, kernel_size)
    kernel_v = Quantize(kernel_v, min(weight_bits, 8))
    return kernel_v

def DefaultConv(input_v, step_i, bias_i,
        kernel_v, step_k, bias_k,
        padding, input_bits):
    output_size = input_v.shape[1] + padding * 2 - kernel_v.shape[2] + 1
    kernel_size = kernel_v.shape[2]
    output_channels = kernel_v.shape[0]
    output = numpy.zeros([output_channels, output_size, output_size])
    # add padding
    constant = int(-bias_i/step_i)
    input_v = numpy.pad(input_v, ((0,0), (padding, padding), (padding, padding)),
            'constant', constant_values=constant)
    input = input_v * step_i + bias_i

    kernel_v = kernel_v.reshape(kernel_v.shape[0], -1)
    kernel = kernel_v * step_k.reshape(step_k.size, 1) + bias_k.reshape(bias_k.size, 1)
    for i in range(output.shape[1]):
        for j in range(output.shape[2]):
            tmp_input_window = input[:, i:i+kernel_size, j:j+kernel_size]
            tmp_input_window = tmp_input_window.reshape(-1, 1)
            tmp_output = numpy.matmul(kernel, tmp_input_window)
            output[:,i,j] = tmp_output.flatten()
    output_bits = min(input_bits, 8)
    step_o = numpy.abs(output).max() * 0.7 / (2.**(output_bits - 1))
    bias_o = numpy.mean(output)
    output_v = (output - bias_o) / step_o
    output_v = numpy.floor(output_v).astype(int)
    upper_bound = 2 ** (output_bits - 1) - 1
    lower_bound = - 2 ** (output_bits - 1)
    output_v[output_v > upper_bound] = upper_bound
    output_v[output_v < lower_bound] = lower_bound
    return output_v, step_o, bias_o

if __name__=='__main__':
    args = BuildOptions()
    numpy.random.seed(1)
    supported_precisions = {
            16: [16],
            4:  [ 2],
            }

    if (args.input_bits not in supported_precisions.keys()) or\
            (args.weight_bits not in supported_precisions[args.input_bits]):
        raise Exception ("Precision configuration not supported.")

    # generate input_v and kernel_v
    ## input = input_v * step_i - bias_i
    input_v = BuildInput(args.input_bits, args.input_size, args.input_channels)
    step_i = numpy.random.rand()
    bias_i = (numpy.random.rand() - 0.5) * 2. * input_v.max() * 0.7 * step_i

    ## kernel[i] = kernel_v[i] * step_v[i] - bias_i[i]
    kernel_v = BuildKernel(args.weight_bits, args.kernel_size,
            args.input_channels, args.output_channels)
    step_k = numpy.random.rand(args.output_channels)
    bias_k = (numpy.random.rand(args.output_channels) - 0.5) * 10.
    output_v, step_o, bias_o = DefaultConv(
            input_v, step_i, bias_i,
            kernel_v, step_k, bias_k,
            args.padding, args.input_bits)
    print(output_v + 2**(args.input_bits - 1))

    # write the data into data.h
    fp = open('data.h', 'w')
    ## write the settings
    for arg in vars(args):
        value = getattr(args, arg)
        if (type(value) == int):
            fp.write('const int {} = {};\n'.format(arg, getattr(args, arg)))

    ## write input
    input_str = write.InputData2Str(input_v, args.input_bits, step_i, bias_i)
    fp.write(input_str)

    ## write kernel
    kernel_str = write.KernelData2Str(kernel_v, args.weight_bits, step_k, bias_k)
    fp.write(kernel_str)

    ## write output
    fp.write('const float step_o = {};\n'.format(step_o))
    if ((args.input_bits == 16) and (args.weight_bits == 16)):
        fp.write('const float bias_o = {};\n'.format(bias_o))
    else:
        fp.write('const float minv_o = {};\n'.format(
            bias_o - (2. ** (args.input_bits - 1)) * step_o))

    key = 0
    i = 0
    for item in output_v.flatten():
        key ^= (item + (2**(args.input_bits - 1)) + i)
        i += 1
    fp.write('const int key_ref = {};\n'.format(key))
    fp.close()

    ## set the flag
    EXTRA_FLAG = ''
    EXTRA_OBJECTS = ''
    if ((args.input_bits == 16) and (args.weight_bits == 16)):
        EXTRA_FLAG += '\" -DQ15_Q15\"'
    elif ((args.input_bits == 4) and (args.weight_bits == 2)):
        EXTRA_FLAG += '\" -DQ3_Q1\"'
        EXTRA_OBJECTS += '\" ./comp_q3.o ./comp_q1.o ./comp_q3_q1.o ./util.o\"'
    # run command

    subprocess.call('make EXTRA_FLAG={} EXTRA_OBJECTS={}'.format(
        EXTRA_FLAG, EXTRA_OBJECTS), shell=True)
    subprocess.call('cp test_perf.bin /media/jiecaoyu/NODE_F411RE/', shell=True)
