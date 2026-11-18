import json
from pathlib import Path
import tempfile
import unittest
from client.protocol import Frame, LineDecoder, parse_frame
from client.transport import SerialLink


class ProtocolTests(unittest.TestCase):
    def test_exact_firmware_data(self):
        self.assertEqual(parse_frame("DATA 0 0 ARROW 1000 0 0 -18 0 1 1"),
                         Frame(0, 0, "ARROW", (1000, 0, 0), -18, 0, True, True))

    def test_bad_frames(self):
        for line in ("DATA 0", "DATA 1 0 BAD 0 0 1000 18 0 1 1",
                     "DATA 1 0 HORIZ 0 0 1000 19 0 1 1",
                     "DATA 1 0 HORIZ 0 0 1000 18 0 2 1",
                     "DATA -1 0 HORIZ 0 0 1000 18 0 1 1",
                     "DATA 1 0 HORIZ 0 0 99999 18 0 1 1"):
            with self.subTest(line=line), self.assertRaises(ValueError):
                parse_frame(line)

    def test_fragmentation_crlf(self):
        decoder = LineDecoder()
        self.assertEqual(decoder.feed(b"PO"), [])
        self.assertEqual(decoder.feed(b"NG\r\nOK\n"), ["PONG", "OK"])

    def test_resynchronizes_after_overlong_and_non_ascii(self):
        decoder = LineDecoder(8)
        self.assertEqual(decoder.feed(b"x" * 10000 + b"\nPING\n"), ["PING"])
        self.assertEqual(decoder.feed(b"MO\xffDE\nGET\n"), ["GET"])
        self.assertEqual(decoder.buffer, b"")


class FakeSerial:
    def __init__(self, *args, **kwargs):
        self.written = bytearray()
        self.received = bytearray(b"PONG LEVEL3 1 LSM9DS0\r\n")
        self.closed = False
        self.limit = 2
        self.options = kwargs

    @property
    def in_waiting(self):
        return len(self.received)

    def write(self, data):
        count = min(self.limit, len(data))
        self.written.extend(data[:count])
        return count

    def read(self, count):
        result = bytes(self.received[:count])
        del self.received[:count]
        return result

    def close(self):
        self.closed = True


class TransportTests(unittest.TestCase):
    def test_partial_writes_and_log_has_every_actual_byte(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "uart.jsonl"
            link = SerialLink("COM99", path, factory=FakeSerial)
            link.send("PING")
            self.assertEqual(link.poll(), b"PONG LEVEL3 1 LSM9DS0\r\n")
            while link.pending:
                link.poll()
            self.assertEqual(link.serial.written, b"PING\n")
            self.assertEqual(link.serial.options["timeout"], 0)
            self.assertEqual(link.serial.options["write_timeout"], 0)
            link.close()
            records = [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines()]
            self.assertTrue(all(record["time"] for record in records))
            tx = b"".join(bytes.fromhex(r["hex"]) for r in records if r["direction"] == "TX")
            rx = b"".join(bytes.fromhex(r["hex"]) for r in records if r["direction"] == "RX")
            self.assertEqual(tx, b"PING\n")
            self.assertEqual(rx, b"PONG LEVEL3 1 LSM9DS0\r\n")

    def test_zero_write_keeps_pending_and_capacity_is_bounded(self):
        with tempfile.TemporaryDirectory() as directory:
            link = SerialLink("COM99", Path(directory)/"log", factory=FakeSerial)
            link.serial.limit = 0
            link.send("PING")
            link.poll()
            self.assertEqual(link.pending, b"PING\n")
            with self.assertRaises(BufferError):
                link.send("X" * 4096)
            link.close()


class GuiTests(unittest.TestCase):
    def setUp(self):
        import tkinter as tk
        from client.gui import LevelWindow
        try:
            self.root = tk.Tk()
        except tk.TclError:
            self.skipTest("Tk display unavailable")
        self.root.withdraw()
        class Link:
            def __init__(self): self.commands = []
            def send(self, command): self.commands.append(command)
            def poll(self): return b""
            def close(self): pass
        self.link = Link()
        self.window = LevelWindow(self.root, self.link)

    def tearDown(self):
        self.window.close()

    def test_ping_and_mode_require_board_confirmation(self):
        self.assertEqual(self.link.commands, ["PING"])
        self.window.set_mode("ARROW")
        self.assertEqual(self.link.commands, ["PING"])
        self.window.handle("PONG LEVEL3 1 LSM9DS0", 1.0)
        self.assertEqual(self.link.commands[-1], "GET")
        self.window.set_mode("ARROW")
        self.assertEqual(self.link.commands[-1], "MODE ARROW")
        self.assertIsNone(self.window.last_frame)
        self.window.handle("OK MODE ARROW", 1.1)
        self.assertIsNone(self.window.pending_mode)
        self.window.handle("DATA 1 100 ARROW 0 1000 0 0 18 1 1", 1.2)
        self.root.update_idletasks()
        self.assertEqual(self.window.last_frame.mode, "ARROW")
        self.assertIn("+1.000", self.window.axes[1].get())

    def test_sensor_error_does_not_show_stale_values(self):
        self.window.handle("DATA 1 100 HORIZ 0 0 0 0 0 0 1", 1.0)
        self.assertEqual(self.window.axes[0].get(), "X: —")


if __name__ == "__main__":
    unittest.main()
