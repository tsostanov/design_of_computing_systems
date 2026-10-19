import tkinter as tk
from tkinter import ttk
import time
from .model import Board
from .transport import exchange

class SimulatorWindow:
    def __init__(self, root, port, log):
        self.root, self.port, self.log = root, port, log
        self.board = Board(log)
        self.started = time.monotonic()
        self.mouse_down = self.key_down = False
        self.key_release_job = None
        self.focus_job = None
        self.job = None
        self.closed = False
        root.title("SDK-1.1M — ЛР №2, вариант 3")
        root.geometry("620x360")
        root.minsize(560, 340)
        frame = ttk.Frame(root, padding=20)
        frame.pack(fill="both", expand=True)
        ttk.Label(frame, text="Азбука Морзе • UART", font=("Segoe UI", 18, "bold")).pack(anchor="w")
        ttk.Label(frame, text=f"Порт: {port.port}   •   115200, 8N1").pack(anchor="w", pady=(4, 14))
        self.canvas = tk.Canvas(frame, height=80, highlightthickness=0)
        self.canvas.pack(fill="x")
        self.lights = {}
        for x, name, text in [(65, "green", "Передача"), (225, "yellow", "Точка"), (385, "red", "Тире")]:
            self.lights[name] = self.canvas.create_oval(x, 0, x+38, 38, fill="#343b43", outline="")
            self.canvas.create_text(x+19, 58, text=text, font=("Segoe UI", 10))
        self.button = ttk.Button(frame, text="Удерживай кнопку мышью или пробелом")
        self.button.pack(fill="x", pady=(5, 10))
        self.button.bind("<ButtonPress-1>", self._mouse_press)
        root.bind("<ButtonRelease-1>", self._mouse_release)
        root.bind("<KeyPress-space>", self._key_press)
        root.bind("<KeyRelease-space>", self._key_release)
        root.bind("<FocusOut>", self._focus_out)
        self.status = tk.StringVar()
        ttk.Label(frame, textvariable=self.status, justify="left").pack(anchor="w")
        self.error = tk.StringVar()
        ttk.Label(frame, textvariable=self.error, foreground="#b3261e", wraplength=550).pack(anchor="w", pady=(6, 0))
        ttk.Label(frame, text="Буквы и + отправляй из PuTTY. Режим прерываний здесь логический.",
                  wraplength=550).pack(anchor="w", side="bottom", pady=(12, 0))
        root.protocol("WM_DELETE_WINDOW", self.close)
        self.render()

    def _mouse_press(self, _event=None):
        self.mouse_down = True

    def _mouse_release(self, _event=None):
        self.mouse_down = False

    def _key_press(self, _event=None):
        if self.key_release_job is not None:
            self.root.after_cancel(self.key_release_job)
            self.key_release_job = None
        self.key_down = True
        return "break"

    def _key_release(self, _event=None):
        # On systems producing release/press pairs during repeat, the next press
        # cancels this deferred release before it reaches the model.
        if self.key_release_job is not None:
            self.root.after_cancel(self.key_release_job)
        self.key_release_job = self.root.after_idle(self._release_key)
        return "break"

    def _release_key(self):
        self.key_release_job = None
        self.key_down = False

    def _focus_out(self, _event=None):
        if self.focus_job is not None:
            self.root.after_cancel(self.focus_job)
        self.focus_job = self.root.after_idle(self._release_if_unfocused)

    def _release_if_unfocused(self):
        self.focus_job = None
        # Focus moving between widgets in this window is not a release.
        if self.root.focus_displayof() is None:
            self.mouse_down = self.key_down = False

    def render(self):
        b = self.board
        for name, on, color in [("green", b.green, "#28a745"),
                                ("yellow", b.feedback == ".", "#edbf26"),
                                ("red", b.feedback == "-", "#dc3545")]:
            self.canvas.itemconfigure(self.lights[name], fill=color if on else "#343b43")
        self.status.set(
            f"Сейчас: {b.current or '—'}    Очередь ({len(b.queue)}/8): {''.join(b.queue) or '—'}\n"
            f"Ввод кнопкой: {b.input or '—'}\n"
            f"Логический режим: {'прерывания' if b.interrupts else 'опрос'}")

    def tick(self):
        self.job = None
        if self.closed:
            return
        now = int((time.monotonic() - self.started) * 1000) & 0xFFFFFFFF
        try:
            exchange(self.port, self.board, now, self.mouse_down or self.key_down)
        except (OSError, ValueError) as exc:
            self.error.set(f"Обмен остановлен: {exc}. Закрой окно и проверь COM-порт.")
            self.log(f"Serial exchange stopped: {exc}")
            self.port.close()
            return
        self.render()
        self.job = self.root.after(5, self.tick)

    def close(self):
        if self.closed:
            return
        self.closed = True
        if self.job is not None:
            self.root.after_cancel(self.job)
        if self.key_release_job is not None:
            self.root.after_cancel(self.key_release_job)
        if self.focus_job is not None:
            self.root.after_cancel(self.focus_job)
        self.port.close()
        self.root.destroy()
