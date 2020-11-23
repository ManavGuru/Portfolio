import os 
import smtplib
import codecs
from email.mime.text import MIMEText
from email.mime.image import MIMEImage
from email.mime.multipart import MIMEMultipart


SENDER_EMAIL = 'automaticmailer6@gmail.com'
SENDER_PASSWORD = '#2134@TAMU'
# file = codecs.open ('c_h_template.html', 'r', 'utf-8')

body =  '<html><body>'  
body += '<h1><center><font size= "6" color= #FF9E33><b>	Your Daily dose of C & H  </b></font></center></h1>'
body += '<p><center><img src="cid:0"></center></p>'
body += '</body></html>'

def send_email (receiver_id, subject): 
	img_data = open("./Images/8", 'rb').read()
    
	msg = MIMEMultipart()
	msg['Subject'] = 'Daily dose of Calvin and Hobbes'
	msg['From'] = 'automaticmailer6@gmail.com'
	text = MIMEText(body,'html','utf-8')
	msg.attach(text)
	# msg.attach(MIMEText('<html><body><h1>Hello</h1>' +'<p><img src="cid:0"></p>' +'</body></html>', 'html', 'utf-8'))
	image = MIMEImage(img_data, name=os.path.basename("CH_reading_2.0.jpg"))
	image.add_header('Content-ID', '<0>')
	msg.attach(image)
	
	with smtplib.SMTP_SSL('smtp.gmail.com', 465) as server: 
		server.ehlo()
		server.ehlo()
		server.login(SENDER_EMAIL, SENDER_PASSWORD)
		server.sendmail(SENDER_EMAIL, receiver_id, msg.as_string())
   

