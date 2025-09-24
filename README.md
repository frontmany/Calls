# Build for msvc
* clone the repository


* run fetch_dependencies.bat file
(this will run the cmake which will install all the dependencies)


* run build_opus.bat and build_portaudio.bat files
(this bat files will buld all configurations debug/release of a certain libraries)


* however there is still one library you should build manually. Go to vendor/cryptopp folder and find cryptest.sln, open it and (super important) - go to cryptlib project properties, then  C/C++, then Code generation and set up runtime libraty to MDd or MD. If you not do that, your project wouldn't link. Then build both configurations of cryptlib debug with MDd runtime /release with MD runtime 


* run genDebug.bat or genRelease.bat depends on what configuration you want. Go to vendor\json\include\nlohmann and copy the json.hpp file one directory up to vendor\json\include


---
After these five steps you may go to build directory and open calls.sln
