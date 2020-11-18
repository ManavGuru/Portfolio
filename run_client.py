import gym
import time
import numpy as np
import matplotlib.pyplot as plt
import random
from virtual.virtual_env_client import Queue, Client

import redis
import time
encoding = 'utf-8'
redis_db = redis.StrictRedis(host = 'localhost', port=6379, decode_responses = True, db=0)
channel =  redis_db.pubsub()
# tuned so round_robin barely fails
top_limit = 2
n_queues = 2
queues = [
    Queue(4*1.1e+3*1024, fairness=1e+2), 
    Queue(2*6e+3*1024, fairness=1e+3)    
]
n_clients = 6
clients = [
    Client(bitrate_dist=(2.9e+6, 1e+6), length_dist=(10*60, 10*5), initial_buffer=10)
    for _ in range(n_clients)
]

action_cardinality = n_queues**n_clients
# actions have pre-existing knowledge built in to if statement
actions = [
    [(i//(n_queues**c))%n_queues for c in range(n_clients)]
    for i in range(action_cardinality) 
    if sum([(i//(n_queues**c))%n_queues for c in range(n_clients)]) == top_limit
]

def string2action (action_in): 
    new_action = [int(i) for i in action_in if i.isdigit()]
    return new_action

no_of_runs = 102
for run_id in range (2,no_of_runs):
    table_name = "state_action_table:"
    table_name += str(run_id-1)
    if(run_id == 2): 
        action = actions[random.randint(0,14)]
    else:
        curr_state_action = redis_db.hmget(table_name, "action")
        # print("Current action to take",curr_state_action[-1])
        print("Reading from",table_name,curr_state_action)
        if(curr_state_action == [None]):
            time.sleep(2)
            curr_state_action = redis_db.hmget(table_name, "action")
            # print("Current action to take",curr_state_action[-1])
            print("Reading from_none",table_name,curr_state_action)
        action = string2action(curr_state_action[-1])
        # print("Taking this action:",action)
    reward = 0
    states = [[0,0,0] for _ in range(n_clients)]
    # setting the queues 
    for i,q in enumerate(queues): 
        c_set = np.where(np.array(action) == i)[0]
        s,r,_,_ = q.step(10, [clients[c] for c in c_set])
    for j,c in enumerate(c_set): 
        states[c] = s[j]
    reward += r
    print("State:{0} Reward:{1}".format(states, reward/6))  
    write_redis = {"run_id":run_id,"state": str(states), "action": str(action), "reward": reward/6}
    redis_db.hmset(table_name,write_redis)
    redis_db.sadd("master_table", run_id)
    print("publishing current_run:", run_id)
    redis_db.publish("latest_run", run_id)
    time.sleep(0.1)
redis_db.publish("latest_run",-22)