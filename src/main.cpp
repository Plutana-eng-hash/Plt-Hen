#include "vypex/core.hpp"

#include <orbis/libkernel.h>
#include <orbis/VideoOut.h>
#include <orbis/UserService.h>
#include <orbis/Pad.h>
#include <sys/types.h>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace {
constexpr int W = 1920, H = 1080, BPP = 4, BUFFERS = 2;
constexpr std::uint32_t PURPLE = 0xFF8B5CF6;
constexpr std::uint32_t BG = 0xFF0B0B10, PANEL = 0xFF15151E;
constexpr std::uint32_t CARD = 0xFF1D1D29, WHITE = 0xFFF4F1FA;
constexpr std::uint32_t MUTED = 0xFF9995A8, GREEN = 0xFF55D69E;
constexpr std::uint32_t AMBER = 0xFFF2C36B;

static const std::uint8_t font[36][5] = {
 {0x7E,0x11,0x11,0x11,0x7E},{0x7F,0x49,0x49,0x49,0x36},{0x3E,0x41,0x41,0x41,0x22},
 {0x7F,0x41,0x41,0x22,0x1C},{0x7F,0x49,0x49,0x49,0x41},{0x7F,0x09,0x09,0x09,0x01},
 {0x3E,0x41,0x49,0x49,0x7A},{0x7F,0x08,0x08,0x08,0x7F},{0x00,0x41,0x7F,0x41,0x00},
 {0x20,0x40,0x41,0x3F,0x01},{0x7F,0x08,0x14,0x22,0x41},{0x7F,0x40,0x40,0x40,0x40},
 {0x7F,0x02,0x0C,0x02,0x7F},{0x7F,0x04,0x08,0x10,0x7F},{0x3E,0x41,0x41,0x41,0x3E},
 {0x7F,0x09,0x09,0x09,0x06},{0x3E,0x41,0x51,0x21,0x5E},{0x7F,0x09,0x19,0x29,0x46},
 {0x46,0x49,0x49,0x49,0x31},{0x01,0x01,0x7F,0x01,0x01},{0x3F,0x40,0x40,0x40,0x3F},
 {0x1F,0x20,0x40,0x20,0x1F},{0x7F,0x20,0x18,0x20,0x7F},{0x63,0x14,0x08,0x14,0x63},
 {0x07,0x08,0x70,0x08,0x07},{0x61,0x51,0x49,0x45,0x43},
 {0x3E,0x51,0x49,0x45,0x3E},{0x00,0x42,0x7F,0x40,0x00},
 {0x62,0x51,0x49,0x49,0x46},{0x22,0x41,0x49,0x49,0x36},
 {0x18,0x14,0x12,0x7F,0x10},{0x2F,0x49,0x49,0x49,0x31},
 {0x3E,0x49,0x49,0x49,0x32},{0x01,0x71,0x09,0x05,0x03},
 {0x36,0x49,0x49,0x49,0x36},{0x26,0x49,0x49,0x49,0x3E}
};
constexpr int MAX_CONTROLLERS = 4;
int video=-1, active=0, pads[MAX_CONTROLLERS]={-1,-1,-1,-1}, user=-1;
int controller_count=0;
void* mapped=nullptr; off_t mem_offset=0; size_t mem_size=0;
char* fb[BUFFERS]={nullptr,nullptr}; OrbisKernelEqueue queue=0; bool allocated=false;
int glyph(char c) {
 if(c>='A'&&c<='Z') return c-'A';
 if(c>='0'&&c<='9') return 26+c-'0';
 return -1;
}
void rect(int x,int y,int w,int h,std::uint32_t color) {
 if(!fb[active]) return;
 if(x<0){w+=x;x=0;} if(y<0){h+=y;y=0;}
 if(x+w>W)w=W-x; if(y+h>H)h=H-y; if(w<=0||h<=0)return;
 auto* p=reinterpret_cast<std::uint32_t*>(fb[active]);
 for(int yy=y;yy<y+h;++yy) for(int xx=x;xx<x+w;++xx)p[yy*W+xx]=color;
}
void text(const char* s,int x,int y,int scale,std::uint32_t color) {
 for(;*s;++s,x+=6*scale) {
  int g=glyph(*s); if(g<0) continue;
  for(int col=0;col<5;++col) for(int row=0;row<7;++row)
   if(font[g][col]&(1u<<row)) rect(x+col*scale,y+row*scale,scale,scale,color);
 }
}
bool graphics_init() {
 video=sceVideoOutOpen(ORBIS_VIDEO_USER_MAIN,ORBIS_VIDEO_OUT_BUS_MAIN,0,0);
 if(video<0)return false;
 const size_t needed=BUFFERS*W*H*BPP;
 mem_size=(needed+0x1FFFFF)&~static_cast<size_t>(0x1FFFFF);
 if(sceKernelAllocateDirectMemory(0,sceKernelGetDirectMemorySize(),mem_size,0x200000,3,&mem_offset)<0)return false;
 allocated=true;
 if(sceKernelMapDirectMemory(&mapped,mem_size,0x33,0,mem_offset,0x200000)<0)return false;
 fb[0]=static_cast<char*>(mapped); fb[1]=fb[0]+W*H*BPP;
 OrbisVideoOutBufferAttribute attr;
 sceVideoOutSetBufferAttribute(&attr,0x80000000,1,0,W,H,W);
 if(sceVideoOutRegisterBuffers(video,0,reinterpret_cast<void**>(fb),BUFFERS,&attr)!=0)return false;
 if(sceKernelCreateEqueue(&queue,"vypex_flip")<0)return false;
 if(sceVideoOutAddFlipEvent(queue,video,nullptr)<0)return false;
 if(sceVideoOutSetFlipRate(video,0)<0)return false;
 return true;
}
void graphics_end() {
 if(video>=0){sceVideoOutClose(video);video=-1;}
 if(queue){sceKernelDeleteEqueue(queue);queue=0;}
 if(mapped){sceKernelMunmap(mapped,mem_size);mapped=nullptr;}
 if(allocated){sceKernelReleaseDirectMemory(mem_offset,mem_size);allocated=false;}
}
bool present(std::int64_t frame) {
 if(sceVideoOutSubmitFlip(video,active,1,frame)<0)return false;
 for(;;){
  OrbisVideoOutFlipStatus st={};
  if(sceVideoOutGetFlipStatus(video,&st)<0)return false;
  if(st.flipArg==frame)break;
  OrbisKernelEvent ev; int count=0; OrbisKernelUseconds timeout=1000000;
  if(sceKernelWaitEqueue(queue,&ev,1,&count,&timeout)!=0)return false;
 }
 active=(active+1)%BUFFERS; return true;
}
void card(int x,int y,int w,int h,const char* heading,const char* line1,const char* line2,std::uint32_t status) {
 rect(x,y,w,h,CARD); rect(x,y,5,h,PURPLE);
 text(heading,x+28,y+25,3,WHITE); text(line1,x+28,y+82,2,MUTED);
 text(line2,x+28,y+119,2,status);
}
void render(int page,int selected,const vypex::Config& cfg,const vypex::PluginRegistry& plugins,int frame) {
 rect(0,0,W,H,BG);
 rect(0,0,W,112,PANEL); rect(0,108,W,4,PURPLE);
 text("VYPEX CR CK",54,34,5,WHITE); text("NATIVE PS4 HOME BREW",610,49,2,MUTED);
 text("FW PROFILE 14 02",1510,48,2,PURPLE);
 rect(0,112,390,H-112,PANEL);
 const char* names[]={"OVERVIEW","SYSTEM","CONTROLLERS","PLUGINS","SETTINGS","ABOUT"};
 for(int i=0;i<6;++i){
  int y=160+i*82;
  if(i==page){rect(28,y-18,330,66,0xFF29213A);rect(28,y-18,5,66,PURPLE);}
  text(names[i],65,y,3,i==page?WHITE:MUTED);
 }
 text("CROSS SELECT",55,925,2,MUTED); text("CIRCLE BACK",55,965,2,MUTED);
 text("VYPEX BUILD 0 1",55,1030,2,PURPLE);
 text(names[page],440,166,4,WHITE);
 text("PLAYSTATION 4  /  CONTROL PANEL",442,213,2,MUTED);
 if(page==0){
  card(440,275,690,220,"HEN RUNTIME","STATUS  NOT LOADED","THIS APP DOES NOT ENABLE JAILBREAK",AMBER);
  card(1160,275,690,220,"FIRMWARE PROFILE","TARGET  14 02","KERNEL PATCHES  NOT PRESENT",AMBER);
  card(440,530,690,220,"PLUGIN MANAGER","REGISTERED ENTRIES",plugins.count()?"PLUGIN RECORDS AVAILABLE":"NO PLUGINS REGISTERED",WHITE);
  card(1160,530,690,220,"CONFIGURATION","UPDATE BLOCKING",cfg.block_updates?"ENABLED":"DISABLED",cfg.block_updates?GREEN:MUTED);
  text("THIS IS THE NATIVE VYPEX USER MODE INTERFACE",445,830,2,MUTED);
  text("PRIVILEGED HEN FEATURES REQUIRE A SEPARATE COMPATIBLE RUNTIME",445,870,2,AMBER);
 } else if(page==1){
  card(440,275,1410,190,"SYSTEM STATUS","FIRMWARE PROFILE  14 02","KERNEL ACCESS  NOT AVAILABLE IN THIS BUILD",AMBER);
  card(440,495,1410,190,"RUNTIME","VYPEX NATIVE CORE","USER MODE APPLICATION",GREEN);
  card(440,715,1410,190,"FEATURE AVAILABILITY","DISPLAY AND CONTROLLER UI","NO KERNEL PATCH OR PAYLOAD LOADER",MUTED);
 } else if(page==2){
  card(440,275,1410,190,"CONTROLLER INPUT","ACTIVE CONTROLLERS",controller_count?"CONTROLLER CONNECTED":"NO SUPPORTED CONTROLLER DETECTED",controller_count?GREEN:AMBER);
  card(440,495,1410,190,"INPUT BACKEND","NATIVE SCEPAD API","UP TO FOUR PS4-RECOGNIZED CONTROLLERS",WHITE);
  card(440,715,1410,190,"COMPATIBILITY","DUALSHOCK 4 AND OS-EXPOSED DEVICES","XBOX / DUALSENSE NEED A PS4-COMPATIBLE ADAPTER",AMBER);
 } else if(page==3){
  card(440,275,1410,190,"PLUGIN REGISTRY","REGISTERED PLUGINS",plugins.count()?"ENTRIES ARE STORED IN THE LOCAL REGISTRY":"NO PLUGINS REGISTERED",WHITE);
  card(440,495,1410,190,"PLUGIN LOADING","STATUS  UNAVAILABLE","PRX LOADING NEEDS A PRIVILEGED RUNTIME",AMBER);
  card(440,715,1410,190,"SAFETY","NO FILES WILL BE LOADED","REGISTRY ONLY - NO EXECUTION BACKEND",MUTED);
 } else if(page==4){
  const char* opts[]={"BLOCK SYSTEM UPDATES","SHOW NOTIFICATIONS","PLUGIN AUTOLOAD"};
  const bool vals[]={cfg.block_updates,cfg.show_notifications,cfg.plugin_autoload};
  for(int i=0;i<3;++i){
   int y=285+i*180; rect(440,y,1410,135,CARD);
   text(opts[i],480,y+32,3,WHITE);
   rect(1590,y+30,190,66,vals[i]?0xFF244B3D:0xFF343440);
   text(vals[i]?"ON":"OFF",1650,y+52,3,vals[i]?GREEN:MUTED);
   if(selected==i)rect(440,y,5,135,PURPLE);
  }
  text("CROSS TO TOGGLE  /  UP DOWN TO SELECT",445,885,2,MUTED);
 } else {
  card(440,275,1410,190,"VYPEX CR CK","ORIGINAL NATIVE HOME BREW FOUNDATION","PURPLE INTERFACE  /  OPENORBIS TOOLCHAIN",PURPLE);
  card(440,495,1410,190,"BUILD TARGET","PLAYSTATION 4","FIRMWARE PROFILE 14 02",WHITE);
  card(440,715,1410,190,"PROJECT STATUS","UI FOUNDATION","NOT A HEN OR SYSTEM SHELL REPLACEMENT",AMBER);
 }
 rect(390,1040,W-390,40,PANEL);
 text("DPAD NAVIGATE",430,1052,1,MUTED);
 text("CROSS SELECT",720,1052,1,MUTED);
 text("VYPEX",1740,1052,1,PURPLE);
 (void)frame;
}
}
int main() {
 vypex::Config config=vypex::Config::defaults();
 vypex::PluginRegistry plugins;
 OrbisUserServiceInitializeParams params={};
 params.priority=ORBIS_KERNEL_PRIO_FIFO_LOWEST;
 const bool user_ok=sceUserServiceInitialize(&params)==0;
 if(user_ok)sceUserServiceGetInitialUser(&user);
 const bool pad_ok=scePadInit()==0;
 if(pad_ok&&user>=0){
  for(int i=0;i<MAX_CONTROLLERS;++i){
   pads[i]=scePadOpen(user,ORBIS_PAD_PORT_TYPE_STANDARD,i,nullptr);
   if(pads[i]>=0)++controller_count;
  }
 }
 if(!graphics_init()){
  sceKernelDebugOutText(0,"Vypex: native display initialization failed\n");
  graphics_end();
  for(int i=0;i<MAX_CONTROLLERS;++i)if(pads[i]>=0)scePadClose(pads[i]);
  if(pad_ok)scePadTerminate();
  if(user_ok)sceUserServiceTerminate();
  return 1;
 }
 int page=0,selected=0; std::int64_t frame=1; std::uint32_t previous=0;
 bool running=true;
 while(running){
  std::uint32_t now=0;
  controller_count=0;
  for(int i=0;i<MAX_CONTROLLERS;++i){
   if(pads[i]<0)continue;
   OrbisPadData current={};
   if(scePadRead(pads[i],&current,1)==0&&current.connected){
    ++controller_count;
    if(now==0)now=current.buttons;
   } else {
    scePadClose(pads[i]); pads[i]=-1;
   }
  }
  if((frame%120)==0&&user>=0){
   for(int i=0;i<MAX_CONTROLLERS;++i)if(pads[i]<0)
    pads[i]=scePadOpen(user,ORBIS_PAD_PORT_TYPE_STANDARD,i,nullptr);
  }
  const std::uint32_t pressed=now&~previous; previous=now;
  if(pressed&ORBIS_PAD_BUTTON_UP){if(page==4)selected=(selected+2)%3;else page=(page+5)%6;}
  if(pressed&ORBIS_PAD_BUTTON_DOWN){if(page==4)selected=(selected+1)%3;else page=(page+1)%6;}
  if(pressed&ORBIS_PAD_BUTTON_LEFT)page=(page+5)%6;
  if(pressed&ORBIS_PAD_BUTTON_RIGHT)page=(page+1)%6;
  if(pressed&ORBIS_PAD_BUTTON_CROSS){
   if(page==4){
    if(selected==0)config.block_updates=!config.block_updates;
    if(selected==1)config.show_notifications=!config.show_notifications;
    if(selected==2)config.plugin_autoload=!config.plugin_autoload;
   }
  }
  if(pressed&ORBIS_PAD_BUTTON_CIRCLE){if(page!=0)page=0;else running=false;}
  render(page,selected,config,plugins,static_cast<int>(frame));
  if(!present(frame++)){sceKernelDebugOutText(0,"Vypex: display flip failed\n");break;}
 }
 graphics_end(); for(int i=0;i<MAX_CONTROLLERS;++i)if(pads[i]>=0)scePadClose(pads[i]); if(pad_ok)scePadTerminate(); if(user_ok)sceUserServiceTerminate();
 return 0;
}
