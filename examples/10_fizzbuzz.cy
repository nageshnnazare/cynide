# Classic FizzBuzz from 1 through 100

for n in range(1, 101):
    let div3 = n % 3 == 0
    let div5 = n % 5 == 0
    if div3 and div5:
        print("FizzBuzz")
    elif div3:
        print("Fizz")
    elif div5:
        print("Buzz")
    else:
        print(n)
