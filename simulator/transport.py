"""Nonblocking serial transport. Unsent suffixes remain in the board's TX queue."""
def exchange(port, board, now, pressed=False):
    waiting = port.in_waiting
    if waiting:
        board.feed(port.read(min(waiting, 256)))
    board.update(now, pressed)
    data = board.peek_output(64)
    if data:
        written = port.write(data)
        if not 0 <= written <= len(data):
            raise OSError("Invalid write result from serial port")
        board.consume_output(written)
