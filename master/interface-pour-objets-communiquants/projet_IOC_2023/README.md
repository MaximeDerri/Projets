# Master 1: PROJET IOC

# ETUDIANTS:
- Derri Maxime
- Normand François

## Objectif du projet et technologies utilisées

L'objectif du projet d'IOC est de réaliser un ensemble logiciel et matériel permettant de récupérer des informations sur l'environnement
par des capteurs, de les stocker et de les redistribuer à des clients web.

Le matériel à prendre en compte est le suivant:
>- Un / des ESP32.
>- Un Raspberry Pi.
>- Un ou des PCs.

Les ESP32 sont chargés de récupérer les informations sur l'environnement, le Raspberry Pi de stocker ces informations et de les partager à des clients web.

Pour la partie logiciel, nous utilisons:
>- Arduino pour programmer les ESP32.
>- SQLITE 3 pour le stockage des données.
>- Python3 pour gérer la base de données et pour la partie web.
>- Javascript / HTML utilisés par Python pour générer les pages web.
>- Un Broker MQTT "Mosquitto".
>- MQTT Paho pour les clients en python et des sources d'Adafruit pour les clients MQTT avec Arduino.

Les différentes composantes du projet communiqueront sur un réseau local afin de:
>- faire des requêtes au site web (client <-> serveur).
>- se brancher à un Broker MQTT disposé sur le Raspberry Pi pour faire circuler les données entre les ESP32, la base de données et le site web en implémentant
un client MQTT.


## Détail de l'architecture
Dans un premier temps, nous allons nous intéresser à la partie centrale du projet, c'est à dire les différentes composantes autour du Broker MQTT.
Enfin, nous parlerons de la partie web et les échanges entre les clients et le site web.

# Broker MQTT et les clients

## Broker MQTT
Le rôle du Broker MQTT est d'orienter les requêtes entre les différents clients MQTT.
Les messages sont orientés vers des "topic" dont les clients sont abonnés. L'idée est de remplacer la partie applicative des requêtes:
on n'utilise pas d'opcode mais les topics.
Les ESP32, le script de la base de données et le site web possèdent un client MQTT afin de faire circuler des données et des commandes entre eux.

## ESP32
Les ESP32 s'abonnent à leurs propres topic dans le but de les différencier, dont les noms sont:
>- /esp32/nom_de_esp32/led	=> Demande de mise à jour de l'état de la led (client web -> serveur web -> broker -> esp32).
>- /esp32/nom_de_esp32/screen 	=> De même, mais pour changer le message sur l'écran de l'ESP32.
>- /esp32/nom_de_esp32/setup 	=> Message reçu de la base de données. quand un ESP32 s'allume, il envoie une requête à la base de données pour
>				   Récupérer le dernier état à jour. Sinon, initialisé par défaut (par la base de données, et nouvelle entrée créer dans les tables).

Les ESP32 n'envoient des messages qu'à la base de données lors de requête pour enregistrer les données perçus des capteurs.

Les noms des ESP32 sont disposés dans le code, en dur, dans une macro. *Il est nécessaire de le changer et de recompiler le code pour chaque ESP32
(ou créer plusieurs fois le même fichier avec un nom différent...)*.

Le code des ESP32 est ordonné comme en TP, c'est à dire en forme d'automate.
Comme expliqué plus haut, il est possible de manipuler la led et l'écran au travers de requêtes depuis un client web.
En plus de cela, l'esp dispose:
>- D'un bouton poussoir blanc qui permet également de couper / allumer la led. Le code correspondant est une ISR déclanchée à l'activation du bouton.
>- D'un autre bouton poussoir qui sert de compteur, à chaque appuis le code augmente de 1 son compteur. Le gestionnaire est également une ISR.

Le but recherché ici est d'implémenter et manipuler des ISR.

>- D'une photorésistance, dont les données ne sont pas affichées sur l'écran comme celui-ci est déjà utilisé. Cependant, elles sont sauvegardées dans la base de donnée.

Les données manipulés par les esp sont envoyées à interval régulier à la base de donnée, dont une description du schéma et de sa gestion par un script est décrite juste en dessous. L'interval de temps correspondant peut être configuré.

L'implémentation de MQTT est assez simple, et comme avec python nous devons proposer à un objet MQTT une fonction de callback lors de la réception d'un message.
Il faut cependant surveiller régulièrement l'état de la connection et au besoin la relancer (fait dans loop).

Le traitement des messages du callback est géré par un switch, qui vérifie le topic et déclanche une nouvelle fonction de traitement correspondant. Cela permet entre autre
lisibilitée et extensibilité. Comme les topics sont utilisé pour un context précis, on est censé recevoir des messages de la même forme ce qui simplifie en partie le traitement
du contenu (on utilise des marqueurs pour séparer des champs et on a implémenté une fonction qui ressemble à un split de python mais orienté pour notre code).

## Base de donnée

### schéma de la base
Pour le stockage des données, nous avons opté pour une base de données, SQLITE 3.
Son avantage est sa petite taille, très utile pour des applications embarquées.
En contre partie, elle n'implémente pas toutes les opérations possibles d'autres bases de données plus complexe tel que PostgreSQL ou MySQL. De plus, il n'y a pas de serveur
pour SQLITE, la base de données est contenue dans un seul et unique fichier qui peut être ouvert dans un terminal ou par un script (ici, par python qui implémente de base des moyens de manipuler SQLITE 3 ).

Le schéma de notre base de données à été réalisé pour être facilement extensible.
Il est simple d'étendre la base à de nouveaux capteur en créant simplement une table. La seul contrainte est qu'il faut que la table contienne une clef étrangère pointant vers
un client (par son id - entier).

Nous implémentons une table qui identifie les clients MQTT par leurs nom, et par un entier en tant que clef primaire, incrémentée automatiquement pour servir d'id unique.

Ensuite, pour chaque capteurs nous avons implémenté une table qui peut contenir un identifiant de l'échantillon, l'état enregistré d'un composant, la date de sauvegarde (fonction sql DATETIME()) ainsi qu'une clef étrangère pointant vers l'id du client correspondant.

Actuellement, les entrées pour l'état de la led et le compteur d'appuis du bouton poussoir ne sont conçus pour contenir qu'une seul entrée par client, en ajoutant une
contrainte à la table où l'id du client référencé doit être obligatoirement indiqué et doit être unique. Dans le cas contraire, la requête est refusée par la base de données.
Nous avons fait ce choix car nous trouvons qu'il n'est pas forcement utile de stocker plusieurs entrées différentes pour le contexte de ces valeurs.
Il est cependant possible de changer cela en modifiant la clef étrangère.

L'autre entrée est celle de la photorésistance, qui contient ici plusieurs entrées différentes par client. Chaque entrée est identifiée par un entier unique, afin d'être
manipulée par un trigger.

Nous avons également fait en sorte que la base de données reste cohérente dans les données insérées, en utilisant des contraintes avec CHECK et en surveillant les suppressions dans les tables pour garder la cohérence avec ON DELETE CASCADE sur l'id d'un client (pas par son nom qui pourrait être modifié, mais par l'id censé être unique).

Dans un soucis d'espace de stockage du Raspberry Pi, nous avons décidé d'implémenter un trigger sur les insertions dans la table de la photorésistance.
Pour chaque id client référencé par l'insertion dans la table, on va compter le nombre d'entrées lui étant associées. Si le nombre est supérieur à X, alors on
supprime la plus ancienne (qu'on repère avec les id des entrées de la table).
Ce X est configurable dans le schéma de la base de données, au niveau du "WHEN X < (...)" du trigger.

L'implémentation d'un trigger dans notre context peut aider à gérer l'espace de stockage mais peut dans certains cas poser des problèmes:
>- Si les insertions sont très peu espacés et nombreuses, la capacité de traitment de la base peut saturer car c'est SQLITE 3 qui est utilisé.
>- Plus X est grand, plus le temps de travail tu trigger augmente.

L'implémentation du trigger est surtout une piste d'extension de la base, qui fonctionne dans notre cas. Mais si cela pose problème, il suffit de retirer le trigger du code
et de le remplacer par une autre solution.

Nous fournissons avec notre schéma sql un script de test insérant des données s'il est nécessaire de faire des tests sur la structure de la base.

### script de la base de donnée
Pour contrôler notre base, nous passons par un script écris en python.

Ce script est également extensible simplement pour pouvoir continuer à coller au schéma de la base: il faut ajouter de nouveaux cas de "switch" (en fait des if, car les switch façon python3 ne sont apparus que dans les versions très récentes), et les brancher aux nouvelles fonctions.

Ce dernier est abonné aux topics suivants:
>- /bdd/save 	=> requete de sauvegarde d'un esp.
>- /bdd/get 	=> requete de récupération du client MQTT du serveur web.
>- /bdd/init 	=> requete d'initialisation d'un esp lors de son allumage.

Le script garde à jour une liste des esp connectés et de leur identifiants dans la base afin d'éviter constament d'aller chercher l'id d'un client dans la base.
La liste est mise à jour lors d'un traitement du topic /bdd/init: si le client existe, alors on va chercher les derniers échantillons et on lui envoie pour setup ses valeurs.
Sinon, on créer une nouvelle entrée dans la table client et on envoie à l'esp des valeurs par défaut afin qu'il puisse commencer à travailler.
A la fin du traitement, la liste est mise à jour afin de garder une trace des clients qui communique avec le script.

L'idée derrière est double:
>- Un esp qui n'est pas recensé par la liste et qui fait une requete visant la base de données verra sa requete non traitée.
>- Un esp pourrait être déplacé à un autre endroit pour surveiller son environnement avec un nouveau programme et un nouveau nom. Puis par la suite, revenir à son ancien
emplacement en récupérant l'ancien nom, et reprendre l'échantillonage où il s'était arrêté.

Le script python va s'exécuter dans un terminal, et traitera donc les requêtes par sa fonction de callback des messages reçus.
Pour terminer le programme, on peut écrire "exit" dans le terminal et le script quittera car le thread main attend dans un while une entrée du clavier et l'examine.

# Site et serveur web
Pour utiliser le server web, dans l'ordre :
- Lancer le client MQTT de la BD "python3 bdd_mqtt.py" depuis le répertoire "mqtt_clients"
- Lancer le server web "python3 app.py" depuis le répertoire web_server => ça lance aussi le client MQTT du server
- Allumer un ESP32 => à la fin de son setup, il envoie un message pour prévenir le serveur web qu'il est allumé et connecté

## Client MQTT du serveur web
Le serveur va disposer d'un client MQTT. Il va cependant recevoir que très peu de messages: un par connection d'ESP pour autoriser l'accès a sa page dédiée. Le client MQTT du serveur est principalement utilisé pour envoyer des messages vers les ESP32 pour changer l'état de leur LED et écrire sur leur écran. L'actualisation des données présentées par le site web se fera elle en liaison directe avec la BD dans un soucis de temps de réponse.

## Serveur web
Pour implémenter notre serveur web, nous nous servons du microframework Python `Flask`. `Flask` gére les demandes de connexions envoyées par les clients de manière transparente et facilite la génération des réponses HTML aux requêtes HTTP faites par le client par le biai de son moteur de template.
`Flask` fonctionne en associant une fonction Python à une ressource du serveur web. Par exemple, le code suivant défini la racine '/' du site web comme étant une page HTML affichant "Hello World !".

```python
from flask import Flask

app = Flask(__name__)

@app.route('/', methodes=['GET'])
def index():
    return "Hello world !"

if __name__ == "__main__":
    app.run()
```
Dans cet exemple, la fonction `index()` est appelée à chaque requete pour la ressource '/'.

## Ressources du serveur web
Notre serveur web dispose des ressources suivantes:
- "/index", page web d'accueil qui présente la liste des ESPs avec leurs état (connecté ou non) et le lien vers leur page dédiée.
- "/esp/", page web affichant les valeurs des capteurs et actionneurs des ESPs.
- "/getLed", ressource acessible en GET pour aquérir l'etat initiale de la led de l'esp.
- "/getBp", ressource accessible en GET renvoyant la valeur du compteur d'un ESP.
- "/getPres", ressource accessible en GET renvoyant la valeur de la photoresistance d'un ESP.
- "/setScreen", ressource accessible en POST permettant d'écrire sur l'écran d'un ESP.
- "/setLed", ressource accessible en POST permettant de forcer un état à la LED d'un ESP.

## génération des pages web
Les pages web que le serveur produit sont générées par le moteur de template de `Flask`. 
Ce moteur permet de générer des pages HTML calquées sur un modèle pouvant pendre des paramètres issues des reqêtes HTTP. Le code suivant va afficher un message de bienvenue à un client en utilisant le paramètre de requête 'nom':
- On créé notre template dans le dossier ./templates:
```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Arguments</title>
</head>
<body>
    <h1>Bonjour, {{nom}}.</h1>
</body>
</html>
```
- On défini une fonction python qui sera appelée sur la route '/arguments'
```python
@app.route('/arguments', methodes=['GET'])
def arguments () :
    args = request.args
    nom = args.get('nom')
    return render_template("exemple_argument_requete.html", nom=nom)
```
=> La requête GET sur '/arguments?nom=Francois' va générer une page HTML adaptée affichant le message "Bonjour, Francois."
On va s'en servir pour utiliser le modèle de la page des ESPs pour les deux ESPs.

## Mise à jour dynamique des valeurs affichées par les pages des ESPs
Les pages des ESPs prennent en parametre de requête l'identifiant de l'ESP dont elles affichent les valeurs. Elle affichent les valeurs de:
- La LED
- Du compteur associé au bouton poussoir
- De la photoresistance

Parmis les valeurs affichées, celles de la photoresistance et du compteur associé au BP sont suceptibles d'évoluer sans action du client.
On veut donc qu'elles s'actualisent sans qu'on ait à recharger manuellement la page. On va pour cela devoir écrire des fonctions javascript qui vont s'executer dans la page HTML. Ces fonctions javascript vont mettre à jour ces valeurs par l'intermédiaire de requêtes `AJAX`. Le code ci dessous permet de mettre à jour dynamiquement la valeur affichée pour la valeur de la photoresistance:
```javascript
<script>
    function refreshPhotoresValue() {
      var espId = document.getElementById("espID").textContent; // On récupère l'ID de l'ESP présent sur la page HTML
      
      fetch('/getPres?id=' + espId) //
          .then(response => response.json())
          .then(data => {
              document.getElementById('photoresValue').innerText = data.pres_val;
          });
    }
setInterval(refreshPhotoresValue, 1000);
</script>
```
Ce script javascript est implémenté directement dans la page HTML générée pour l'un des ESPs. Il va faire une requête AJAX GET à la ressource '/getPres' avec pour argument de requête l'id de l'ESP concerné.  Le fonctionnement de la ressource '/getPres' sera expliqué plus loin. Cela va avoir pour effet de modifier la valeur affichée sur la page HTML. Pour que la valeur soit mise à jour dynamiquement et péridiqument on utilise la fonction `setInterval(updatePhotoresValue, 1000)` qui va appeler `updatePhotoresValue()` toutes les 1000ms.
On implémente également une fonction adaptée pour le compteur du bouton poussoir `refreshBpCount()` qui va accéder à la ressource '/getBp'.

## Les autres ressources accessibles en GET
Pour récupérer les valeurs des capteurs de l'ESP, on interroge directement la BD. Notre BD est implémentée en SQLITE3 qui offre une interface pour python. Voici la fonction python associée à la requete GET sur '/getPres':
```python
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
```

## Les ressources accessibles en POST
Les dernières ressources du site web sont:
- setLed
- setScreen

setLed appellera la fonction `def setLed ()`
```python
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
```

Qui est appelée par la fonction javaScript `toggleLed ()`
```javascript
// Fonction pour inverser la valeur de la LED
		function toggleLed() {
			var espId = document.getElementById("espID").textContent;
			var ledValue = document.getElementById('ledValue');

			data = {
				new_led_val : -1
			};

			if (ledValue.innerHTML === '1') {
				ledValue.innerHTML = '0';
				data.new_led_val = 0;
			} else {
				ledValue.innerHTML = '1';
				data.new_led_val = 1;
			}

			fetch('/setLed?id=' + espId, {
					method: 'POST',
					headers: {
					'Content-Type': 'application/json'
					},
					body: JSON.stringify(data)
				}) 
		}
```
La fonction python `def setLed ()` transmettra a l'ESP concerné un message lui indiquant quelle valeur la led doit prendre à présent.
On raisonne de la même manière pour setScreen.
