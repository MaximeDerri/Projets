open Ast
open Reprint
open Test
open Exec


let help () =
  print_string "GAS project - deterministic pushdown automaton\n";
  print_string "*** [-base for step 1&2  |  -prog for step 3] ***\n";
  print_string "./main -reprint file                  ->  reprint automaton from AST\n";
  print_string "./main -test [-base | -prog]  file    ->  verify determinism of automaton from AST\n";
  print_string "./main -exec [-base | -prog]  file    ->  interpret automaton\n";;
  (* ADD OTHERS *)


(* MAIN *)
let main () =
  match Sys.argv with
  | [|_; "-reprint"; file|] ->
    let c = open_in file in
    let lexbuf = Lexing.from_channel c in
    let ast = Parser.automaton Lexer.main lexbuf in
    close_in c;
    print_automaton ast;

  | [|_; "-test"; opt; file|] ->
    let c = open_in file in
    let lexbuf = Lexing.from_channel c in
    let ast = Parser.automaton Lexer.main lexbuf in
    close_in c;
    begin
      match opt with
      | "-base" ->
        begin
          match check_automaton_base ast with (* step 1 & 2*)
          | _ -> (); (* skip result, we just want the test *)
        end
      | "-prog" ->
        begin
          match check_automaton_prog ast with (* step 3 *)
          | _ -> (); (* skip result, we just want the test *)
        end
      | _ -> help ();
    end

  | [|_; "-exec"; opt; file|] ->
    let c = open_in file in
    let lexbuf = Lexing.from_channel c in
    let ast = Parser.automaton Lexer.main lexbuf in
    close_in c;
    begin
      match opt with
      | "-base" -> interpret_automaton_base ast;
      | "-prog" -> interpret_automaton_prog ast;
      | _ -> help ();
    end

  | _ -> help ();;


let () = main ();;

