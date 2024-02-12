from math import sqrt
import random
import copy

random.seed(42)

THRESHOLD = 5

RD_MIN_X = 0.
RD_MAX_X = 100000000.
RD_MIN_Y = 0.
RD_MAX_Y = 100000000.
RD_AVG_NB_SIZE = 50
RD_DEV_NB_SIZE = 10
RD_SPEED_X = 5
RD_SPEED_Y = 5

NB_TRAJECTORIES = 0
NB_CLUSTERS = 1
SIZE_CLUSTER_R = 10000
SIZE_CLUSTER_S = 10000

OUT = open(f't_{RD_AVG_NB_SIZE}_{NB_TRAJECTORIES}_{NB_CLUSTERS}x{SIZE_CLUSTER_R}x{SIZE_CLUSTER_S}', 'w+')

def flip():
	return 1 if random.random() < .95 else -1

def get_tag():
	return round(random.random())

def generate_rnd(id):
	trajectory = []
	x0 = random.uniform(RD_MIN_X, RD_MAX_X)
	y0 = random.uniform(RD_MIN_Y, RD_MAX_Y)
	trajectory.append([x0,y0])
	speed_x = abs(random.gauss(0, RD_SPEED_X))
	speed_y = abs(random.gauss(0, RD_SPEED_Y))
	size = int(abs(random.gauss(RD_AVG_NB_SIZE, RD_DEV_NB_SIZE)))
	for p in range(1, size):
		next_x = trajectory[-1][0] + flip() * abs(random.gauss(speed_x, speed_x))
		next_y = trajectory[-1][1] + flip() * abs(random.gauss(speed_y, speed_y))
		trajectory.append([next_x, next_y])
	return id, get_tag(),	trajectory

def alter_trajectory(id, tag, pattern):
	trajectory = copy.deepcopy(pattern)
	for i, p in enumerate(trajectory):
		delta = random.random() * THRESHOLD * THRESHOLD
		delta_x = sqrt(random.random() * delta)
		delta_y = sqrt(delta - delta_x**2)
		trajectory[i][0] += delta_x * flip()
		trajectory[i][1] += delta_y * flip()
	return id, tag, trajectory

def write(out, id, tag, trajectory):
	out.write(f"{id} {tag} {repr(trajectory).replace(' ', '')}\n")

for i in range(NB_TRAJECTORIES):
	write(OUT, *generate_rnd(i))

for i in range(NB_CLUSTERS):
	id, tag, pattern = generate_rnd(NB_TRAJECTORIES+i*(SIZE_CLUSTER_R+SIZE_CLUSTER_S))
	for j in range(SIZE_CLUSTER_R+SIZE_CLUSTER_S):
		write(OUT, *alter_trajectory(id+j, 0 if j < SIZE_CLUSTER_R else 1, pattern))

OUT.close()
