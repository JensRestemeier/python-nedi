from PIL import Image
import nedi

img = Image.open("test.png")
for i in range(4):
    img = nedi.nedi(img)
img.save("test-nedi.png")