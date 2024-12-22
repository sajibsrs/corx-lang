# CORN Programming Language

## # Comments
```corn
# Single line comment.

// This initializes the vector with default values.

/**
* A multiline comment block.
* Used to describe complex logic, important details,
* or provide additional context about the code.
*/

# Box Comment:
# This topmost section is for the description of the topic it is intended for.
# -----------------------------------------------------------------------------
# - These lines are for specific points that start with a [-].
# - These can have special keywords to describe parameters, return values, etc.
# - Syntax is similar to other documentation formats.
# - Example provided below.
#
# - @param <type> [tag] Description of the parameter.
# - @return <type> Description of the return value.
# -----------------------------------------------------------------------------
```
## # Variable
```corn
float val;
int num = 10;
```

## # Constant
### # Compile-Time
```corn
uconst float pi = 3.1416;
```
### # Runtime
```corn
const float pi = 3.1416;
```
## # Function
```corn
string walk() {}
string run(string name) {}
```

## # Interface
```corn
interface actions {
    int average() {}
}
```

## # Structure
```corn
struct animal {
    float val;
    int num = 10;
}
```

## Type Definition

### Primitive Types
```corn
type number int;
```

### Aggregate Types

### Composite Types

## # Method
Member variable can be used by their name if there's no naming conflict. Also, `this` keyword is available inside method definition.

```corn
```
## # Constructor
- When simply called like [vec2 pos = vec2()], that calls the constructor. This allocates static memory and return value. Which is same as calling [vec2 pos = vec2]. If there is no constructor, a struct will always provide implicit one. e.g. [location : location () {}].
- When called like [vec2 *pos = new vec2()] dynamic memory gets allocated, and pointer to the memory returned.
- No explicit return value, implicitly returns `this`.

## # Destructor
- No access modifiers.
- No explicit return value.

## # Memory Management

## Code Organization
### Module

## Memory Management
## Concurrency
## Error Handling
