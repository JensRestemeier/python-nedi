import _nedi
from PIL import Image
from enum import IntEnum

class WrapMode(IntEnum):
	CLAMP = 0,
	WRAP = 1,
	MIRROR = 2,

def nedi(src_img : Image, kernel=3, wrap_x=WrapMode.CLAMP, wrap_y=WrapMode.CLAMP) -> Image:
    if src_img.mode != "RGBA":
        src_img = src_img.convert("RGBA")

    dest, dest_width, dest_height = _nedi.py_nedi(src_img.tobytes(), src_img.width, src_img.height, kernel, wrap_x, wrap_y)

    return Image.frombytes("RGBA", (dest_width, dest_height), dest)
