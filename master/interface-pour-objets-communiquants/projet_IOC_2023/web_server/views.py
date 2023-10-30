from flask    import Blueprint, render_template, redirect, url_for, request
from app_mqtt import ESP

views = Blueprint(__name__,"views")

@views.route("/esp/", methods=['GET'])
def esp():
    args = request.args
    esp_id = args.get('esp_id')

    if(ESP[int(esp_id) - 1]) :
        return render_template("esp_data.html", espId=esp_id, led_val="n/a", bp_val="n/a", pres_val="n/a")
    else :
        return "ESP non connecté"

@views.route("/", methods=['GET'])
def goIndex ():
    return redirect(url_for("views.home"))

@views.route("/index", methods=['GET'])
def home():
    return render_template("index.html", esp1_state=str(ESP[0]), esp2_state=str(ESP[1]))