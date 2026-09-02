from pymodbus.client import ModbusSerialClient
c = ModbusSerialClient(port="COM3", baudrate=9600, bytesize=8,
                       parity="N", stopbits=1, timeout=1)
c.connect()
r = c.read_input_registers(0, 1, slave=1)
print("Antwort:", r.registers if not r.isError() else r)
c.close()
