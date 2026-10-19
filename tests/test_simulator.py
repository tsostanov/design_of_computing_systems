from pathlib import Path
import os
import re
import subprocess
import unittest
from simulator import model
from simulator.model import Board, CODES, DECODE
from simulator.transport import exchange

ROOT = Path(__file__).resolve().parents[1]

def drain(board):
    result = bytes(board.tx)
    board.consume_output(len(result))
    return result

def press(board, start, duration):
    board.update(start, True)
    board.update(start + 30, True)
    board.update(start + duration, False)
    board.update(start + duration + 30, False)

class SimulatorTests(unittest.TestCase):
    def test_alphabet_reference(self):
        expected = ".- -... -.-. -.. . ..-. --. .... .. .--- -.- .-.. -- -. --- .--. --.- .-. ... - ..- ...- .-- -..- -.-- --..".split()
        self.assertEqual(list(CODES.values()), expected)
        for c, code in zip("ABCDEFGHIJKLMNOPQRSTUVWXYZ", expected):
            self.assertEqual(DECODE[code], c)

    def test_configuration_matches_firmware(self):
        source = (ROOT / "Core/Inc/lab_config.h").read_text()
        names = {
            "BUTTON_DEBOUNCE_MS": "DEBOUNCE_MS", "BUTTON_LONG_PRESS_MS": "LONG_PRESS_MS",
            "MORSE_FEEDBACK_MS": "FEEDBACK_MS", "MORSE_END_PAUSE_MS": "END_PAUSE_MS",
            "MORSE_DOT_MS": "DOT_MS", "MORSE_DASH_MS": "DASH_MS",
            "MORSE_SYMBOL_GAP_MS": "SYMBOL_GAP_MS", "MORSE_LETTER_GAP_MS": "LETTER_GAP_MS",
            "MORSE_INPUT_CAPACITY": "INPUT_CAPACITY", "MORSE_LETTER_CAPACITY": "LETTER_CAPACITY",
            "MORSE_RESULT_CAPACITY": "RESULT_CAPACITY", "UART_RX_CAPACITY": "RX_CAPACITY",
            "UART_TX_CAPACITY": "TX_CAPACITY",
        }
        for c_name, py_name in names.items():
            value = int(re.search(r"#define\s+" + c_name + r"\s+(\d+)U", source)[1])
            self.assertEqual(value, getattr(model, py_name), c_name)

    def test_protocol_matches_real_c_application(self):
        exe = os.environ.get("FIRMWARE_TEST_EXE")
        if not exe:
            self.skipTest("Run python tests/run_tests.py for the C/Python comparison")
        expected = bytes.fromhex(subprocess.check_output([exe, "protocol"], text=True).strip())
        b = Board()
        b.update(0)
        output = bytearray(drain(b))
        for byte in b"a\r\n?+b+":
            b.feed(bytes([byte]))
            for _ in range(3):
                b.update(0)
                output.extend(drain(b))
        self.assertEqual(bytes(output), expected)

    def test_queue_capacity_and_fifo(self):
        b = Board()
        b.feed(b"ETEEEEEEEE")
        b.update(0)
        self.assertEqual(b.current, "E")
        self.assertEqual("".join(b.queue), "TEEEEEEE")
        output = drain(b)
        self.assertEqual(output.count(b"transmitting symbol:"), 9)
        self.assertEqual(output.count(b"error: letter queue full"), 1)
        b.update(199)
        self.assertTrue(b.green)
        b.update(200)
        self.assertFalse(b.green)
        b.update(799)
        self.assertFalse(b.green)
        b.update(800)
        self.assertEqual(b.current, "T")
        self.assertTrue(b.green)
        b.update(1399)
        self.assertTrue(b.green)
        b.update(1400)
        self.assertFalse(b.green)

    def test_button_while_transmitting_and_holding(self):
        b = Board()
        b.feed(b"T")
        b.update(0)
        drain(b)
        press(b, 0, 100)
        self.assertEqual(b.feedback, ".")
        self.assertTrue(b.green)
        b.update(500, True)
        b.update(530, True)
        b.update(5000, True)
        self.assertNotIn(b"received symbol", drain(b))
        self.assertFalse(b.green)
        b.update(5100, False)
        b.update(5130, False)
        self.assertEqual(b.input, ".-")
        b.update(7129)
        self.assertNotIn(b"received symbol", drain(b))
        b.update(7130)
        self.assertIn(b"received symbol: A", drain(b))

    def test_unknown_overflow_and_recovery(self):
        for count in (5, 9):
            b = Board()
            for i in range(count):
                press(b, i * 200, 100)
            b.update((count - 1) * 200 + 2130)
            error = b"invalid morse sequence" if count == 5 else b"morse input overflow"
            self.assertIn(error, drain(b))
        press(b, 6000, 100)
        b.update(8130)
        self.assertIn(b"received symbol: E", drain(b))

    def test_tick_wrap_and_short_press_boundary(self):
        b = Board()
        start = 0xFFFFFFFF - 100
        b.feed(b"E")
        b.update(start)
        drain(b)
        b.update(98)
        self.assertTrue(b.green)
        b.update(99)
        self.assertFalse(b.green)
        press(b, 0xFFFFFFFF - 50, 100)
        b.update((0xFFFFFFFF - 50 + 2130) & 0xFFFFFFFF)
        self.assertIn(b"received symbol: E", drain(b))

    def test_rx_overflow_and_output_backpressure(self):
        b = Board(tx_capacity=64)
        b.feed(b"!" * 300)
        self.assertEqual(len(b.rx), 256)
        self.assertEqual(b.rx_overflow, 44)
        b.update(0)
        first = drain(b)
        self.assertIn(b"rx_overflow=44", first)
        remaining = len(b.rx)
        self.assertGreater(remaining, 0)
        for _ in range(300):
            b.update(0)
            drain(b)
        self.assertEqual(len(b.rx), 0)
        self.assertEqual(b.rx_overflow, 44)

    def test_switch_preserves_animation_and_queue(self):
        b = Board()
        b.feed(b"ABC")
        b.update(0)
        drain(b)
        b.feed(b"+")
        b.update(100)
        b.update(101)
        self.assertFalse(b.interrupts)
        self.assertEqual(b.current, "A")
        self.assertEqual("".join(b.queue), "BC")
        self.assertIn(b"+\r\ninterrupts off\r\n", drain(b))
        b.feed(b"+")
        b.update(102)
        b.update(103)
        self.assertTrue(b.interrupts)

    def test_partial_serial_writes_keep_suffix(self):
        class Port:
            in_waiting = 0
            written = bytearray()
            def write(self, data):
                amount = min(len(data), 3)
                self.written.extend(data[:amount])
                return amount
        p, b = Port(), Board()
        b.feed(b"a\r\n?")
        for _ in range(100):
            exchange(p, b, 0)
        expected = (b"\r\ninterrupts on\r\na\r\ntransmitting symbol: A\r\n"
                    b"\r\n?\r\nerror: unsupported input\r\n")
        self.assertEqual(bytes(p.written), expected)
        self.assertEqual(b.tx, b"")

    def test_gui_controls_and_led_rendering(self):
        import tkinter as tk
        from simulator.gui import SimulatorWindow
        try:
            root = tk.Tk()
        except tk.TclError:
            self.skipTest("Tk display unavailable")
        root.withdraw()
        class Port:
            port = "TEST"
            def close(self): pass
        window = SimulatorWindow(root, Port(), lambda text: None)
        try:
            self.assertIn("вариант 3", root.title())
            self.assertIn("Логический режим", window.status.get())
            window._key_press()
            window._key_press()
            self.assertTrue(window.key_down)
            window._key_release()
            window._key_press()
            root.update_idletasks()
            self.assertTrue(window.key_down)
            window._focus_out()
            root.update_idletasks()
            self.assertFalse(window.key_down)
            window._mouse_press()
            self.assertTrue(window.mouse_down)
            window._mouse_release()
            self.assertFalse(window.mouse_down)
            window.board.green = True
            window.board.feedback = "-"
            window.render()
            self.assertEqual(window.canvas.itemcget(window.lights["green"], "fill"), "#28a745")
            self.assertEqual(window.canvas.itemcget(window.lights["red"], "fill"), "#dc3545")
        finally:
            window.close()

if __name__ == "__main__":
    unittest.main()
