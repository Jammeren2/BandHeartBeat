#include "installed_camera.h"
#include <iostream>
int main(){if(!OwnedRegistration(L"{5B4131AF-3298-47AA-9C12-33407A81914B}",L"BandHeartBeat Virtual Camera",L"Both"))return 1;if(OwnedRegistration(L"{other}",L"BandHeartBeat Virtual Camera",L"Both"))return 2;if(OwnedRegistration(L"{5B4131AF-3298-47AA-9C12-33407A81914B}",L"Unrelated",L"Both"))return 3;std::cout<<"PASS upgrade ownership guard pure inputs; no registry writes\n";}
