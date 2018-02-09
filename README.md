# `hed`

## Regular expressions syntax

Expressions are passed as arguments to the `-e` program option. They
consist of one or more blocks separated by a delimiter (one of the `/|:,`
characters). The blocks appear in the following order:

If `-g` is used instead of `-e`, the expression is applied at the canvas level
rather than the histogram. While `-e` and `-g` options can be intermixed,
all `-e` expressions are applied before `-g` ones.

1. Flags
2. Regex
3. Format string
4. Expressions

*Example*: `snt/^jet_/Jet /norm` selects only histograms whose names start with
`"jets_"`, replaces that with `"Jets "`, sets the histogram's title to the
modified string, and normalizes the histogram.

### Delimiters

The delimiter is determined by the first character of `/|:,` used after the
flags block. The delimiter remains the same throughout the expression, but is
determined anew for every expression.

To be inserted as literal characters, delimiters need to be escaped with `\`.
Only the delimiter in use needs to be escaped. The other characters from the
set of possible delimiters are inserted literally.

### Flags

Flags are single-character modifiers used in the first expression block.
Flags in the first group modify the outcome of regex matching.

* `s` - select. Retain only matching histograms.
* `i` - invert matching. String replacement, selection, and commands apply
        if regex didn't match.
* number - if there are multiple regex matches, specifies which ones to modify.
  * 0 - every match
  * n - every match after nth
  * -n - every match before nth (from the end)
* `m` - modifies the match index to mean exactly at.

Flags in the second group specify which field (string) to modify.

* `g` - group. Determines which histograms go on the same canvas.
* `n` - name of the histogram.
* `t` - title of the histogram.
* `x` - x-axis title.
* `y` - y-axis title.
* `z` - z-axis title.
* `l` - legend label.
* `d` - directory path.
* `f` - file name.

0, 1, or 2 of field flags may be specified.
The first field flag specifies which field to use as the source string.
The second field flag specifies which field will be set to the modified string.
If the second flag is omitted, it is assumed to be the same as the first.
If the first flag is omitted, it is assumed to be `g`.
If `g` has not been set, it's value is the most recent value of `n`.

The history of field modifications is retained, and a specific version can be
accessed with an index. Only the first field can be indexed. The second field
is always assumed to the latest. Indices can be negative, as in python.

Finally, the `+` flag provides means for string concatenation. Both field flags
must be specified for this. `+` before/after the second flag causes the
(modified) value of the first field to be prepended/appended to the second
field.

### [Boost regex](http://www.boost.org/libs/regex) syntax
* [Regex](
   http://www.boost.org/libs/regex/doc/html/boost_regex/syntax/perl_syntax.html)
* [Format string](
   http://www.boost.org/libs/regex/doc/html/boost_regex/format/boost_format_syntax.html)
* [Captures](
   http://www.boost.org/libs/regex/doc/html/boost_regex/captures.html)

### Expressions

The expressions field contains zero or more expressions.
An expression may be a function call.
Multiple expressions need to be enclosed in mathcing `{}` and separated by `;`.

Functions modify matched histograms in different ways, e.g. set line colors,
rebin, normalize, etc.

*Example*: `d/central/line_color 2` makes lines of histograms in the directory
path containing string "central" red.

List of commands with arguments ([] signify optional arguments):
* `norm [1]` normalize histogram
* `scale double [string]` scale histogram. Second argument can be `width` to
  divide by bin width
* `line_color int` set line color
