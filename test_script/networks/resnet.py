from __future__ import absolute_import, division, print_function

import subprocess
import os
import time

log_name = '/home/jiecaoyu/screenlog.0'

def file_len(fname):
    f = open(fname)
    for i, l in enumerate(f):
        pass
    f.close()
    return i + 1

def execute(inst):
    print(inst)
    subprocess.call(inst, shell=True)
    return

def wait_for_result(pre_line):
    while True:
        time.sleep(10)
        cur_line = file_len(log_name)
        if (cur_line > pre_line):
            return cur_line
        print('copy again')
        execute('cp test_perf.bin /media/jiecaoyu/NODE_F411RE/')
    return 0

def print_result(fname, start_line):
    print('\n\n\n')
    f = open(fname)
    for i, l in enumerate(f):
        if (i > start_line):
            print(l, end='')
    f.close()
    return

def run(params_list, start_line):
    pre_line = start_line
    for params in params_list:
        inst = 'python run_test.py'
        inst += ' --weight-bits 2 '
        inst += ' --input-bits 4 '
        inst += ' --input-channels {} '.format(params[0])
        inst += ' --output-channels {} '.format(params[1])
        inst += ' --input-size {} '.format(params[2])
        inst += ' --kernel-size {} '.format(params[3])
        inst += ' --padding {} '.format(params[4])
        execute(inst)
        pre_line = wait_for_result(pre_line)

params_list = [
        [  16,   16, 32, 3, 1],
        [  16,   32, 16, 3, 1],
        [  32,   32, 16, 3, 1],
        [  32,   64,  8, 3, 1],
        [  64,   64,  8, 3, 1],
        ]

start_line = file_len(log_name)
os.chdir('../single_layer/')

run(params_list, start_line)
execute('bash command.sh')
execute('make clean')

print_result(log_name, start_line)
