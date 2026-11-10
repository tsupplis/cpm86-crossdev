#!/usr/bin/env python3
"""Run one editor case with a live PTY and paced keyboard input."""

import errno
import fcntl
import os
import pty
import re
import select
import subprocess
import sys
import termios
import time


def console_session():
    """Make the slave PTY the emulator's controlling terminal."""
    os.setsid()
    fcntl.ioctl(0, termios.TIOCSCTTY, 0)


def main():
    """Keep terminal input open until the editor exits; retain console output."""
    log, keys, emulator, image, *arguments = sys.argv[1:]
    with open(keys, "rb") as source:
        keyboard = source.read()
    master, slave = pty.openpty()
    process = subprocess.Popen(
        [emulator, image, *arguments], stdin=slave, stdout=slave, stderr=slave,
        preexec_fn=console_session
    )
    os.close(slave)
    deadline = time.monotonic() + 10
    next_key = time.monotonic() + 0.5
    index = 0
    try:
        with open(log, "wb") as output:
            while True:
                now = time.monotonic()
                if now >= deadline:
                    raise TimeoutError("editor did not return to the OS")
                if process.poll() is None and index < len(keyboard) and now >= next_key:
                    os.write(master, keyboard[index:index + 1])
                    index += 1
                    next_key = now + 0.1
                readable, _, _ = select.select([master], [], [], 0.02)
                if readable:
                    try:
                        chunk = os.read(master, 65536)
                    except OSError as error:
                        if error.errno != errno.EIO:
                            raise
                        break
                    if not chunk:
                        break
                    output.write(chunk)
                elif process.poll() is not None:
                    break
        with open(log, "rb") as source:
            text = re.sub(rb"\x1b\[[0-?]*[ -/]*[@-~]", b"", source.read())
        with open(log + ".text", "wb") as output:
            output.write(b" ".join(text.split()))
        return process.wait(timeout=1)
    finally:
        if process.poll() is None:
            process.kill()
            process.wait()
        os.close(master)


if __name__ == "__main__":
    sys.exit(main())
