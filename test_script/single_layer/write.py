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
    return None

def KernelData2Str(data, bits, step_k, bias_k):
    if bits == 16:
        output_channels = data.shape[0]
        data = numpy.moveaxis(data, 1, -1)
        data = data.flatten()
        input_str = 'const q15_t kernel_v[{}] = '.format(data.size) + '{'
        for item in data:
            input_str += '{}'.format(item) + ','
        input_str += '};\n'

        input_str += 'const float step_k[{}] = '.format(output_channels) + '{'
        for item in step_k:
            input_str += '{}'.format(item) + ','
        input_str += '};\n'

        input_str += 'const float bias_k[{}] = '.format(output_channels) + '{'
        for item in bias_k:
            input_str += '{}'.format(item) + ','
        input_str += '};\n'

        return input_str
    return None
