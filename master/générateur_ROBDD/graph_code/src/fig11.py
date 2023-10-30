#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from os import listdir
from os.path import isfile



tab = "graph_code/src/.result/"
a=listdir(tab)

for file in listdir(tab):
    x=[]
    y=[]
    z=[]
    a=[]
    b=[]
    f = open(tab+"/"+str(file),"r")
    text=f.read()
    s=text.split("#")
    
    for i in range(len(s)):
        tmp=s[i]
        c=tmp.split(';') 
        x.append(int(c[0]))
        y.append(int(c[1]))
        z.append(int(c[2]))
        a.append(c[3])
        b.append(c[4])
  
# A dictionary which represents data
data_dict = {'No.Variables(n)':x,'No.samples':y,'No.Unique Sizes':z,'Compute time':a,'Seconds per ROBDD' :b}

df = pd.DataFrame(data_dict)
df.to_csv("graph_code/src/.result/"+str(file)+".csv", index=False)
  
# show the dataframe
# by default head() show 
# first five rows from top
df.head()