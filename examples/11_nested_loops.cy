# Nested for loops — multiplication table style output

print("5x5 product grid (row major)")
for row in range(1, 6):
    for col in range(1, 6):
        let product = row * col
        print(row, col, product)
