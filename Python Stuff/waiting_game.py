import time 
import random

target = random.randint(2,6) 
print("\nYour target time is {} seconds".format(target))
input("-----------------------------Press a key to start----------------------------------")
tic = time.perf_counter()

input('\n-------------------------Press a key after {} seconds-------------------------------'.format(target))
elapsed = time.perf_counter()-tic

if elapsed == target: 
	print("\nYou WIN!")
elif elapsed > target: 
	print("\n{0:.3f} Too slow".format(elapsed-target))
else:
	print("\n{0:.3f} Too fast".format(target-elapsed)) 

