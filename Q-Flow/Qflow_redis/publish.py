import redis
import time

redis_db = redis.StrictRedis(host = 'localhost', port=6379, decode_responses = True, db=0)
encoding = 'utf-8'

channel = redis_db.pubsub()

for i in range(10): 
    redis_db.publish("test",time.time())
    time.sleep(1)
    write_redis = {"run_id":3,"state": str("states"), "action": str("action"), "reward": 6}
    redis_db.hmset("table_name", write_redis)