"""Line framing and strict parsing, independent of Tk and serial hardware."""
from dataclasses import dataclass


@dataclass(frozen=True)
class Frame:
    sequence: int
    ms: int
    mode: str
    axes: tuple[int, int, int]
    dx: int
    dy: int
    sensor_ok: bool
    oled_ok: bool


def parse_frame(line: str) -> Frame:
    parts = line.split()
    if len(parts) != 11 or parts[0] != "DATA" or parts[3] not in ("HORIZ", "ARROW"):
        raise ValueError("Invalid DATA message")
    sequence, ms = int(parts[1]), int(parts[2])
    axes = tuple(int(value) for value in parts[4:7])
    dx, dy, sensor, oled = map(int, parts[7:11])
    if (not 0 <= sequence <= 0xFFFFFFFF or not 0 <= ms <= 0xFFFFFFFF
            or any(abs(value) > 2500 for value in axes)
            or abs(dx) > 18 or abs(dy) > 18 or dx*dx + dy*dy > 350
            or sensor not in (0, 1) or oled not in (0, 1)):
        raise ValueError("DATA value outside protocol limits")
    return Frame(sequence, ms, parts[3], axes, dx, dy, bool(sensor), bool(oled))


class LineDecoder:
    """Bound memory even if the peer sends arbitrary data without a newline."""
    def __init__(self, limit=256):
        self.buffer = bytearray()
        self.limit = limit
        self.discard = False

    def feed(self, data: bytes) -> list[str]:
        result = []
        for value in data:
            if value in (10, 13):
                if self.discard:
                    self.discard = False
                elif self.buffer:
                    result.append(self.buffer.decode("ascii"))
                self.buffer.clear()
            elif not self.discard:
                if value < 32 or value > 126 or len(self.buffer) >= self.limit:
                    self.buffer.clear()
                    self.discard = True
                else:
                    self.buffer.append(value)
        return result
