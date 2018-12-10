from __future__ import absolute_import, division, print_function

import numpy

def InputData2Str(data, bits, step_i, bias_i):
    if bits == 16:
        data = numpy.moveaxis(data, 0, -1)
        data = data.flatten()
        input_str = 'const q15_t input_v[{}] = '.format(data.size) + '{'
        for item in data:
            input_str += '{}'.format(item) + ','
        input_str += '};\n'

        input_str += 'const float step_i ={};\n'.format(step_i)
        input_str += 'const float bias_i ={};\n'.format(bias_i)

        return input_str
    elif bits == 4:
        data += 2 ** (bits - 1)
        print(data)
        data = numpy.moveaxis(data, 0, -1)
        data = data.reshape(-1, data.shape[-1])
        input_str = 'const char input_v[{:.0f}] = '.format(
                data.shape[0] * numpy.ceil(float(data.shape[1]) / 2.0)) + '{'
        for line in data:
            if len(line) % 2 == 1:
                line = numpy.concatenate((line, numpy.zeros(1).astype(int)))
            for i in range(int(len(line) / 2)):
                input_str += '0b' + \
                        '{:04b}'.format(line[2 * i]) + \
                        '{:04b}'.format(line[2 * i + 1]) + ','
        input_str += '};\n'

        input_str += 'const float step_i ={};\n'.format(step_i)
        input_str += 'const float minv_i ={};\n'.format(
                bias_i - step_i * (2. ** (bits - 1)))
        return input_str
    return None

def KernelData2Str(data, bits, step_k, bias_k):
    if bits == 16:
        output_channels = data.shape[0]
        data = numpy.moveaxis(data, 1, -1)
        data = data.flatten()
        kernel_str = 'const q15_t kernel_v[{}] = '.format(data.size) + '{'
        for item in data:
            kernel_str += '{}'.format(item) + ','
        kernel_str += '};\n'

        kernel_str += 'const float step_k[{}] = '.format(output_channels) + '{'
        for item in step_k:
            kernel_str += '{}'.format(item) + ','
        kernel_str += '};\n'

        kernel_str += 'const float bias_k[{}] = '.format(output_channels) + '{'
        for item in bias_k:
            kernel_str += '{}'.format(item) + ','
        kernel_str += '};\n'

        return kernel_str
    elif bits == 2:
        output_channels = data.shape[0]
        data += 2 ** (bits - 1)
        print(data)
        data = numpy.moveaxis(data, 1, -1)
        data = data.reshape(data.shape[0], -1)

        kernel_str = 'const char kernel_v[{:.0f}] = '.format(
                numpy.ceil(float(data.shape[1]) / 4.) * data.shape[0]) + '{'
        for line in data:
            if len(line) % 4 != 0:
                line = numpy.concatenate(
                        (line, numpy.zeros(4 - len(line) % 4).astype(int)))
            for i in range(int(len(line) / 4)):
                kernel_str += '0b' + \
                        '{:02b}'.format(line[4 * i + 0]) + \
                        '{:02b}'.format(line[4 * i + 1]) + \
                        '{:02b}'.format(line[4 * i + 2]) + \
                        '{:02b}'.format(line[4 * i + 3]) + \
                        ','
        kernel_str += '};\n'

        kernel_str += 'const float step_k[{}] = '.format(output_channels) + '{'
        for item in step_k:
            kernel_str += '{}'.format(item) + ','
        kernel_str += '};\n'

        kernel_str += 'const float minv_k[{}] = '.format(output_channels) + '{'
        for index in range(len(bias_k)):
            kernel_str += '{}'.format(bias_k[index] \
                    - (2.0 ** (bits - 1)) * step_k[index]) + ','
        kernel_str += '};\n'

        kernel_str += 'const float step_k_K[{}] = '.format(output_channels) + '{'
        for index in range(len(bias_k)):
            kernel_str += '{}'.format(step_k[index] * data[index].sum()) + ','
        kernel_str += '};\n'

        return kernel_str
    return None
