import redis
import time 

r = redis.StrictRedis(host = 'localhost', port=6379, db=0)

for i in range (10):
	table_name = "state_action_table:"
	table_name += str(i+1)
	# read state from clients, write to DB
	curr_state_action = r.hmget(table_name,"state1","state2",
	"state3","state4","state5","state6", "action")
	print("Current State : action",curr_state_action)

