{
open Parser
exception Lexing_error of string
exception Lexing_eof_error of string
}

let space = [' ''\n''\t''\r']
let symbol = ['a'-'z''A'-'Z''0'-'9']



rule main = parse
  | space                     {main lexbuf}
  | "/*"                      {after_comment lexbuf}
  | "input"                   {INPUT}
  | "stack"                   {STACK}
  | "states"                  {STATES}
  | "state"                   {STATE}
  | "initial"                 {INITIAL}
  | "symbols"                 {SYMBOLS}
  | "transitions"             {TRANSITIONS}
  | "program"                 {PROGRAM}
  | "case"                    {CASE}
  | "of"                      {OF}
  | "next"                    {NEXT}
  | "begin"                   {BEGIN}
  | "end"                     {END}
  | "top"                     {TOP}
  | "push"                    {PUSH}
  | "pop"                     {POP}
  | "change"                  {CHANGE}
  | "reject"                  {REJECT}
  | ':'                       {DB_DOTS}
  | ','                       {COMMA}
  | ';'                       {SEMICOLON}
  | '('                       {LPAREN}
  | ')'                       {RPAREN}
  | symbol as s               {CHAR(s)}
  | eof                       {EOF}
  | _			                    {raise (Lexing_error("lexing error: unexpected character -> " ^ Lexing.lexeme lexbuf))}


and after_comment = parse
  | "*/"                      {main lexbuf}
  | eof                       {raise(Lexing_eof_error("eof is reach in comment"))}
  | _                         {after_comment lexbuf}