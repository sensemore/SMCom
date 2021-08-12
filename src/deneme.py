from wired import *
import time

dev = Wired()
ver = dev.get_version(0xFF)
print("Got version:",ver)
print("="*60)
mac = dev.get_mac_adress(0xFF)
print("Got mac:",mac)
print("="*60)
dev.start_batch_measurement(0xFF,"2G",12800,10000)
print("="*60)