from pathlib import Path
import re,xml.etree.ElementTree as E
base=Path(__file__).resolve().parent.parent
root=E.parse(base/'Assets/PlaneHub-logo.svg').getroot()
sw=['import AppKit','enum PlaneHubLogo {','static func draw(in ctx: CGContext) {']
c=['/* Generated from the original PlaneHub SVG; geometry and colors preserved. */','static void draw_planehub_logo(GpGraphics *g) {']
def color(s):
 return {'black':(0,0,0),'white':(255,255,255)}.get(s) or tuple(int(s[i:i+2],16) for i in (1,3,5))
for e in root.iter():
 tag=e.tag.split('}')[-1]
 if tag not in ('rect','path'):continue
 r,g,b=color(e.get('fill'))
 sw.append(f'ctx.setFillColor(CGColor(red: {r}/255.0, green: {g}/255.0, blue: {b}/255.0, alpha: 1))')
 col=f'0xff{r:02x}{g:02x}{b:02x}'
 if tag=='rect':
  x,y,w,h,rx=[float(e.get(k,'0')) for k in ('x','y','width','height','rx')]
  sw.append(f'ctx.addPath(CGPath(roundedRect: CGRect(x:{x},y:{y},width:{w},height:{h}), cornerWidth:{rx},cornerHeight:{rx},transform:nil));ctx.fillPath()')
  c.append(f'rounded(g,{x}f,{y}f,{w}f,{h}f,{rx}f,{col},0);')
  continue
 sw.append('do { let p = CGMutablePath()')
 c.append('{ GpPath*p; GpBrush*b; GdipCreatePath(1,&p);')
 toks=re.findall(r'[A-Za-z]|[-+]?(?:\d*\.\d+|\d+\.?\d*)(?:[eE][-+]?\d+)?',e.get('d'));i=0;cmd='';x=y=sx=sy=0
 def point(x,y):return f'CGPoint(x:{x},y:{y})'
 def f(x):return f'{float(x)}f'
 while i<len(toks):
  if toks[i].isalpha():cmd=toks[i];i+=1
  assert cmd in ('M','L','H','V','C','Z'),cmd
  if cmd=='Z':
   sw.append('p.closeSubpath()');c.append('GdipClosePathFigure(p);');x,y=sx,sy;cmd='';continue
  count={'M':2,'L':2,'H':1,'V':1,'C':6}[cmd];a=list(map(float,toks[i:i+count]));i+=count
  if cmd=='M':
   x,y=a;sx,sy=x,y;sw.append(f'p.move(to:{point(x,y)})');c.append('GdipStartPathFigure(p);');cmd='L'
  elif cmd in ('L','H','V'):
   nx,ny=a if cmd=='L' else (a[0],y) if cmd=='H' else (x,a[0]);sw.append(f'p.addLine(to:{point(nx,ny)})');c.append(f'GdipAddPathLine(p,{f(x)},{f(y)},{f(nx)},{f(ny)});');x,y=nx,ny
  elif cmd=='C':
   sw.append(f'p.addCurve(to:{point(a[4],a[5])},control1:{point(a[0],a[1])},control2:{point(a[2],a[3])})');c.append('GdipAddPathBezier(p,'+','.join(f(v) for v in [x,y]+a)+');');x,y=a[4:]
 sw.append('ctx.addPath(p);ctx.fillPath() }');c.append(f'GdipCreateSolidFill({col},&b);GdipFillPath(g,b,p);GdipDeleteBrush(b);GdipDeletePath(p); }}')
sw+=['}','}'];c+=['}']
(base/'Sources/PlaneHubLogo.swift').write_text('\n'.join(sw))
(base/'Windows/planehub_logo.h').write_text('\n'.join(c))
print('Generated native vector drawing from SVG for both platforms.')
