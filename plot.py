import numpy as np
from struct import iter_unpack

h = np.fromiter(
    (i[0] for i in iter_unpack('I', open('hist.bin', 'rb').read())),
    int)
x = np.linspace(0, 100, len(h))

import matplotlib.pyplot as plt
plt.bar(x, h)
plt.show()

