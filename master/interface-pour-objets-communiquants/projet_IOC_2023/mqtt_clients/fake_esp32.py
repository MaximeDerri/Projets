import time 
import random
import paho.mqtt.client as mqtt

# Faux esp32
name  = "esp32_2"
led   = 0
count = 0
pres  = 0

def fake_save() :
    global name, led, count, pres
    if (random.random() > 0.80) :
        count = count + 1
    pres = random.randint(0, 100)
    res = name + "#led:" + str(led) + "#counter:" + str(count) + "#pres:" + str(pres)
    return res
 
# Callbacks
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connexion établie avec succès au broker MQTT")
        client.subscribe("test_topic")
        client.subscribe("/esp32/esp32_2/screen")
        client.subscribe("/esp32/esp32_2/led")
    else:
        print("La connexion au broker MQTT a échoué")

def on_message(client, userdata, msg):
    print("Message reçu sur le sujet : " + msg.topic + " - Contenu : " + str(msg.payload.decode("UTF-8")))

    if(msg.topic == "/esp32/esp32_2/led") :
        led = int(msg.payload.decode("UTF-8"))
        print("Nouvelle valeur de LED:"+str(led))
    elif(msg.topic == "/esp32/esp32_2/screen") :
        print("Ecran mis à jour")


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

# Annonce de connexion
client.publish("/server/cnct", name)
client.publish("/bdd/init", name)

# Attente de sortie
while True:
    time.sleep(0.2)
    save = fake_save()
    client.publish("/bdd/save", save)


client.loop_stop()
