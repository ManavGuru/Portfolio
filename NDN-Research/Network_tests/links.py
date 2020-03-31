import webbrowser 

web_a = "https://www.youtube.com/watch?v=FV2tMP37ygs" 
count = 1
while 1:
	key = raw_input()
	if(key == 'n') :
		print 'Opening a new tab','\nTab no:',count
		webbrowser.open_new_tab(web_a)
		count += 1
