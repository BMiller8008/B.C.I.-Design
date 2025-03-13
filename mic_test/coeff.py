import numpy as np
from scipy.signal import iirfilter

fs = 8000.0
# Example: 2nd-order Butterworth high-pass at 300 Hz
b_hp, a_hp = iirfilter(
    N=2,           # 2nd order
    Wn=300/(fs/2), # normalized cutoff
    btype='highpass',
    ftype='butter'
)
print("High-pass b=", b_hp, " a=", a_hp)

# 2nd-order Butterworth low-pass at 3400 Hz
b_lp, a_lp = iirfilter(
    N=2,
    Wn=3400/(fs/2),
    btype='lowpass',
    ftype='butter'
)
print("Low-pass b=", b_lp, " a=", a_lp)
