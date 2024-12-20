# CORN Programming Language

## # Variable
```cmd
float val;
int num = 10;
```

## # Constant
```cmd
const float pi = 3.1416;
```

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

## # `struct` : `resp`
```cmd
struct animal {
    string name;
    int legs = 4;
}

resp actions {
    string walk();
}

animal : actions {
    string walk() {
        return "Walking!";
    }
}
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
