# Strings: literals, comparisons, and escape sequences supported by the lexer

let greeting = "Hello"
let target = "Hello"

# Equality checks compare C-style string contents via strcmp lowering.
if greeting == target:
    print("same greeting")
else:
    print("different")

let alt = "World"
if greeting != alt:
    print("greeting is not World")

# Demonstrate escapes: newline, tab, backslash, quote
print("Line one\nLine two")
print("Column\tTabbed")
print("path\\file")
print("She said \"Hi\"")
