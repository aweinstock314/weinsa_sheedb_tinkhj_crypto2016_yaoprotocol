#!/usr/bin/env python
import IPython
import random
import re
import subprocess
import time

def lessthan_ungarbled(x,y):
    pipe = subprocess.PIPE
    p = subprocess.Popen(['./bin/yaoprotocol', 'evaluator', str(x), str(y)], stdout=pipe, stderr=pipe)
    (z,_) = p.communicate()
    reg = 'result\\[0\\]: ([0-9]*)'
    return bool(int(re.findall(reg, z)[0], 10))

def lessthan_garbled(x,y):
    pipe = subprocess.PIPE
    #stderr = None
    stderr = subprocess.PIPE
    port = str(random.randint(8000, 60000))
    print("Port: %s" % port)
    print("%d, %d" % (x,y))
    r = subprocess.Popen(['./bin/yaoprotocol', 'receiver', port, str(y)], stdout=pipe, stderr=stderr)
    s = subprocess.Popen(['./bin/yaoprotocol', 'sender', 'localhost', port, str(x)], stdout=pipe, stderr=stderr)
    (z1,_) = s.communicate()
    (z2,_) = r.communicate()
    print("z1: %r" % z1)
    print("z2: %r" % z2)
    reg = 'Result: ([01]*)'
    return bool(int(re.findall(reg, z1)[0], 10))

def try_random_inputs(howmany, maxval, lessthan=lessthan_ungarbled):
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
    #try_random_inputs(1000, 2**8)
    #try_random_inputs(1000, 2**64)
    try_random_inputs(100, 2**8, lessthan=lessthan_garbled)
