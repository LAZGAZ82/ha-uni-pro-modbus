import logging
logging.basicConfig(level=logging.DEBUG)
from pymodbus.server import StartSerialServer
from pymodbus.transaction import ModbusRtuFramer
from pymodbus.datastore import (ModbusSequentialDataBlock,
                                ModbusSlaveContext, ModbusServerContext)

store = ModbusSlaveContext(
    ir=ModbusSequentialDataBlock(0, [123] * 100),
    hr=ModbusSequentialDataBlock(0, [456] * 100),
)
context = ModbusServerContext(slaves={1: store}, single=False)

print("RTU-Slave auf COM3, 9600 8N1, Unit 1 - Strg+C beendet")
StartSerialServer(context=context, framer=ModbusRtuFramer,
                  port="COM3", baudrate=9600,
                  bytesize=8, parity="N", stopbits=1)
