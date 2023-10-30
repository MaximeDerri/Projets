type automaton = 
  | Automaton of def * transition

and
transition =
  | Transition of (value * value * value * value * stack) list
  | Transition_p of (value * value * value * action) list

and
def =
  | Def of head * head * head * head * head

and
value =
  | Epsilon
  | Char of char

and
stack =
  | Stack of value list

and
action =
  | Push of char
  | Pop
  | Change of char
  | Reject

and
head =
  | Input_symb of value list
  | Stack_symb of value list
  | States of value list
  | Init_state of value
  | Init_stack of value