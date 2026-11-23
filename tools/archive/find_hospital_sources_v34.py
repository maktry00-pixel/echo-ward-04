from pathlib import Path
import urllib.request,urllib.parse,re,json,concurrent.futures
R=Path('D:/Projects/ECHO');O=R/'assets/external/hospital_v34';terms=['hospital','medical','laboratory','morgue','door','window','wall','ceiling','radiator','generator','ventilation','supermarket','sink','locker','cabinet']
def fetch(term):
 url='https://www.innerscene.com/tools/library/3d-parts?'+urllib.parse.urlencode({'q':term,'format':'glb'})
 with urllib.request.urlopen(urllib.request.Request(url,headers={'User-Agent':'EchoWardAssetReview/34'}),timeout=70) as f:s=f.read().decode('utf-8')
 p=json.loads(re.search(r'<script id="__NEXT_DATA__"[^>]*>(.*?)</script>',s)[1])['props']['pageProps'];(O/('catalog_'+term+'.json')).write_text(json.dumps(p,ensure_ascii=False,indent=2));return term,p
allparts={}
with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:
 for term,p in pool.map(fetch,terms):
  print(term,p['total'],[(a['title'],a['size']) for a in p['parts'][:8]],flush=True)
  for a in p['parts']:allparts[a['id']]=a
(O/'catalog_combined.json').write_text(json.dumps(list(allparts.values()),ensure_ascii=False,indent=2));print('UNIQUE',len(allparts))