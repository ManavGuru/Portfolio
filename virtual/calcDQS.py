import math

# Sample call:
# FOR STALL; returns output during entire stall region:
# dqs = interruptDQS(Q0, stallCnt, currStLen);
#   Q0 = 5, initial quality, assumed to be max
#   stallCnt = number of stall events up to and including current event
#   currStLen = length of current stall event
# dqs = playbackDQS(Q0, playbackCnt, playbackLen);
#   Q0 = 5
#   playbackCnt = number of playback periods up to and including current
#   playbackLen = total time of playback period


# len within last decision period
def interruptDQS(Q0, stallCnt, currStLen):
    if(stallCnt == 1): # Only initial
        T1 = 1
        T2 = 2
        a = 1.3
        thr = 89
    elif(stallCnt == 2): # Only single.
        T1 = 0
        T2 = 0
        a = 1.6
        thr = 28
    elif(stallCnt): # Multiple.
        T1 = 0
        T2 = 0
        a = 0.3
        thr = 116
    
    # Computing m.
    m = ((Q0 - 1 -a) / (thr - T2))
        
    sol = []
    t = 0
    
    while (t < currStLen):
        
        if t<T1:
            dqs = Q0
            
        elif t<=T2:
            if(T1 == T2):
                dqs = Q0
            else:
                dqs = Q0 - a/2 * (1 + math.cos(float(3.14159*(t-T2)/(T2-T1))))
        else:
            dqs = Q0 - (a + m*(t - T2))

        if dqs < 1:
            sol.append(1)
        else:
            sol.append(dqs)
        
        t = t+1

    return sol


def playbackDQS(Q0, playbackCnt, playbackLen):
    # We have a video belonging to one of the three categories - single, multiple and those with just initial.
    if(playbackCnt == 1): # Only initial
        T1 = 0
        T2 = 3
        a = 1.0
        thr = 140
    elif(playbackCnt == 2): # Only single.
        T1 = 0
        T2 = 2
        a = 1.5
        thr = 390
    else: # Multiple.
        T1 = 0
        T2 = 0
        a = 0.0
        thr = 294
    
    # Computing m.
    m = ((5 - Q0 -a) / (thr - T2))
        
    sol = []
    t = 0   
        
    while (t < playbackLen):
        
        if t<T1:
            dqs = Q0
        elif t<=T2:
             if(T1 == T2):
                dqs = Q0
             else:
                dqs = Q0 + a/2 * (1 + math.cos(float(3.14159*(t-T2)/(T2-T1))))
        else:
            dqs = Q0 + (a + m*(t - T2))

        if dqs > 5:
            sol.append(5)
        else:
            sol.append(dqs)
        
        t = t+1

    return sol
