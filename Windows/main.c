#ifndef UNICODE
#define UNICODE
#endif
#define _UNICODE
#define WINVER 0x0A00
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <math.h>
#include "layout.h"

typedef void GpImage; typedef void GpGraphics; typedef void GpBrush; typedef void GpPen; typedef void GpPath;
typedef struct { UINT32 version; void *callback; BOOL noThread, noCodecs; } StartupInput;
typedef struct { UINT id, length; WORD type; void *value; } PropertyItem;
#define API(name, args) __declspec(dllimport) int WINAPI name args
API(GdiplusStartup, (ULONG_PTR*, const StartupInput*, void*));
__declspec(dllimport) void WINAPI GdiplusShutdown(ULONG_PTR);
API(GdipLoadImageFromFile, (const WCHAR*, GpImage**));
API(GdipDisposeImage, (GpImage*));
API(GdipGetImageWidth, (GpImage*, UINT*)); API(GdipGetImageHeight, (GpImage*, UINT*));
API(GdipGetPropertyItemSize, (GpImage*, UINT, UINT*)); API(GdipGetPropertyItem, (GpImage*, UINT, UINT, PropertyItem*));
API(GdipImageRotateFlip, (GpImage*, int));
API(GdipCloneBitmapAreaI, (int,int,int,int,int,GpImage*,GpImage**));
API(GdipSaveImageToFile, (GpImage*,const WCHAR*,const CLSID*,const void*));
API(GdipCreateFromHDC, (HDC,GpGraphics**)); API(GdipDeleteGraphics, (GpGraphics*));
API(GdipSetSmoothingMode, (GpGraphics*,int)); API(GdipSetInterpolationMode, (GpGraphics*,int));
API(GdipDrawImageRectI, (GpGraphics*,GpImage*,int,int,int,int));
API(GdipCreateSolidFill, (UINT32,GpBrush**)); API(GdipDeleteBrush, (GpBrush*));
API(GdipFillRectangle, (GpGraphics*,GpBrush*,float,float,float,float));
API(GdipCreatePen1, (UINT32,float,int,GpPen**)); API(GdipDeletePen, (GpPen*));
API(GdipDrawLine, (GpGraphics*,GpPen*,float,float,float,float));
API(GdipCreatePath, (int,GpPath**)); API(GdipAddPathArc, (GpPath*,float,float,float,float,float,float));
API(GdipStartPathFigure, (GpPath*)); API(GdipAddPathLine, (GpPath*,float,float,float,float));
API(GdipAddPathBezier, (GpPath*,float,float,float,float,float,float,float,float));
API(GdipSetClipRect, (GpGraphics*,float,float,float,float,int));
API(GdipClosePathFigure, (GpPath*)); API(GdipFillPath, (GpGraphics*,GpBrush*,GpPath*));
API(GdipDrawPath, (GpGraphics*,GpPen*,GpPath*)); API(GdipDeletePath, (GpPath*));
API(GdipScaleWorldTransform, (GpGraphics*,float,float,int));
API(GdipTranslateWorldTransform, (GpGraphics*,float,float,int)); API(GdipRotateWorldTransform, (GpGraphics*,float,int));
API(GdipSaveGraphics, (GpGraphics*,UINT*)); API(GdipRestoreGraphics, (GpGraphics*,UINT));

static HWND mainWindow, exportButton;
static UINT dpi = 96; static float scale = 1;
static GpImage *image, *tiles[3]; static CropLayout layout;
static WCHAR source[MAX_PATH], status[160] = L"一张长图，连成一组。", detail[200] = L"自动选择两张或三张 · 每张 3:4";
static ULONGLONG animStart, logoStart; static int hover[2]; static int previewHover; static HFONT fonts[4];
static const COLORREF bg = RGB(246,248,244), ink = RGB(41,51,46), muted = RGB(122,122,122);
static const UINT32 pink = 0xffe8a6ce;
static int px(float x) { return (int)roundf(x*scale); }
static float clamp(float x) { return x<0?0:x>1?1:x; }
static void rect(GpGraphics *g,float x,float y,float w,float h,UINT32 color) {
    GpBrush *b; GdipCreateSolidFill(color,&b); GdipFillRectangle(g,b,x,y,w,h); GdipDeleteBrush(b);
}
static void line(GpGraphics*g,float x,float y,float x2,float y2,UINT32 color,float width) {
    GpPen*p; GdipCreatePen1(color,width,2,&p); GdipDrawLine(g,p,x,y,x2,y2); GdipDeletePen(p);
}
static void rounded(GpGraphics*g,float x,float y,float w,float h,float r,UINT32 fill,UINT32 stroke) {
    if (r < 0.01f) { rect(g,x,y,w,h,fill); if(stroke) { line(g,x,y,x+w,y,stroke,1); line(g,x+w,y,x+w,y+h,stroke,1); line(g,x+w,y+h,x,y+h,stroke,1); line(g,x,y+h,x,y,stroke,1); } return; }
    GpPath*p; GdipCreatePath(0,&p); float d=2*r;
    GdipAddPathArc(p,x,y,d,d,180,90); GdipAddPathArc(p,x+w-d,y,d,d,270,90);
    GdipAddPathArc(p,x+w-d,y+h-d,d,d,0,90); GdipAddPathArc(p,x,y+h-d,d,d,90,90); GdipClosePathFigure(p);
    if(fill) { GpBrush*b; GdipCreateSolidFill(fill,&b); GdipFillPath(g,b,p); GdipDeleteBrush(b); }
    if(stroke) { GpPen*pen; GdipCreatePen1(stroke,1,2,&pen); GdipDrawPath(g,pen,p); GdipDeletePen(pen); }
    GdipDeletePath(p);
}
#include "planehub_logo.h"
static void text(HDC dc,const WCHAR *s,float x,float y,int font,COLORREF color,int centered) {
    HGDIOBJ old=SelectObject(dc,fonts[font]); SetTextColor(dc,color); SetBkMode(dc,TRANSPARENT);
    SIZE sz; GetTextExtentPoint32W(dc,s,(int)wcslen(s),&sz);
    TextOutW(dc,px(x)-(centered?sz.cx/2:0),px(y),s,(int)wcslen(s)); SelectObject(dc,old);
}
static void graphics(HDC dc,GpGraphics**g) {
    GdipCreateFromHDC(dc,g); GdipSetSmoothingMode(*g,4); GdipSetInterpolationMode(*g,7); GdipScaleWorldTransform(*g,scale,scale,0);
}
static void error(HWND owner,const WCHAR *s) { MessageBoxW(owner,s,L"3:4图片快切",MB_OK|MB_ICONWARNING); }
static void dispose(GpImage**p) { if(*p) { GdipDisposeImage(*p); *p=NULL; } }
static void load_image(const WCHAR *path) {
    if(wcslen(path)>=MAX_PATH) {error(mainWindow,L"图片路径过长，请移动到路径较短的文件夹。");return;}
    GpImage* next=NULL; GpImage* slices[3]={0}; UINT w=0,h=0;
    if (GdipLoadImageFromFile(path,&next) || !next) { error(mainWindow,L"无法读取图片，请选择 JPG、PNG、BMP 或 TIFF 图片。"); return; }
    UINT size=0;
    if (!GdipGetPropertyItemSize(next,0x112,&size) && size>=sizeof(PropertyItem) && size<65536) {
        PropertyItem *p=malloc(size);
        if (p && !GdipGetPropertyItem(next,0x112,size,p) && p->type==3 && p->length>=2 && p->value) {
            unsigned o=*(unsigned short*)p->value; int map[]={0,0,4,2,6,5,1,7,3}; if(o>=2&&o<=8) GdipImageRotateFlip(next,map[o]);
        } free(p);
    }
    GdipGetImageWidth(next,&w); GdipGetImageHeight(next,&h);
    if(w>50000||h>50000||(uint64_t)w*h>200000000) { GdipDisposeImage(next); error(mainWindow,L"图片过大，请先缩小到 2 亿像素以内。"); return; }
    CropLayout l=crop_layout((int)w,(int)h);
    if (!l.count) { GdipDisposeImage(next); error(mainWindow,L"图片尺寸过小。"); return; }
    for(int i=0;i<l.count;i++) if(GdipCloneBitmapAreaI(l.x+3*l.unit*i,l.y,3*l.unit,l.height,0x26200a,next,&slices[i])) {
        for(int j=0;j<3;j++) dispose(&slices[j]); GdipDisposeImage(next); error(mainWindow,L"图片解码失败或内存不足。"); return;
    }
    dispose(&image); for(int i=0;i<3;i++) { dispose(&tiles[i]); tiles[i]=slices[i]; }
    image=next;layout=l; wcscpy(source,path); animStart=GetTickCount64();
    double loss=100*(1-(double)l.width*l.height/((double)w*h));
    swprintf(status,160,L"将切分成 %d 张 3:4 图片",l.count);
    if(loss<0.05) swprintf(detail,200,L"每张 %d × %d · 无需裁剪",3*l.unit,l.height);
    else swprintf(detail,200,L"每张 %d × %d · 居中裁去约 %.1f%%",3*l.unit,l.height,loss);
    EnableWindow(exportButton,TRUE); InvalidateRect(mainWindow,NULL,FALSE); InvalidateRect(exportButton,NULL,TRUE);
}
static void choose_image(void) {
    WCHAR path[MAX_PATH]=L""; OPENFILENAMEW of={0}; of.lStructSize=sizeof(of); of.hwndOwner=mainWindow; of.lpstrFile=path; of.nMaxFile=MAX_PATH;
    of.lpstrFilter=L"图片 (JPG, PNG, BMP, TIFF)\0*.jpg;*.jpeg;*.png;*.bmp;*.tif;*.tiff\0所有文件\0*.*\0";
    of.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR; of.lpstrTitle=L"上传图片";
    if(GetOpenFileNameW(&of)) load_image(path);
}
static void export_images(void) {
    if(!image) return;
    BROWSEINFOW bi={0}; bi.hwndOwner=mainWindow; bi.lpszTitle=L"选择导出位置"; bi.ulFlags=BIF_RETURNONLYFSDIRS|BIF_NEWDIALOGSTYLE;
    PIDLIST_ABSOLUTE id=SHBrowseForFolderW(&bi); if(!id) return;
    WCHAR parent[MAX_PATH]; BOOL ok=SHGetPathFromIDListW(id,parent); CoTaskMemFree(id); if(!ok) return;
    const WCHAR *file=wcsrchr(source,L'\\'); file=file?file+1:source;
    WCHAR stem[MAX_PATH]; wcscpy(stem,file); WCHAR *dot=wcsrchr(stem,L'.'); if(dot&&dot!=stem)*dot=0;
    if(wcslen(parent)+2*wcslen(stem)+40>=MAX_PATH) { error(mainWindow,L"路径过长，请选择较短的保存位置或缩短原图文件名。"); return; }
    WCHAR folder[MAX_PATH],paths[3][MAX_PATH]; int created=0;
    for(int n=1;n<10000;n++) {
        if(n==1) swprintf(folder,MAX_PATH,L"%ls\\%ls_连页",parent,stem);
        else swprintf(folder,MAX_PATH,L"%ls\\%ls_连页_%d",parent,stem,n);
        if(CreateDirectoryW(folder,NULL)) {created=1;break;}
        if(GetLastError()!=ERROR_ALREADY_EXISTS)break;
    }
    if(!created) {error(mainWindow,L"无法创建输出文件夹，请检查保存权限和磁盘空间。");return;}
    CLSID png={0x557cf406,0x1a04,0x11d3,{0x9a,0x73,0,0,0xf8,0x1e,0xf3,0x2e}};
    for(int i=0;i<layout.count;i++) {
        swprintf(paths[i],MAX_PATH,L"%ls\\%ls_%02d.png",folder,stem,i+1);
        if(GdipSaveImageToFile(tiles[i],paths[i],&png,NULL)) {
            for(int j=0;j<=i;j++)DeleteFileW(paths[j]); RemoveDirectoryW(folder);
            error(mainWindow,L"保存失败，请检查磁盘空间和写入权限。");return;
        }
    }
    swprintf(status,160,L"已导出 %d 张 3:4 图片",layout.count);wcscpy(detail,L"按 01、02、03 的顺序，连起来发布。");
    InvalidateRect(mainWindow,NULL,FALSE); ShellExecuteW(mainWindow,L"open",folder,NULL,NULL,SW_SHOWNORMAL);
}
static void draw_mark(GpGraphics*g) {
    double t=(GetTickCount64()-logoStart)/1000.0; t=fmod(t,7);
    float phase=t<0.9?0:t<3.2?(float)((t-.9)/2.3):t<5.3?1:clamp((float)(1-(t-5.3)/1.5));
    BOOL reduce=FALSE; SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION,0,&reduce,0); if(!reduce)phase=0;
    float b=clamp((phase-.15f)/.25f), raw=clamp((phase-.46f)/.44f), p=raw*raw*(3-2*raw);
    const float width=120,height=120.f*4/9,x=547,y=31;
    for(int i=0;i<3;i++) {
        UINT saved;GdipSaveGraphics(g,&saved);
        GdipTranslateWorldTransform(g,(i-1)*5*p,((int[]){-3,3,-1})[i]*p,0);
        GdipSetClipRect(g,x+i*width/3,y,width/3,height,1);
        GdipTranslateWorldTransform(g,x,y+(height-56*width/164)/2,0);
        GdipScaleWorldTransform(g,width/164,width/164,0);
        draw_planehub_logo(g);GdipRestoreGraphics(g,saved);
    }
    for(int i=1;i<3;i++)line(g,x+i*width/3,y+7,x+i*width/3,y+height-7,((UINT32)(217*b*(1-p))<<24)|0xffffff,1);

}
static void paint(HWND hwnd) {
    PAINTSTRUCT ps; HDC out=BeginPaint(hwnd,&ps); RECT r;GetClientRect(hwnd,&r);
    HDC dc=CreateCompatibleDC(out); HBITMAP bitmap=CreateCompatibleBitmap(out,r.right,r.bottom); HGDIOBJ old=SelectObject(dc,bitmap);
    HBRUSH back=CreateSolidBrush(bg);FillRect(dc,&r,back);DeleteObject(back);
    GpGraphics*g;graphics(dc,&g);draw_mark(g);
    if(image) {
        float raw=clamp(((float)(GetTickCount64()-animStart)/1000-.18f)/.85f),p=1-powf(1-raw,3);
        BOOL motion=TRUE;SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION,0,&motion,0);if(!motion)p=1;
        float h=fminf(300,598.f/(layout.count*.75f)),w=h*.75f,step=w*(1-.11f*p),total=w+step*(layout.count-1);
        float angles3[]={-2.58f,1.26f,2.69f},angles2[]={-2,2},offs3[]={-10,9,-5},offs2[]={-9,9};
        for(int i=0;i<layout.count;i++) {
            UINT state;GdipSaveGraphics(g,&state);
            GdipTranslateWorldTransform(g,(700-total)/2+w/2+i*step,295+(layout.count==3?offs3[i]:offs2[i])*p,0);
            GdipRotateWorldTransform(g,(layout.count==3?angles3[i]:angles2[i])*p,0);
            for(int j=8;j>0;j--)rounded(g,-w/2-j,-h/2+6-j,w+2*j,h+2*j,5,((UINT32)(4*p)<<24),0);
            GdipDrawImageRectI(g,tiles[i],(int)(-w/2),(int)(-h/2),(int)w,(int)h); GdipRestoreGraphics(g,state);
        }
    } else rounded(g,38,140,624,314,20,0,previewHover?pink:0x30808080);
    GdipDeleteGraphics(g);
    text(dc,L"3:4图片快切",36,41,0,ink,0); text(dc,L"让长图，自然连起来。",37,80,2,muted,0);
    if(!image) text(dc,L"点击上传，或把图片拖到这里",350,288,1,ink,1);
    else if(previewHover) text(dc,L"点击更换图片 · 或拖入新图片",42,136,3,ink,0);
    text(dc,status,36,501,1,ink,0);text(dc,detail,36,529,3,muted,0);
    BitBlt(out,0,0,r.right,r.bottom,dc,0,0,SRCCOPY);SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);EndPaint(hwnd,&ps);
}
static void draw_button(DRAWITEMSTRUCT *d) {
    int secondary=d->CtlID==100, index=secondary?0:1, enabled=!(d->itemState&ODS_DISABLED), active=enabled&&(hover[index]||(d->itemState&ODS_SELECTED));
    HBRUSH back=CreateSolidBrush(bg);FillRect(d->hDC,&d->rcItem,back);DeleteObject(back);
    GpGraphics*g;graphics(d->hDC,&g);
    rounded(g,1,1,118,44,12,enabled?(active?0xff292929:0xff000000):0x40000000,0);
    const WCHAR*label=secondary?L"上传图片":L"导出";HGDIOBJ old=SelectObject(d->hDC,fonts[1]);SIZE size;GetTextExtentPoint32W(d->hDC,label,(int)wcslen(label),&size);SelectObject(d->hDC,old);
    float tw=size.cx/scale,th=size.cy/scale;
    if(secondary)text(d->hDC,label,60-tw/2,(46-th)/2,1,RGB(41,123,87),0);
    else {
        float left=(120-(14+9+tw))/2;UINT32 color=pink;
        line(g,left+1,20,left+1,32,color,1.6f);line(g,left+1,32,left+13,32,color,1.6f);line(g,left+13,32,left+13,20,color,1.6f);
        line(g,left+7,25,left+7,13,color,1.6f);line(g,left+7,13,left+3,17,color,1.6f);line(g,left+7,13,left+11,17,color,1.6f);
        text(d->hDC,label,left+23,(46-th)/2,1,RGB(232,166,206),0);
    }
    if(d->itemState&ODS_FOCUS)rounded(g,4,4,112,38,9,0,0xffe8a6ce);
    GdipDeleteGraphics(g);
}
static LRESULT CALLBACK button_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp,UINT_PTR id,DWORD_PTR data) {
    int i=(int)data;
    if(msg==WM_MOUSEMOVE&&!hover[i]) {hover[i]=1;TRACKMOUSEEVENT track={sizeof(track),TME_LEAVE,hwnd,0};TrackMouseEvent(&track);InvalidateRect(hwnd,NULL,FALSE);}
    if(msg==WM_MOUSELEAVE) {hover[i]=0;InvalidateRect(hwnd,NULL,FALSE);}
    if(msg==WM_SETCURSOR&&IsWindowEnabled(hwnd)) {SetCursor(LoadCursorW(NULL,IDC_HAND));return TRUE;}
    return DefSubclassProc(hwnd,msg,wp,lp);
}
static LRESULT CALLBACK window_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp) {
    switch(msg) {
    case WM_MOUSEMOVE: {
        int x=GET_X_LPARAM(lp),y=GET_Y_LPARAM(lp);
        int inside=x>=px(38)&&x<=px(662)&&y>=px(130)&&y<=px(454);
        if(inside!=previewHover){previewHover=inside;InvalidateRect(hwnd,NULL,FALSE);}
        TRACKMOUSEEVENT track={sizeof(track),TME_LEAVE,hwnd,0};TrackMouseEvent(&track);
        if(inside)SetCursor(LoadCursorW(NULL,IDC_HAND));return 0;
    }
    case WM_MOUSELEAVE:previewHover=0;InvalidateRect(hwnd,NULL,FALSE);return 0;
    case WM_LBUTTONUP:if(GET_X_LPARAM(lp)>=px(38)&&GET_X_LPARAM(lp)<=px(662)&&GET_Y_LPARAM(lp)>=px(130)&&GET_Y_LPARAM(lp)<=px(454))choose_image();return 0;
    case WM_PAINT:paint(hwnd);return 0;
    case WM_ERASEBKGND:return 1;
    case WM_DRAWITEM:draw_button((DRAWITEMSTRUCT*)lp);return TRUE;
    case WM_TIMER:InvalidateRect(hwnd,NULL,FALSE);return 0;
    case WM_COMMAND:if(LOWORD(wp)==100)choose_image();if(LOWORD(wp)==101)export_images();return 0;
    case WM_DROPFILES:{WCHAR path[MAX_PATH];HDROP drop=(HDROP)wp;UINT length=DragQueryFileW(drop,0,NULL,0);if(length<MAX_PATH&&DragQueryFileW(drop,0,path,MAX_PATH))load_image(path);else error(hwnd,L"图片路径过长。");DragFinish(drop);return 0;}
    case WM_DESTROY:KillTimer(hwnd,1);PostQuitMessage(0);return 0;
    }
    return DefWindowProcW(hwnd,msg,wp,lp);
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE previous,LPWSTR args,int show) {
    (void)previous;(void)args;
    CoInitializeEx(NULL,COINIT_APARTMENTTHREADED);INITCOMMONCONTROLSEX controls={sizeof(controls),ICC_STANDARD_CLASSES};InitCommonControlsEx(&controls);
    StartupInput input={1,NULL,FALSE,FALSE};ULONG_PTR token;if(GdiplusStartup(&token,&input,NULL))return 1;
    HDC screen=GetDC(NULL);dpi=GetDeviceCaps(screen,LOGPIXELSX);ReleaseDC(NULL,screen);scale=dpi/96.f;
    int sizes[]={25,14,13,11};for(int i=0;i<4;i++)fonts[i]=CreateFontW(-px(sizes[i]),0,0,0,i<2?FW_SEMIBOLD:FW_NORMAL,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Microsoft YaHei UI");
    WNDCLASSW cls={0};cls.hInstance=instance;cls.lpfnWndProc=window_proc;cls.lpszClassName=L"RedNoteCrop";cls.hCursor=LoadCursorW(NULL,IDC_ARROW);cls.hIcon=LoadIconW(instance,MAKEINTRESOURCEW(1));RegisterClassW(&cls);
    DWORD style=WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_CLIPCHILDREN;RECT rect={0,0,px(700),px(570)};AdjustWindowRect(&rect,style,FALSE);
    mainWindow=CreateWindowExW(WS_EX_ACCEPTFILES,cls.lpszClassName,L"3:4图片快切",style,CW_USEDEFAULT,CW_USEDEFAULT,rect.right-rect.left,rect.bottom-rect.top,NULL,NULL,instance,NULL);
    if(!mainWindow)return 1;
    exportButton=CreateWindowW(L"BUTTON",L"导出",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,px(544),px(514),px(120),px(46),mainWindow,(HMENU)101,instance,NULL);
    EnableWindow(exportButton,FALSE);SetWindowSubclass(exportButton,button_proc,1,1);
    logoStart=GetTickCount64();SetTimer(mainWindow,1,33,NULL);ShowWindow(mainWindow,show);UpdateWindow(mainWindow);
    int argc;WCHAR**argv=CommandLineToArgvW(GetCommandLineW(),&argc);if(argv&&argc>1)load_image(argv[1]);if(argv)LocalFree(argv);
    MSG msg;while(GetMessageW(&msg,NULL,0,0)>0){if(msg.message==WM_KEYDOWN&&(GetKeyState(VK_CONTROL)&0x8000)&&msg.wParam=='O'){choose_image();continue;}if(!IsDialogMessageW(mainWindow,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}
    dispose(&image);for(int i=0;i<3;i++)dispose(&tiles[i]);for(int i=0;i<4;i++)DeleteObject(fonts[i]);GdiplusShutdown(token);CoUninitialize();return 0;
}
