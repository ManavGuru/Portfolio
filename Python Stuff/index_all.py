
def index_all(search_list, key):
	indices = list()
	for i in range (len(search_list)):
		if(search_list[i]==key):
			indices.append([i])
		elif isinstance(search_list[i],list):
			for index in index_all(search_list[i],item):
				indices.append([i]+index)
	return indices 

input_1 = input("Enter a List: ")
input_2 = input("Enter element to be found: ")
print(index_all(input_1, input_2))