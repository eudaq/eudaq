#
# common functions
#

# convert an integer byte-wise to characters
def its(val, nbytes):
    result = ""
    for byte in range(nbytes-1, -1, -1):
        result += chr((val >> byte*8) & 0xFF)
    return result

# interpret characters in a string as bytes and convert them to an integer
def sti(val, nbytes):
    result = 0x0
    for byte in range(nbytes-1, -1, -1):
        t = ord(val[byte])
        result = result | (t << ((nbytes-byte-1)*8))
    return result

# split a bit vector into a list of ints
def vtl(vector, bitwidth, nValues, out_arr, arr_idx):
    mask = (1 << bitwidth) - 1
    for i in range(nValues):
        t = int(vector & mask)
        out_arr[i][arr_idx] = t
        vector = vector >> bitwidth
