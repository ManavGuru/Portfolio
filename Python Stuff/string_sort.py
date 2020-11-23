import re 

def sort_string(phrase): 
	words = phrase.split(" ")
	words.sort(key = lambda s: s.casefold())
	return words


input_1 = input("Enter a phrase: ")
print(sort_string(input_1))
