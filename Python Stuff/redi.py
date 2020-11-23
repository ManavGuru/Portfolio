import redis

r = redis.Redis(host = 'localhost', port=6379, db=0)
r.delete('myhash')
for j in range (0,10): 
	r.hset ('myhash', j, 'QoE,  no_of_stalls, action')

print(r.hgetall('myhash'))