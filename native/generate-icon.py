"""Regenerate the Windows icon from LOGO/miniatura.png (requires Pillow)."""
from pathlib import Path
from PIL import Image

root = Path(__file__).resolve().parent
source = Image.open(root.parent / "LOGO" / "miniatura.png").convert("RGBA")
# Fit the complete brand image in a transparent square without distortion.
side = max(source.size)
canvas = Image.new("RGBA", (side, side), (0, 0, 0, 0))
canvas.alpha_composite(source, ((side - source.width) // 2, (side - source.height) // 2))
canvas.save(root / "assets" / "pli-app.ico", sizes=[(n, n) for n in (16, 20, 24, 32, 40, 48, 64, 128, 256)])
