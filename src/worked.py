f = open("../Wiredv1_0_12.bin", "rb")

data = []

while True:
    temp = f.read(240)
    if temp == b'':
        break
    data.append(temp)
