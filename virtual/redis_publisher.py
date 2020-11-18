import redis
import time

r = redis.StrictRedis(host = 'localhost', port=6379, db=0)


for i in range (10):
	table_name = "state_action_table:"
	# read state from clients, write to DB
	SAt = {"state1": "[12,0,5]", "state2": "[12,0,5]", "state3": "[12,0,5]", 
		"state4": "[12,0,5]", "state5": "[12,0,5]", "state6": "[12,0,5]" , "action":'[0,0,0,0,1,1]'}
	table_name += str(i+1)
	r.hmset(table_name, SAt)

