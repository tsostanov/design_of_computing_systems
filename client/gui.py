"""Tk client: the board remains the source of truth for mode and pointer."""
import math
import time
import tkinter as tk
from tkinter import ttk
from .protocol import Frame, LineDecoder, parse_frame


class LevelWindow:
    def __init__(self, root, link, demo=False):
        self.root, self.link, self.demo = root, link, demo
        self.decoder = LineDecoder()
        self.connected = False
        self.closed = False
        self.failed = False
        self.pending_mode = None
        self.mode_deadline = 0.0
        self.last_data = 0.0
        self.ping_sent = time.monotonic()
        self.ping_deadline = self.ping_sent + 3.0
        self.started = self.ping_sent
        root.title("ЛР 3 · Уровень · SDK-1.1М")
        root.geometry("790x610")
        root.minsize(720, 560)
        outer = ttk.Frame(root, padding=20)
        outer.pack(fill="both", expand=True)
        ttk.Label(outer, text="Электронный уровень", font=("Segoe UI", 21, "bold")).pack(anchor="w")
        subtitle = "Демонстрация без стенда" if demo else "SDK-1.1М · акселерометр LSM9DS · 115200 бит/с"
        ttk.Label(outer, text=subtitle).pack(anchor="w", pady=(3, 12))
        self.status = tk.StringVar(value="Проверка связи: PING…")
        ttk.Label(outer, textvariable=self.status, wraplength=730).pack(anchor="w")
        row = ttk.Frame(outer)
        row.pack(fill="x", pady=15)
        self.axes = [tk.StringVar(value=f"{axis}: —") for axis in "XYZ"]
        for variable in self.axes:
            ttk.Label(row, textvariable=variable, font=("Consolas", 15, "bold"), width=20).pack(side="left", expand=True)
        self.canvas = tk.Canvas(outer, background="#101c2e", highlightthickness=0, height=280)
        self.canvas.pack(fill="both", expand=True)
        self.mode_text = tk.StringVar(value="Режим стенда: —")
        ttk.Label(outer, textvariable=self.mode_text).pack(anchor="w", pady=(10, 4))
        controls = ttk.Frame(outer)
        controls.pack(fill="x")
        self.buttons = []
        for text, mode in (("Горизонталь", "HORIZ"), ("Стрелка вниз", "ARROW")):
            button = ttk.Button(controls, text=text, command=lambda m=mode: self.set_mode(m), state="disabled")
            button.pack(side="left", padx=(0, 10))
            self.buttons.append(button)
        self.ping_button = ttk.Button(controls, text="Проверить связь", command=self.ping)
        self.ping_button.pack(side="right")
        self.detail = tk.StringVar(value="Ожидание показаний…")
        ttk.Label(outer, textvariable=self.detail, wraplength=730).pack(anchor="w", pady=(12, 0))
        self.last_frame = None
        self.canvas.bind("<Configure>", lambda _: self.draw())
        root.protocol("WM_DELETE_WINDOW", self.close)
        if link:
            link.send("PING")
        self.tick()

    def enable(self, enabled):
        for button in self.buttons:
            button.configure(state="normal" if enabled else "disabled")

    def ping(self):
        if self.closed or self.failed:
            return
        if self.link:
            self.link.send("PING")
        self.ping_sent = time.monotonic()
        self.ping_deadline = self.ping_sent + 3.0
        self.status.set("Проверка связи: PING…")

    def set_mode(self, mode):
        if not self.connected or self.pending_mode:
            return
        if self.demo:
            self.demo_mode = mode
            return
        self.link.send("MODE " + mode)
        self.pending_mode = mode
        self.mode_deadline = time.monotonic() + 3.0
        self.enable(False)
        self.status.set("Ожидание подтверждения режима…")

    def handle(self, line, now):
        if line.startswith("PONG LEVEL3 1 "):
            self.connected = True
            self.ping_deadline = 0
            self.last_data = now
            self.status.set("Связь установлена · " + line.split()[-1])
            self.enable(not self.pending_mode)
            self.link.send("GET")
        elif line.startswith("DATA "):
            try:
                frame = parse_frame(line)
            except ValueError as error:
                self.status.set("Ошибка протокола: " + str(error))
                return
            self.show_frame(frame, now)
        elif line.startswith("OK MODE "):
            mode = line[8:]
            if mode == self.pending_mode:
                self.pending_mode = None
                self.enable(self.connected)
                self.status.set("Режим подтверждён стендом")
        elif line.startswith("ERR "):
            self.pending_mode = None
            self.enable(self.connected)
            self.status.set("Стенд сообщил: " + line)

    def show_frame(self, frame, now):
        self.last_frame, self.last_data = frame, now
        for axis, variable, value in zip("XYZ", self.axes, frame.axes):
            variable.set(f"{axis}: {value / 1000:+.3f} g" if frame.sensor_ok else f"{axis}: —")
        self.mode_text.set("Режим стенда: " + ("горизонталь" if frame.mode == "HORIZ" else "стрелка вниз"))
        sensor = "работает" if frame.sensor_ok else "ошибка / нет свежих данных"
        oled = "работает" if frame.oled_ok else "ошибка / инициализация"
        self.detail.set(f"Датчик: {sensor} · OLED: {oled} · Кадр {frame.sequence} · Время стенда {frame.ms} мс")
        self.draw()

    def draw(self):
        canvas = self.canvas
        canvas.delete("all")
        w, h = canvas.winfo_width(), canvas.winfo_height()
        cx, cy = w / 2, h / 2
        radius = min(w * .36, h * .37)
        canvas.create_oval(cx-radius, cy-radius, cx+radius, cy+radius, outline="#44526b", width=2)
        canvas.create_line(cx-radius-15, cy, cx+radius+15, cy, fill="#2b3c54", dash=(4, 5))
        canvas.create_line(cx, cy-radius-15, cx, cy+radius+15, fill="#2b3c54", dash=(4, 5))
        frame = self.last_frame
        if not frame or not frame.sensor_ok:
            canvas.create_text(cx, cy, text="Нет данных датчика", fill="#eebd75", font=("Segoe UI", 15))
            return
        dx, dy = frame.dx * radius / 18, frame.dy * radius / 18
        color = "#63e6bd"
        if frame.mode == "HORIZ":
            canvas.create_line(cx-dx, cy-dy, cx+dx, cy+dy, fill=color, width=5)
        elif dx or dy:
            canvas.create_line(cx, cy, cx+dx, cy+dy, fill=color, width=5, arrow="last", arrowshape=(16, 20, 8))
        else:
            canvas.create_oval(cx-6, cy-6, cx+6, cy+6, fill=color, outline="")
        canvas.create_text(w-12, h-12, anchor="se", text="X →   Y ↑", fill="#a3b3c9", font=("Segoe UI", 10))

    def demo_frame(self, now):
        angle = (now - self.started) * .5
        x, y = int(650 * math.sin(angle)), int(650 * math.cos(angle))
        mode = getattr(self, "demo_mode", "HORIZ")
        norm = math.hypot(x, y)
        dx, dy = ((-x, y) if mode == "ARROW" else (y, x))
        return Frame(int((now-self.started)*10), int((now-self.started)*1000), mode,
                     (x, y, 760), round(dx*18/norm), round(dy*18/norm), True, True)

    def tick(self):
        if self.closed:
            return
        now = time.monotonic()
        try:
            if self.demo:
                self.connected = True
                self.ping_deadline = 0
                self.status.set("Демонстрация · искусственные данные, стенд не подключён")
                self.enable(True)
                self.show_frame(self.demo_frame(now), now)
            else:
                for line in self.decoder.feed(self.link.poll()):
                    self.handle(line, now)
                if self.ping_deadline and now > self.ping_deadline:
                    self.ping_deadline = 0
                    self.connected = False
                    self.enable(False)
                    self.status.set("Нет ответа PONG. Проверь COM-порт, прошивку и запуск процессора.")
                if self.last_data and now - self.last_data > 2.0:
                    self.connected = False
                    self.enable(False)
                    self.status.set("Показания не обновляются. Проверь связь и нажми «Проверить связь».")
                    for axis, variable in zip("XYZ", self.axes):
                        variable.set(f"{axis}: —")
                    self.last_frame = None
                    self.detail.set("Нет свежих данных со стенда")
                    self.draw()
                if self.pending_mode and now > self.mode_deadline:
                    self.pending_mode = None
                    self.enable(self.connected)
                    self.status.set("Нет подтверждения смены режима")
        except Exception as error:
            self.status.set("Ошибка соединения или журнала: " + str(error))
            self.connected = False
            self.failed = True
            self.ping_button.configure(state="disabled")
            self.enable(False)
            self.last_frame = None
            self.draw()
            # Keep the error window visible; release resources immediately.
            try:
                self.link.close()
            except Exception:
                pass
            return
        self.root.after(30, self.tick)

    def close(self):
        if self.closed:
            return
        self.closed = True
        try:
            if self.link:
                self.link.close()
        finally:
            self.root.destroy()
