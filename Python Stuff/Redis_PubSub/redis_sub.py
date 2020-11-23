import redis
# connect with redis server as Bob
bob_r = redis.Redis(host='localhost', port=6379, db=0)
bob_p = bob_r.pubsub()
# subscribe to classical music
bob_p.subscribe('classical_music')
# connect with redis server as Alice
alice_r = redis.Redis(host='localhost', port=6379, db=0)
# publish new music in the channel epic_music
alice_r.publish('classical_music', 'Alice Music')
# ignore Bob subscribed message
bob_p.get_message()
# now bob can find alice’s music by simply using get_message()
new_music = bob_p.get_message()['data']
print(new_music)
# Alice published another
alice_r.publish('classical_music', 'Alice 2nd Music')
#Lets grab it!
another_music = bob_p.get_message()['data']
print(another_music)