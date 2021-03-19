#!/usr/bin/env python

from subprocess import call
from concurrent.futures import ThreadPoolExecutor as Pool
import os
import sys
from random import *
import argparse

# parse command line arguments
parser = argparse.ArgumentParser(description='Solve ARC Tasks.')
parser.add_argument('--tasks', type=int, nargs='*',
                    help='the tasks to run on, defaults to all tasks')
parser.add_argument('--train', type=str, default='t', help='run on training set or validation set?')
parser.add_argument('--depth', type=int, default=2, help='depth to run the search at')
parser.add_argument('--version', type=str, default=str(randint(0,10**9)), help='can put a version on running if want to save/make sure not recomputing')

def str2bool(v):
    if isinstance(v, bool):
       return v
    if v.lower() in ('yes', 'true', 't', 'y', '1'):
        return True
    elif v.lower() in ('no', 'false', 'f', 'n', '0'):
        return False
    else:
        raise argparse.ArgumentTypeError('Boolean value expected.')

    
args = parser.parse_args()
args.train = str2bool(args.train)

if args.tasks is None:
    num_val_tasks = 419
    num_train_tasks = 416
    args.tasks = range(0, num_train_tasks) if args.train else range(0, num_val_tasks)


print("Updating to version", args.version)
dataset = 'Train' if args.train else 'Validation'
print(f'Tasks={args.tasks}\nDataset={dataset}\nDepth={args.depth}')

parallel = 6

os.system('mkdir -p store/version/')
os.system('mkdir -p store/tmp/')

def outdated(i):
    fn = 'store/version/%d.txt'%i
    os.system('touch '+fn)
    f = open(fn, 'r')
    t = f.read()
    f.close()
    return t != args.version

def update(i):
    fn = 'store/version/%d.txt'%i
    os.system('touch '+fn)
    f = open(fn, 'w')
    f.write(args.version)
    f.close()

run_list = [i for i in args.tasks if outdated(i)]   # don't recompute if already  have it

global done
done = 0

n = len(run_list)
def run(i):
    if call(['/usr/bin/time', './run', str(i), str(args.depth)], stdout=open('store/tmp/%d_out.txt'%i,'w'), stderr=open('store/tmp/%d_err.txt'%i,'w')):
        print(i, "Crashed")
        return i
    os.system('cp store/tmp/%d_out.txt store/%d_out.txt'%(i,i))
    update(i)
    global done
    done += 1
    print("%d / %d     \r"%(done, n), end = "")
    sys.stdout.flush()
    return i

call(['make','-j'])
scores = []
with Pool(max_workers=parallel) as executor:
    for i in executor.map(run, run_list):
        pass
print("Done!       ")
