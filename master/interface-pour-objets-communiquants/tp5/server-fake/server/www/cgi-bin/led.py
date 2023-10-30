#!/usr/bin/env python
import cgi, os, time,sys
form = cgi.FieldStorage()
val = form.getvalue('val')

s2fName = '/tmp/s2f_fw'
f2sName = '/tmp/f2s_fw'
lcdName = '/dev/led0_ND'

s2f = open(s2fName,'w+')
f2s = open(f2sName,'r',0)
lcdfd = open(lcdName,'w')

s2f.write("w %s\n" % val)
s2f.flush()
res = '0'
if form.getvalue('val') == "0\n":
  rest = '0'
else:
  res = '1'

lcdfd.write(res_2)
lcdfd.flush()

f2s.close()
s2f.close()
lcdfd.close()

html="""
<head>
  <title>Peri Web Server</title>
  <META HTTP-EQUIV="Refresh" CONTENT="1; URL=/cgi-bin/main.py">
</head>
<body>
LEDS:<br/>
<form method="POST" action="led.py">
  <input name="val" cols="20"></input>
  <input type="submit" value="Entrer">
  set %s
</form>
</body>
""" % (val,)

print html
