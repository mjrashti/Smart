#!/usr/bin/python
import sys, pupynere
ll = [l.strip().split(',') for l in open(sys.argv[1]) if not l.startswith('#')]
vv = zip(*[map(float, l) for l in ll])
nc = pupynere.netcdf_file(sys.argv[1]+'.nc', 'w')
nc.createDimension('dim', None)
for i in range(len(vv)):
	nc.createVariable('var_%02d' % i, 'd', ('dim',))[:] = vv[i]

##my alternative
#import sys, pupynere
#ll = [l.strip().split(',') for l in open(sys.argv[1]) if not l.startswith('#')]
#vv = zip(*[map(float, l) for l in ll])
#nc = pupynere.netcdf_file(sys.argv[1]+'.nc', 'w')
#dims = ()
#for i in range(len(vv)):
#	dim_name = 'dim_%d' % i
#	if i == 1:
#		nc.createDimension(dim_name, None)
#	else:
#		nc.createDimension(dim_name, sys.argv[2])
#	dims = dims + (dim_name,)  
#print dims
#nc.createVariable('point', 'd', ('dim_1',))[:] = ll[:]
