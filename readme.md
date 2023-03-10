| Characters | Description                                                                                                                                                                   |
|------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| \[xyz\]    | A character class. Matches any one of the enclosed characters.                                                                                                                |
| [^xyz]     | A negated or complemented character class. That is, it matches<br>          anything that is not enclosed in the brackets                                                     |
| .          | 1) Matches any single character.<br>2) Inside a character class, the dot loses its special meaning and matches a literal dot.                                                 |
| x*         | Matches the preceding item "x" 0 or more times. |
| x+         | Matches the preceding item "x" 1 or more times. Equivalent to {1,}.                                                                                                           |
| x?         | Matches the preceding item "x" 0 or 1 times                                                                                                                                   |
| x{n}       | Where "n" is a positive integer, matches exactly "n" occurrences of the preceding item "x"                                                                                    |
| x{n,}      | Where "n" is a positive integer, matches at least "n" occurrences of the preceding item "x"                                                                                   |
| x{n,m}     | Where "n" is 0 or a positive integer, "m" is a positive integer, and m > n, matches at least "n" and at most "m" occurrences of the preceding item "x".                       |
