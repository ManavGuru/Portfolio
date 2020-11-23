import pandas as pd # lib for managing data frames 
import numpy as np # linear algebra 
import csv #lib to read csv file 
import matplotlib.pyplot as plt #plotting lin

data_df = pd.read_csv("test_file.csv")
data_array = np.array(data_df)
lpf_values=[]
analog_values = []

for i in range(len(data_array)):
	temp  = (data_array[i][1].split('=')) # ('analog value','0.30')
	temp2 =(data_array[i][0].split('='))
	analog_values.append(float(temp2[1]))
	lpf_values.append(float(temp[1]))

plt.plot(analog_values, label = "Analog Values")
plt.plot(lpf_values, label="lpf_values")
plt.ylabel('Analog Values')
plt.legend()
plt.show()