import smtplib


SENDER_EMAIL = 'automaticmailer6@gmail.com'
SENDER_PASSWORD = '#2134@TAMU'
f = open('test.txt','r')

body = f.read()
print(body)
def send_email (receiver_id, subject): 
	message = 'Subject: {}\n\n{}'.format(subject, body)
	with smtplib.SMTP_SSL('smtp.gmail.com', 465) as server: 
		server.login(SENDER_EMAIL, SENDER_PASSWORD)
		server.sendmail(SENDER_EMAIL, receiver_id, message)