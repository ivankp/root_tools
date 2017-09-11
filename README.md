# plot

## Regular expressions syntax

Regular expressions are passed as arguments to the `-r` program option.
They consist of one or more blocks separated by a delimiter (one of the `/|:` characters).
The blocks appear in the following order:

1. Flags
2. Regex
3. Format string
4. Extra blocks

### Delimiters

The delimiter is determined by the first character of `/|:` used after the flags block.
The delimiter remains the same throughout the expression,
but is determined anew for every expression.

### Escaping characters

A backslash `\` is used to escape characters.
This is necessary in order to insert a literal character instead of a delimiter,
or a new line `\n`, or a tab `\t`.
If `\` precedes a character the cannot be escaped, i.e. is neither `n`, `t`, or the delimiter,
a literal `\` is inserted.
To insert a literal `\` instead of escaping a character, `\\` must be used.

Examples:
* `\n` is a new line.
* `\b` is a backslash followed by `b`.
* `\\n` is a backslash followed by `n`.
* `\\\n` is a backslash followed by new-line.
* `\\\\n` is 2 backslashes followed by new-line.
* `\\\\b` is 4 backslashes followed by `b`.

Only the delimiter in use needs to be escaped.
The other characters from the set of possible delimiters are inserted literally.

Example 1: `g/:\//`

### [Boost regex](http://www.boost.org/libs/regex) syntax
* [Regex](
   http://www.boost.org/libs/regex/doc/html/boost_regex/syntax/perl_syntax.html)
* [Format string](
   http://www.boost.org/libs/regex/doc/html/boost_regex/format/boost_format_syntax.html)
* [Captures](
   http://www.boost.org/libs/regex/doc/html/boost_regex/captures.html)
