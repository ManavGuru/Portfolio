import re 

def is_palindrome(phrase): 
	forwards = ''.join(re.findall('[a-z]+',phrase.lower()))
	backwards = forwards[::-1]
	return forwards == backwards 

input_1 = input("Enter a phrase: ")
print(is_palindrome(input_1))