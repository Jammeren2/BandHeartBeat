#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include "compositor.h"
inline std::vector<BYTE> DownsampleGray(const std::vector<BYTE>&pixels){std::vector<BYTE>gray(160*120);for(int y=0;y<120;y++)for(int x=0;x<160;x++){int i=((y*4)*640+x*4)*4;gray[y*160+x]=static_cast<BYTE>((pixels[i]*29+pixels[i+1]*150+pixels[i+2]*77)>>8);}return gray;}
class FaceWorker {
 std::mutex lock;std::condition_variable wake;bool quit=false,pending=false;std::vector<BYTE>gray;std::vector<FaceBox>boxes;ULONGLONG result_at=0;std::thread worker;
public:
 explicit FaceWorker(bool enabled){if(!enabled)return;worker=std::thread([this]{try{winrt::init_apartment(winrt::apartment_type::multi_threaded);auto detector=winrt::Windows::Media::FaceAnalysis::FaceDetector::CreateAsync().get();for(;;){std::vector<BYTE>input;{std::unique_lock<std::mutex>g(lock);wake.wait(g,[this]{return quit||pending;});if(quit)break;input=gray;pending=false;}auto bytes=winrt::Windows::Security::Cryptography::CryptographicBuffer::CreateFromByteArray(input);auto bitmap=winrt::Windows::Graphics::Imaging::SoftwareBitmap::CreateCopyFromBuffer(bytes,winrt::Windows::Graphics::Imaging::BitmapPixelFormat::Gray8,160,120);auto detected=detector.DetectFacesAsync(bitmap).get();std::vector<FaceBox>found;for(auto f:detected){auto b=f.FaceBox();found.push_back({static_cast<float>(b.X*4),static_cast<float>(b.Y*4),static_cast<float>(b.Width*4),static_cast<float>(b.Height*4)});}std::lock_guard<std::mutex>g(lock);boxes=std::move(found);result_at=GetTickCount64();}}catch(const winrt::hresult_error&e){std::wcerr<<L"Face detector disabled HRESULT 0x"<<std::hex<<e.code()<<L"; NO FACE fallback\n";}});}
 ~FaceWorker(){{std::lock_guard<std::mutex>g(lock);quit=true;wake.notify_all();}if(worker.joinable())worker.join();}
 void Submit(const std::vector<BYTE>&input){if(!worker.joinable())return;std::lock_guard<std::mutex>g(lock);gray=input;pending=true;wake.notify_one();}
 std::vector<FaceBox> Result(ULONGLONG tick,bool&fresh){std::lock_guard<std::mutex>g(lock);fresh=result_at&&tick-result_at<=1500;return fresh?boxes:std::vector<FaceBox>{};}
};
