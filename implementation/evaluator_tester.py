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
    #print("z1: %r" % z1)
    #print("z2: %r" % z2)
    reg = 'Result: ([01]*)'
    try:
        res = bool(int(re.findall(reg, z1)[0], 10))
        print(res)
        return res
    except:
        # treat bind failures/hangs as failures
        return None

def try_random_inputs(howmany, maxval, lessthan=lessthan_ungarbled):
    r = lambda: random.randint(0, maxval)
    num_wrong = 0
    for i in range(howmany):
        if not i % 100:
            print('Iteration %d' % (i,))
        x, y = r(), r()
        b = lessthan(x,y)
        if (x < y) != b:
            num_wrong += 1
            print('Found counterexample: %d, %d' % (x,y))
            print(x < y)
            print(b)
            print('---')
    print("wrongness: %d/%d" % (num_wrong, howmany))

if __name__ == '__main__':
    #try_random_inputs(1000, 2**8-1)
    #try_random_inputs(1000, 2**64-1)
    try_random_inputs(100, 2**8-1, lessthan=lessthan_garbled)
