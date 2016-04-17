#!/usr/bin/env python
import subprocess
import random
import re
import IPython

def lessthan(x,y):
    pipe = subprocess.PIPE
    p = subprocess.Popen(['./bin/yaoprotocol', 'evaluator', str(x), str(y)], stdout=pipe, stderr=pipe)
    (z,_) = p.communicate()
    reg = 'result\\[0\\]: ([0-9]*)'
    return bool(int(re.findall(reg, z)[0], 10))

def try_random_inputs(howmany, maxval):
    r = lambda: random.randint(0, maxval)
    for i in range(howmany):
        if not i % 100:
            print('Iteration %d' % (i,))
        x, y = r(), r()
        b = lessthan(x,y)
        if (x < y) != b:
            print('Found counterexample: %d, %d' % (x,y))
            print(x < y)
            print(b)
            print('---')

if __name__ == '__main__':
    try_random_inputs(1000, 100)
    try_random_inputs(1000, 2**8)
    try_random_inputs(1000, 2**64)
