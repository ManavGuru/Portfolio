# make a better interface with virtual system, use 'open_ai-gym' style
import numpy as np
import matplotlib.pyplot as plt
import gym
#-------------------------------------------------------------------#
try:
    import calcDQS
except:
    from virtual import calcDQS

######################################################################
# Temporary: Constants

# time step between decisions in seconds
TIME_STEP = 10
# maximum QoE obtainable
MAX_QOE = 5

######################################################################
# contained classes
class Queue(gym.Env):
    
    def __init__(self, bandwidth, fairness=100, fair_reward=False):
        # alotted service rate in bits per second
        self.bandwidth = bandwidth
        # magnitude of dirichlet distribution for distributing bandwidth
        self.fairness = fairness
        # whether reward is aggregated with log10
        self.fair_reward = fair_reward

    def reset(self):
        return self
    
    def step(self, elapsed_time, clients):
        '''
        Simulate a time step, service to clients
        ----------------------------------------------------------------
        Parameters
        elapsed_time: amount of time to simulate
        clients:      iterable of Client objects implementing step(elapsed_time, bandwidth)
        ----------------------------------------------------------------
        Returns
        ob, reward, episode_over, info : tuple
            ob:           array of state information
            reward:       QoE, to be increased
            episode_over: always false
            info:         none, doesn't matter
        '''
        if len(clients) == 0:
            return [], 0, False, None

        dist = np.random.dirichlet(np.full(len(clients), self.fairness))

        state = []
        rewards = []
        for i, c in enumerate(clients):
            s, r, _ , _ =c.step(elapsed_time, self.bandwidth * dist[i])
            state.append(s)
            rewards.append(r)
        
        reward = 0
        if self.fair_reward:
            reward = sum(np.log10(rewards))
        else:
            reward = sum(rewards)

        return state, reward, False, None

class Video(gym.Env):
    def __init__(
        self,
        length,
        bitrate,
        initial_buffer
    ):
        '''
        ----------------------------------------------------------------
        Parameters
        bitrate:            bitrate of video (just one; may be distribution in future)
        initial_buffer:     buffer (in seconds) to acquire before attempt playing
        length:             play length in seconds
        ----------------------------------------------------------------
        Returns
        self:               For chaining
        '''
        # bits-per-second
        self.bitrate = bitrate

        # running time remaining
        self.video_remaining_time = max(0, length)
        # fill in seconds
        self.initial_buffer = initial_buffer
        self.buffer_fill = 0
        self.buffering = True
    
    def step(self, elapsed_time, bandwidth):
        '''
        Execute a time step.
        ----------------------------------------------------------------
        Parameters
        elapsed_time:   length of time step in seconds
        bandwidth:      bandwidth for this time step in bits per second
        ----------------------------------------------------------------
        Returns
        remaining, stalled, play_time : tuple
            remaining:  time not consumed, in seconds
            stalled:    boolean, whether stall was observed
            play_time:  amount of time play occurred
        '''
        # buffering
        if self.buffering:
            if bandwidth == 0:
                return 0, False, 0
            time_to_start = (min(self.initial_buffer, self.video_remaining_time) - self.buffer_fill) * self.bitrate / bandwidth
            if time_to_start >= elapsed_time :
                self.buffer_fill += elapsed_time * bandwidth / self.bitrate
                return 0, False, 0
            else:
                self.buffer_fill = min(self.initial_buffer, self.video_remaining_time)
                self.buffering = False
                return elapsed_time - time_to_start, False, 0

        # stall
        if bandwidth < self.bitrate:
            smooth_run_time = self.buffer_fill/(1-bandwidth/self.bitrate)
            if smooth_run_time < min(self.video_remaining_time, elapsed_time):
                self.video_remaining_time -= smooth_run_time
                self.buffer_fill = 0
                self.buffering = True
                return elapsed_time - smooth_run_time, True, smooth_run_time

        # video ends
        if self.video_remaining_time <= elapsed_time:
            self.buffer_fill = 0
            play_time = self.video_remaining_time
            self.video_remaining_time = 0
            return elapsed_time - self.video_remaining_time, False, play_time

        # otherwise
        self.video_remaining_time -= elapsed_time
        self.buffer_fill = min(max(0, self.buffer_fill + elapsed_time * (bandwidth / self.bitrate - 1)), self.video_remaining_time)
        return 0, False, elapsed_time

class Client(gym.Env):
    
    def __init__(
        self, 
        bitrate_dist,
        length_dist,
        initial_buffer=10,
        volatility=1
    ):
        '''
        ----------------------------------------------------------------
        Parameters
        bitrate_dist:       normal distribution for bit rate. Tuple, (mean, var)
        initial_buffer:     buffer to acquire before attempt at playing a video
        length_dist:        normal distribution for video length. Tuple, (mean, var)
        volatility:         quickness to update qoe. Between 0 and 1
        ----------------------------------------------------------------
        Returns
        self:               For chaining
        '''
        # rate at which bytes are consumed (per second)
        self.bitrate_dist = bitrate_dist 
        # Average length of video to generate
        self.length_dist = length_dist
        # amount to buffer before starting/resuming play
        self.initial_buffer = initial_buffer
        # quickness of qoe update
        self.volatility = volatility
        # start
        self.reset()
        
    def reset(self):
        '''
        Set up client with new video
        ----------------------------------------------------------------
        Parameters
        ----------------------------------------------------------------
        Returns
        self:   For chaining
        '''
        # QoE reset
        self.qoe = MAX_QOE
        # time since start or since last stall, whichever is most recent
        self.smooth_time = 0
        # number of stalls experienced
        self.stall_count = 0
        # create video
        self.video = Video(
            self.length_dist[0] + np.random.randn() * self.length_dist[1],
            self.bitrate_dist[0] + np.random.randn() * self.bitrate_dist[1],
            self.initial_buffer
        )
        # set state
        self.state = [
            self.video.buffer_fill, 
            self.stall_count, 
            # self.smooth_time,
            self.qoe
        ]
        return self.state

    def step(self, elapsed_time, bandwidth):
        '''
        Execute a time step.
        ----------------------------------------------------------------
        Parameters
        elapsed_time:     length of time step in seconds
        bandwidth:        bandwidth for this time step in bits per second
        ----------------------------------------------------------------
        Returns
        ob, reward, episode_over, info : tuple
            ob:           array of state information
            reward:       QoE, to be increased
            episode_over: always false
            info:         none, doesn't matter
        '''
        remaining_time = elapsed_time

        # execute time in videos
        while remaining_time > 0:
            
            
            remaining_time, stall, play_time = self.video.step(remaining_time, bandwidth)

            # stall
            if stall:
                self.stall_count += 1
            
            # update qoe
            qoe_transitions = None
            if self.video.buffering:
                qoe_transitions = calcDQS.interruptDQS(self.qoe, self.stall_count + 1, elapsed_time - play_time)
            else:
                qoe_transitions = calcDQS.playbackDQS(self.qoe, self.stall_count + 1, play_time)
            if len(qoe_transitions) > 0:
                self.qoe = min(5, max(qoe_transitions[-1], 0))

            # reset all video-related functionality
            if self.video.video_remaining_time == 0:
                self.stall_count = 0
                self.qoe = 5
                self.video = Video(
                    self.length_dist[0] + np.random.randn() * self.length_dist[1],
                    self.bitrate_dist[0] + np.random.randn() * self.bitrate_dist[1],
                    self.initial_buffer
                )
                    
        # produce resulting state
        self.state = [
            self.video.buffer_fill, 
            self.stall_count, 
            # self.smooth_time,
            self.qoe
        ]
        return self.state, self.qoe, False, None

######################################################################################