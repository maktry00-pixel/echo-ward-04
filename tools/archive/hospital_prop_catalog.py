"""Metre-sized, deliberately simple kit for the numbered furnishing schedule.

Parts use box centres, sizes, material keys and collision flags. These are the
same data used for placement bounds and instanced geometry in the real game.
No downloaded art or cooked engine assets are required.
"""
MATERIALS = {
    'steel': [.28,.34,.35], 'white': [.72,.76,.70], 'wood': [.27,.14,.065],
    'cloth': [.16,.29,.28], 'dark': [.025,.035,.038], 'red': [.42,.045,.035],
    'blue': [.035,.16,.28], 'green': [.08,.22,.065], 'paper': [.71,.66,.51],
    'glass': [.12,.32,.35], 'light': [.65,.87,.82], 'yellow': [.65,.45,.055],
}

def make_catalog(names):
    result={}
    for number, name in names.items():
        n=int(number)
        if n>=188 or n in (160,161,162,164,165,166,167,168,169,170,171,172):continue
        parts=[]; mount='floor'; support=[]; top=0
        def box(x,y,z,w,d,h,mat='steel',solid=True):
            parts.append([x,y,z,w,d,h,mat,solid])
        def body(w,d,h,mat='steel'):box(0,0,h/2,w,d,h,mat)
        def legs(w,d,h):
            for x in (-w/2+.05,w/2-.05):
                for y in (-d/2+.05,d/2-.05):box(x,y,h/2,.05,.05,h)
        def wheels(w,d):
            for x in (-w/2+.06,w/2-.06):
                for y in (-d/2+.08,d/2-.08):box(x,y,.09,.13,.15,.18,'dark')
        def shelf(w=1.1,d=.42,h=1.9,mat='steel'):
            for x in (-w/2+.025,w/2-.025):box(x,0,h/2,.05,d,h,mat)
            box(0,-d/2+.015,h/2,w,.03,h,mat)
            for z in (.12,.55,.98,1.41,h-.05):box(0,0,z,w,d,.045,mat)
        def screen(x=0,y=0,z=1.25,w=.42):
            box(x,y,z,w,.10,w*.67,'dark');box(x,y+.056,z,w*.86,.015,w*.52,'glass',False)
            box(x,y-.02,z-.26,.07,.08,.25);box(x,y,z-.39,.3,.20,.025)
        def table(w=1.2,d=.65,h=.76,mat='wood'):
            legs(w,d,h-.05);box(0,0,h-.03,w,d,.06,mat)
        if n in (1,2,3,31,32,33,34):
            w,d,h=(.95,2.05,.64) if n in (1,2,3) else (.78,1.9,.85)
            legs(w,d,h);box(0,0,h,w,d,.12)
            box(0,0,h+.12,w-.06,d-.05,.18,'cloth' if n in (31,32) else 'white')
            if n in (1,2,3):box(0,-d/2+.08,h+.38,w,.07,.55);box(0,-d/2+.38,h+.26,w-.2,.35,.08,'paper',False)
            if n in (2,34):wheels(w,d)
            top=h+.21
        elif n in (4,5,6,7,8,9,10,11,12,61,68,86,87,88,92,123):
            w,d,h=(.48,.44,.65) if n==4 else (.45,.5,1.85) if n==8 else (.8,.48,1.85)
            if n==6:w,d,h=1.2,1,2.1
            if n==88:w,d,h=.5,.45,.7
            if n in (87,92):w,d,h=.65,.22,1.1;mount='wall'
            body(w,d,h,'wood' if n in (7,9) else 'steel')
            box(0,d/2+.012,h/2,w-.05,.025,h-.06,'dark' if n==86 else 'white')
            box(w/2-.1,d/2+.035,h*.53,.035,.04,.16,'dark',False)
            if n in (5,10,61):box(0,d/2+.03,h*.68,w-.15,.02,h*.38,'glass',False)
            if n==86:
                for z in (.25,.5,.75,1,1.25,1.5):box(0,d/2+.04,z,w-.12,.025,.05,'glass',False)
            top=h
        elif n in (13,14,15):shelf(1.1,.5,1.85,'wood' if n==14 else 'steel');top=1.41
        elif n in (16,17,18,19,20,21,22,23,24,37,38,39,107,130,132):
            w,d,h=(1.25,.65,.76)
            if n in (17,18):w,d=1.3,.8
            if n in (19,37):w,d,h=.75,.5,.52 if n==19 else .82
            if n in (23,24):w,d,h=1.4,.65,1.05
            if n in (38,39,107):w,d,h=.65,.5,.85
            table(w,d,h,'steel' if n in (20,21,22,37,38,39,107,130) else 'wood')
            if n in (38,39,107):box(0,0,.25,w,d,.05);wheels(w,d)
            if n in (16,23,24):box(-w*.3,0,h*.45,w*.32,d*.9,h*.85,'white')
            if n==132:screen(0,0,h+.35,.3)
            top=h
        elif n in (25,26,27,28,29,30,35,145):
            w,d,h=(.46,.48,.46)
            if n==27:w,d=.65,.65
            if n in (28,29,145):w,d=1.55,.58
            if n==30:w,d,h=.55,.55,.35
            if n==35:w,d=.8,1.05
            legs(w,d,h);box(0,0,h,w,d,.10,'wood' if n in (29,145) else 'cloth')
            if n!=30:box(0,-d/2+.04,h+.28,w,.08,.55,'wood' if n in (29,145) else 'cloth')
            if n in (26,35):wheels(w,d)
            if n in (27,28,35):
                for x in (-w/2,w/2):box(x,0,h+.17,.08,d,.22,'cloth')
        elif n==36:legs(.28,1.35,.25);box(0,0,1.1,.12,1.35,1.7,'cloth');wheels(.28,1.35)
        elif n in (42,44,50,51,52,53,54,55,76,79,80,82,85,89):
            if n in (44,76,85):mount='wall';body(.5,.14,.45,'dark');screen(0,.03,.25,.36)
            elif n in (51,52,53,54,79,80):
                mount='surface';support=[16,20,21,22,23,37,38,89]
                if n==79:body(.2,.38,.4,'dark')
                else:screen(0,0,.30,.4)
            else:body(.55,.45,.72,'white');screen(0,0,1.1,.45);wheels(.55,.45)
        elif n in (47,48):
            # Open diagnostic ring, with a separate patient sliding table.
            for x in (-.85,.85):box(x,-.55,1.05,.42,.85,2.1,'white')
            box(0,-.55,1.95,2.1,.85,.35,'white');box(0,-.55,.2,2.1,.85,.35,'white')
            box(0,.55,.72,.58,2.05,.15,'cloth');box(0,.25,.32,.45,.8,.64,'white');top=.8
        elif n in (45,46,49,57,58,59,62,63,69,70,73,74,75,77,94,95,96,97,98,99,100,108,109,110,114,115,116,122,124,125,128,129,135,136,137,138,139,140,141,142,143,144,146,147,148,149,150,152,153,154,155,173,185,186,187):
            if n==45:
                mount='ceiling';box(0,0,-.4,.07,.07,.8);box(.35,0,-.8,.75,.07,.07);box(.7,0,-.9,.48,.48,.12,'light',False)
            elif n in (46,49,58,59,62,63,94,95,96,97,98,99,100,122,124,125,128,129):
                w,d,h=(1.05,.68,1.3)
                if n in (94,96,98,99):w,d,h=1.6,.85,1.35
                body(w,d,h,'white' if n in (46,49,58,59,62,63,125,128,129) else 'steel')
                box(0,d/2+.02,h*.65,w*.7,.03,h*.38,'glass',False)
                for x in (-w*.3,0,w*.3):box(x,d/2+.04,.25,.04,.015,.18,'dark',False)
                if n==59:box(0,0,1.55,w,.68,.08);top=1.3
            elif n in (57,69):
                box(0,0,.1,.85,1.35,.2,'dark');box(0,-.45,.75,.08,.08,1.5);box(0,-.45,1.35,.65,.08,.08);box(0,.2,.65,.45,.45,.12,'cloth')
            elif n==70:
                mount='wall'
                for x in (-.4,.4):box(x,0,1.15,.06,.1,2.3,'wood')
                for i in range(9):box(0,.03,.2+i*.25,.8,.06,.04,'wood')
            elif n==73:
                # Raised shallow therapy basin: does not silently cut an existing slab.
                for x in (-.95,.95):box(x,0,.25,.14,2.4,.5,'white')
                for y in (-1.2,1.2):box(0,y,.25,2.04,.14,.5,'white')
                box(0,0,.05,1.8,2.26,.1,'blue');box(0,0,.28,1.8,2.26,.015,'glass',False)
            elif n in (74,75):
                for x in (-.3,.3):box(x,0,.65,.045,.12,1.3)
                for z in (.2,.5,.8,1.1):box(0,0,z,.65,.08,.04)
            elif n==77:body(1.1,.5,.8,'wood');box(0,.29,.7,1.0,.15,.04,'paper',False)
            elif n==108:
                for x in (-.4,0,.4):box(x,0,.09,.16,.8,.18,'wood')
                for y in (-.35,-.175,0,.175,.35):box(0,y,.21,1.05,.14,.06,'wood')
                top=.24
            elif n==109:body(.65,.5,.5,'wood');box(0,.26,.25,.07,.02,.5,'steel',False);top=.5
            elif n in (110,154):
                wheels(.6,.9);box(0,0,.22,.65,.9,.08,'wood');box(0,-.4,.65,.65,.06,.9);box(0,-.4,1.1,.65,.06,.06)
            elif n==114:body(.35,.4,.42,'white');box(0,-.25,.65,.4,.18,.5,'white');box(0,.08,.45,.45,.5,.12,'white')
            elif n==115:
                box(0,0,.035,.85,.85,.07,'white');box(0,-.35,1.1,.04,.04,2.2);box(0,-.24,2.15,.2,.25,.04)
            elif n==116:body(.06,.9,1.8,'white')
            elif n in (135,136,137,138):
                w,d,h=1.85,4.3,1.45
                if n in (135,138):w,d,h=2.15,5.5,2.4
                box(0,0,.7,w,d,.75,'white' if n==135 else 'blue');box(0,-.25,h*.72,w*.9,d*.65,h*.5,'white' if n==135 else 'blue')
                box(0,d*.22,h*.82,w*.82,.08,h*.29,'glass',False)
                for x in (-w/2,w/2):
                    for y in (-d*.32,d*.32):box(x,y,.35,.22,.65,.65,'dark')
                if n==135:
                    for x in (-w/2-.02,w/2+.02):box(x,0,1.35,.03,.55,.15,'red',False);box(x,0,1.35,.03,.16,.55,'red',False)
            elif n in (139,140,141,142):
                w=3 if n in (139,140,141) else 1.5
                for x in (-w/2,w/2):box(x,0,.75,.12,.12,1.5)
                box(0,0,1.2,w,.09,.12,'red' if n==139 else 'steel')
                if n in (140,141):
                    for x in range(10):box(-w/2+(x+.5)*w/10,0,.7,.035,.035,1.3)
            elif n in (143,144):
                for x in (-1.5,1.5):
                    for y in (-1.5,1.5):box(x,y,1.35,.1,.1,2.7,'wood')
                box(0,0,2.75,3.3,3.3,.15,'wood')
            elif n in (146,147,148,155):
                h=4.5 if n==146 else 2.4;box(0,0,h/2,.08,.08,h)
                box(0,0,h,.6,.12,.38,'light' if n in (146,147) else 'red',False)
            elif n in (149,150,152):
                h=3.5 if n==149 else .8 if n==150 else 1.1
                box(0,0,h*.4,.16,.16,h*.8,'wood');box(0,0,h*.8,h*.55,h*.5,h*.55,'green',n==149)
                if n==152:body(.32,.32,.28,'red')
            elif n==153:box(0,0,.6,.035,.035,1.2,'wood');box(0,0,.1,.2,.06,.22)
            elif n==173:body(1.1,2,.06)
            elif n in (185,186,187):
                w=1.1 if n==185 else 1.75
                body(w,w,.25,'dark')
                for x in (-w/2,w/2):
                    for y in (-w/2,w/2):box(x,y,1.25,.08,.08,2.5)
                box(0,0,2.45,w,w,.12);box(0,0,1.3,w*.55,w*.55,1.7,'glass');box(0,0,1.3,.2,.2,1.5,'light',False)
        elif n in (40,41,56,64,65,66,81,83,90,106,113,126,127,131,133,134,178,183):
            mount='surface';support=[16,17,18,19,20,21,22,23,24,37,38,13,14,15,4,89,108,109]
            if n==131:support=[1,2,3,31,34,39,13];body(.42,.55,.09,'paper')
            elif n==113:support=[112];box(0,-.15,.16,.035,.035,.32);box(0,-.06,.31,.035,.2,.035)
            elif n==178:body(.15,.15,.03);box(0,0,.2,.03,.03,.4);box(0,0,.42,.25,.25,.15,'paper',False)
            elif n in (64,66,126,127):body(.13,.13,.22,'blue' if n==64 else 'paper')
            elif n==65:
                body(.25,.15,.05);[box(x*.06,0,.12,.025,.025,.16,'glass',False) for x in (-1,0,1)]
            elif n==183:body(.22,.3,.035,'paper')
            elif n==81:body(.4,.15,.035,'dark')
            elif n in (83,90):body(.25,.2,.10,'dark');box(0,0,.14,.28,.07,.05,'dark',False)
            elif n==133:body(.35,.25,.2)
            elif n==134:
                body(.2,.16,.23,'paper');box(0,.085,.13,.13,.01,.08,'red',False)
            else:body(.32,.22,.07,'steel')
        elif n in (43,67,72,84,91,93,101,104,112,117,118,119,120,121,151,156,158,163,176,177,180,181,182):
            if n in (43,84,93,117,118,119,163,176,180,181):
                mount='wall';w,d,h={43:(.8,.08,.18),84:(.2,.25,.15),93:(.12,.06,.18),117:(.45,.04,.6),118:(.12,.12,.23),119:(.22,.12,.18),163:(.25,.06,.07),176:(.12,.035,.08),180:(.32,.04,.32),181:(.4,.04,.17)}[n]
                body(w,d,h,'paper' if n in (180,181) else 'glass' if n==117 else 'steel')
                if n==180:box(0,d/2+.015,h*.55,.015,.01,h*.32,'dark',False);box(.04,d/2+.02,h*.6,.09,.01,.012,'dark',False)
                if n==176:
                    for x in (-.025,.025):box(x,d/2+.005,h/2,.01,.01,.01,'dark',False)
            elif n in (177,104,101):
                mount='ceiling';body(1.1,.16,.07,'light' if n==177 else 'steel')
            elif n==112:
                legs(.55,.45,.78);box(0,0,.81,.6,.48,.12,'white');box(0,0,.88,.4,.3,.025,'glass',False);top=.89
            elif n in (67,120,121):body(.30,.30,.45,'yellow' if n==67 else 'dark');box(0,0,.47,.33,.33,.04)
            elif n==91:box(0,0,1.5,.08,.08,3);box(0,0,2.5,1.2,.05,.05)
            elif n==72:body(.65,1.7,.035,'blue')
            elif n==151:
                for x,y,h in ((0,0,.18),(.06,.04,.13),(-.05,-.03,.21)):box(x,y,h/2,.025,.025,h,'green',False)
            elif n in (156,158):body(.3,.3,.035,'light' if n==156 else 'dark')
            elif n==182:body(1.4,1.8,.012,'cloth')
        if not parts:
            # Catalog entries without a zone quantity are never silently used.
            continue
        bounds=[min(p[i]-p[i+3]/2 for p in parts) for i in range(3)]+[max(p[i]+p[i+3]/2 for p in parts) for i in range(3)]
        # The placement anchor is a footprint centre. Apparatus and chairs have
        # asymmetric parts; normalise them so the fitted and rendered bounds
        # are identical for all four right-angle orientations.
        for axis in (0,1):
            centre=(bounds[axis]+bounds[axis+3])/2
            for p in parts:p[axis]-=centre
            bounds[axis]-=centre;bounds[axis+3]-=centre
        result[str(n)]=dict(name=name,parts=parts,bounds=bounds,mount=mount,support=support,top=top)
    return result
