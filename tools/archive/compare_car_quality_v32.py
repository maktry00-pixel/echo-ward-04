"""Prepare faithful before/after comparisons from packaged 1920x1080 captures."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
import shutil,json
R=Path('D:/Projects/ECHO');U=R/'unreal/EchoWard';D=R/'artifacts/echo_car_quality_v32';A=U/'Saved/PackageTests/ArrivalV29/Saved/Prototype/ArrivalCaptures';C=U/'Saved/PackageTests/CarsV31/Saved/Prototype/BookWorldCaptures';(D/'after').mkdir(exist_ok=True)
for name in ['08_interior.png','09_all_panels_open.png']:shutil.copy2(A/name,D/'after'/name)
for name in ['kia-carnival','2025-toyota-camry-xv80-hybrid','parking_horror']:shutil.copy2(C/('world_v31_'+name+'.png'),D/'after'/('world_v31_'+name+'.png'))
font=ImageFont.truetype('C:/Windows/Fonts/arial.ttf',24);canvas=Image.new('RGB',(1920,1120),(10,13,16));draw=ImageDraw.Draw(canvas)
# Native pixel crops; no sharpening, no interpolation, no repainting.
rect=(0,480,960,1000)
for i,version in enumerate(['before','after']):
 im=Image.open(D/version/'08_interior.png').convert('RGB');assert im.size==(1920,1080);canvas.paste(im.crop(rect),(i*960,40));draw.text((i*960+18,5),'До' if i==0 else 'После: исходные текстуры + LOD0',font=font,fill='white')
# The outside panels use identical source frames, reduced only to fit the overview.
for i,version in enumerate(['before','after']):
 im=Image.open(D/version/'09_all_panels_open.png').convert('RGB');im.thumbnail((960,540));canvas.paste(im,(i*960,580))
canvas.save(D/'before_after.jpg',quality=95)
full=Image.open(D/'after/08_interior.png').convert('RGB');full.save(D/'preview.jpg',quality=95)
print('V32 comparisons prepared: native crops, source frames preserved.')
