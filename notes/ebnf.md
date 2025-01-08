# Rules of EBNF (Extended Backus-Naur Form)

EBNF (Extended Backus-Naur Form) is used to define the syntax of programming languages and other formal grammars. Below are the core rules:

## 1. **Production Rules**
A production rule defines how a non-terminal symbol can be expanded into other symbols (terminal or non-terminal).
- **Syntax:**
```ebnf
<non-terminal> = <expression> ;
```
- **Example:**
```ebnf
identifier = letter , { letter | digit | "_" } ;
```

## 2. **Terminals**
Terminals are literal values (tokens) in the language, enclosed in quotes.
- **Example:**
```ebnf
"+" | "-" | "if" | "{" | "}" | ";" | "int"
```

## 3. **Concatenation**
Use a comma `,` to indicate sequential elements.
- **Example:**
```ebnf
statement = identifier , "=" , expression , ";" ;
```

## 4. **Alternation (Choice)**
Use a vertical bar `|` to separate alternatives.
- **Example:**
```ebnf
type_specifier = "int" | "float" | "char" ;
```

## 5. **Repetition**
Use `{ }` to indicate zero or more repetitions.
- **Example:**
```ebnf
digits = digit , { digit } ;
```

Use `{ }+` to indicate one or more repetitions (some EBNF variants use this convention).
- **Example:**
```ebnf
digits = digit , { digit }+ ;
```

## 6. **Optional Elements**
Use `[ ]` to indicate that an element is optional (appears zero or one time).
- **Example:**
```ebnf
expression = term , [ "+" , term ] ;
```

## 7. **Grouping**
Use `( )` to group elements and control precedence.
- **Example:**
```ebnf
term = "(" , expression , ")" | literal ;
```

## 8. **Exclusions**
Use `-` to indicate exclusions (elements that should not match).
- **Example:**
```ebnf
character = letter | digit | symbol - "'" ;
```

## 9. **Comments**
Comments in EBNF are often denoted by `(* ... *)` or `// ...`, depending on the tool or standard.
- **Examples:**
```ebnf
(* This is a comment *)
rule = "example" ; // Inline comment
```

## 10. **Special Symbols**
Some EBNF variants use `::=` instead of `=` for definitions.
- **Example:**
```ebnf
identifier ::= letter , { letter | digit | "_" } ;
```

## 11. **Repetition Modifiers (Some Variants)**
- `?`: Zero or one occurrence.
  **Example:**
```ebnf
optional_term = term ? ;
```
- `*`: Zero or more occurrences.
  **Example:**
```ebnf
repeated_term = term * ;
```
- `+`: One or more occurrences.
  **Example:**
```ebnf
non_empty_repeated_term = term + ;
```

## 12. **Escape Sequences**
Special characters can be escaped in terminal definitions to prevent misinterpretation.
**Example:** `"\\n"` for a newline character.

EBNF provides flexibility and a clear structure for defining formal grammars, enabling concise and human-readable descriptions of syntax rules.
