import paho.mqtt.client as mqtt
import sqlite3
import os
import time

# msg to bdd_mqtt.py
SAVE = "/bdd/save"
GET = "/bdd/get"
INIT = "/bdd/init"

client_list = set() #(name, id)
addr = "localhost"
name = "ioc23"
db_name = "ioc_database.db"
dir_clients = os.getcwd()
dir_bdd = "../db"



#None or the id of the client
#if id is found in the list, then the client had been initialized.
def get_id_from_list(name):
	for (cli_id, cli_name) in client_list:
		if cli_name == name:
			return cli_id
	return None

#get led state from db
def get_led_state(cli_id, db_co):
	db_cur = db_co.cursor()
	res = db_cur.execute("SELECT state FROM led_sample WHERE id_client = ?", (cli_id,))
	row = res.fetchone()
	if row is None:
		return 0
	else:
		return row[0]

#get pb counter from db
def get_pb_counter(cli_id, db_co):
	db_cur = db_co.cursor()
	res = db_cur.execute("SELECT counter FROM pb_sample WHERE id_client = ?", (cli_id,))
	row = res.fetchone()
	if row is None:
		return 0
	else:
		return row[0]

#get last photoresistor sample value from db
def get_last_pres_val(cli_id, db_co):
	db_cur = db_co.cursor()
	res = db_cur.execute("SELECT val FROM pres_sample WHERE id_client = ? \
	ORDER BY id DESC LIMIT 1", (cli_id,))
	row = res.fetchone()
	if row is None:
		return 0
	else:
		return row[0]

#get every photoresistor samples from db
def get_pres_samples(cli_id, db_co):
	db_cur = db_co.cursor()
	res = db_cur.execute("SELECT * FROM pres_sample WHERE id_client = ? \
	ORDER BY id ASC", (cli_id,))
	return res # None or list of tuples

#update new led state
def update_led_state(cli_name, state, db_co):
	db_cur = db_co.cursor()
	cli_id = get_id_from_list(cli_name)
	if cli_id == None:
		return None
	
	db_cur.execute("UPDATE led_sample SET state = ?, date_sample = DATETIME() WHERE id_client = ?",
	(state, cli_id))
	db_co.commit()
	return 0 #default. None if id is not found

#update pb counter
def update_pb_counter(cli_name, counter, db_co):
	db_cur = db_co.cursor()
	cli_id = get_id_from_list(cli_name)
	if cli_id == None:
		return None
	
	db_cur.execute("UPDATE pb_sample SET counter = ?, date_sample = DATETIME() WHERE id_client = ?",
	(counter, cli_id))
	db_co.commit()
	return 0 #default. None if id is not found

#save new photoresistor sample
def save_new_pres_sample(cli_name, val, db_co):
	db_cur = db_co.cursor()
	cli_id = get_id_from_list(cli_name)
	if cli_id == None:
		return None
	
	db_cur.execute("INSERT INTO pres_sample (id_client, val, date_sample) VALUES \
	(?, ?, DATETIME())", (cli_id, val))
	db_co.commit()
	return 0 #default, like previous



#INIT
def perform_init(client, data, msg, db_co):
	cli_name = str(msg.payload.decode("utf-8"))
	if len(cli_name) == 0:
		return

	db_cur = db_co.cursor()
	init_str = ""

	#in database ?
	res = db_cur.execute("SELECT * FROM client WHERE name = ?", (cli_name,))
	row = res.fetchone()
	if row is None:
		print("New client")
		res = db_cur.execute("INSERT INTO client (name) VALUES (?)", (cli_name,))
		db_co.commit() #save change, otherwise they will not be applied
		

		#get numerical id abd updating client_list
		res = db_cur.execute("SELECT * FROM client WHERE name = ?", (cli_name,))
		row = res.fetchone()
		if row is None:
			print("error in init - new client")
			return
		client_list.add(row)
		init_str = "led:0#counter:0#pres:0#screen:HelloWorld!"
		
		#creating the two last entries in db
		cli_id,_ = row
		db_cur.execute("INSERT INTO led_sample (id_client, state, date_sample) VALUES (?, 0, DATETIME())", (cli_id,))
		db_co.commit()
		db_cur.execute("INSERT INTO pb_sample (id_client, counter, date_sample) VALUES (?, 0, DATETIME())", (cli_id,))
		db_co.commit()

	else:
		client_list.add(row) # updating client_list
		cli_id,_ = row

		#get led state from db
		init_str = "led:" + str(get_led_state(cli_id, db_co)) + "#"
			
		#get pb counter from db
		init_str += "counter:" + str(get_pb_counter(cli_id, db_co)) + "#"
		
		#get photoresistor last sample (last val for cli_id)
		init_str += "pres:" + str(get_last_pres_val(cli_id, db_co)) + "#"
		
		#initial screen msg
		init_str += "screen:HelloWorld!"

	client.publish("/esp32/" + cli_name + "/setup", init_str)
	print("INIT:")
	print(client_list)
	print(init_str)

#SAVE
def perform_save(client, data, msg, db_co):
	msg = str(msg.payload.decode("utf-8"))
	if len(msg) == 0:
		return
	
	#begin by name
	#type:value
	l = msg.split('#')
	if len(l) < 2:
		return

	cli_name = l[0]
	l = l[1:]

	for cell in l:
		e = cell.split(':')
		if len(e) != 2:
			continue

		if e[0] == "led":
			update_led_state(cli_name, int(e[1]), db_co)
		elif e[0] == "counter":
			update_pb_counter(cli_name, int(e[1]), db_co)
		elif e[0] == "pres":
			save_new_pres_sample(cli_name, int(e[1]), db_co)
	#envoyer message au server: nouvelles données

#GET
def perform_get(client, data, msg, db_co):
	cli_name = str(msg.payload.decode("utf-8"))
	if len(cli_name) == 0:
		return
	
	cli_id = get_id_from_list(cli_name)
	if cli_id is None:
		client.publish("/serv/summary", "NotFound")
		return
	
	#information and number of photoresistor samples
	str_get =  "led:" + str(get_led_state(cli_id, db_co)) + "#"
	str_get += "counter:" + str(get_pb_counter(cli_id, db_co)) + "#"
	ll = get_pres_samples(cli_id, db_co)
	l = list()
	l_len = 0 #pas d'operation len() sur les listes de la bdd
	for e in ll:
		l_len += 1
		l += [e] #oblige de reconstruire une liste car c'est un object cursor

	str_get += "pres:" + str(l_len)
	client.publish("/serv/summary", str_get)
	for (i,_,v,d) in l:
		str_get = "id:" + str(i) + "#val:" + str(v) + "#date:" + str(d)
		client.publish("/serv/pres_data_set", str_get)



#listener
def listen_msg(client, data, msg):	
	#init db connection
	db_co = sqlite3.connect(db_name)

	topic = msg.topic

	#switch like
	if topic == SAVE:
		perform_save(client, data, msg, db_co)

	elif topic == GET:
		perform_get(client, data, msg, db_co)

	elif topic == INIT:
		perform_init(client, data, msg, db_co)

	else:
		printf("Unknown topic")

	#close and goto original directory
	db_co.close()



# get addr from config
try:
	f = open("addr_config.txt", "r")
	addr = f.readline()
	addr = addr.strip("\0\n\r ")

finally:
	f.close()


os.chdir(dir_bdd)

# init mqtt
client = mqtt.Client(name)
client.on_message = listen_msg
client.connect(addr)
client.loop_start()
client.subscribe("/bdd/#")

while True:
	is_end = input().strip("\n\r")
	if is_end == "exit":
		break

client.loop_stop()
os.chdir(dir_clients)
