# coding=utf-8
import serial, requests, json
import time
ser = serial.Serial('COM3', 9600)  ## 9600¬O§Aªºbaud rate

account = ""
password = ""
newpassword = ""
token = ""

while 1:
	
	string = ser.readline().strip()
	print "str: " + string
	if string == "Stage:0":
		account = ""
		password = ""
		newpassword = ""
		token = ""
		continue
	elif string == "Stage:2":
		string = ser.readline().strip()
		account = string
		print "account: " + account
	elif string == "Stage:3":
		while 1:
			string = ser.readline().strip()
			if string == "D":
				print "pass: " + password
				break
			password += string
	elif string == "Stage:4":
		while 1:
			string = ser.readline().strip()
			if string == "D":
				print account, password, token
				print "token: " + token
				r = requests.post("http://oberonfrog.org:5000/login", data = {'username': account, 'password': password, 'token': token})
				req = json.loads(r.content)
				ser.write(str(req['res']))
				break
			token += string
	elif string == "Stage:5":
		while 1:
			string = ser.readline().strip()
			if string == "D":
				print "new pass: " + newpassword
				break
			newpassword += string
			
	elif string == "Stage:6":
		while 1:
			string = ser.readline().strip()
			if string == "D":
				print account, password, token, newpassword
				print "token: " + token
				r = requests.post("http://oberonfrog.org:5000/update", data = {'username': account, 'password': password, 'token': token, 'new_password': newpassword})
				req = json.loads(r.content)
				ser.write(str(req['res']))
				break
			token += string
	elif string == "Stage:9":
		ser.write(time.strftime("%Y-%m-%d %H:%M:%S", time.localtime()) );
	
print "test"