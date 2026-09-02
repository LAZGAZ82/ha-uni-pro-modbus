import serial, time
s = serial.Serial("COM3", 9600, timeout=1)
s.reset_input_buffer()
s.write(bytes([0x01, 0x04, 0x00, 0x00, 0x00, 0x01, 0x31, 0xCA]))
time.sleep(0.3)
data = s.read(20)
print("Echo:", data.hex(" ") if data else "NICHTS empfangen")
s.close()
