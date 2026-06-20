# KITES : RISC - V SIMULATOR

Kites a is RISC V simulator and assembly code editor built for RISC V ISA.
Kites support I,M,D and F extenstion in single cyle mod and I,M extension in 5 Stage pipelining mode.(D,F will be added in future releases).

## Downloading and Installation

Prebuilt libraries are available for Linux,Windows on [Release Page](https://github.com/hghit-harshit/Kites/releases)

### Ubuntu 
- Run chmod +x for the Kites AppImage.
- Run the file.

### Windows
- All the required files are inside the zip file, so you can just run the file.
- In case you get popup regarding some missing dll please contact the us.

## Building

To build Kites you will first neet to install Qt(version above 6.9)
Then simply build using

```bash
cmake -DCMAKE_PREFIX_PATH="path/to/Qt/6.x.x/<compiler>" ..
```
