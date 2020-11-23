top_limit = 6
n_queues = 2
n_clients = 6
action_cardinality = n_queues**n_clients
action_cardinality = n_queues**n_clients
actions = [
    [(i//(n_queues**c))%n_queues for c in range(n_clients)]
    for i in range(action_cardinality) 
    if sum([(i//(n_queues**c))%n_queues for c in range(n_clients)]) == top_limit
]

print(actions)
