open Ast

(* struct for exec *)
type exec_t =
  { (* exec_transition *)
    (* organized transitions:
          we tried to organize the transitions according to the states (between the
          verification step and the execution step) to try to make the code a bit more
          efficient for interpretation in a case where there will be a lot of
          transitions. We check the char (type) which represent a state *)
    transitions: (char * (value * value * value * value * stack) list) list
  ; (* current config *)
    state: char
  ; stack: char list
  ; input: char list }

(* type prog_t for step 3 *)
type exec_p =
  { (* exec_prog *)
    (* transitions according to the program *)
    transitions: (value * value * value * action) list
  ; (* current config *)
    state: char
  ; stack: char list
  ; input: char list }

(* ---------- *)

let sep_parts () = print_string "========================================\n"

let string_of_char_list (l : char list) (sep : string) : string =
  (* fun *)
  let rec string_aux (l' : char list) (s : string) : string =
    match l' with
    | [] ->
        s
    | c :: l'' ->
        string_aux l'' (s ^ sep ^ String.make 1 c)
  in
  (* end fun *)
  match l with [] -> "" | c :: l' -> string_aux l' (String.make 1 c)

let rec search_in_list (l : value list) (e : char) : bool =
  match l with
  | [] ->
      false
  | Char x :: l' ->
      if x = e then true else search_in_list l' e
  | Epsilon :: l' ->
      search_in_list l' e

(* value list -> char list *)
let char_list_of_value_list (l : value list) : char list =
  (* fun *)
  let rec get_list_aux (l : value list) (lr : char list) : char list =
    match l with
    | [] ->
        lr
    | Epsilon :: l' ->
        get_list_aux l' lr
    | Char c :: l' ->
        get_list_aux l' (c :: lr)
  in
  (* end fun *)
  get_list_aux l []

let cut_list (l : 'a list) = match l with [] -> [] | a :: l' -> l'
