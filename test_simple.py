from PIL import Image
import nedi

img = Image.open("JFG-SRC/JET FORCE GEMINI#2A2A4C16#0#2_all.png")
for i in range(4):
    img = nedi.scale(img)
img.save("test.png")