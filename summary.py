inds = range(0,419)
train = False
if train:
    inds = range(0, 416)

inds = list(inds)
compressed = ''

memories = []
times = []

def read(fn):
    try:
        f = open(fn, 'r')
        t = f.read()
        f.close()
        return t
    except:
        return ''

score = [0,0,0,0]
for i in inds:
    t = read('store/%d_out.txt'%i)
    line = t[t.index('Task #'):].split('\n')[0]
    #print(line)
    if line.count('Correct'): s = 3
    elif line.count('Candidate'): s = 2
    elif line.count('Dimensions'): s = 1
    else: s = 0
    score[s] += 1
    compressed += str(s)

    t = read('store/tmp/%d_err.txt'%i)
    # if t:
    #     memories.append([int(t.split('maxresident')[0].split(' ')[-1]), i])
    #     m,s = t.split('elapsed')[0].split(' ')[-1].split(':')
    #     times.append([float(m)*60+float(s), i])

for i in range(3,0,-1):
    score[i-1] += score[i]

print(compressed)
print()
print("Total: % 4d" % score[0])
print("Size : % 4d" % score[1])
print("Cands: % 4d" % score[2])
print("Correct:% 3d"% score[3])

# memories.sort(reverse=True)
# it = 0
# for mem,i in memories:
#     print("%d : %.1f GB"%(i, mem/2**20))
#     it += 1
#     if it == 5: break
# print()
# times.sort(reverse=True)
# it = 0
# for secs,i in times:
#     print("%d : %.1f s"%(i, secs))
#     it += 1
#     if it == 5: break

# exit(0)
# for i in inds:
#     t = read('store/tmp/%d_err.txt'%i)
#     if t:
#         print(t.count("Features: 4"))
# import numpy as np
# from sklearn import cross_validation, linear_model
# from math import log10

# #Estimate times
# x, y = [], []
# for i in inds:
#     t = read('store/tmp/%d_err.txt'%i)
#     if t:
#         m,s = t.split('elapsed')[0].split(' ')[-1].split(':')
#         y.append(float(m)*60+float(s))
#         f = [float(i) for i in t.split('Features: ')[-1].split('\n')[0].split(' ')]
#         p = []
#         print(f, y[-1])
#         for i in range(len(f)):
#             for j in range(i):
#                 p.append(f[i]*f[j])
#             p.append(f[i])
#         p = [f[0], f[3], f[0]*f[3]]
#         x.append(p)

# """loo = cross_validation.LeaveOneOut(len(y))
# regr = linear_model.LinearRegression()
# scores = cross_validation.cross_val_score(regr, x, y, scoring='neg_mean_squared_error', cv=loo,)
# print(10**((-scores.mean())**.5))"""
# model = linear_model.LinearRegression()
# model.fit(x, y)
# r_sq = model.score(x, y)

# loo = cross_validation.LeaveOneOut(len(y))
# scores = cross_validation.cross_val_score(model, x, y, scoring='neg_mean_squared_error', cv=loo,)
# print(((-scores.mean())**.5))

# print('coefficient of determination:', r_sq)
# print('intercept:', model.intercept_)
# print('slope:', model.coef_)
