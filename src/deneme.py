from wired import *
import time



dev = Wired()
time.sleep(1)

ver = dev.get_version(0xFF)
print("Got version:",ver)
print("-"*60)
mac = dev.get_mac_adress(0xFF)
print("Got mac:",mac)
print("="*60)
sample = 100
#dev.start_batch_measurement(0xFF,"16G",12800,sample)
print("#"*60)
#meas = dev.read_measurement(0xFF,sample)
meas = dev.measure(0xFF,"16G",12800,sample)
telems = dev.get_all_telemetry(0xFF)
