# CORN Programming Language

## # Variable
```cmd
float val;
int num = 10;
```

## # Constant
### # Compile-Time
```cmd
const float pi = 3.1416;
```
### # Runtime

## # Function
```cmd
string walk() {}
string run(string name) {}
```

## # Responsibility / Interface
```cmd
resp actions {
    int average() {}
}
```

## # Structure
```cmd
struct animal {
    float val;
    int num = 10;
}
```

## # Method
Member variable can be used by their name if there's no naming conflict. Also, `this` keyword is available inside method definition.

**Prototype: #1**
```cmd
struct vec2 {
    int x = 1;
    int y = 2;
}

# Methods
vec2 : {
    vec2() {}

    int sum() {
        return x + y;
    }
}
```

**Prototype: #2**
```cmd
struct vec2 {
    int x = 1;
    int y = 2;
}

# Methods
vec2 : vec2() {}
vec2 : int sum() {
    return x + y;
}
```

## # Responsibility / Interface (`resp`)
**Prototype: #1**
```cmd
struct vec2 {
    int x = 1;
    int y = 2;
}

resp calculations {
    int sum();
}

resp actions {
    vec2 *add(int val);
}

# Implementation
vec2 : calculations, actions {
    int sum() {
        return x + y;
    }

    vec2 *add(int val) {
        x = x + val;
        y = y + val;

        return this;
    }
}

# Uses
vec2 v = vec2;

# Everything is public by default
int x = v.x; // 1
int y = v.y; // 2

int sum = v.sum(); // 3
vec2 *p = v.add(10); // now, v = {11, 12}
```

**Prototype: #2**
```cmd
struct vec2 {
    int x = 1;
    int y = 2;
}

resp calc {
    int sum();
}

resp act {
    vec2 *add(int val);
}

# Implementation
vec2 <- calc, act; // Implements

vec2 : int sum() {
    return x + y;
}

vec2 : vec2 *add(int val) {
    x = x + val;
    y = y + val;

    return this;
}

# Uses
vec2 v = vec2;

# Everything is public by default
int x = v.x; // 1
int y = v.y; // 2

int sum = v.sum(); // 3
vec2 *p = v.add(10); // now, v = {11, 12}
```

## Type Definition

### Primitive Types
```cmd
type number int;
```

### Aggregate Types

### Composite Types
## Code Organization
### Module

## Memory Management
## Concurrency
## Error Handling
