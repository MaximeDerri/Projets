MENHIR=menhir
OCAMLC=ocamlc
OCAMLLEX=ocamllex

SOURCES = ast.ml parser.ml lexer.ml tools.ml reprint.ml test.ml exec.ml main.ml

C_FOLDER = ../compile

OBJECTS = $(SOURCES:.ml=.cmo)

.PHONY: all 

all: create_compile_dir main
	mv *.cmo $(C_FOLDER)
	mv *.cmi $(C_FOLDER)
	mv *.automaton $(C_FOLDER)
	mv *.conflicts $(C_FOLDER)
	mv main ../main

create_compile_dir:
	mkdir -p $(C_FOLDER)

main: ast.cmo parser.cmi parser.cmo lexer.cmo tools.cmo reprint.cmo test.cmo exec.cmo main.cmo
	$(OCAMLC) -o $@ $(OBJECTS)

%.cmo: %.ml
	$(OCAMLC) -c $< -o $@

%.cmi: %.mli
	$(OCAMLC) -c $< -o $@

%.ml %.mli: %.mly
	#rm -f $(<:.mly=.conflicts)
	$(MENHIR) -v --infer $<

%.ml: %.mll
	$(OCAMLLEX) $<

parser.mly: ast.ml

lexer.mll: parser.ml

cleanall:
	rm -fr parser.mli parser.ml lexer.ml *.cmo main *.cmi *~ *.automaton *.conflicts

clean:
	rm -fr parser.mli parser.ml lexer.ml *.cmo *.cmi *~

parser.cmo: ast.cmo parser.cmi

lexer.cmo: parser.cmo

tools.cmo: ast.cmo

reprint.cmo: ast.cmo tools.cmo

test.cmo: ast.cmo tools.cmo

exec.cmo: ast.cmo tools.cmo test.cmo

main.cmo: parser.cmo lexer.cmo ast.cmo reprint.cmo test.cmo exec.cmo