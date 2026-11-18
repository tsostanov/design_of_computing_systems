import argparse
import sys
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description="ЛР 3, вариант 3: электронный уровень")
    parser.add_argument("port", nargs="?", help="COM-порт стенда, например COM5")
    parser.add_argument("--log", type=Path, default=Path("level-uart.jsonl"), help="Журнал всех байтов RX/TX")
    parser.add_argument("--demo", action="store_true", help="Показать интерфейс с искусственными данными")
    args = parser.parse_args()
    if not args.port and not args.demo:
        parser.error("Укажи COM-порт или --demo")
    try:
        import tkinter as tk
        from tkinter import messagebox
        from .gui import LevelWindow
        from .transport import SerialLink
        root = tk.Tk()
        root.withdraw()
        link = None
        if not args.demo:
            try:
                link = SerialLink(args.port, args.log)
            except Exception as error:
                messagebox.showerror("Не удалось подключиться", str(error), parent=root)
                root.destroy()
                return 1
        LevelWindow(root, link, args.demo)
        root.deiconify()
        root.mainloop()
        return 0
    except ImportError as error:
        print("Не хватает зависимости: " + str(error), file=sys.stderr)
        print("Установи Python с Tkinter и pip install -r client/requirements.txt", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
