class calculate_QoE(object): 
    def __init__(self,current_buff_state, previous_buff_state=None, initial=True):
        #buffer state x(t)
        self.current_buff_state= current_buff_state
        #previous buffer state x(t-1)
        self.previous_buff_state= previous_buff_state
        #flag to mark the first step/ initial buffering
        self.initial= initial
        self.reward = 0
        self.lagrange_multiplier = 0
    def calculate_reward(self, current_buff_state, which_queue):
        #constant cost 
        # self.lagrange_multiplier= lagrange_multiplier
        c1 = 2
        reward = 0
        if(current_buff_state==0 and self.initial == True and self.previous_buff_state == None):
            self.reward += (-(1)- self.lagrange_multiplier*(which_queue))
            self.initial = False
            self.previous_buff_state = 0
        elif(current_buff_state==0 and self.previous_buff_state != 0 ): 
            self.reward += -(1+c1)-self.lagrange_multiplier*(which_queue)
            self.previous_buff_state = current_buff_state
        elif(current_buff_state !=0 and self.previous_buff_state == 0): 
            self.reward += 0
            self.previous_buff_state = current_buff_state

        return self.reward, self.current_buff_state, self.initial

    def update_lagrange_multiplier(self, new_multiplier):
        self.lagrange_multiplier += (0.01)*new_multiplier


# lambda 