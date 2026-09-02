import time
from pymodbus.client import ModbusTcpClient
c = ModbusTcpClient("192.168.1.159", port=502)
c.connect()
print("Dauertest laeuft - Strg+C beendet")
while True:
    r = c.read_input_registers(0, 1, slave=1)
    print("Antwort:", r.registers if not r.isError() else r)
    time.sleep(1)
