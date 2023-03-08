# The binary and library support

- Any ASCII symbols strictly matched except special key symbols
- Key symbol '.' that matches any one symbol except special key symbol
- So-called 'one-of' expression embraced in \[\] like '\[Abz123\]'. Ranges like \[A-Z0-9\] are not supported yet
- Key symbols like '\*', '+' for specifying number of possible occurrences of a preceding regular symbol or expression
  These symbols could be applied to a preceding regular symbol as well as to '.' or 'one-of' expression
- The parser check the input pattern for correctness
- The parser optimizes input pattern to make fewer steps and possible branches to match input strings

See unit tests to get familiar with what binary and library could do
