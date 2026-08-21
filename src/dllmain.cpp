#include <Windows.h>
#include <string>
#include <unordered_map>
#include <list>
#include <mutex>
#include <cstdio>
#include <unordered_set>
#include <intrin.h>

using TextFn=void*(__fastcall*)(void*,const char*);
using AssignFn=void*(__fastcall*)(void*,const char*,size_t);
using FontFn=void(__fastcall*)(void*,const char*);
using GlyphFn=void*(__fastcall*)(void*,void*,const char*,void*);
struct PB{BYTE v,m;};
static HMODULE g_dll; static TextFn g_real1,g_real2,g_real3; static AssignFn g_assign;
static FontFn g_fontCompile;
static GlyphFn g_glyphBuild;
static size_t g_textStringOffset1=0,g_textStringOffset2=0;
static uintptr_t g_menuLabelAssignSite=0;
static uintptr_t g_controlLabelAssignSite1=0,g_controlLabelAssignSite2=0;
static BYTE* g_imageBegin; static BYTE* g_imageEnd; static thread_local bool g_assignBusy=false;
static thread_local uintptr_t g_callsite=0;
static std::unordered_map<std::string,std::string> g_dict;
static std::list<std::string> g_text; static std::mutex g_lock;
static std::unordered_set<std::string> g_missing; static std::mutex g_missingLock;
static std::unordered_set<std::string> g_traced;
static std::string g_missingPath,g_tracePath; static bool g_traceEnabled=false;
static INIT_ONCE g_once=INIT_ONCE_STATIC_INIT; static BOOL g_ok=FALSE;
#define X(a) {a,0xff}
#define W {0,0}
// UI control constructors: relative operands are deliberately wildcarded.
static const PB P1[]={X(0x48),X(0x89),X(0x5c),X(0x24),X(0x20),X(0x48),X(0x89),X(0x4c),X(0x24),X(0x08),X(0x55),X(0x56),X(0x57),X(0x41),X(0x54),X(0x41),X(0x55),X(0x41),X(0x56),X(0x41),X(0x57),X(0x48),X(0x8b),X(0xec),X(0x48),X(0x83),X(0xec),X(0x20),X(0x4c),X(0x8b),X(0xe2),X(0x48),X(0x8b),X(0xd9),X(0xe8),W,W,W,W,X(0x90),X(0x48),X(0x8d),X(0x05)};
static const PB P2[]={X(0x48),X(0x89),X(0x5c),X(0x24),X(0x18),X(0x48),X(0x89),X(0x6c),X(0x24),X(0x20),X(0x48),X(0x89),X(0x4c),X(0x24),X(0x08),X(0x56),X(0x57),X(0x41),X(0x54),X(0x41),X(0x56),X(0x41),X(0x57),X(0x48),X(0x83),X(0xec),X(0x20),X(0x48),X(0x8b),X(0xfa),X(0x48),X(0x8b),X(0xd9),X(0xe8),W,W,W,W,X(0x90),X(0x48),X(0x8d),X(0x05),W,W,W,W,X(0x48),X(0x89),X(0x03),X(0x45),X(0x33),X(0xe4),X(0x4c),X(0x89),X(0xa3),X(0x38),X(0x01),X(0x00),X(0x00),X(0x48),X(0x8d),X(0x93)};
static const PB P3[]={X(0x48),X(0x89),X(0x5c),X(0x24),X(0x18),X(0x48),X(0x89),X(0x6c),X(0x24),X(0x20),X(0x48),X(0x89),X(0x4c),X(0x24),X(0x08),X(0x56),X(0x57),X(0x41),X(0x54),X(0x41),X(0x56),X(0x41),X(0x57),X(0x48),X(0x83),X(0xec),X(0x20),X(0x48),X(0x8b),X(0xfa),X(0x48),X(0x8b),X(0xd9),X(0xe8),W,W,W,W,X(0x90),X(0x48),X(0x8d),X(0x05),W,W,W,W,X(0x48),X(0x89),X(0x03),X(0x45),X(0x33),X(0xe4),X(0x4c),X(0x89),X(0xa3),X(0x38),X(0x01),X(0x00),X(0x00),X(0x48),X(0x8d),X(0xb3)};
static const PB PA[]={X(0x48),X(0x89),X(0x5c),X(0x24),X(0x10),X(0x48),X(0x89),X(0x6c),X(0x24),X(0x18),X(0x56),X(0x57),X(0x41),X(0x56),X(0x48),X(0x83),X(0xec),X(0x30),X(0x4c),X(0x8b),X(0x71),X(0x18),X(0x49),X(0x8b),X(0xf0),X(0x48),X(0x8b),X(0xea)};
// mset::Text::setText(const char*). The string member offset is decoded from
// the ADD RCX, imm32 instruction instead of being fixed to one build.
static const PB PT[]={X(0x48),X(0x85),X(0xd2),X(0x74),W,X(0x49),X(0xc7),X(0xc0),X(0xff),X(0xff),X(0xff),X(0xff),X(0x0f),X(0x1f),X(0x40),X(0x00),X(0x49),X(0xff),X(0xc0),X(0x42),X(0x80),X(0x3c),X(0x02),X(0x00),X(0x75),W,X(0x48),X(0x81),X(0xc1),W,W,W,W};
// Menu::MenuItem label copy followed by its separate callback/binding copy.
static const PB PM[]={X(0x48),X(0x8d),X(0x4f),X(0x10),X(0xe8),W,W,W,W,X(0x48),X(0x8b),X(0xd5),X(0x48),X(0x8d),X(0x4f),X(0x38),X(0xe8),W,W,W,W};
// Font compiled-string cache entry. Both layout measurement and Slug drawing
// consume the compiled result produced by this function.
static const PB PF[]={X(0x48),X(0x85),X(0xd2),X(0x0f),X(0x84),W,W,W,W,X(0x48),X(0x89),X(0x5c),X(0x24),X(0x08),X(0x48),X(0x89),X(0x6c),X(0x24),X(0x18),X(0x56),X(0x57),X(0x41),X(0x56),X(0x48),X(0x83),X(0xec),X(0x30),X(0x48),X(0x8b),X(0xf2),X(0x4c),X(0x8b),X(0xf1),X(0x80),X(0x3a),X(0x00),X(0x0f),X(0x84),W,W,W,W};
// Font glyph/metrics builder called only on a compiled-string cache miss.
// Its third argument is presentation text; the outer cache retains English.
static const PB PG[]={X(0x48),X(0x89),X(0x5c),X(0x24),X(0x18),X(0x4c),X(0x89),X(0x4c),X(0x24),X(0x20),X(0x48),X(0x89),X(0x54),X(0x24),X(0x10),X(0x55),X(0x56),X(0x57),X(0x41),X(0x54),X(0x41),X(0x55),X(0x41),X(0x56),X(0x41),X(0x57),X(0x48),X(0x8d),X(0xac),X(0x24),W,W,W,W,X(0xb8),W,W,W,W,X(0xe8),W,W,W,W,X(0x48),X(0x2b),X(0xe0)};
static const PB PL1[]={X(0x48),X(0x85),X(0xf6),X(0x74),W,X(0x49),X(0xc7),X(0xc0),X(0xff),X(0xff),X(0xff),X(0xff),X(0x49),X(0xff),X(0xc0),X(0x42),X(0x80),X(0x3c),X(0x06),X(0x00),X(0x75),W,X(0x48),X(0x8d),X(0x8b),X(0xb0),X(0x00),X(0x00),X(0x00),X(0x48),X(0x8b),X(0xd6),X(0xe8),W,W,W,W,X(0xc7),X(0x83),X(0xb4),X(0x01),X(0x00),X(0x00),X(0x00),X(0x00),X(0x80),X(0xbf)};
static const PB PL2[]={X(0x48),X(0x85),X(0xff),X(0x74),W,X(0x49),X(0xc7),X(0xc0),X(0xff),X(0xff),X(0xff),X(0xff),X(0x49),X(0xff),X(0xc0),X(0x42),X(0x80),X(0x3c),X(0x07),X(0x00),X(0x75),W,X(0x48),X(0x8d),X(0x8b),X(0xb0),X(0x00),X(0x00),X(0x00),X(0x48),X(0x8b),X(0xd7),X(0xe8),W,W,W,W,X(0x90),X(0x48),X(0x8b),X(0xc3),X(0x48),X(0x8b),X(0x5c),X(0x24),X(0x60),X(0x48),X(0x8b),X(0x6c),X(0x24),X(0x68)};
#undef X
#undef W

static std::string trim(const std::string&s){auto a=s.find_first_not_of(" \t\r\n");if(a==s.npos)return{};auto b=s.find_last_not_of(" \t\r\n");return s.substr(a,b-a+1);}
static bool loadDict(){char p[MAX_PATH];DWORD n=GetModuleFileNameA(g_dll,p,MAX_PATH);if(!n||n>=MAX_PATH)return false;char*q=strrchr(p,'\\');if(!q)return false;strcpy_s(q+1,MAX_PATH-(q-p+1),"dictionary.txt");FILE*f=nullptr;if(fopen_s(&f,p,"r")||!f)return false;char line[2048];while(fgets(line,sizeof line,f)){std::string s=trim(line);if(s.empty()||s[0]=='#')continue;auto z=s.find(';');if(z==s.npos)z=s.find('\t');if(z==s.npos)continue;auto k=trim(s.substr(0,z)),v=trim(s.substr(z+1));if(!k.empty()&&!v.empty())g_dict[k]=v;}fclose(f);return !g_dict.empty();}
static void logMissing(const char*kind,const char*s,size_t n){if(!s||n<2||n>256)return;bool alpha=false;for(size_t i=0;i<n;i++){unsigned char c=s[i];if(c=='/'||c=='\\'||c<0x20||c>0x7e)return;if((c>='A'&&c<='Z')||(c>='a'&&c<='z'))alpha=true;}if(!alpha)return;std::string text(s,n),key=std::string(kind)+"\t"+text;std::lock_guard<std::mutex>l(g_missingLock);if(!g_missing.insert(key).second)return;uintptr_t base=(uintptr_t)GetModuleHandleW(nullptr);FILE*f=nullptr;if(!g_missingPath.empty()&&!fopen_s(&f,g_missingPath.c_str(),"ab")&&f){fprintf(f,"%s\t0x%llX\t%s\r\n",kind,(unsigned long long)(g_callsite-base),text.c_str());fclose(f);}}
static bool traceable(const char*s,size_t n){if(!s||n<2||n>256)return false;bool alpha=false;for(size_t i=0;i<n;i++){unsigned char c=s[i];if(c<0x20||c>0x7e)return false;if((c>='A'&&c<='Z')||(c>='a'&&c<='z'))alpha=true;}return alpha;}
static void traceAssign(const char*s,size_t n,uintptr_t site,bool imageSource,bool known,bool translated){
 if(!g_traceEnabled||!traceable(s,n))return;void*frames[12]{};USHORT count=CaptureStackBackTrace(0,_countof(frames),frames,nullptr);uintptr_t base=(uintptr_t)GetModuleHandleW(nullptr);
 char id[96];sprintf_s(id,"%llX:%u:%.*s",(unsigned long long)(site-base),(unsigned)n,(int)(n>48?48:n),s);std::lock_guard<std::mutex>l(g_missingLock);if(!g_traced.insert(id).second)return;
 FILE*f=nullptr;if(fopen_s(&f,g_tracePath.c_str(),"ab")||!f)return;fprintf(f,"ASSIGN\t0x%llX\t%lu\t%s\t%s\t%s\t",(unsigned long long)(site-base),GetCurrentThreadId(),imageSource?"image":"heap",known?"known":"missing",translated?"translated":"untouched");
 for(USHORT i=0;i<count;i++){uintptr_t p=(uintptr_t)frames[i];if(i)fputc(',',f);if(p>=base&&p<(uintptr_t)g_imageEnd)fprintf(f,"+0x%llX",(unsigned long long)(p-base));else fprintf(f,"@0x%llX",(unsigned long long)p);}fprintf(f,"\t%.*s\r\n",(int)n,s);fclose(f);
}
static const char* trImpl(const char*s){if(!s||!*s)return s;std::string r(s);auto a=r.find_first_not_of(" \t"),b=r.find_last_not_of(" \t");if(a==r.npos)return s;std::string key=r.substr(a,b-a+1);auto i=g_dict.find(key);if(i==g_dict.end()){logMissing("CTOR",key.c_str(),key.size());return s;}std::string out=r.substr(0,a)+i->second+r.substr(b+1);std::lock_guard<std::mutex>l(g_lock);g_text.push_back(std::move(out));return g_text.back().c_str();}
static const char* tr(const char*s){__try{return trImpl(s);}__except(EXCEPTION_EXECUTE_HANDLER){return s;}}
static void*h1(void*a,const char*s){g_callsite=(uintptr_t)_ReturnAddress();return g_real1(a,tr(s));} static void*h2(void*a,const char*s){g_callsite=(uintptr_t)_ReturnAddress();return g_real2(a,tr(s));}
static void*h3(void*a,const char*s){g_callsite=(uintptr_t)_ReturnAddress();return g_real3(a,tr(s));}
// Only call sites verified as UI presentation models may receive translated
// strings. Workspace IDs, asset keys, paths and filters stay English.
static bool displayAssignSite(uintptr_t rva){static const uintptr_t sites[]={0xCF67E3,0xA605E9,0xCC3BEF,0x75777,0x759F6,0x12613B,0x126DC9,0x40A33C,0x4C9346,0xBBBECE,0xBBC202,0xBBCCA7,0xBBCD36,0xBBCFF1,0xBBD2E3,0xBBD6B4,0xBBDBC2,0xBBDDD8,0xBBE0E8,0xBC2AD5,0xBC2B14,0xBC2B58,0xBC2BDB,0xBCB647,0xBD8D22,0xBD8E32,0xBD8FA0,0xBD90BF,0x87E23E,0x87F142,0x88FE99,0x89376A,0x4E3077,0x7B24DE,0xBFB425};for(auto x:sites)if(rva==x)return true;return false;}
static void*ha(void*a,const char*s,size_t n){if(g_assignBusy||!s||n==0||n>256)return g_assign(a,s,n);g_callsite=(uintptr_t)_ReturnAddress();bool imageSource=(BYTE*)s>=g_imageBegin&&(BYTE*)s+n<g_imageEnd;g_assignBusy=true;std::string key(s,n);auto i=g_dict.find(key);const char*out=s;size_t outLen=n;bool translated=false;if(g_callsite==g_menuLabelAssignSite&&i!=g_dict.end()){out=i->second.c_str();outLen=i->second.size();translated=true;}traceAssign(s,n,g_callsite,imageSource,i!=g_dict.end(),translated);g_assignBusy=false;return g_assign(a,out,outLen);}
static void*ht1(void*a,const char*s){if(!s)return a;g_callsite=(uintptr_t)_ReturnAddress();const char*out=tr(s);return g_assign((BYTE*)a+g_textStringOffset1,out,strlen(out));}
static void*ht2(void*a,const char*s){if(!s)return a;g_callsite=(uintptr_t)_ReturnAddress();const char*out=tr(s);return g_assign((BYTE*)a+g_textStringOffset2,out,strlen(out));}
static void hf(void*a,const char*s){g_callsite=(uintptr_t)_ReturnAddress();g_fontCompile(a,tr(s));}
static void*hg(void*a,void*out,const char*s,void*options){g_callsite=(uintptr_t)_ReturnAddress();return g_glyphBuild(a,out,tr(s),options);}
static bool textSec(BYTE*&p,size_t&n){BYTE*b=(BYTE*)GetModuleHandleW(nullptr);if(!b)return false;auto*d=(IMAGE_DOS_HEADER*)b;auto*t=(IMAGE_NT_HEADERS64*)(b+d->e_lfanew);auto*s=IMAGE_FIRST_SECTION(t);for(WORD i=0;i<t->FileHeader.NumberOfSections;i++,s++)if(!memcmp(s->Name,".text",5)){p=b+s->VirtualAddress;n=s->Misc.VirtualSize;return true;}return false;}
static BYTE*find1(const PB*p,size_t n){BYTE*b;size_t z;if(!textSec(b,z)||z<n)return nullptr;BYTE*f=nullptr;for(size_t i=0;i<=z-n;i++){bool ok=true;for(size_t j=0;j<n;j++)if(p[j].m&&(b[i+j]&p[j].m)!=p[j].v){ok=false;break;}if(ok){if(f)return nullptr;f=b+i;}}return f;}
static bool findTextSetters(BYTE*&a,BYTE*&b){BYTE*p;size_t z;if(!textSec(p,z)||z<_countof(PT))return false;a=b=nullptr;for(size_t i=0;i<=z-_countof(PT);i++){bool ok=true;for(size_t j=0;j<_countof(PT);j++)if(PT[j].m&&(p[i+j]&PT[j].m)!=PT[j].v){ok=false;break;}if(ok){if(!a)a=p+i;else if(!b)b=p+i;else return false;}}if(!a||!b)return false;DWORD oa=*(DWORD*)(a+29),ob=*(DWORD*)(b+29);if(oa==ob)return false;if(oa>ob){BYTE*t=a;a=b;b=t;}return true;}
static void jump(BYTE*p,const void*t){p[0]=0x48;p[1]=0xb8;*(const void**)(p+2)=t;p[10]=0xff;p[11]=0xe0;}
static bool hook(BYTE*t,size_t n,const void*h,TextFn&r){BYTE*x=(BYTE*)VirtualAlloc(nullptr,n+16,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);if(!x)return false;memcpy(x,t,n);jump(x+n,t+n);DWORD old;if(!VirtualProtect(t,n,PAGE_EXECUTE_READWRITE,&old)){VirtualFree(x,0,MEM_RELEASE);return false;}jump(t,h);for(size_t i=12;i<n;i++)t[i]=0x90;VirtualProtect(t,n,old,&old);FlushInstructionCache(GetCurrentProcess(),t,n);r=(TextFn)x;return true;}
static bool patchOnly(BYTE*t,size_t n,const void*h){DWORD old;if(!VirtualProtect(t,n,PAGE_EXECUTE_READWRITE,&old))return false;jump(t,h);for(size_t i=12;i<n;i++)t[i]=0x90;VirtualProtect(t,n,old,&old);FlushInstructionCache(GetCurrentProcess(),t,n);return true;}
static bool hookFont(BYTE*t){BYTE*x=(BYTE*)VirtualAlloc(nullptr,64,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);if(!x)return false;size_t q=0;x[q++]=0x48;x[q++]=0x85;x[q++]=0xd2;x[q++]=0x75;x[q++]=0x0c;x[q++]=0x48;x[q++]=0xb8;*(BYTE**)(x+q)=t+0x164;q+=8;x[q++]=0xff;x[q++]=0xe0;memcpy(x+q,t+9,14);q+=14;jump(x+q,t+23);q+=12;DWORD old;if(!VirtualProtect(t,23,PAGE_EXECUTE_READWRITE,&old)){VirtualFree(x,0,MEM_RELEASE);return false;}jump(t,hf);for(size_t i=12;i<23;i++)t[i]=0x90;VirtualProtect(t,23,old,&old);FlushInstructionCache(GetCurrentProcess(),t,23);g_fontCompile=(FontFn)x;return true;}
static BOOL CALLBACK install(PINIT_ONCE,PVOID,PVOID*){char local[MAX_PATH];DWORD ln=GetEnvironmentVariableA("LOCALAPPDATA",local,MAX_PATH);if(ln&&ln<MAX_PATH){std::string dir=std::string(local)+"\\Marmoset Toolbag 5";g_missingPath=dir+"\\ChineseLocalizer_missing.tsv";g_tracePath=dir+"\\ChineseLocalizer_trace.tsv";}char modulePath[MAX_PATH]{};DWORD mn=GetModuleFileNameA(g_dll,modulePath,MAX_PATH);if(mn&&mn<MAX_PATH){char*q=strrchr(modulePath,'\\');if(q){strcpy_s(q+1,MAX_PATH-(q-modulePath+1),"trace.enabled");g_traceEnabled=GetFileAttributesA(modulePath)!=INVALID_FILE_ATTRIBUTES;}}if(!loadDict())return TRUE;BYTE*d=find1(PA,_countof(PA)),*m=find1(PM,_countof(PM)),*font=find1(PF,_countof(PF)),*t1=nullptr,*t2=nullptr;if(!d||!m||!font||!findTextSetters(t1,t2))return TRUE;BYTE*base=(BYTE*)GetModuleHandleW(nullptr);auto*dos=(IMAGE_DOS_HEADER*)base;auto*nt=(IMAGE_NT_HEADERS64*)(base+dos->e_lfanew);g_imageBegin=base;g_imageEnd=base+nt->OptionalHeader.SizeOfImage;g_menuLabelAssignSite=(uintptr_t)(m+9);g_textStringOffset1=*(DWORD*)(t1+29);g_textStringOffset2=*(DWORD*)(t2+29);if(g_textStringOffset1<0x20||g_textStringOffset2>0x1000)return TRUE;TextFn temp=nullptr;if(!hook(d,18,ha,temp))return TRUE;g_assign=(AssignFn)temp;if(!patchOnly(t1,12,ht1)||!patchOnly(t2,12,ht2)||!hookFont(font))return TRUE;g_ok=TRUE;return TRUE;}
extern "C" __declspec(dllexport) BOOL WINAPI InstallHook(){InitOnceExecuteOnce(&g_once,install,nullptr,nullptr);return g_ok;}
static DWORD WINAPI installThread(LPVOID){if(InstallHook()){wchar_t name[96];swprintf_s(name,L"Local\\ToolbagChineseHookReady_%lu",GetCurrentProcessId());HANDLE e=OpenEventW(EVENT_MODIFY_STATE,FALSE,name);if(e){SetEvent(e);CloseHandle(e);}}return 0;}
BOOL APIENTRY DllMain(HMODULE h,DWORD r,LPVOID){if(r==DLL_PROCESS_ATTACH){g_dll=h;DisableThreadLibraryCalls(h);HANDLE t=CreateThread(nullptr,0,installThread,nullptr,0,nullptr);if(t)CloseHandle(t);}return TRUE;}
