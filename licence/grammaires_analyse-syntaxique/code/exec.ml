open Ast
open Tools
open Test

(* return an init struct for exec step 1 & 2 *)
let init_exec_base
    (t : (char * (value * value * value * value * stack) list) list) (st : char)
    (sta : char) : exec_t =
  print_string "Input:\n" ;
  let input_tmp =
    try read_line ()
    with End_of_file -> failwith "Error, reach eof -> no input"
  in
  let l_inp = List.init (String.length input_tmp) (String.get input_tmp) in
  let () = print_string "\n" in
  {transitions= t; state= st; stack= [sta]; input= l_inp}

(* return an init struct for exec step 3 *)
let init_exec_prog (t : (value * value * value * action) list) (st : char)
    (sta : char) : exec_p =
  print_string "Input:\n" ;
  let input_tmp =
    try read_line ()
    with End_of_file -> failwith "Error, reach eof -> no input"
  in
  let l_inp = List.init (String.length input_tmp) (String.get input_tmp) in
  let () = print_string "\n" in
  {transitions= t; state= st; stack= [sta]; input= l_inp}

(* aux for search_transition *)
let top_l (l : char list) : char option =
  match l with [] -> None | e :: l -> Some e

(* check the current config *)
(* if possible, return the transition to perform *)
let check_config_and_search_transition_base (exec : exec_t) :
    (value * value * value * value * stack) option =
  (* fun *)
  let rec search_transition_list
      (l : (char * (value * value * value * value * stack) list) list) =
    (* fun *)
    let rec search_transition_aux
        (t : (value * value * value * value * stack) list) =
      match (top_l exec.stack, top_l exec.input) with
      (* reach end, success *)
      | None, None ->
          let () = print_string "Execution completed successfully !\n" in
          None
          (* reach end, error*)
      | None, Some inp ->
          let () = print_string "Execution error, stack is empty\n" in
          None
          (* searching Epsilon transition to clear stack *)
      | Some sta_s, None -> (
        match t with
        | [] ->
            (* 0 transition for the state *)
            let () = print_string "Execution error, input is empty\n" in
            None
        | (Char c1, Epsilon, Char symb, Char c3, Stack sta) :: t'
          when symb = sta_s ->
            Some (Char c1, Epsilon, Char symb, Char c3, Stack sta)
        | e :: t' ->
            search_transition_aux t' (* searching transition *) )
      | Some sta_s, Some inp -> (
        match t with
        | [] ->
            (* 0 transition for the state *)
            let () =
              print_string
                ( "Execution error, no matched transition "
                ^ "found to continue\n" )
            in
            None
        | (Char c1, c2, Char symb, Char c3, Stack sta) :: t' when symb = sta_s
          -> (
          match c2 with
          | Epsilon ->
              Some (Char c1, c2, Char symb, Char c3, Stack sta)
          | Char c when c = inp ->
              Some (Char c1, c2, Char symb, Char c3, Stack sta)
          | _ ->
              search_transition_aux t' (*symb match but not the input *) )
        | e :: t' ->
            search_transition_aux t' (*no match *) )
    in
    (* end fun *)
    match l with
    | [] ->
        failwith "Error from search_transition, no state match"
    | (s, t) :: l' when s = exec.state ->
        search_transition_aux t
    | tmp :: l' ->
        search_transition_list l'
  in
  (* end fun *)
  search_transition_list exec.transitions

(* perform the next transition *)
let rec perform_transition_base (exec : exec_t) : exec_t option =
  let tmp = check_config_and_search_transition_base exec in
  match tmp with
  | None ->
      None (* performing transition *)
  | Some (Char c1, c2, Char symb, Char c3, Stack sta) ->
      let new_stack =
        match exec.stack with
        (* technically never used *)
        | [] ->
            print_string "Execution error, stack is empty\n" ;
            None (* extract new stack *)
        | e :: s' ->
            Some (char_list_of_value_list sta @ s')
      in
      let new_input =
        if c2 = Epsilon then Some exec.input
        else
          match exec.input with
          (* technically never used *)
          | [] ->
              print_string "Execution error, input is empty\n" ;
              None (* extract new input list *)
          | e :: i' ->
              Some i'
      in
      Some
        { transitions= exec.transitions
        ; state= c3
        ; stack= Option.get new_stack
        ; input= Option.get new_input }
  | _ ->
      failwith "Error from perform_transition, transition is not correct"

let search_transition (exec : exec_p) : action option =
  let rec search_aux tr top next : action option =
    match tr with
    | [] ->
        None
    | (Char st, inp, symb, act) :: l -> (
      match (inp, symb, next) with
      | Char a, Char b, Some n when st = exec.state && a = n && b = top ->
          Some act
      | Char a, Epsilon, Some n when st = exec.state && a = n ->
          Some act
      | Epsilon, Char b, Some n when st = exec.state && b = top ->
          Some act
      | Epsilon, Epsilon, Some n when st = exec.state ->
          Some act
      | Epsilon, Char b, None when st = exec.state && b = top ->
          Some act
      | Epsilon, Epsilon, None when st = exec.state ->
          Some act
      | _ ->
          failwith "Error from search_transition - no matched transition" )
    | a :: l ->
        search_aux l top next
  in
  let sta = top_l exec.stack in
  let inp = top_l exec.input in
  if sta = None then
    let () = print_string "Execution failed - stack empt\n" in
    None
  else search_aux exec.transitions (Option.get sta) inp

(* perform transition for prog - step 3 *)
let perform_transition_prog (exec : exec_p) : exec_p option =
  if exec.stack = [] && exec.input = [] then
    let () = print_string "Execution completed successfully !\n" in
    None
  else
    let tmp = search_transition exec in
    match tmp with
    | None ->
        None
    | Some e -> (
        let n_in = top_l exec.input in
        match e with
        | Push x ->
            if n_in = None then
              Some
                { transitions= exec.transitions
                ; state= exec.state
                ; stack= exec.stack @ [x]
                ; input= exec.input }
            else
              Some
                { transitions= exec.transitions
                ; state= exec.state
                ; stack= exec.stack @ [x]
                ; input= cut_list exec.input }
        | Pop ->
            if n_in = None then
              Some
                { transitions= exec.transitions
                ; state= exec.state
                ; stack= cut_list exec.stack
                ; input= exec.input }
            else
              Some
                { transitions= exec.transitions
                ; state= exec.state
                ; stack= cut_list exec.stack
                ; input= cut_list exec.input }
        | Change x ->
            if n_in = None then
              Some
                { transitions= exec.transitions
                ; state= x
                ; stack= exec.stack
                ; input= exec.input }
            else
              Some
                { transitions= exec.transitions
                ; state= x
                ; stack= exec.stack
                ; input= cut_list exec.input }
        | Reject ->
            let () = print_string "Reject statement was reached\n" in
            None )

(* similars, but needed because of struct type... *)
let print_config_base (exec : exec_t) : unit =
  print_string ("{State : " ^ String.make 1 exec.state ^ "}\n") ;
  print_string ("{Stack : " ^ string_of_char_list exec.stack "|" ^ "}\n") ;
  print_string ("{Input : " ^ string_of_char_list exec.input "" ^ "}\n\n")

let print_config_prog (exec : exec_p) : unit =
  print_string ("{State : " ^ String.make 1 exec.state ^ "}\n") ;
  print_string ("{Stack : " ^ string_of_char_list exec.stack "|" ^ "}\n") ;
  print_string ("{Input : " ^ string_of_char_list exec.input "" ^ "}\n\n")

let rec exec_automaton_base (exec : exec_t) : unit =
  print_config_base exec ;
  match perform_transition_base exec with
  | None ->
      () (* end was print *)
  | Some e ->
      exec_automaton_base e

(* check automaton, prepare struct and launch exec *)
let interpret_automaton_base (autom : automaton) : unit =
  let test = check_automaton_base autom in
  match test with
  | None ->
      ()
  | Some l -> (
    match autom with
    | Automaton (Def (_, _, _, Init_state (Char c1), Init_stack (Char c2)), _)
      ->
        let config = init_exec_base l c1 c2 in
        let () = exec_automaton_base config in
        sep_parts ()
    | _ ->
        failwith "Error from interpret_automaton_base, automaton is not correct"
    )

(* exec for prog *)
let rec exec_automaton_prog (exec : exec_p) : unit =
  print_config_prog exec ;
  match perform_transition_prog exec with
  | None ->
      ()
  | Some e ->
      exec_automaton_prog e

(* check, prepare and launch *)
let interpret_automaton_prog (autom : automaton) : unit =
  (* verif *)
  sep_parts () ;
  match autom with
  | Automaton
      ( Def (_, _, _, Init_state (Char c1), Init_stack (Char c2))
      , Transition_p tr ) ->
      let config = init_exec_prog tr c1 c2 in
      let () = exec_automaton_prog config in
      sep_parts ()
  | _ ->
      failwith "Error from interpret_automaton_prog, automaton is not correct"
