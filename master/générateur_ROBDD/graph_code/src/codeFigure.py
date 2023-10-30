from __future__ import division
import matplotlib.pyplot as plt
import numpy as np
import __future__
from os import listdir
from os.path import isfile


def nombre(s):
    low=189898
    up=0
    for i in range(len(s)):
        tmp=s[i]
        c=tmp.split(';')
        c0=c[0]
        c1=c[1]
        if int(c0)<low or int(c1)<low:
            if int(c0)<low: 
                low=int(c0)
            else:
                low=int(c1)
        if int(c1)>up or int(c0)>up:
            if int(c1)>up:
                up=int(c1)
            else: 
                up=int(c0)
    return low,up


separateurx=[0.5,1,1,2,5,5,10,20,20,50]
separateury=[0.2,2,20,0.5,0.5,1,2,0.5,0.5,0.5]


tab = "graph_code/src/.result/"

a=listdir(tab)
k=0
for file in listdir(tab):
    power=False
    x=[]
    y=[]
    f = open(tab+"/"+str(file),"r")
    text=f.read()
    s=text.split("#")

    low,upper=nombre(s)

    
    for i in range(len(s)):
        tmp=s[i]
        c=tmp.split(';')
        if power:
            x.append(int(c[0]))
            y.append(int(c[1]))  
           
        else:
            x.append(int(c[0]))
            y.append(int(c[1]))
            
    plt.plot(x,y,"o", linestyle="--")  
    tmp = 0
    if upper > 20:
        tmp = upper // 10
    else:
        tmp = 1
    
    plt.xticks(np.arange(min(x),max(x)+1,2))
    plt.yticks(np.arange(min(y),max(y)+1,tmp))
    plt.xlabel("ROBDD node count for "+str(file)+ " variables")
    plt.ylabel("Number of boolean functions")
    
    k+=1
    power=False
    plt.show()
    
    f.close()
