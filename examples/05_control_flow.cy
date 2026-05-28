# if/ elif / else and nested conditionals

let score = 85

if score >= 90:
    print("grade A")
elif score >= 80:
    print("grade B")
elif score >= 70:
    print("grade C")
else:
    print("grade F")


let logged = true
let role = "admin"

if logged:
    if role == "admin":
        print("welcome admin")
    else:
        print("welcome user")
else:
    print("please log in")

let x = 15
if x > 10 and x < 20:
    print("x is between 10 and 20")
