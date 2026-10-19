"""Portable application model; all elapsed times are unsigned 32-bit milliseconds."""
from collections import deque

CODES = dict(zip("ABCDEFGHIJKLMNOPQRSTUVWXYZ", (
    ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..",
    ".---", "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.",
    "...", "-", "..-", "...-", ".--", "-..-", "-.--", "--..",
)))
DECODE = {value: key for key, value in CODES.items()}
DEBOUNCE_MS = 30
LONG_PRESS_MS = 600
DOT_MS = 200
DASH_MS = 600
SYMBOL_GAP_MS = 200
LETTER_GAP_MS = 600
FEEDBACK_MS = 200
END_PAUSE_MS = 2000
LETTER_CAPACITY = 8
INPUT_CAPACITY = 8
RESULT_CAPACITY = 8
RX_CAPACITY = 256
TX_CAPACITY = 2048

def elapsed(now, then):
    return (now - then) & 0xFFFFFFFF

class Board:
    def __init__(self, log=None, tx_capacity=TX_CAPACITY):
        self.log = log or (lambda message: None)
        self.tx_capacity = tx_capacity
        self.rx = deque()
        self.tx = bytearray()
        self.queue = deque()
        self.results = deque()
        self.interrupts = True
        self.requested_mode = None
        self.mode_reply_pending = True
        self.rx_overflow = self.reported_rx = 0
        self.lost_results = self.reported_results = 0
        self.raw = self.stable = False
        self.raw_time = self.press_time = 0
        self.input = ""
        self.input_overflow = False
        self.last_input = self.feedback_time = 0
        self.feedback = ""
        self.green = False
        self.current = ""
        self.phase = "idle"
        self.phase_time = self.position = 0
        self.last_leds = (False, "")

    def feed(self, data):
        for byte in data:
            if len(self.rx) < RX_CAPACITY:
                self.rx.append(byte)
            else:
                self.rx_overflow += 1

    def peek_output(self, limit=64):
        return bytes(self.tx[:limit])

    def consume_output(self, count):
        if not 0 <= count <= len(self.tx):
            raise ValueError("Invalid serial write count")
        del self.tx[:count]

    def _write(self, packet):
        if len(self.tx) + len(packet) > self.tx_capacity:
            return False
        self.tx.extend(packet)
        return True

    def _message(self, text):
        return self._write(("\r\n" + text + "\r\n").encode("ascii"))

    def _start(self, letter, now):
        self.current = letter
        self.position = 0
        self.phase = "on"
        self.phase_time = now
        self.green = True
        self.log(f"{now:10d} ms  transmitting LED letter {letter}")

    def _input(self, sign, now):
        if len(self.input) < INPUT_CAPACITY:
            self.input += sign
        else:
            self.input_overflow = True
        self.last_input = self.feedback_time = now
        self.feedback = sign

    def _button(self, now, pressed):
        if pressed != self.raw:
            self.raw = pressed
            self.raw_time = now
        if pressed != self.stable and elapsed(now, self.raw_time) >= DEBOUNCE_MS:
            self.stable = pressed
            if pressed:
                self.press_time = now
            else:
                self._input("-" if elapsed(now, self.press_time) >= LONG_PRESS_MS else ".", now)

    def _morse(self, now):
        if self.feedback and elapsed(now, self.feedback_time) >= FEEDBACK_MS:
            self.feedback = ""
        if self.stable:
            self.last_input = now
        if not self.stable and self.input and elapsed(now, self.last_input) >= END_PAUSE_MS:
            result = 1 if self.input_overflow else DECODE.get(self.input, 0)
            if len(self.results) < RESULT_CAPACITY:
                self.results.append(result)
            else:
                self.lost_results += 1
            self.input = ""
            self.input_overflow = False
        age = elapsed(now, self.phase_time)
        if self.phase == "on":
            code = CODES[self.current]
            duration = DOT_MS if code[self.position] == "." else DASH_MS
            if age >= duration:
                self.green = False
                self.phase_time = now
                self.position += 1
                if self.position < len(code):
                    self.phase = "gap"
                else:
                    self.phase = "letter_gap"
                    self.current = ""
        elif self.phase == "gap" and age >= SYMBOL_GAP_MS:
            self.green = True
            self.phase = "on"
            self.phase_time = now
        elif self.phase == "letter_gap" and age >= LETTER_GAP_MS:
            if self.queue:
                self._start(self.queue.popleft(), now)
            else:
                self.phase = "idle"

    def _serial_command(self, now):
        if self.mode_reply_pending:
            if self.requested_mode is not None:
                return False
            if self._message("interrupts on" if self.interrupts else "interrupts off"):
                self.mode_reply_pending = False
                return True
            return False
        if not self.rx:
            return False
        byte = self.rx[0]
        letter = chr(byte).upper() if 65 <= byte <= 90 or 97 <= byte <= 122 else ""
        accepted = bool(letter) and (self.phase == "idle" or len(self.queue) < LETTER_CAPACITY)
        message = ""
        if letter:
            message = f"transmitting symbol: {letter}" if accepted else "error: letter queue full"
        elif byte not in (43, 13, 10):
            message = "error: unsupported input"
        packet = bytes([byte])
        if message:
            packet += ("\r\n" + message + "\r\n").encode("ascii")
        if not self._write(packet):
            return False
        self.rx.popleft()
        if accepted:
            if self.phase == "idle":
                self._start(letter, now)
            else:
                self.queue.append(letter)
        if byte == 43:
            self.requested_mode = not self.interrupts
            self.mode_reply_pending = True
        return True

    def update(self, now, pressed=False):
        now &= 0xFFFFFFFF
        if self.requested_mode is not None:
            self.interrupts = self.requested_mode
            self.requested_mode = None
        self._button(now, bool(pressed))
        self._morse(now)
        if self.rx_overflow != self.reported_rx:
            if self._message(f"error: uart rx_overflow={self.rx_overflow} line=0 tx=0"):
                self.reported_rx = self.rx_overflow
        if self.lost_results != self.reported_results:
            if self._message(f"error: button result queue overflow ({self.lost_results})"):
                self.reported_results = self.lost_results
        if self.results:
            result = self.results[0]
            message = ("error: invalid morse sequence" if result == 0 else
                       "error: morse input overflow" if result == 1 else
                       f"received symbol: {result}")
            if self._message(message):
                self.results.popleft()
        # Bounded work: keep UI responsive even under a continuous serial flood.
        for _ in range(64):
            if not self._serial_command(now):
                break
        leds = (self.green, self.feedback)
        if leds != self.last_leds:
            self.log(f"{now:10d} ms  green={int(self.green)} "
                     f"yellow={int(self.feedback == '.')} red={int(self.feedback == '-')}")
            self.last_leds = leds
