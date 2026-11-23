"""Open existing corridor side pockets and protect broad basement halls."""
import copy,math

def tighten_escape_geometry(d):
    # Thin narrowing partitions used to leave long side strips accessible only
    # from their ends. Regular narrow apertures make those strips true recesses.
    parts=[];slots=[]
    for p in d['partitions']:
        ax,ay,z=p['a'];bx,by,_=p['b'];horizontal=abs(ay-by)<.001
        start,end=(ax,bx) if horizontal else (ay,by)
        if p['type']!='niche' or end-start<5:parts.append(p);continue
        centres=[];t=start+2
        while t<end-1.5:centres.append(t);t+=6
        cursor=start
        for centre in centres:
            lo,hi=centre-.6,centre+.6
            if lo>cursor:
                q=copy.deepcopy(p);q['a']=[cursor,ay,z] if horizontal else [ax,cursor,z];q['b']=[lo,ay,z] if horizontal else [ax,lo,z];parts.append(q)
            cursor=hi
            # Determine which side contains the main walking centreline.
            site=next(l for l in d['locations'] if l['id']==p['site']);ox,oy,_=site['origin']
            if horizontal:
                cy=min((oy+y+h/2 for x,y,w,h in site['corridors'] if ox+x<=centre<=ox+x+w),key=lambda v:abs(v-ay))
                side='south' if cy>ay else 'north';entry=[centre,ay,z];approach=[centre,cy,z]
            else:
                cx=ox+48
                side='east' if cx>ax else 'west';entry=[ax,centre,z];approach=[cx,centre,z]
            slots.append(dict(site=p['site'],entry=entry,side=side,corridor_point=approach,
                opening_m=.9,large_pass=False,internal=True,slot=True,entry_depth_m=.55,
                rect=[entry[0]-.5,entry[1]-.5,1,1]))
        if cursor<end:
            q=copy.deepcopy(p);q['a']=[cursor,ay,z] if horizontal else [ax,cursor,z];parts.append(q)
    d['partitions']=parts
    # Wide UT basement halls need targets on both long sides, not merely along
    # the centreline. Mirror existing external pockets to the opposite edge.
    halls=[r['rect'] for r in d['infrastructure_zones']]
    mirrored=[]
    for n in d['escape_niches']:
        if n['site']!='UT' or n.get('internal'):continue
        for x,y,w,h in halls:
            ex,ey,z=n['entry']
            if abs(z+4.8)<.01 and x<ex<x+w and abs(ey-y)<.01 and n['side']=='south':
                m=copy.deepcopy(n);m['entry']=[ex,y+h,z];m['side']='north';m['rect']=[ex-1,y+h,2,2];m['corridor_point']=[ex,y+h-2,z];mirrored.append(m)
    # Landings, blind lift backs and the isolation bypass are the remaining
    # dense-grid distance maxima. Give each a real door-sized side refuge.
    special=[]
    def pocket(site,x,y,z,side):
        if side=='east':rect=[x-2,y-1,2,2];approach=[x+1.5,y,z]
        elif side=='south':rect=[x-1,y-2,2,2];approach=[x,y+1.5,z]
        else:raise ValueError(side)
        special.append(dict(site=site,entry=[x,y,z],side=side,rect=rect,corridor_point=approach,opening_m=.9,large_pass=False))
    for l in d['locations']:
        ox,oy,z=l['origin'];site=l['id']
        if site not in ('ST','RF','UT'):pocket(site,ox-8,oy+98,z,'east')
        if site in ('B1','B2','2F','3F','SV'):pocket(site,ox+46,oy,z,'south')
        if site in ('B1','B2','1F','2F','3F'):pocket(site,ox-8,oy+10,z,'east')
    pocket('UT',40,230,-4.8,'east')
    pocket('1F',40,268,0,'south')
    pocket('B2',-8,58,-9.6,'east')
    for r in halls:pocket('UT',r[0]+r[2]-2,r[1],-4.8,'south')
    mirrored += special
    for n in mirrored:
        x,y,w,h=n['rect'];d['passages'].append([x,y,w,h,n['entry'][2]])
    for n in slots+mirrored:
        n['id']=f'NICHE_{len(d["escape_niches"])+1:03}';d['escape_niches'].append(n)
        x,y,z=n['entry'];ns=n['side'] in ('north','south')
        for sign in (-1,1):
            rect=[x+sign*.725-.275,y-.11,.55,.22] if ns else [x-.11,y+sign*.725-.275,.22,.55]
            d['collision_features'].append(dict(id=n['id']+'_jamb_'+str(sign),rect=rect,z=z))
