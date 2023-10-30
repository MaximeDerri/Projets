import sqlite3
from   app_mqtt import *
from   flask import Flask
from   flask import request, jsonify
from   views import views

# App init
app = Flask(__name__)
app.register_blueprint(views, url_prefix="/")

#appelée par un post sur /setScreen. Envoie un message mqtt
@app.route('/setScreen', methods=['POST'])
def setScreen () :
    args = request.args
    data = request.get_json()

    id = args.get('id')

    if(id == "1") :
        client.publish("/esp32/esp32_1/screen", data['new_lcd_val'])
        print ("message LCD\""+data['new_lcd_val']+"\" envoyé à esp32_1" )
    elif(id == "2") :
        client.publish("/esp32/esp32_2/screen", data['new_lcd_val'])
        print ("message LCD\""+data['new_lcd_val']+"\" envoyé à esp32_2" )

    return "Données écran reçues", 200

#appelée par un post sur /setLed. Envoie un message mqtt
@app.route('/setLed', methods=['POST'])
def setLed () :
    args = request.args
    data = request.get_json()

    id = args.get('id')

    if(id == "1") :
        client.publish("/esp32/esp32_1/led", data['new_led_val'])

    elif(id == "2") :
        client.publish("/esp32/esp32_2/led", data['new_led_val'])

    return "Commande LED reçue", 200

@app.route('/getState', methods=['GET'])
def getState () :
    return jsonify(ESP)

#récupère la valeur du compteur du bp pour l'esp ID
@app.route('/getBp', methods=['GET'])
def getCount () :
    
    args = request.args
    id = args.get('id')

    conn = sqlite3.connect('../db/ioc_database.db')
    cursor = conn.cursor()

    cursor.execute("SELECT counter FROM pb_sample WHERE id_client = ? ORDER BY id DESC LIMIT 1;", (id,))
    res = cursor.fetchone()

    cursor.close()
    conn.close()

    return jsonify(bp_count=(str(res[0])))

#récupère la valeur de la photoresistance pour l'esp ID
@app.route('/getPres', methods=['GET'])
def getPres () :
    
    args = request.args
    id = args.get('id')

    conn = sqlite3.connect('../db/ioc_database.db')
    cursor = conn.cursor()

    cursor.execute("SELECT val FROM pres_sample WHERE id_client = ? ORDER BY id DESC LIMIT 1;", (id,))
    res = cursor.fetchone()

    cursor.close()
    conn.close()

    return jsonify(pres_val=(str(res[0])))

#récupère la valeur de led pour l'esp ID
@app.route('/getLed', methods=['GET'])
def getLed () -> str:
    id = request.args.get('id')

    conn = sqlite3.connect('../db/ioc_database.db')
    cursor = conn.cursor()

    cursor.execute("SELECT state FROM led_sample WHERE id_client = ? ORDER BY id DESC LIMIT 1;", (id,))
    res = cursor.fetchone()

    cursor.close()
    conn.close()

    return jsonify(value=(str(res[0])))

if __name__ == '__main__':
    app.run(debug = True)
