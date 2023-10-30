open Ast
open Tools

(* ex: a, v, f, Z   or   a; d; f  ... *)
let string_of_value_list (l : value list) (sep : string) : string =
  (* fun *)
  let rec string_of_list_aux (l : value list) (s : string) : string =
    match l with
    | [] ->
        s
    | Epsilon :: l' ->
        string_of_list_aux l' (s ^ sep)
    | Char c :: l' ->
        string_of_list_aux l' (s ^ sep ^ String.make 1 c)
  in
  (* end fun *)
  match l with
  | [] ->
      " "
  | Epsilon :: l' ->
      string_of_list_aux l' ""
  | Char c :: l' ->
      string_of_list_aux l' (String.make 1 c)

(* automaton's definition string *)
let string_of_def (autom : def) (s : string) : string =
  match autom with
  | Def
      ( Input_symb l1
      , Stack_symb l2
      , States l3
      , Init_state (Char c1)
      , Init_stack (Char c2) ) ->
      "input symbols: "
      ^ string_of_value_list l1 ", "
      ^ "\n" ^ "stack symbols: "
      ^ string_of_value_list l1 ", "
      ^ "\n" ^ "states: "
      ^ string_of_value_list l1 ", "
      ^ "\n" ^ "initial state: " ^ String.make 1 c1 ^ "\n" ^ "initial stack: "
      ^ String.make 1 c2
  | _ ->
      failwith "Error from string_of_def, automaton's "
      ^ "definition is not correct"

(* " " or char *)
let string_of_char_epsilon (v : value) : string =
  match v with Epsilon -> " " | Char c -> String.make 1 c

(* string of action *)
let string_of_action (a : action) : string =
  match a with
  | Push c ->
      "push(" ^ String.make 1 c ^ ")"
  | Pop ->
      "pop"
  | Change c ->
      "change(" ^ String.make 1 c ^ ")"
  | Reject ->
      "reject"

(* automaton's transitions string *)
let string_of_transitions (autom : transition) : string =
  (* fun *)
  let rec string_of_transitions_aux
      (l : (value * value * value * value * stack) list) (s : string) : string =
    match l with
    | [] ->
        s
    | (c1, c2, symb, c3, Stack st) :: l' ->
        let t =
          s ^ "(" ^ string_of_char_epsilon c1 ^ ", " ^ string_of_char_epsilon c2
          ^ ", "
          ^ string_of_char_epsilon symb
          ^ ", " ^ string_of_char_epsilon c3 ^ ", "
          ^ string_of_value_list st ";"
          ^ ")\n"
        in
        string_of_transitions_aux l' t
  in
  (* end fun *)
  (* fun *)
  let rec string_of_prog_aux (l : (value * value * value * action) list)
      (s : string) : string =
    match l with
    | [] ->
        s
    | (sta, inp, symb, act) :: l' ->
        let t =
          s ^ "(" ^ string_of_char_epsilon sta ^ ", "
          ^ string_of_char_epsilon inp ^ ", "
          ^ string_of_char_epsilon symb
          ^ ", " ^ string_of_action act ^ ")\n"
        in
        string_of_prog_aux l' t
  in
  (* end fun *)
  match autom with
  | Transition l ->
      string_of_transitions_aux l "transitions:\n\n"
  | Transition_p l ->
      string_of_prog_aux l "program:\n\n"

(* print automaton from AST (/!\ in part 1 & 2 form /!\) *)
let print_automaton (autom : automaton) : unit =
  match autom with
  | Automaton (def, transit) ->
      let tmp =
        string_of_def def "" ^ "\n\n"
        ^ string_of_transitions transit
        ^ "\n" ^ "end\n"
      in
      sep_parts () ; print_string tmp ; sep_parts ()
