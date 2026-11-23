"""Bake authored Godot wall clues into readable native texture/mesh assets.
These are technical exports of existing text, not new story content.
"""
from pathlib import Path
import hashlib,json
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parents[1]
RAW=ROOT/"unreal/EchoWard/RawAssets/Campaign"
data=json.loads((RAW/"campaign.json").read_text(encoding="utf-8"))
mesh_id="campaign_sign_quad"
quad=RAW/"Meshes"/f"SM_{mesh_id}.obj"
quad.write_text("v 0 -0.5 -0.5\nv 0 0.5 -0.5\nv 0 0.5 0.5\nv 0 -0.5 0.5\nvt 1 0\nvt 0 0\nvt 0 1\nvt 1 1\nvn -1 0 0\nf 1/1/1 4/4/1 3/3/1\nf 1/1/1 3/3/1 2/2/1\n")
data["meshes"]=[m for m in data["meshes"] if m["id"]!=mesh_id]+[{"id":mesh_id,"file":str(quad.relative_to(RAW)).replace("\\","/")}]
data["instances"]=[i for i in data["instances"] if not i.get("sign")]
for row in data.get("signs",[]):
    text=row["text"];sid="sign_"+hashlib.sha256((text+str(row["font_size"])).encode()).hexdigest()[:16]
    font=ImageFont.truetype(str(ROOT/"assets/ui/fonts/NotoSans.ttf"),row["font_size"]*2)
    box=ImageDraw.Draw(Image.new("RGBA",(1,1))).multiline_textbbox((0,0),text,font=font,spacing=8,align="center")
    width,height=int(box[2]-box[0]+20),int(box[3]-box[1]+20)
    im=Image.new("RGBA",(width,height),(0,0,0,0))
    color=tuple(int(max(0,min(1,c))*255) for c in row["color"])
    ImageDraw.Draw(im).multiline_text((10-box[0],10-box[1]),text,font=font,fill=color,spacing=8,align="center")
    file=RAW/"Textures"/f"T_{sid}.png";im.save(file)
    if not any(t["id"]==sid for t in data["textures"]):data["textures"].append({"id":sid,"source":"authored_label","file":str(file.relative_to(RAW)).replace("\\","/"),"width":width,"height":height})
    material={"id":sid,"color":[1,1,1,1],"roughness":1,"metallic":0,"uv":[1,1],"albedo":sid,"normal":"","roughness_texture":"","normal_scale":0,"roughness_channel":0,"masked":False,"emission":[0,0,0],"emission_energy":0,"organic":False,"wetness":0,"sign":True}
    if not any(m["id"]==sid for m in data["materials"]):data["materials"].append(material)
    transform=json.loads(json.dumps(row["transform"]));transform["scale"][1]*=width/2*row["pixel_size"]*100;transform["scale"][2]*=height/2*row["pixel_size"]*100
    data["instances"].append({"path":row["path"],"owner":"","mesh":mesh_id,"materials":[sid],"transform":transform,"visible":True,"shadow":False,"sign":True})
(RAW/"campaign.json").write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding="utf-8")
print(f"BAKED {len(data.get('signs',[]))} authored signs")
