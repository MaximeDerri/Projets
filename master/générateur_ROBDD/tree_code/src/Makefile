CXX = g++
CXXFLAGS = -Wall -Wextra -pedantic -Wformat=2 -Wformat-security -Werror=format-security #-g -Og
CXXFLAGS_HARDENED = -Ofast #-fstack-protector-strong -D_FORTIFY_SOURCE=2 -O1 #-O3 or -Ofast maybe ?
CPPFLAGS = -I../header -I../lib/BigInt-0.5.0-dev/release

####### # /!\ space
N1 = -DMIN=1000 # min range | for figure 10 and 11
N2 = -DMAX=2000 # max range | for figure 10 and 11
T  = -DNBTHREAD=1 #threads if > 1
D  = -DAUTH_MULT=0 #0: uniques values only | for figure 10 and 11
L  = -DHT_LIST_SIZE=5 #if > 1 | length estimation for buckets/hash-set list
K  = -DKEEP_DOT=0 #remove .dot files after creating .pdf
P  = -DSHOW_MODE=1 #0: terminal else graphiques (python)
C  = -DINT_LENGTH=20 #size of generated values for f10 and f11 (0 < n <= 1000)
###Ex:
#G  = -DPATH_GRAPH=\"/home/hellyiard/Cours/Master/Master1/cours/ALGAV/projet/graphs/\"
G   = #ABSOLUTE PATH directory to store graphs
#######

###PYTHON PATH
PY = -DPY_PATH=\"/usr/bin/python3\"
###

SRC = tree_analyzer.cpp tree.cpp result.cpp pool.cpp job.cpp
OBJ = $(SRC:.cpp=.o)
TO_RECONF = tree_analyzer.o tree.o #to apply macro

#main rule
all: analyzer

#exec compilation
analyzer: $(OBJ)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CXXFLAGS_HARDENED) -o ../../$@ $^
	-rm -f $(TO_RECONF)

%.o:%.cpp
	$(CXX) $(CPPFLAGS) $(N1) $(N2) $(T) $(D) $(L) $(K) $(P) $(C) $(G) $(PY) $(CXXFLAGS) $(CXXFLAGS_HARDENED) -o $@ -c $<

#dep
depend:
	$(CXX) -MM $(CPPFLAGS) $(SRC) > .depend
-include .depend

clean:
	-rm -f $(OBJ) .depend

#virtual targets
.PHONY: all clean