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
while True:
	table_name = "state_action_table:"
	# read state from clients, write to DB
	message = p_sub.get_message()
	if (message):
		current_run_id = str(message['data'])
		if(int(current_run_id) == -22):
			break
		print("run_id:",current_run_id)
	if(current_run_id == previous_run_id):
		pass
	else:
		if (ignore_flag): 
			ignore_flag = 0
			print(ignore_flag)
		else:
			action_to_take = actions[random.randint(0,14)]
			SAt = {"action": str(action_to_take)}
			table_name += current_run_id
			previous_run_id = current_run_id
			print(table_name, SAt)
			redis_db.hmset(table_name, SAt)
			temp =(redis_db.hmget(table_name, "reward"))
			if((temp[0] is None)):
				print(temp)
			else:
				reward_list.append(float(temp[0]))
# print("Rewards are: ",reward_list)
# print("avg_qoe: {}".format(sum(reward_list)/len(reward_list)))
plt.title('Rewards')
plt.xlabel('Time')
plt.ylabel('Reward')
plt.plot(reward_list, label='Episodic Average')
plt.legend()
plt.savefig('eval_results.png')
plt.show()