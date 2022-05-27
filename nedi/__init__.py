import _nedi
from PIL import Image

def scale(src_img : Image) -> Image:
    if src_img.mode != "RGBA":
        src_img = src_img.convert("RGBA")

    dest, dest_width, dest_height = _nedi.py_nedi(src_img.tobytes(), src_img.width, src_img.height)

    return Image.frombytes("RGBA", (dest_width, dest_height), dest)
