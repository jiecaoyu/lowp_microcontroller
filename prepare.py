from __future__ import absolute_import, division, print_function


# prepare the 2-bit weights look up table
fp = open('include/util.h', 'w')
fp.write('extern const uint32_t klist2[256];\n')
fp.close()

fp = open('src/util.c', 'w')
fp.write('#include \"arm_math.h\"\n\nconst uint32_t klist2[256] = {')
for i in range(256):
    tmp = i
    tmp_v = 0
    for j in range(4):
        item = tmp % 4
        tmp = tmp >> 2
        tmp_v += (item << (j * 8))
    fp.write(str(tmp_v) + ',')
fp.write('};\n')
fp.close()
