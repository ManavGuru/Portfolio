def get_prime_factor(x):
	factors = list()
	divisor = 2
	while(divisor<=x):
		if(x % divisor == 0):
			x = x/ divisor
			factors.append(divisor)
		else:
			divisor = divisor + 1
	return(factors) 

print("Enter a number: ")
input1 = int(input())

print("It's Prime factors are: {0}".format(get_prime_factor(input1)))