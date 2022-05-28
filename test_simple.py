from PIL import Image
import nedi

img_base = Image.open("test.png")
for k in range(2,6):
    img = img_base
    for i in range(4):
        img = nedi.nedi(img, kernel=k)
    img.save("test-nedi-%i.png" % k)