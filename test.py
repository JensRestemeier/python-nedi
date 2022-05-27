from PIL import Image
import glob, os, os.path
import nedi

def roundPot(x):
    x = x | (x >> 1)
    x = x | (x >> 2)
    x = x | (x >> 4)
    x = x | (x >> 8)
    x = x | (x >> 16)
    return x - (x >> 1)

def rescale(img):
    # Trim the stretched area from using non-pot texture sizes:
    y_max = img.height - 1
    while y_max > 1:
        match = True
        for x in range(img.width):
            match = img.getpixel((x,y_max)) == img.getpixel((x,y_max-1))
            if not match:
                break
        if not match:
            break
        y_max -= 1

    x_max = img.width - 1
    while x_max > 1:
        match = True
        for y in range(y_max):
            match = img.getpixel((x_max,y)) == img.getpixel((x_max-1,y))
            if not match:
                break
        if not match:
            break
        x_max -= 1

    img = img.crop((0,0,x_max + 1, y_max + 1))

    # scale x8
    for i in range(3):
        img = nedi.scale(img)

    # scale to POT
    img = img.resize((roundPot(img.width), roundPot(img.height)), resample=Image.Resampling.BOX)

    return img

def main():
    for src in glob.glob("JFG-SRC/*.png"):
        # print (src)
        img = Image.open(src)

        img = rescale(img)

        dst = os.path.join("JFG-DST", os.path.basename(src))
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        img.save(dst)

if __name__ == "__main__":
    main()
