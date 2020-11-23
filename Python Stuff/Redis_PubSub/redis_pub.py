import redis 
# connect with redis server as Alice
alice_r = redis.Redis(host='localhost', port=6379, db=0)
# publish new music in the channel epic_music
alice_r.publish('classical_music', 'Alice Music')