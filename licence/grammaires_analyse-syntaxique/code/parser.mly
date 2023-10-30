(* paser for part 1 and 2 *)
%{
open Ast
let v_state = ref(Epsilon) (* state for step 3 *)
let v_symb = ref(Epsilon) (* symb for step 3 *)
%}

%token INPUT SYMBOLS STACK STATES STATE INITIAL TRANSITIONS
%token SEMICOLON COMMA DB_DOTS LPAREN RPAREN BEGIN END EOF
%token PROGRAM CASE OF NEXT TOP PUSH POP CHANGE REJECT
%token <char> CHAR
%start <Ast.automaton> automaton
%%

automaton: d=def t=transitions end_  {Automaton(d,t)}

end_: EOF     {()}
    | END EOF {()}

def: a=input_symbols b=stack_symbols c=states d=initial_state e=initial_stack  {Def(a,b,c,d,e)}

input_symbols: INPUT SYMBOLS DB_DOTS l=char_list  {Input_symb(l)}

stack_symbols: STACK SYMBOLS DB_DOTS l=char_list {Stack_symb(l)}

states: STATES DB_DOTS l=char_list  {States(l)}

initial_state: INITIAL STATE DB_DOTS c=CHAR {Init_state(Char(c))}

initial_stack: INITIAL STACK DB_DOTS c=CHAR  {Init_stack(Char(c))}

transitions: TRANSITIONS DB_DOTS l=transition_list      {Transition(l)}
           | PROGRAM DB_DOTS CASE STATE OF l=prog_list  {Transition_p(l)} /* step 3 */

transition_list : t=transition l=transition_list {t::l}
                |                                {[]}

transition: LPAREN c1=CHAR COMMA c2=char_or_epsilon COMMA symb=CHAR COMMA c3=CHAR COMMA st=stack RPAREN  {(Char(c1), c2, Char(symb), Char(c3), st)}


/* step 3 */

prog_list: state_prog NEXT OF l=next_of END ll=prog_list  {l @ ll}
         | state_prog TOP OF l=top_of END ll=prog_list    {l @ ll}
         |                                                {[]}

next_of: c=CHAR DB_DOTS act=action l=next_of  {(!v_state, Char(c), !v_symb, act)::l}
       |                                      {[]}

top_of: c=CHAR DB_DOTS act=action l=top_of         {(!v_state, Epsilon, Char(c), act)::l}
      | symb_prog NEXT OF l=next_of END ll=top_of  {l@ll}
      |                                            {[]}

state_prog: c=CHAR DB_DOTS BEGIN CASE  {v_state := Char(c); v_symb := Epsilon}

symb_prog: c=CHAR DB_DOTS BEGIN CASE  {v_symb := Char(c)}

action: PUSH c=CHAR    {Push(c)}
      | POP            {Pop}
      | CHANGE c=CHAR  {Change(c)}
      | REJECT         {Reject}

/* step 3*/

stack: l=stack_list  {Stack(l)}
     |               {Stack([])}

stack_list: c=CHAR SEMICOLON l=stack_list  {Char(c)::l}
          | c=CHAR                         {Char(c)::[]}

char_list: c=CHAR l=char_list_aux {Char(c)::l}
         |                        {[]}
char_list_aux: COMMA c=CHAR l=char_list_aux {Char(c)::l}
             |                              {[]}

char_or_epsilon: c=CHAR  {Char(c)}
               |         {Epsilon}