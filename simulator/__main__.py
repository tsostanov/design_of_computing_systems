import argparse
import logging
from pathlib import Path
import sys
import tkinter as tk
from .gui import SimulatorWindow

def main():
    parser = argparse.ArgumentParser(description="Симулятор UART стенда SDK-1.1M, ЛР №2")
    parser.add_argument("--port", required=True, help="Один конец виртуальной пары, например COM10")
    parser.add_argument("--log", default="simulator.log", help="Файл журнала светодиодов")
    args = parser.parse_args()
    try:
        import serial
    except ImportError:
        print("Установи зависимости: python -m pip install -r simulator/requirements.txt", file=sys.stderr)
        return 1
    logging.basicConfig(level=logging.INFO, format="%(message)s",
                        handlers=[logging.FileHandler(Path(args.log), encoding="utf-8"),
                                  logging.StreamHandler()])
    try:
        port = serial.Serial(port=args.port, baudrate=115200, bytesize=serial.EIGHTBITS,
                             parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE,
                             timeout=0, write_timeout=0, xonxoff=False, rtscts=False, dsrdtr=False)
    except (serial.SerialException, ValueError) as exc:
        print(f"Не удалось открыть {args.port}: {exc}", file=sys.stderr)
        return 1
    try:
        root = tk.Tk()
        window = SimulatorWindow(root, port, logging.info)
        window.tick()
        root.mainloop()
    finally:
        port.close()
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
