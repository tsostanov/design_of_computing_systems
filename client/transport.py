"""Nonblocking serial transport. The log contains every actual RX/TX chunk."""
from datetime import datetime
import json


class SerialLink:
    def __init__(self, port, log_path, baud=115200, factory=None):
        self.log = open(log_path, "a", encoding="utf-8", buffering=1)
        try:
            if factory is None:
                import serial
                factory = serial.Serial
            self.serial = factory(port, baudrate=baud, timeout=0, write_timeout=0)
        except Exception:
            self.log.close()
            raise
        self.pending = bytearray()

    def record(self, direction, data):
        self.log.write(json.dumps({"time": datetime.now().astimezone().isoformat(timespec="milliseconds"),
                                   "direction": direction,
                                   "hex": data.hex(),
                                   "ascii": data.decode("ascii", errors="backslashreplace")},
                                  ensure_ascii=False) + "\n")

    def send(self, command):
        data = (command + "\n").encode("ascii")
        if len(self.pending) + len(data) > 4096:
            raise BufferError("Serial output queue is full")
        self.pending.extend(data)

    def poll(self):
        if self.pending:
            # write_timeout=0 can accept only a prefix. Keep the unsent suffix.
            count = self.serial.write(self.pending)
            if count:
                self.record("TX", bytes(self.pending[:count]))
                del self.pending[:count]
        data = self.serial.read(min(self.serial.in_waiting, 4096))
        if data:
            self.record("RX", data)
        return data

    def close(self):
        try:
            self.serial.close()
        finally:
            self.log.close()
