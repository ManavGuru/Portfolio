import sched 
import time 

def set_alarm(alarm_time): 
	sound = ("\a \a \a \a \a \a")
	s = sched.scheduler(time.time, time.sleep)
	s.enterabs(alarm_time, 1, print, argument =(sound,))
	s.run()