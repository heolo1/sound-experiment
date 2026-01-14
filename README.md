# sounds

Some dumb project I'm making to experiment with sounds or whatever

Heavily inspired by Sebastian Lague's audio processing videos on YouTube

Requires SDL2 and the CUDA C++ whatever to be installed,
if you want to compile you'll probably need to change the -arch=sm_XX flag of
the NVCCFLAGS variable in the Makefile into whatever can run on your GPU

miniaudio submodule initialized with a sparse-checkout:

```sh
git sparse-checkout init --no-cone
git sparse-checkout set /miniaudio.h
```
