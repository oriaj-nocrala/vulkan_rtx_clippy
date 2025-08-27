# Clippy RTX - Claude Code Reference

## Key File Locations

### Core RTX Implementation
- `src/RayTracingPipeline.cpp:224` - `createAccelerationStructures()` - Complete BLAS implementation
- `src/RayTracingPipeline.cpp:343` - `beginSingleTimeCommands()` - Command buffer helpers
- `include/RayTracingPipeline.h:9` - Constructor with commandPool + graphicsQueue params
- `src/ClippyRTXApp.cpp:275` - RTX pipeline instantiation with command pool/queue

### Build System
- `CMakeLists.txt:72` - Shader compilation with SPIR-V 1.5 flags
- `build/` - Build directory, run `make && ./ClippyRTX` from here

### BLAS Implementation Status
- ✅ Step 1: Size calculation (669,568 bytes for 9,528 triangles)
- ✅ Step 2: Buffer allocation (device local memory)  
- ✅ Step 3: Acceleration structure creation
- ✅ Step 4: **Real GPU building with command buffers**

## Testing Commands
```bash
cd /home/oriaj/Prog/CPP/rtx_clippy/build
make && timeout 10 ./ClippyRTX
```

## Architecture Notes
- RayTracingPipeline now has real command buffer building
- Command pool/graphics queue properly integrated
- Step-by-step validation approach maintained
- Ready for TLAS implementation next