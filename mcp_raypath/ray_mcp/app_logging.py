from __future__ import annotations

import logging
import sys
from pathlib import Path


def setup(log_file: str = "mcp_server.log") -> None:
    """Configure the root logger with a stderr stream and a file handler.

    Safe to call multiple times — subsequent calls are no-ops once
    handlers are already attached to the root logger.
    """
    if logging.root.handlers:
        return

    log_dir = Path("../logs")
    log_dir.mkdir(exist_ok=True)

    logging.basicConfig(
        level=logging.DEBUG,
        format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
        handlers=[
            logging.StreamHandler(sys.stderr),
            logging.FileHandler(log_dir / log_file, mode="a", encoding="utf-8"),
        ],
    )
