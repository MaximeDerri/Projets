open Ast
open Tools

(* check multiples symbols in def (input symb, stack symb, states) *)
let check_duplicate_declarations (l : value list) : bool =
  (* getting char list, sort it with unique values and compar length *)
  let cl = char_list_of_value_list l in
  let cl' =
    List.sort_uniq (fun x y -> if x > y then 1 else if x = y then 0 else -1) cl
  in
  if List.length cl = List.length cl' then true else false

(* checking definition *)
let check_def (autom : def) : bool =
  print_string "Checking definition...\n" ;
  match autom with
  | Def
      ( Input_symb l1
      , Stack_symb l2
      , States l3
      , Init_state (Char c1)
      , Init_stack (Char c2) ) ->
      if l1 = [] then
        let () = print_string "Error, input symbols empty\n" in
        false
      else if not (check_duplicate_declarations l1) then
        let () = print_string "Error, input symbols - duplicate symbols\n" in
        false
      else if l2 = [] then
        let () = print_string "Error, stack symbols empty\n" in
        false
      else if not (check_duplicate_declarations l2) then
        let () = print_string "Error, stack symbols - duplicate symbols\n" in
        false
      else if l3 = [] then
        let () = print_string "Error, states empty\n" in
        false
      else if not (check_duplicate_declarations l3) then
        let () = print_string "Error, states - duplicate symbols\n" in
        false
      else if not (search_in_list l3 c1) then
        let () =
          print_string
            ("Error, initial state was not found in the" ^ " list of states\n")
        in
        false
      else if not (search_in_list l2 c2) then
        let () =
          print_string
            ( "Error, initial stack was not found in "
            ^ "the list of stack symbols\n" )
        in
        false
      else true
  | _ ->
      let () = print_string "Error, definition is not correct\n" in
      false

(* return a list of tuple where state st that match in transitions *)
let get_transitions_by_state_base (t : transition) (st : char) :
    (value * value * value * value * stack) list =
  (* fun *)
  let rec get_aux (l : (value * value * value * value * stack) list)
      (lr : (value * value * value * value * stack) list) =
    match l with
    | [] ->
        lr
    | (Char s, a, b, c, d) :: l' when s = st ->
        get_aux l' ((Char s, a, b, c, d) :: lr)
    | tmp :: l' ->
        get_aux l' lr
  in
  (* end fun *)
  match t with
  | Transition l ->
      get_aux l []
  | Transition_p _ ->
      failwith "Error, Transition_p type found in get_transition_by_state_base"

(* permit to sort transitions by their states *)
let split_transition_by_state_base (t : transition) (s : char list) :
    (char * (value * value * value * value * stack) list) list =
  (* fun *)
  let rec split_aux (t' : transition) (st : char list)
      (lr : (char * (value * value * value * value * stack) list) list) =
    match st with
    | [] ->
        lr
    | e :: st' ->
        split_aux t' st' ((e, get_transitions_by_state_base t e) :: lr)
    (* list of (state_s * transitions_of_state_s) *)
  in
  (* end fun *)
  split_aux t s []

(* return the list of char states symbols *)
let extract_states_list (def : def) : char list =
  match def with
  | Def (_, _, States c, _, _) ->
      char_list_of_value_list c
  | _ ->
      failwith "Bad automaton definition in extract_states_list"

(* used by check_transitions_in_def *)
let extract_char_value (v : value) : char =
  match v with
  | Char c ->
      c
  | _ ->
      failwith
        "Error from extract_char_value, Char expected but Epsilon was found"

(* return the cartesian product of input_char and stack_symbols *)
(* [a;b] * [c;d] -> [(a,c);(a,d);(b,c);(d;b)]*)
let cartesian_inp_stack (def : def) : (char * char) list =
  (* fun *)
  let rec cartesian_aux (inp : char list) (symb : char list)
      (lr : (char * char) list) =
    (* fun *)
    let rec aux (inp' : char) (symb' : char list) (lr' : (char * char) list) =
      match symb' with
      | [] ->
          lr'
      | e :: symb'' ->
          aux inp' symb'' ((inp', e) :: lr')
    in
    (* end fun *)
    match inp with
    | [] ->
        lr
    | e :: inp' ->
        let tmp = aux e symb [] in
        cartesian_aux inp' symb (lr @ tmp)
  in
  (* end fun *)
  match def with
  | Def (Input_symb a, Stack_symb b, _, _, _) ->
      cartesian_aux (char_list_of_value_list a) (char_list_of_value_list b) []
  | _ ->
      failwith "Error from cartesian_inp_symb, bad automaton definition"

(* look if transitions are deterministics *)
let rec is_deterministic_base
    (l : (char * (value * value * value * value * stack) list) list)
    (inp_stack : (char * char) list) : bool =
  (* fun *)
  let rec is_det_aux (t : char * (value * value * value * value * stack) list)
      (is : (char * char) list) =
    (* fun *)
    let rec test_transition (cur : (value * value * value * value * stack) list)
        (test : char * char) (count : int) : bool =
      match cur with
      | [] ->
          if count > 1 then false else true
      | (_, inp, Char stack_s, _, _) :: cur' -> (
          if stack_s <> snd test then
            test_transition cur' test count (* no match*)
          else
            match inp with
            (* Epsilon -> all symb *)
            | Epsilon ->
                test_transition cur' test (count + 1)
            | Char c ->
                if c = fst test then
                  (*  match *)
                  test_transition cur' test (count + 1)
                else (* no match *)
                  test_transition cur' test count )
      | _ ->
          failwith
            "Error from is_deterministic_base, current transition is not \
             correct"
    in
    (* end fun *)
    match is with
    | [] ->
        true
    | e :: is' ->
        if not (test_transition (snd t) e 0) then
          let () =
            print_string
              ( "Error from state: '"
              ^ String.make 1 (fst t)
              ^ "', more than one possibility has been found for input: '"
              ^ String.make 1 (fst e)
              ^ "' and stack symbol: '"
              ^ String.make 1 (snd e)
              ^ "'\n" )
          in
          false
          (* for the same input symb(or epsilon) and stack symbol, to many transitions *)
        else is_det_aux t is'
  in
  (* end fun *)
  match l with
  | [] ->
      true
  | e :: l' ->
      if is_det_aux e inp_stack then is_deterministic_base l' inp_stack
      else false

(* verfy if symbols from transitions are in def *)
let check_transitions_in_def_base (autom : automaton) : bool =
  (* fun *)
  let rec check_aux (t : (value * value * value * value * stack) list)
      (inp : value list) (stack_s : value list) (st : value list) : bool =
    (* fun *)
    let rec stack_aux (stack : value list) : bool =
      match stack with
      | [] ->
          true
      | Char c :: stack' ->
          if not (search_in_list stack_s c) then
            let () =
              print_string
                ("Error, '" ^ String.make 1 c ^ "' is not in stack symbols\n")
            in
            false
          else stack_aux stack'
      | Epsilon :: stack' ->
          stack_aux stack'
    in
    (* end fun *)
    match t with
    | [] ->
        true
    | (Char c1, c2, Char symb, Char c3, Stack sta) :: l' ->
        if not (search_in_list st c1) then
          (* current state *)
          let () =
            print_string ("Error, '" ^ String.make 1 c1 ^ "' is not in states\n")
          in
          false
        else if
          not
            (* input symbol *)
            (match c2 with Epsilon -> true | Char c -> search_in_list inp c)
        then
          let () =
            print_string
              ( "Error, '"
              ^ String.make 1 (extract_char_value c2)
              ^ "' is not in input symbols\n" )
          in
          false
        else if not (search_in_list stack_s symb) then
          (* stack symbol *)
          let () =
            print_string
              ("Error, '" ^ String.make 1 symb ^ "' is not in stack symbols\n")
          in
          false
        else if not (search_in_list st c3) then
          (* dest state *)
          let () =
            print_string ("Error, '" ^ String.make 1 c3 ^ "' is not in states\n")
          in
          false
        else if not (stack_aux sta) then (* stack *)
          false
        else check_aux l' inp stack_s st
    | _ ->
        failwith
          "Error from check_transitions_in_def_base, transition is not correct"
  in
  (* end fun *)
  match autom with
  | Automaton
      ( Def
          ( Input_symb l1
          , Stack_symb l2
          , States l3
          , Init_state (Char c1)
          , Init_stack (Char c2) )
      , Transition l ) ->
      check_aux l l1 l2 l3
  | _ ->
      failwith
        "Error from check_transitions_in_def_base, definition is not correct"

(* true if 0 transitions *)
let is_transitions_empty (t : transition) : bool =
  match t with
  | Transition l -> (
    match l with [] -> true | _ -> false )
  | Transition_p l -> (
    match l with [] -> true | _ -> false )

(* checking transitions *)
let check_transitions_base (d : def) (t : transition) (st : char list)
    (inp_stack : (char * char) list) :
    (char * (value * value * value * value * stack) list) list option =
  print_string "Checking transitions...\n" ;
  (* empty ?*)
  if is_transitions_empty t then
    let () = print_string "Error, transitions empty - nothing to do\n" in
    None (* transitions content not ok ?*)
  else if not (check_transitions_in_def_base (Automaton (d, t))) then None
  else
    let t = split_transition_by_state_base t st in
    (* look determinsm *)
    if is_deterministic_base t inp_stack then Some t
    else
      (* not... *)
      let () = print_string "Pushdown automaton is not deterministic\n" in
      None

(* check if autom have a good definition and is a deterministic pushdown automaton *)
let check_automaton_base (autom : automaton) :
    (char * (value * value * value * value * stack) list) list option =
  sep_parts () ;
  match autom with
  | Automaton (def, t) -> (
      if not (check_def def) then
        let () = sep_parts () in
        None
      else
        let tmp =
          check_transitions_base def t (extract_states_list def)
            (cartesian_inp_stack def)
        in
        match tmp with
        | None ->
            sep_parts () ; None
        | _ ->
            print_string "Automaton ok !\n" ;
            sep_parts () ;
            tmp )

(* test for prog*)

let rec is_deterministic_prog
    (l : (char * (value * value * value * action) list) list)
    (inp_stack : (char * char) list) : bool =
  let rec is_det_aux (t : char * (value * value * value * action) list)
      (is : (char * char) list) =
    let rec test_transition (cur : (value * value * value * action) list)
        (test : char * char) (count : int) : bool =
      match cur with
      | [] ->
          if count > 1 then false else true
      | (_, inp, Char stack_s, _) :: cur' -> (
          if stack_s <> snd test then test_transition cur' test count
          else
            match inp with
            | Epsilon ->
                test_transition cur' test (count + 1)
            | Char c ->
                if c = fst test then test_transition cur' test (count + 1)
                else test_transition cur' test count )
      | _ ->
          failwith
            "Error from is_deterministic_base_prog, current transition is not \
             correct"
    in
    match is with
    | [] ->
        true
    | e :: is' ->
        if not (test_transition (snd t) e 0) then
          let () =
            print_string
              ( "Error from state: '"
              ^ String.make 1 (fst t)
              ^ "', more than one possibility has been found for input: '"
              ^ String.make 1 (fst e)
              ^ "' and stack symbol: '"
              ^ String.make 1 (snd e)
              ^ "'\n" )
          in
          false
        else is_det_aux t is'
  in
  match l with
  | [] ->
      true
  | e :: l' ->
      if is_det_aux e inp_stack then is_deterministic_prog l' inp_stack
      else false

let get_transitions_by_state_prog (t : transition) (st : char) :
    (value * value * value * action) list =
  (* fun *)
  let rec get_aux (l : (value * value * value * action) list)
      (lr : (value * value * value * action) list) =
    match l with
    | [] ->
        lr
    | (Char e, i, s, a) :: l' when e = st ->
        get_aux l' ((Char e, i, s, a) :: lr)
    | tmp :: l' ->
        get_aux l' lr
  in
  (* end fun *)
  match t with
  | Transition _ ->
      failwith "Error, Transition type found in get_transition_by_state_prog"
  | Transition_p l ->
      get_aux l []

let check_transitions_in_def_prog (autom : automaton) : bool =
  let rec check_aux (t : (value * value * value * action) list)
      (etats : value list) (symbs : value list) (stack_s : value list) : bool =
    match t with
    | [] ->
        true
    | (Char c1, c2, c3, a) :: l' ->
        if not (search_in_list stack_s c1) then
          (* current state *)
          let () =
            print_string ("Error, '" ^ String.make 1 c1 ^ "' is not in states\n")
          in
          false
        else if
          not
            (* input symbol *)
            (match c2 with Epsilon -> true | Char c -> search_in_list etats c)
        then
          let () =
            print_string
              ( "Error, '"
              ^ String.make 1 (extract_char_value c2)
              ^ "' is not in input symbols\n" )
          in
          false
        else if
          not
            (match c3 with Epsilon -> true | Char c -> search_in_list symbs c)
        then
          (* stack symbol *)
          let () =
            print_string
              ( "Error, '"
              ^ String.make 1 (extract_char_value c3)
              ^ "' is not in stack symbols\n" )
          in
          false
        else if
          (* action *)
          match a with
          | Change c ->
              search_in_list etats c
          | Push c ->
              search_in_list symbs c
          | _ ->
              true
        then
          let () =
            print_string
              "Error, action contains an operation that is not in states\n"
          in
          false
        else check_aux l' etats symbs stack_s
    | _ ->
        failwith
          "Error from check_transitions_in_def_prog, transition is not correct"
  in
  (* end fun *)
  match autom with
  | Automaton
      ( Def
          ( Input_symb l1
          , Stack_symb l2
          , States l3
          , Init_state (Char c1)
          , Init_stack (Char c2) )
      , Transition_p l ) ->
      check_aux l l1 l2 l3
  | _ ->
      failwith
        "Error from check_transitions_in_def_prog, definition is not correct"

let split_transition_by_state_prog (t : transition) (s : char list) :
    (char * (value * value * value * action) list) list =
  (* fun *)
  let rec split_aux (t' : transition) (st : char list)
      (lr : (char * (value * value * value * action) list) list) =
    match st with
    | [] ->
        lr
    | e :: st' ->
        split_aux t' st' ((e, get_transitions_by_state_prog t e) :: lr)
    (* list of (state_s * transitions_of_state_s) *)
  in
  (* end fun *)
  split_aux t s []

let check_transitions_prog (d : def) (t : transition) (st : char list)
    (inp_stack : (char * char) list) :
    (char * (value * value * value * action) list) list option =
  print_string "Checking transitions...\n" ;
  if is_transitions_empty t then
    let () = print_string "Error, transitions empty - nothing to do\n" in
    None
  else if not (check_transitions_in_def_prog (Automaton (d, t))) then None
  else
    let t = split_transition_by_state_prog t st in
    if is_deterministic_prog t inp_stack then Some t
    else
      let () = print_string "Pushdown automaton is not deterministic\n" in
      None

let check_automaton_prog (autom : automaton) :
    (char * (value * value * value * action) list) list option =
  sep_parts () ;
  match autom with
  | Automaton (def, t) -> (
      if not (check_def def) then
        let () = sep_parts () in
        None
      else
        let tmp =
          check_transitions_prog def t (extract_states_list def)
            (cartesian_inp_stack def)
        in
        match tmp with
        | None ->
            sep_parts () ; None
        | _ ->
            print_string "Automaton ok !\n" ;
            sep_parts () ;
            tmp )
