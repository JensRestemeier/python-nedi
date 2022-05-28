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

def upscale(img, steps):
    # apply the algorithm a few times. I noticed the way the image is interpolated introduces a small shift, flipping the image is supposed to counteract that.
    for i in range(steps):
        img = nedi.nedi(img, kernel=3)
        img = img.transpose(Image.Transpose.ROTATE_180)

    if (steps % 2) != 0:
        img = img.transpose(Image.Transpose.ROTATE_180)
    return img

def process(img):
    # Trim the stretched area from using non-pot texture sizes:
    # (This will unfortunately trim intentional padding as well, causing many alignment problems.)
    y_max = img.height - 1
    while y_max > 0:
        match = True
        for x in range(img.width):
            match = img.getpixel((x,y_max)) == img.getpixel((x,y_max-1))
            if not match:
                break
        if not match:
            break
        y_max -= 1

    x_max = img.width - 1
    while x_max > 0:
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
    img = upscale(img, 3)

    # scale down to POT
    img = img.resize((roundPot(img.width), roundPot(img.height)), resample=Image.Resampling.BOX)

    return img

def main():
    # apply to a directory of PNG files
    for src in glob.glob("JFG-SRC/*.png"):
        # print (src)
        img = Image.open(src)

        img = process(img)

        dst = os.path.join("JFG-DST", os.path.basename(src))
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        img.save(dst)

if __name__ == "__main__":
    main()
