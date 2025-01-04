# Lexical Analysis (Scanning)

## Lexeme
The actual text (sequence of characters) in the source code.

Example:
- `int`, `x`, `=`, `10`, `;`

## Token
The category or type assigned to a lexeme by the lexer.

Example:
- `int` -> `KEYWORD`
- `x` -> `IDENTIFIER`
- `10` -> `NUMBER`

## Pattern
The rule or description that defines what a valid lexeme looks like for a particular token. Often written using regular expressions.

Example:
- `KEYWORD`: Matches words like `int`, `float`, `return`, etc.
- `IDENTIFIER`: Matches a sequence of letters and digits starting with a letter (e.g., `[a-zA-Z][a-zA-Z0-9]*`).
- `NUMBER`: Matches a sequence of digits (e.g., `[0-9]+`).

## Methods
### Table-Driven Scanners
### Direct-Coded Scanners
### Hand-Coded Scanners
