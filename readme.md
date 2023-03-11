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
| \d         | Matches any digit (Arabic numeral). Equivalent to [0-9]. For example, /\d/ or /[0-9]/ matches "2" in "B2 is the suite number". |
| \D         | Matches any character that is not a digit (Arabic numeral). Equivalent to [^0-9]. For example, /\D/ or /[^0-9]/ matches "B" in "B2 is the suite number". |
| \w         | Matches any alphanumeric character from the basic Latin alphabet, including the underscore. Equivalent to [A-Za-z0-9_]. For example, /\w/ matches "a" in "apple", "5" in "$5.28", and "3" in "3D". |
| \W         | Matches any character that is not a word character from the basic Latin alphabet. Equivalent to [^A-Za-z0-9_]. For example, /\W/ or /[^A-Za-z0-9_]/ matches "%" in "50%". |
| \s         | Matches a single white space character, including space, tab, form feed, line feed, and other Unicode spaces. Equivalent to [ \f\n\r\t\v] |
| \S         | Matches a single character other than white space. Equivalent to [^ \f\n\r\t\v]
| \t         | Matches a horizontal tab. |
| \r         | Matches a carriage return. |
| \n         | Matches a linefeed. |
| \v         | Matches a vertical tab. |
| \f         | Matches a form-feed. |
| \\         | Matchers a \\ |
