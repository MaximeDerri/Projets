import paho.mqtt.client as mqtt

ESP = [False, False]  #liste gardant l'état des ESPs

# Callbacks
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connexion établie avec succès au broker MQTT")
        client.subscribe("/server/cnct")

    else:
        print("La connexion au broker MQTT a échoué")

def on_message(client, userdata, msg):
    print("Message reçu sur le sujet : " + msg.topic + " - Contenu : " + str(msg.payload.decode("UTF-8")))

    if(msg.topic == "/server/cnct" and msg.payload.decode("UTF-8") == "esp32_1") : 
        ESP[0] = True
    elif(msg.topic == "/server/cnct" and msg.payload.decode("UTF-8") == "esp32_2") :
        ESP[1] = True


# Binding des callbacks
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

# Adressse et port du broker
broker_address = "localhost"
try:
	f = open("addr_config.txt", "r")
	broker_address = f.readline()
	broker_address = broker_address.strip("\0\n\r ")
finally:
	f.close()
broker_port = 1883

# Connection
client.connect(broker_address, broker_port)
client.loop_start()

# Annonce
client.publish("all", "server: mqtt client online")

