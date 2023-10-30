MENHIR=menhir
OCAMLC=ocamlc
OCAMLLEX=ocamllex

S_FOLDER=code/
C_FOLDER=compile/

.SILENT: all cleanall clean

all:
	+$(MAKE) -sC code

cleanall:
	rm -R $(C_FOLDER)

clean:
	rm -fr $(C_FOLDER)parser.mli $(C_FOLDER)parser.ml $(C_FOLDER)lexer.ml $(C_FOLDER)*.cmo $(C_FOLDER)*.cmi $(C_FOLDER)*~

