import sys

with open(sys.argv[1], "r") as f:
    data = f.read()
    lines = data.split(";")
    for line in lines[1].split("\n"):
        if "#define CYW43_WIFI_FW_LEN" in line:
            cyw43_wifi_fw_len = int(line.split(")")[0].split("(")[-1])
        if "#define CYW43_CLM_LEN" in line:
            cyw43_clm_len = int(line.split(")")[0].split("(")[-1])
    data = lines[0]
    bits = data.split(",")
    bits[0] = bits[0].split("{")[-1]
    bits[-1] = bits[-1].split("}")[0]
    for i in range(len(bits)):
        bits[i] = bits[i].strip()
        bits[i] = bits[i].strip(',')
        bits[i] = int(bits[i], base=0)
    print(f"Start {bits[4]}, end {bits[-1]}, num {len(bits)}")
    print(bits[:10])

print(f"Wifi {cyw43_wifi_fw_len}, clm {cyw43_clm_len}")

data = (
    cyw43_wifi_fw_len.to_bytes(4, 'little', signed=True) +
    cyw43_clm_len.to_bytes(4, 'little', signed=True) +
    bytearray(bits)
)

with open(sys.argv[2], "w") as f:
    for b in data:
        f.write(f".byte 0x{b:02x}\n")
