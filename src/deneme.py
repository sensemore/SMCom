from wired import *
import time

dev = Wired()
ver = dev.get_version(0xFF)
print("Got version:",ver)
print("="*60)
mac = dev.get_mac_adress(0xFF)
print("Got mac:",mac)
print("="*60)
dev.start_batch_measurement(0xFF,"2G",12800,100)
print("="*60)

byte_offset = 0 # Start from the beginning
data_len = 100 * 6 #Convert to bytes

data = [*tuple(byte_offset.to_bytes(4, "little")),*tuple(data_len.to_bytes(4, "little"))]
print("Sending:",data)
dev.write(0xFF, SMCOM_WIRED_MESSAGES.GET_BATCH_MEASUREMENT_CHUNK.value, data, len(data))
