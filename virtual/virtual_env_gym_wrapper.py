# make a better interface with virtual system, use 'open_ai-gym' style
import numpy as np
import matplotlib.pyplot as plt
import gym
from gym import spaces 
import redis
import time
#-------------------------------------------------------------------#
try:
    import calcDQS
except:
    from virtual import calcDQS

######################################################################################
# Temporary: Constants

# time step between decisions in seconds
TIME_STEP = 10
# maximum QoE obtainable
MAX_QOE = 5

######################################################################################

top_limit = 2
n_queues = 2
n_clients = 6
action_cardinality = n_queues**n_clients
actions = [
    [(i//(n_queues**c))%n_queues for c in range(n_clients)]
    for i in range(action_cardinality) 
    if sum([(i//(n_queues**c))%n_queues for c in range(n_clients)]) == top_limit
]

print(actions)

#######################################################################################

class VideoStreamEnv(gym.Env):
    

    def __init__(self):
        '''
        set up video stream environment
        ----------------------------------------------------------------
        Parameters
        queues:      list of Queue objects
        clients:     list of Client objects
        time_step:   time to pass in each step in seconds
        fair_reward: whether queues should use fair sum-of-log (True) or straight sum (False)
        ----------------------------------------------------------------
        Returns
        self:        for chaining
        '''
        
        self.action_space =  spaces.Discrete(15)  #no_clients c 2
       
        observations_max = np.array([np.finfo(np.float32).max, np.finfo(np.float32).max, 5,
            np.finfo(np.float32).max, np.finfo(np.float32).max, 5,
            np.finfo(np.float32).max, np.finfo(np.float32).max, 5,
            np.finfo(np.float32).max, np.finfo(np.float32).max, 5,
            np.finfo(np.float32).max, np.finfo(np.float32).max, 5,
            np.finfo(np.float32).max, np.finfo(np.float32).max, 5,]).reshape(1,6,3)
        observations_min = np.array([0,0,0,
            0,0,0,
            0,0,0,
            0,0,0,
            0,0,0,
            0,0,0]).reshape(1,6,3)

        self.observation_space = spaces.Box(observations_min, observations_max, shape=(1,6,3))
      
        self.temp = 0
        self.step_count = 0
        self.temp = 0
        self.gamma = 0.9999
        self.done = False
        self.episode_length = 1000
        self.episode_count = 0
    
    def step(self, action):
        '''
        Execute a time step with the given action
        ----------------------------------------------------------------
        Parameters
        action:           assignment of clients to queues.
                          len() should be same as clients
                          entries should be integer index of queue
        ----------------------------------------------------------------
        Returns
        ob, reward, episode_over, info : tuple
            ob:           array of state information
            reward:       QoE, to be increased
            episode_over: always false
            info:         none, doesn't matter
        '''
        self.step_count = self.step_count+1
        if(self.step_count==int(self.episode_length)):
            self.done = True
        else:
            self.done = False
        action = actions[action] #action = [1,1,0,0,0,0]

        states = [[0,0,0] for _ in range(n_clients)]
        reward = 1
        for i, q in enumerate(self.queues):
            c_set = np.where(np.array(action) == i)[0]
            s, r, _, _ = q.step(self.time_step, [self.clients[c] for c in c_set])
            for j, c in enumerate(c_set):
                states[c] = s[j]
            reward += r
        
        self.temp = reward / 6
        self.state = states
        return np.array(states).reshape(-1,6,3), ((reward/len(self.clients))), self.done, {}   

    def reset(self):
        self.step_count = 0
        for c in self.clients:
            c.reset()
        self.state = [c.state for c in self.clients]
        return np.array(self.state).reshape(-1,6,3)

    def render(self, mode='human', close=False):
        raise NotImplementedError
