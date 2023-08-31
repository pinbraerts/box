import numpy as np
from struct import iter_unpack

w = 1000
v = np.fromiter(
    (i[0] for i in iter_unpack('I', open('velocity.bin', 'rb').read())),
    int)
x = np.linspace(0, w, len(v))

k, p, e, l = np.array(
    list(iter_unpack('ffff', open('energy.bin', 'rb').read()))).transpose()
t = np.arange(len(k))



import matplotlib.pyplot as plt
fig, ax = plt.subplots()
ax.bar(x, v, label='velocity', width=w / len(v))
#ax.plot(t, k, marker='+', linestyle='none', label='kinetic')
#ax.plot(t, p, marker='+', linestyle='none', label='potential')
#ax.plot(t, e, marker='+', linestyle='none', label='full')
#ax.plot(t, l, marker='+', linestyle='none', label='lagrangian')
plt.legend()
plt.show()

