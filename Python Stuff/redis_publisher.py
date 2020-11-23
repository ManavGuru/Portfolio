import redis
import time
import random

r = redis.StrictRedis(host = 'localhost', port=6379, db=0)

top_limit = 2
n_queues = 2
# queues = [
#     Queue(4*1.1e+3*1024, fairness=1e+2), 
#     Queue(2*6e+3*1024, fairness=1e+3)    
# ]
n_clients = 6
# clients = [
#     Client(bitrate_dist=(2.9e+6, 1e+6), length_dist=(10*60, 10*5), initial_buffer=10)
#     for _ in range(n_clients)
# ]

action_cardinality = n_queues**n_clients
# actions have pre-existing knowledge built in to if statement
actions = [
    [(i//(n_queues**c))%n_queues for c in range(n_clients)]
    for i in range(action_cardinality) 
    if sum([(i//(n_queues**c))%n_queues for c in range(n_clients)]) == top_limit
]

for i in range (10):
	table_name = "state_action_table:"
	current_ts = time.time()
	# read state from clients, write to DB
	action_to_take = actions[random.randint(0,14)]
	SAt = {"action": str(action_to_take), "timestamp": str(current_ts)}
	table_name += str(i+1)
	r.hmset(table_name, SAt)

