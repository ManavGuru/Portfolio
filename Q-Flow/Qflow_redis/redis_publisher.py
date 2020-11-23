import redis
import time
import random
import matplotlib.pyplot as plt
redis_db = redis.StrictRedis(host = 'localhost', port=6379, decode_responses = True, db=0)
p_sub = redis_db.pubsub()
p_sub.subscribe("latest_run")
encoding = 'utf-8'
top_limit = 2

n_queues = 2
n_clients = 6

action_cardinality = n_queues**n_clients
# actions have pre-existing knowledge built in to if statement
actions = [
    [(i//(n_queues**c))%n_queues for c in range(n_clients)]
    for i in range(action_cardinality) 
    if sum([(i//(n_queues**c))%n_queues for c in range(n_clients)]) == top_limit
]
previous_run_id = "1"
ignore_flag = 1
current_run_id = []
reward_list = []
a_no = 0
while True:
	table_name = "state_action_table:"
	# read state from clients, write to DB
	message = p_sub.get_message()
	if (message):
		current_run_id = int((message['data']))
		previous_run_id = current_run_id - 1
		print("run_id:",current_run_id)
		if(int(current_run_id) == -22):
			break
		else:
			table_name += str(previous_run_id)
			temp =(redis_db.hget(table_name, "reward"))
			state =(redis_db.hget(table_name, "state"))
			print("State: {0}\nReward: {1}".format(state, temp))
			if((temp is None)):
				print('')
			else:
				reward_list.append(float(temp))
			table_name = "state_action_table:"
			table_name += str(current_run_id)
			action_to_take = actions[a_no]
			a_no += 1 
			if(a_no == 15):
				a_no = 0
			SAt = {"action": str(action_to_take)}
			print(action_to_take)
			redis_db.hmset(table_name, SAt)
print("Rewards are: ",reward_list)
print("avg_qoe: {}".format(sum(reward_list)/len(reward_list)))
plt.title('Rewards')
plt.xlabel('Time')
plt.ylabel('Reward')
plt.plot(reward_list, label='Episodic Average')
plt.legend()
plt.savefig('eval_results.png')
plt.show()