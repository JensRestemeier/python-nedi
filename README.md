# python nedi

This is my implementation of the New Edge-Directed Interpolation algorithm:

```
@ARTICLE{Li01newedge-directed,
    author = {Xin Li and Michael T. Orchard},
    title = {New Edge-Directed Interpolation},
    journal = {IEEE Transactions on Image Processing},
    year = {2001},
    volume = {10},
    pages = {1521--1527}
}
```

My code for this is ancient - I am not sure why I made some of the technical decisions, but it seems to work OK.

A quick example:
```python
from PIL import Image
import nedi

img = Image.open("test.png")
for i in range(4):
    img = nedi.nedi(img)
img.save("test-nedi.png")
```
