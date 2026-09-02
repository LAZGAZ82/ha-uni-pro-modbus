from pymodbus.client import ModbusTcpClient
c = ModbusTcpClient("uni-pro-bridge.local", port=502)
c.connect()
r = c.read_input_registers(0, 1, slave=1)
print("Antwort:", r)
print("exception_code:", getattr(r, "exception_code", None))
c.close()
