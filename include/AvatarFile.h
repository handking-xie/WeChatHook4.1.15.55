#pragma once
#include "SelfQrCgi.h"
#include <wincodec.h>
#include <vector>
#pragma comment(lib,"ole32.lib")
#pragma comment(lib,"windowscodecs.lib")
namespace avatar_file {
template<class T> struct Com {T* p=nullptr;~Com(){if(p)p->Release();}T** out(){return &p;}T* operator->(){return p;}};
inline bool Read(const std::string& path,std::string& image,std::string& error){
 image.clear();error.clear();
 if(path.empty()||path.size()>32760||path.find('\0')!=std::string::npos){error="filepath must be a nonempty UTF-8 local absolute path";return false;}
 int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path.data(),static_cast<int>(path.size()),nullptr,0);
 if(!n){error="filepath is not valid UTF-8";return false;}
 std::wstring wide(n,L'\0');MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path.data(),static_cast<int>(path.size()),wide.data(),n);
 if(n<3||!((wide[0]>=L'A'&&wide[0]<=L'Z')||(wide[0]>=L'a'&&wide[0]<=L'z'))||wide[1]!=L':'||(wide[2]!=L'\\'&&wide[2]!=L'/')){error="filepath must be a drive absolute local file path";return false;}
 HANDLE file=CreateFileW(wide.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(file==INVALID_HANDLE_VALUE){error="cannot open avatar image";return false;}
 LARGE_INTEGER size{};DWORD actual=0;bool ok=GetFileType(file)==FILE_TYPE_DISK&&GetFileSizeEx(file,&size)&&size.QuadPart>0&&size.QuadPart<=4*1024*1024;
 if(ok){image.resize(static_cast<size_t>(size.QuadPart));ok=ReadFile(file,image.data(),static_cast<DWORD>(image.size()),&actual,nullptr)&&actual==image.size();}
 CloseHandle(file);
 if(!ok){image.clear();error="avatar must be a readable file of 1 to 4194304 bytes";return false;}
 HRESULT hr=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
 struct Uninit{bool active;~Uninit(){if(active)CoUninitialize();}} uninit{SUCCEEDED(hr)};
 if(FAILED(hr)&&hr!=RPC_E_CHANGED_MODE){image.clear();error="cannot initialize image decoder";return false;}
 Com<IWICImagingFactory> factory;Com<IWICStream> stream;Com<IWICBitmapDecoder> decoder;Com<IWICBitmapFrameDecode> frame;Com<IWICFormatConverter> converter;
 GUID format{};UINT width=0,height=0;
 ok=SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_IWICImagingFactory,reinterpret_cast<void**>(factory.out())))&&
    SUCCEEDED(factory->CreateStream(stream.out()))&&SUCCEEDED(stream->InitializeFromMemory(reinterpret_cast<BYTE*>(image.data()),static_cast<DWORD>(image.size())))&&
    SUCCEEDED(factory->CreateDecoderFromStream(stream.p,nullptr,WICDecodeMetadataCacheOnLoad,decoder.out()))&&SUCCEEDED(decoder->GetContainerFormat(&format))&&
    (IsEqualGUID(format,GUID_ContainerFormatJpeg)||IsEqualGUID(format,GUID_ContainerFormatPng))&&SUCCEEDED(decoder->GetFrame(0,frame.out()))&&
    SUCCEEDED(frame->GetSize(&width,&height))&&width&&height&&width<=4096&&height<=4096&&
    SUCCEEDED(factory->CreateFormatConverter(converter.out()))&&SUCCEEDED(converter->Initialize(frame.p,GUID_WICPixelFormat32bppBGRA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom));
 if(ok){std::vector<BYTE> pixels(static_cast<size_t>(width)*height*4);ok=SUCCEEDED(converter->CopyPixels(nullptr,width*4,static_cast<UINT>(pixels.size()),pixels.data()));}
 if(!ok){image.clear();error="avatar must decode as JPEG or PNG, dimensions 1 to 4096";}
 return ok;
}
}
