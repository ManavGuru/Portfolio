#Frame Capture
#Author: Manav Gurumoorthy

'''OpenCV code to capture video frames as JPG
In order to capture the frames at a slower speed the target_framerate variable is used. 
The higher this number is the slower the rate of capture is.
eg. if target_framerate is set to 20, one image will be saved for every 20 video frames.
Can also be used without the video pop-up by removing line #30
'''

import cv2

vid = cv2.VideoCapture(0) # opening the camera, 0 is device ID if only 1 camera is connected to sys
path = '/home/ndn1/Manav/Python_code/saved_frames/' 
target_framerate = 20 #frame sampling rate: how often to save one frame. 
count = 0 
file_no = 0
while(True): 
	  
	# Capture the video frame 
	# by frame 
	ret, frame = vid.read() #returns a bool and the frame
	if(ret):
		count += 1
		if (count == target_framerate):
			filename = path + str(file_no)+".jpg" #string manip to create path to save image
			file_no += 1 
			print('Frame Captured at:',filename)  
			cv2.imwrite(filename, frame) # writing to file
			count = 0
			del(filename)
	# Display the resulting frame 
	cv2.imshow('frame', frame)  #comment this to disable video playback
	
	# press 'q' to quit
	if (cv2.waitKey(1) & 0xFF == ord('q')): 
		break
  
# General cleanup 
vid.release() 
# Destroy all the windows 
cv2.destroyAllWindows() 