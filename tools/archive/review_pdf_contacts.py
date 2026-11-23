from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
root=Path(__file__).resolve().parents[1];p=root/'tmp/pdfs/hospital_review_v4/render'
files=sorted(p.glob('page-*.png'))
font=ImageFont.truetype(str(root/'unreal/EchoWard/Content/CampaignData/Fonts/NotoSans.ttf'),18)
for block in range((len(files)+19)//20):
    im=Image.new('RGB',(1640,1650),'#183139');draw=ImageDraw.Draw(im)
    for i,src in enumerate(files[block*20:block*20+20]):
        a=Image.open(src).convert('RGB');a.thumbnail((395,295));x=(i%4)*410;y=(i//4)*330
        im.paste(a,(x,y+27));draw.text((x+5,y+3),src.stem,font=font,fill='white')
    im.save(p.parent/f'contact_{block+1}.png')
print('Contact sheets:',(len(files)+19)//20)
