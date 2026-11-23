from PIL import Image
import numpy as np
from pathlib import Path
p=Path('D:/Projects/ECHO/artifacts/echo_hero_v38/realityscan_pilot_v1/output/masks')
for name in ['photo_5409270847438003749_y.jpg.mask.png','photo_5409270847438003750_y.jpg.mask.png']:
 a=np.array(Image.open(p/name))
 print(name)
 for y in range(180,341,5):
  xs=np.where(a[y]>127)[0]
  print(y, int(xs.min()) if len(xs) else None, int(xs.max()) if len(xs) else None)

