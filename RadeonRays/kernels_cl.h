/* This is an auto-generated file. Do not edit manually*/

static const char g_build_hlbvh_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
"/** \n"\
"    \\file build_bvh.cl \n"\
"    \\author Dmitry Kozlov \n"\
"    \\version 1.0 \n"\
"    \\brief HLBVH build implementation \n"\
" \n"\
"    IntersectorHlbvh implementation is based on the following paper: \n"\
"    \"HLBVH: Hierarchical LBVH Construction for Real-Time Ray Tracing\" \n"\
"    Jacopo Pantaleoni (NVIDIA), David Luebke (NVIDIA), in High Performance Graphics 2010, June 2010 \n"\
"    https://research.nvidia.com/sites/default/files/publications/HLBVH-final.pdf \n"\
" \n"\
"    Pros: \n"\
"        -Very fast to build and update. \n"\
"    Cons: \n"\
"        -Poor BVH quality, slow traversal. \n"\
" */ \n"\
"/************************************************************************* \n"\
"INCLUDES \n"\
"**************************************************************************/ \n"\
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
" \n"\
"/************************************************************************* \n"\
"DEFINES \n"\
"**************************************************************************/ \n"\
"#define PI 3.14159265358979323846f \n"\
"#define KERNEL __kernel \n"\
"#define GLOBAL __global \n"\
"#define INLINE __attribute__((always_inline)) \n"\
"#define HIT_MARKER 1 \n"\
"#define MISS_MARKER -1 \n"\
"#define INVALID_IDX -1 \n"\
" \n"\
"/************************************************************************* \n"\
"EXTENSIONS \n"\
"**************************************************************************/ \n"\
"#ifdef AMD_MEDIA_OPS \n"\
"#pragma OPENCL EXTENSION cl_amd_media_ops2 : enable \n"\
"#endif \n"\
" \n"\
"/************************************************************************* \n"\
"TYPES \n"\
"**************************************************************************/ \n"\
" \n"\
"// Axis aligned bounding box \n"\
"typedef struct \n"\
"{ \n"\
"    float4 pmin; \n"\
"    float4 pmax; \n"\
"} bbox; \n"\
" \n"\
"// Ray definition \n"\
"typedef struct \n"\
"{ \n"\
"    float4 o; \n"\
"    float4 d; \n"\
"    int2 extra; \n"\
"    int doBackfaceCulling; \n"\
"    int padding; \n"\
"} ray; \n"\
" \n"\
"// Intersection definition \n"\
"typedef struct \n"\
"{ \n"\
"    int shape_id; \n"\
"    int prim_id; \n"\
"    int2 padding; \n"\
" \n"\
"    float4 uvwt; \n"\
"} Intersection; \n"\
" \n"\
" \n"\
"/************************************************************************* \n"\
"HELPER FUNCTIONS \n"\
"**************************************************************************/ \n"\
"INLINE \n"\
"int ray_get_mask(ray const* r) \n"\
"{ \n"\
"    return r->extra.x; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"int ray_is_active(ray const* r) \n"\
"{ \n"\
"    return r->extra.y; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"float ray_get_maxt(ray const* r) \n"\
"{ \n"\
"    return r->o.w; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"float ray_get_time(ray const* r) \n"\
"{ \n"\
"    return r->d.w; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"int ray_get_doBackfaceCull(ray const* r) \n"\
"{ \n"\
"    return r->doBackfaceCulling; \n"\
"} \n"\
" \n"\
"/************************************************************************* \n"\
"FUNCTIONS \n"\
"**************************************************************************/ \n"\
"#ifndef APPLE \n"\
"INLINE \n"\
"float4 make_float4(float x, float y, float z, float w) \n"\
"{ \n"\
"    float4 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    res.w = w; \n"\
"    return res; \n"\
"} \n"\
"INLINE \n"\
"float3 make_float3(float x, float y, float z) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    return res; \n"\
"} \n"\
"INLINE \n"\
"float2 make_float2(float x, float y) \n"\
"{ \n"\
"    float2 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    return res; \n"\
"} \n"\
"INLINE \n"\
"int2 make_int2(int x, int y) \n"\
"{ \n"\
"    int2 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    return res; \n"\
"} \n"\
"INLINE \n"\
"int3 make_int3(int x, int y, int z) \n"\
"{ \n"\
"    int3 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    return res; \n"\
"} \n"\
"#endif \n"\
" \n"\
"INLINE float min3(float a, float b, float c) \n"\
"{ \n"\
"#ifdef AMD_MEDIA_OPS \n"\
"    return amd_min3(a, b, c); \n"\
"#else \n"\
"    return min(min(a,b), c); \n"\
"#endif \n"\
"} \n"\
" \n"\
"INLINE float max3(float a, float b, float c) \n"\
"{ \n"\
"#ifdef AMD_MEDIA_OPS \n"\
"    return amd_max3(a, b, c); \n"\
"#else \n"\
"    return max(max(a,b), c); \n"\
"#endif \n"\
"} \n"\
" \n"\
" \n"\
"// Intersect ray against a triangle and return intersection interval value if it is in \n"\
"// (0, t_max], return t_max otherwise. \n"\
"INLINE \n"\
"float fast_intersect_triangle(ray r, float3 v1, float3 v2, float3 v3, float t_max) \n"\
"{ \n"\
"    float3 const e1 = v2 - v1; \n"\
"    float3 const e2 = v3 - v1; \n"\
" \n"\
"#ifdef RR_BACKFACE_CULL \n"\
"    if (ray_get_doBackfaceCull(&r) && dot(cross(e1, e2), r.d.xyz) > 0.f) \n"\
"    { \n"\
"        return t_max; \n"\
"    } \n"\
"#endif // RR_BACKFACE_CULL \n"\
" \n"\
"    float3 const s1 = cross(r.d.xyz, e2); \n"\
" \n"\
"    float denom = dot(s1, e1); \n"\
"    if (denom == 0.f) \n"\
"    { \n"\
"        return t_max; \n"\
"    } \n"\
"     \n"\
"#ifdef USE_SAFE_MATH \n"\
"    float const invd = 1.f / denom; \n"\
"#else \n"\
"    float const invd = native_recip(denom); \n"\
"#endif \n"\
" \n"\
"    float3 const d = r.o.xyz - v1; \n"\
"    float const b1 = dot(d, s1) * invd; \n"\
"    float3 const s2 = cross(d, e1); \n"\
"    float const b2 = dot(r.d.xyz, s2) * invd; \n"\
"    float const temp = dot(e2, s2) * invd; \n"\
" \n"\
"    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f || temp < 0.f || temp > t_max) \n"\
"    { \n"\
"        return t_max; \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        return temp; \n"\
"    } \n"\
"} \n"\
" \n"\
"INLINE \n"\
"float3 safe_invdir(ray r) \n"\
"{ \n"\
"    float const dirx = r.d.x; \n"\
"    float const diry = r.d.y; \n"\
"    float const dirz = r.d.z; \n"\
"    float const ooeps = 1e-8; \n"\
"    float3 invdir; \n"\
"    invdir.x = 1.0f / (fabs(dirx) > ooeps ? dirx : copysign(ooeps, dirx)); \n"\
"    invdir.y = 1.0f / (fabs(diry) > ooeps ? diry : copysign(ooeps, diry)); \n"\
"    invdir.z = 1.0f / (fabs(dirz) > ooeps ? dirz : copysign(ooeps, dirz)); \n"\
"    return invdir; \n"\
"} \n"\
" \n"\
"// Intersect rays vs bbox and return intersection span.  \n"\
"// Intersection criteria is ret.x <= ret.y \n"\
"INLINE \n"\
"float2 fast_intersect_bbox1(bbox box, float3 invdir, float3 oxinvdir, float t_max) \n"\
"{ \n"\
"    float3 const f = mad(box.pmax.xyz, invdir, oxinvdir); \n"\
"    float3 const n = mad(box.pmin.xyz, invdir, oxinvdir); \n"\
"    float3 const tmax = max(f, n); \n"\
"    float3 const tmin = min(f, n); \n"\
"    float const t1 = min(min3(tmax.x, tmax.y, tmax.z), t_max); \n"\
"    float const t0 = max(max3(tmin.x, tmin.y, tmin.z), 0.f); \n"\
"    return make_float2(t0, t1); \n"\
"} \n"\
" \n"\
"// Given a point in triangle plane, calculate its barycentrics \n"\
"INLINE \n"\
"float2 triangle_calculate_barycentrics(float3 p, float3 v1, float3 v2, float3 v3) \n"\
"{ \n"\
"    float3 const e1 = v2 - v1; \n"\
"    float3 const e2 = v3 - v1; \n"\
"    float3 const e = p - v1; \n"\
"    float const d00 = dot(e1, e1); \n"\
"    float const d01 = dot(e1, e2); \n"\
"    float const d11 = dot(e2, e2); \n"\
"    float const d20 = dot(e, e1); \n"\
"    float const d21 = dot(e, e2); \n"\
" \n"\
"    float denom = (d00 * d11 - d01 * d01); \n"\
"     \n"\
"    if (denom == 0.f) \n"\
"    { \n"\
"        return make_float2(0.f, 0.f); \n"\
"    } \n"\
"     \n"\
"#ifdef USE_SAFE_MATH \n"\
"    float const invdenom = 1.f / denom; \n"\
"#else \n"\
"    float const invdenom = native_recip(denom); \n"\
"#endif \n"\
" \n"\
"    float const b1 = (d11 * d20 - d01 * d21) * invdenom; \n"\
"    float const b2 = (d00 * d21 - d01 * d20) * invdenom; \n"\
"    return make_float2(b1, b2); \n"\
"} \n"\
" \n"\
"/************************************************************************* \n"\
"DEFINES \n"\
"**************************************************************************/ \n"\
"#define LEAFIDX(i) ((num_prims-1) + i) \n"\
"#define NODEIDX(i) (i) \n"\
"// Shortcut for delta evaluation \n"\
"#define DELTA(i,j) delta(morton_codes,num_prims,i,j) \n"\
" \n"\
"/************************************************************************* \n"\
"TYPE DEFINITIONS \n"\
"**************************************************************************/ \n"\
"typedef struct \n"\
"{ \n"\
"    int parent; \n"\
"    int left; \n"\
"    int right; \n"\
"    int next; \n"\
"} HlbvhNode; \n"\
" \n"\
"/************************************************************************* \n"\
"FUNCTIONS \n"\
"**************************************************************************/ \n"\
"// The following two functions are from \n"\
"// http://devblogs.nvidia.com/parallelforall/thinking-parallel-part-iii-tree-construction-gpu/ \n"\
"// Expands a 10-bit integer into 30 bits \n"\
"// by inserting 2 zeros after each bit. \n"\
"INLINE uint expand_bits(uint v) \n"\
"{ \n"\
"    v = (v * 0x00010001u) & 0xFF0000FFu; \n"\
"    v = (v * 0x00000101u) & 0x0F00F00Fu; \n"\
"    v = (v * 0x00000011u) & 0xC30C30C3u; \n"\
"    v = (v * 0x00000005u) & 0x49249249u; \n"\
"    return v; \n"\
"} \n"\
" \n"\
"// Calculates a 30-bit Morton code for the \n"\
"// given 3D point located within the unit cube [0,1]. \n"\
"INLINE uint calculate_morton_code(float3 p) \n"\
"{ \n"\
"    float x = min(max(p.x * 1024.0f, 0.0f), 1023.0f); \n"\
"    float y = min(max(p.y * 1024.0f, 0.0f), 1023.0f); \n"\
"    float z = min(max(p.z * 1024.0f, 0.0f), 1023.0f); \n"\
"    unsigned int xx = expand_bits((uint)x); \n"\
"    unsigned int yy = expand_bits((uint)y); \n"\
"    unsigned int zz = expand_bits((uint)z); \n"\
"    return xx * 4 + yy * 2 + zz; \n"\
"} \n"\
" \n"\
"// Make a union of two bboxes \n"\
"INLINE bbox bbox_union(bbox b1, bbox b2) \n"\
"{ \n"\
"    bbox res; \n"\
"    res.pmin = min(b1.pmin, b2.pmin); \n"\
"    res.pmax = max(b1.pmax, b2.pmax); \n"\
"    return res; \n"\
"} \n"\
" \n"\
"// Assign Morton codes to each of positions \n"\
"KERNEL void calculate_morton_code_main( \n"\
"    // Centers of primitive bounding boxes \n"\
"    GLOBAL bbox const* restrict primitive_bounds, \n"\
"    // Number of primitives \n"\
"    int num_primitive_bounds, \n"\
"    // Scene extents \n"\
"    GLOBAL bbox const* restrict scene_bound,  \n"\
"    // Morton codes \n"\
"    GLOBAL int* morton_codes \n"\
"    ) \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    if (global_id < num_primitive_bounds) \n"\
"    { \n"\
"        // Fetch primitive bound \n"\
"        bbox bound = primitive_bounds[global_id]; \n"\
"        // Calculate center and scene extents \n"\
"        float3 const center = (bound.pmax + bound.pmin).xyz * 0.5f; \n"\
"        float3 const scene_min = scene_bound->pmin.xyz; \n"\
"        float3 const scene_extents = scene_bound->pmax.xyz - scene_bound->pmin.xyz; \n"\
"        // Calculate morton code \n"\
"        morton_codes[global_id] = calculate_morton_code((center - scene_min) / scene_extents); \n"\
"    } \n"\
"} \n"\
" \n"\
" \n"\
" \n"\
"// Calculates longest common prefix length of bit representations \n"\
"// if  representations are equal we consider sucessive indices \n"\
"INLINE int delta(GLOBAL int const* morton_codes, int num_prims, int i1, int i2) \n"\
"{ \n"\
"    // Select left end \n"\
"    int left = min(i1, i2); \n"\
"    // Select right end \n"\
"    int right = max(i1, i2); \n"\
"    // This is to ensure the node breaks if the index is out of bounds \n"\
"    if (left < 0 || right >= num_prims)  \n"\
"    { \n"\
"        return -1; \n"\
"    } \n"\
"    // Fetch Morton codes for both ends \n"\
"    int left_code = morton_codes[left]; \n"\
"    int right_code = morton_codes[right]; \n"\
" \n"\
"    // Special handling of duplicated codes: use their indices as a fallback \n"\
"    return left_code != right_code ? clz(left_code ^ right_code) : (32 + clz(left ^ right)); \n"\
"} \n"\
" \n"\
"// Find span occupied by internal node with index idx \n"\
"INLINE int2 find_span(GLOBAL int const* restrict morton_codes, int num_prims, int idx) \n"\
"{ \n"\
"    // Find the direction of the range \n"\
"    int d = sign((float)(DELTA(idx, idx+1) - DELTA(idx, idx-1))); \n"\
" \n"\
"    // Find minimum number of bits for the break on the other side \n"\
"    int delta_min = DELTA(idx, idx-d); \n"\
" \n"\
"    // Search conservative far end \n"\
"    int lmax = 2; \n"\
"    while (DELTA(idx,idx + lmax * d) > delta_min) \n"\
"        lmax *= 2; \n"\
" \n"\
"    // Search back to find exact bound \n"\
"    // with binary search \n"\
"    int l = 0; \n"\
"    int t = lmax; \n"\
"    do \n"\
"    { \n"\
"        t /= 2; \n"\
"        if(DELTA(idx, idx + (l + t)*d) > delta_min) \n"\
"        { \n"\
"            l = l + t; \n"\
"        } \n"\
"    } \n"\
"    while (t > 1); \n"\
" \n"\
"    // Pack span  \n"\
"    int2 span; \n"\
"    span.x = min(idx, idx + l*d); \n"\
"    span.y = max(idx, idx + l*d); \n"\
"    return span; \n"\
"} \n"\
" \n"\
"// Find split idx within the span \n"\
"INLINE int find_split(GLOBAL int const* restrict morton_codes, int num_prims, int2 span) \n"\
"{ \n"\
"    // Fetch codes for both ends \n"\
"    int left = span.x; \n"\
"    int right = span.y; \n"\
" \n"\
"    // Calculate the number of identical bits from higher end \n"\
"    int num_identical = DELTA(left, right); \n"\
" \n"\
"    do \n"\
"    { \n"\
"        // Proposed split \n"\
"        int new_split = (right + left) / 2; \n"\
" \n"\
"        // If it has more equal leading bits than left and right accept it \n"\
"        if (DELTA(left, new_split) > num_identical) \n"\
"        { \n"\
"            left = new_split; \n"\
"        } \n"\
"        else \n"\
"        { \n"\
"            right = new_split; \n"\
"        } \n"\
"    } \n"\
"    while (right > left + 1); \n"\
" \n"\
"    return left; \n"\
"} \n"\
" \n"\
"// Set parent-child relationship \n"\
"KERNEL void emit_hierarchy_main( \n"\
"    // Sorted Morton codes of the primitives \n"\
"    GLOBAL int const* restrict morton_codes, \n"\
"    // Bounds \n"\
"    GLOBAL bbox const* restrict bounds, \n"\
"    // Primitive indices \n"\
"    GLOBAL int const* restrict indices, \n"\
"    // Number of primitives \n"\
"    int num_prims, \n"\
"    // Nodes \n"\
"    GLOBAL HlbvhNode* nodes, \n"\
"    // Leaf bounds \n"\
"    GLOBAL bbox* bounds_sorted \n"\
"    ) \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    // Set child \n"\
"    if (global_id < num_prims) \n"\
"    { \n"\
"        nodes[LEAFIDX(global_id)].left = nodes[LEAFIDX(global_id)].right = indices[global_id]; \n"\
"        bounds_sorted[LEAFIDX(global_id)] = bounds[indices[global_id]]; \n"\
"    } \n"\
"     \n"\
"    // Set internal nodes \n"\
"    if (global_id < num_prims - 1) \n"\
"    { \n"\
"        // Find span occupied by the current node \n"\
"        int2 range = find_span(morton_codes, num_prims, global_id); \n"\
" \n"\
"        // Find split position inside the range \n"\
"        int  split = find_split(morton_codes, num_prims, range); \n"\
" \n"\
"        // Create child nodes if needed \n"\
"        int c1idx = (split == range.x) ? LEAFIDX(split) : NODEIDX(split); \n"\
"        int c2idx = (split + 1 == range.y) ? LEAFIDX(split + 1) : NODEIDX(split + 1); \n"\
" \n"\
"        nodes[NODEIDX(global_id)].left = c1idx; \n"\
"        nodes[NODEIDX(global_id)].right = c2idx; \n"\
"        //nodes[NODEIDX(global_id)].next = (range.y + 1 < num_prims) ? range.y + 1 : -1; \n"\
"        nodes[c1idx].parent = NODEIDX(global_id); \n"\
"        //nodes[c1idx].next = c2idx; \n"\
"        nodes[c2idx].parent = NODEIDX(global_id); \n"\
"        //nodes[c2idx].next = nodes[NODEIDX(global_id)].next; \n"\
"    } \n"\
"} \n"\
" \n"\
"// Propagate bounds up to the root \n"\
"KERNEL void refit_bounds_main( \n"\
"    // Node bounds \n"\
"    GLOBAL bbox* bounds, \n"\
"    // Number of nodes \n"\
"    int num_prims, \n"\
"    // Nodes \n"\
"    GLOBAL HlbvhNode* nodes, \n"\
"    // Atomic flags \n"\
"    GLOBAL int* flags \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    // Start from leaf nodes \n"\
"    if (global_id < num_prims) \n"\
"    { \n"\
"        // Get my leaf index \n"\
"        int idx = LEAFIDX(global_id); \n"\
" \n"\
"        do \n"\
"        { \n"\
"            // Move to parent node \n"\
"            idx = nodes[idx].parent; \n"\
" \n"\
"            // Check node's flag \n"\
"            if (atomic_cmpxchg(flags + idx, 0, 1) == 1) \n"\
"            { \n"\
"                // If the flag was 1 the second child is ready and  \n"\
"                // this thread calculates bbox for the node \n"\
" \n"\
"                // Fetch kids \n"\
"                int lc = nodes[idx].left; \n"\
"                int rc = nodes[idx].right; \n"\
" \n"\
"                // Calculate bounds \n"\
"                bbox b = bbox_union(bounds[lc], bounds[rc]); \n"\
" \n"\
"                // Write bounds \n"\
"                bounds[idx] = b; \n"\
"            } \n"\
"            else \n"\
"            { \n"\
"                // If the flag was 0 set it to 1 and bail out. \n"\
"                // The thread handling the second child will \n"\
"                // handle this node. \n"\
"                break; \n"\
"            } \n"\
"        } \n"\
"        while (idx != 0); \n"\
"    } \n"\
"} \n"\
;
static const char g_common_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
" \n"\
"/************************************************************************* \n"\
"DEFINES \n"\
"**************************************************************************/ \n"\
"#define PI 3.14159265358979323846f \n"\
"#define KERNEL __kernel \n"\
"#define GLOBAL __global \n"\
"#define INLINE __attribute__((always_inline)) \n"\
"#define HIT_MARKER 1 \n"\
"#define MISS_MARKER -1 \n"\
"#define INVALID_IDX -1 \n"\
" \n"\
"/************************************************************************* \n"\
"EXTENSIONS \n"\
"**************************************************************************/ \n"\
"#ifdef AMD_MEDIA_OPS \n"\
"#pragma OPENCL EXTENSION cl_amd_media_ops2 : enable \n"\
"#endif \n"\
" \n"\
"/************************************************************************* \n"\
"TYPES \n"\
"**************************************************************************/ \n"\
" \n"\
"// Axis aligned bounding box \n"\
"typedef struct \n"\
"{ \n"\
"    float4 pmin; \n"\
"    float4 pmax; \n"\
"} bbox; \n"\
" \n"\
"// Ray definition \n"\
"typedef struct \n"\
"{ \n"\
"    float4 o; \n"\
"    float4 d; \n"\
"    int2 extra; \n"\
"    int doBackfaceCulling; \n"\
"    int padding; \n"\
"} ray; \n"\
" \n"\
"// Intersection definition \n"\
"typedef struct \n"\
"{ \n"\
"    int shape_id; \n"\
"    int prim_id; \n"\
"    int2 padding; \n"\
" \n"\
"    float4 uvwt; \n"\
"} Intersection; \n"\
" \n"\
" \n"\
"/************************************************************************* \n"\
"HELPER FUNCTIONS \n"\
"**************************************************************************/ \n"\
"INLINE \n"\
"int ray_get_mask(ray const* r) \n"\
"{ \n"\
"    return r->extra.x; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"int ray_is_active(ray const* r) \n"\
"{ \n"\
"    return r->extra.y; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"float ray_get_maxt(ray const* r) \n"\
"{ \n"\
"    return r->o.w; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"float ray_get_time(ray const* r) \n"\
"{ \n"\
"    return r->d.w; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"int ray_get_doBackfaceCull(ray const* r) \n"\
"{ \n"\
"    return r->doBackfaceCulling; \n"\
"} \n"\
" \n"\
"/************************************************************************* \n"\
"FUNCTIONS \n"\
"**************************************************************************/ \n"\
"#ifndef APPLE \n"\
"INLINE \n"\
"float4 make_float4(float x, float y, float z, float w) \n"\
"{ \n"\
"    float4 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    res.w = w; \n"\
"    return res; \n"\
"} \n"\
"INLINE \n"\
"float3 make_float3(float x, float y, float z) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    return res; \n"\
"} \n"\
"INLINE \n"\
"float2 make_float2(float x, float y) \n"\
"{ \n"\
"    float2 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    return res; \n"\
"} \n"\
"INLINE \n"\
"int2 make_int2(int x, int y) \n"\
"{ \n"\
"    int2 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    return res; \n"\
"} \n"\
"INLINE \n"\
"int3 make_int3(int x, int y, int z) \n"\
"{ \n"\
"    int3 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    return res; \n"\
"} \n"\
"#endif \n"\
" \n"\
"INLINE float min3(float a, float b, float c) \n"\
"{ \n"\
"#ifdef AMD_MEDIA_OPS \n"\
"    return amd_min3(a, b, c); \n"\
"#else \n"\
"    return min(min(a,b), c); \n"\
"#endif \n"\
"} \n"\
" \n"\
"INLINE float max3(float a, float b, float c) \n"\
"{ \n"\
"#ifdef AMD_MEDIA_OPS \n"\
"    return amd_max3(a, b, c); \n"\
"#else \n"\
"    return max(max(a,b), c); \n"\
"#endif \n"\
"} \n"\
" \n"\
" \n"\
"// Intersect ray against a triangle and return intersection interval value if it is in \n"\
"// (0, t_max], return t_max otherwise. \n"\
"INLINE \n"\
"float fast_intersect_triangle(ray r, float3 v1, float3 v2, float3 v3, float t_max) \n"\
"{ \n"\
"    float3 const e1 = v2 - v1; \n"\
"    float3 const e2 = v3 - v1; \n"\
" \n"\
"#ifdef RR_BACKFACE_CULL \n"\
"    if (ray_get_doBackfaceCull(&r) && dot(cross(e1, e2), r.d.xyz) > 0.f) \n"\
"    { \n"\
"        return t_max; \n"\
"    } \n"\
"#endif // RR_BACKFACE_CULL \n"\
" \n"\
"    float3 const s1 = cross(r.d.xyz, e2); \n"\
" \n"\
"    float denom = dot(s1, e1); \n"\
"    if (denom == 0.f) \n"\
"    { \n"\
"        return t_max; \n"\
"    } \n"\
"     \n"\
"#ifdef USE_SAFE_MATH \n"\
"    float const invd = 1.f / denom; \n"\
"#else \n"\
"    float const invd = native_recip(denom); \n"\
"#endif \n"\
" \n"\
"    float3 const d = r.o.xyz - v1; \n"\
"    float const b1 = dot(d, s1) * invd; \n"\
"    float3 const s2 = cross(d, e1); \n"\
"    float const b2 = dot(r.d.xyz, s2) * invd; \n"\
"    float const temp = dot(e2, s2) * invd; \n"\
" \n"\
"    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f || temp < 0.f || temp > t_max) \n"\
"    { \n"\
"        return t_max; \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        return temp; \n"\
"    } \n"\
"} \n"\
" \n"\
"INLINE \n"\
"float3 safe_invdir(ray r) \n"\
"{ \n"\
"    float const dirx = r.d.x; \n"\
"    float const diry = r.d.y; \n"\
"    float const dirz = r.d.z; \n"\
"    float const ooeps = 1e-8; \n"\
"    float3 invdir; \n"\
"    invdir.x = 1.0f / (fabs(dirx) > ooeps ? dirx : copysign(ooeps, dirx)); \n"\
"    invdir.y = 1.0f / (fabs(diry) > ooeps ? diry : copysign(ooeps, diry)); \n"\
"    invdir.z = 1.0f / (fabs(dirz) > ooeps ? dirz : copysign(ooeps, dirz)); \n"\
"    return invdir; \n"\
"} \n"\
" \n"\
"// Intersect rays vs bbox and return intersection span.  \n"\
"// Intersection criteria is ret.x <= ret.y \n"\
"INLINE \n"\
"float2 fast_intersect_bbox1(bbox box, float3 invdir, float3 oxinvdir, float t_max) \n"\
"{ \n"\
"    float3 const f = mad(box.pmax.xyz, invdir, oxinvdir); \n"\
"    float3 const n = mad(box.pmin.xyz, invdir, oxinvdir); \n"\
"    float3 const tmax = max(f, n); \n"\
"    float3 const tmin = min(f, n); \n"\
"    float const t1 = min(min3(tmax.x, tmax.y, tmax.z), t_max); \n"\
"    float const t0 = max(max3(tmin.x, tmin.y, tmin.z), 0.f); \n"\
"    return make_float2(t0, t1); \n"\
"} \n"\
" \n"\
"// Given a point in triangle plane, calculate its barycentrics \n"\
"INLINE \n"\
"float2 triangle_calculate_barycentrics(float3 p, float3 v1, float3 v2, float3 v3) \n"\
"{ \n"\
"    float3 const e1 = v2 - v1; \n"\
"    float3 const e2 = v3 - v1; \n"\
"    float3 const e = p - v1; \n"\
"    float const d00 = dot(e1, e1); \n"\
"    float const d01 = dot(e1, e2); \n"\
"    float const d11 = dot(e2, e2); \n"\
"    float const d20 = dot(e, e1); \n"\
"    float const d21 = dot(e, e2); \n"\
" \n"\
"    float denom = (d00 * d11 - d01 * d01); \n"\
"     \n"\
"    if (denom == 0.f) \n"\
"    { \n"\
"        return make_float2(0.f, 0.f); \n"\
"    } \n"\
"     \n"\
"#ifdef USE_SAFE_MATH \n"\
"    float const invdenom = 1.f / denom; \n"\
"#else \n"\
"    float const invdenom = native_recip(denom); \n"\
"#endif \n"\
" \n"\
"    float const b1 = (d11 * d20 - d01 * d21) * invdenom; \n"\
"    float const b2 = (d00 * d21 - d01 * d20) * invdenom; \n"\
"    return make_float2(b1, b2); \n"\
"} \n"\
;
static const char g_intersect_bvh2level_skiplinks_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
"/** \n"\
"    \\file intersect_2level_skiplinkscl \n"\
"    \\author Dmitry Kozlov \n"\
"    \\version 1.0 \n"\
"    \\brief Intersector implementation based on 2-level BVH with skip links. \n"\
" \n"\
"    IntersectorSkipLinks implementation is based on the modification of the following paper: \n"\
"    \"Efficiency Issues for Ray Tracing\" Brian Smits \n"\
"    http://www.cse.chalmers.se/edu/year/2016/course/course/TDA361/EfficiencyIssuesForRayTracing.pdf \n"\
" \n"\
"    Intersector is using binary BVH with a single bounding box per node. BVH layout guarantees \n"\
"    that left child of an internal node lies right next to it in memory. Each BVH node has a  \n"\
"    skip link to the node traversed next. Intersector builds its own BVH for each scene object  \n"\
"    and then top level BVH across all bottom level BVHs. Top level leafs keep object transforms and \n"\
"    might reference other leafs making instancing possible. \n"\
" \n"\
" \n"\
"    Pros: \n"\
"        -Simple and efficient kernel with low VGPR pressure. \n"\
"        -Can traverse trees of arbitrary depth. \n"\
"        -Supports motion blur. \n"\
"        -Supports instancing. \n"\
"        -Fast to refit. \n"\
"    Cons: \n"\
"        -Travesal order is fixed, so poor algorithmic characteristics. \n"\
"        -Does not benefit from BVH quality optimizations. \n"\
" */ \n"\
" \n"\
"/************************************************************************* \n"\
"INCLUDES \n"\
"**************************************************************************/ \n"\
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
" \n"\
"/************************************************************************* \n"\
"DEFINES \n"\
"**************************************************************************/ \n"\
"#define PI 3.14159265358979323846f \n"\
"#define KERNEL __kernel \n"\
"#define GLOBAL __global \n"\
"#define INLINE __attribute__((always_inline)) \n"\
"#define HIT_MARKER 1 \n"\
"#define MISS_MARKER -1 \n"\
"#define INVALID_IDX -1 \n"\
" \n"\
"/************************************************************************* \n"\
"EXTENSIONS \n"\
"**************************************************************************/ \n"\
"#ifdef AMD_MEDIA_OPS \n"\
"#pragma OPENCL EXTENSION cl_amd_media_ops2 : enable \n"\
"#endif \n"\
" \n"\
"/************************************************************************* \n"\
"TYPES \n"\
"**************************************************************************/ \n"\
" \n"\
"// Axis aligned bounding box \n"\
"typedef struct \n"\
"{ \n"\
"    float4 pmin; \n"\
"    float4 pmax; \n"\
"} bbox; \n"\
" \n"\
"// Ray definition \n"\
"typedef struct \n"\
"{ \n"\
"    float4 o; \n"\
"    float4 d; \n"\
"    int2 extra; \n"\
"    int doBackfaceCulling; \n"\
"    int padding; \n"\
"} ray; \n"\
" \n"\
"// Intersection definition \n"\
"typedef struct \n"\
"{ \n"\
"    int shape_id; \n"\
"    int prim_id; \n"\
"    int2 padding; \n"\
" \n"\
"    float4 uvwt; \n"\
"} Intersection; \n"\
" \n"\
" \n"\
"/************************************************************************* \n"\
"HELPER FUNCTIONS \n"\
"**************************************************************************/ \n"\
"INLINE \n"\
"int ray_get_mask(ray const* r) \n"\
"{ \n"\
"    return r->extra.x; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"int ray_is_active(ray const* r) \n"\
"{ \n"\
"    return r->extra.y; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"float ray_get_maxt(ray const* r) \n"\
"{ \n"\
"    return r->o.w; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"float ray_get_time(ray const* r) \n"\
"{ \n"\
"    return r->d.w; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"int ray_get_doBackfaceCull(ray const* r) \n"\
"{ \n"\
"    return r->doBackfaceCulling; \n"\
"} \n"\
" \n"\
"/************************************************************************* \n"\
"FUNCTIONS \n"\
"**************************************************************************/ \n"\
"#ifndef APPLE \n"\
"INLINE \n"\
"float4 make_float4(float x, float y, float z, float w) \n"\
"{ \n"\
"    float4 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    res.w = w; \n"\
"    return res; \n"\
"} \n"\
"INLINE \n"\
"float3 make_float3(float x, float y, float z) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    return res; \n"\
"} \n"\
"INLINE \n"\
"float2 make_float2(float x, float y) \n"\
"{ \n"\
"    float2 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    return res; \n"\
"} \n"\
"INLINE \n"\
"int2 make_int2(int x, int y) \n"\
"{ \n"\
"    int2 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    return res; \n"\
"} \n"\
"INLINE \n"\
"int3 make_int3(int x, int y, int z) \n"\
"{ \n"\
"    int3 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    return res; \n"\
"} \n"\
"#endif \n"\
" \n"\
"INLINE float min3(float a, float b, float c) \n"\
"{ \n"\
"#ifdef AMD_MEDIA_OPS \n"\
"    return amd_min3(a, b, c); \n"\
"#else \n"\
"    return min(min(a,b), c); \n"\
"#endif \n"\
"} \n"\
" \n"\
"INLINE float max3(float a, float b, float c) \n"\
"{ \n"\
"#ifdef AMD_MEDIA_OPS \n"\
"    return amd_max3(a, b, c); \n"\
"#else \n"\
"    return max(max(a,b), c); \n"\
"#endif \n"\
"} \n"\
" \n"\
" \n"\
"// Intersect ray against a triangle and return intersection interval value if it is in \n"\
"// (0, t_max], return t_max otherwise. \n"\
"INLINE \n"\
"float fast_intersect_triangle(ray r, float3 v1, float3 v2, float3 v3, float t_max) \n"\
"{ \n"\
"    float3 const e1 = v2 - v1; \n"\
"    float3 const e2 = v3 - v1; \n"\
" \n"\
"#ifdef RR_BACKFACE_CULL \n"\
"    if (ray_get_doBackfaceCull(&r) && dot(cross(e1, e2), r.d.xyz) > 0.f) \n"\
"    { \n"\
"        return t_max; \n"\
"    } \n"\
"#endif // RR_BACKFACE_CULL \n"\
" \n"\
"    float3 const s1 = cross(r.d.xyz, e2); \n"\
" \n"\
"    float denom = dot(s1, e1); \n"\
"    if (denom == 0.f) \n"\
"    { \n"\
"        return t_max; \n"\
"    } \n"\
"     \n"\
"#ifdef USE_SAFE_MATH \n"\
"    float const invd = 1.f / denom; \n"\
"#else \n"\
"    float const invd = native_recip(denom); \n"\
"#endif \n"\
" \n"\
"    float3 const d = r.o.xyz - v1; \n"\
"    float const b1 = dot(d, s1) * invd; \n"\
"    float3 const s2 = cross(d, e1); \n"\
"    float const b2 = dot(r.d.xyz, s2) * invd; \n"\
"    float const temp = dot(e2, s2) * invd; \n"\
" \n"\
"    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f || temp < 0.f || temp > t_max) \n"\
"    { \n"\
"        return t_max; \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        return temp; \n"\
"    } \n"\
"} \n"\
" \n"\
"INLINE \n"\
"float3 safe_invdir(ray r) \n"\
"{ \n"\
"    float const dirx = r.d.x; \n"\
"    float const diry = r.d.y; \n"\
"    float const dirz = r.d.z; \n"\
"    float const ooeps = 1e-8; \n"\
"    float3 invdir; \n"\
"    invdir.x = 1.0f / (fabs(dirx) > ooeps ? dirx : copysign(ooeps, dirx)); \n"\
"    invdir.y = 1.0f / (fabs(diry) > ooeps ? diry : copysign(ooeps, diry)); \n"\
"    invdir.z = 1.0f / (fabs(dirz) > ooeps ? dirz : copysign(ooeps, dirz)); \n"\
"    return invdir; \n"\
"} \n"\
" \n"\
"// Intersect rays vs bbox and return intersection span.  \n"\
"// Intersection criteria is ret.x <= ret.y \n"\
"INLINE \n"\
"float2 fast_intersect_bbox1(bbox box, float3 invdir, float3 oxinvdir, float t_max) \n"\
"{ \n"\
"    float3 const f = mad(box.pmax.xyz, invdir, oxinvdir); \n"\
"    float3 const n = mad(box.pmin.xyz, invdir, oxinvdir); \n"\
"    float3 const tmax = max(f, n); \n"\
"    float3 const tmin = min(f, n); \n"\
"    float const t1 = min(min3(tmax.x, tmax.y, tmax.z), t_max); \n"\
"    float const t0 = max(max3(tmin.x, tmin.y, tmin.z), 0.f); \n"\
"    return make_float2(t0, t1); \n"\
"} \n"\
" \n"\
"// Given a point in triangle plane, calculate its barycentrics \n"\
"INLINE \n"\
"float2 triangle_calculate_barycentrics(float3 p, float3 v1, float3 v2, float3 v3) \n"\
"{ \n"\
"    float3 const e1 = v2 - v1; \n"\
"    float3 const e2 = v3 - v1; \n"\
"    float3 const e = p - v1; \n"\
"    float const d00 = dot(e1, e1); \n"\
"    float const d01 = dot(e1, e2); \n"\
"    float const d11 = dot(e2, e2); \n"\
"    float const d20 = dot(e, e1); \n"\
"    float const d21 = dot(e, e2); \n"\
" \n"\
"    float denom = (d00 * d11 - d01 * d01); \n"\
"     \n"\
"    if (denom == 0.f) \n"\
"    { \n"\
"        return make_float2(0.f, 0.f); \n"\
"    } \n"\
"     \n"\
"#ifdef USE_SAFE_MATH \n"\
"    float const invdenom = 1.f / denom; \n"\
"#else \n"\
"    float const invdenom = native_recip(denom); \n"\
"#endif \n"\
" \n"\
"    float const b1 = (d11 * d20 - d01 * d21) * invdenom; \n"\
"    float const b2 = (d00 * d21 - d01 * d20) * invdenom; \n"\
"    return make_float2(b1, b2); \n"\
"} \n"\
" \n"\
"/************************************************************************* \n"\
"EXTENSIONS \n"\
"**************************************************************************/ \n"\
" \n"\
" \n"\
"/************************************************************************* \n"\
"DEFINES \n"\
"**************************************************************************/ \n"\
"#define PI 3.14159265358979323846f \n"\
"#define STARTIDX(x)     (((int)(x.pmin.w)) >> 4) \n"\
"#define SHAPEIDX(x)     (((int)(x.pmin.w)) >> 4) \n"\
"#define LEAFNODE(x)     (((x).pmin.w) != -1.f) \n"\
"#define NEXT(x)     ((int)((x).pmax.w)) \n"\
" \n"\
"/************************************************************************* \n"\
"TYPE DEFINITIONS \n"\
"**************************************************************************/ \n"\
" \n"\
"typedef bbox bvh_node; \n"\
" \n"\
"typedef struct \n"\
"{ \n"\
"    // Shape ID \n"\
"    int id; \n"\
"    // Shape BVH index (bottom level) \n"\
"    int bvh_idx; \n"\
"    // Is the shape disabled? \n"\
"    unsigned int shapeDisabled; \n"\
"    int padding1; \n"\
"    // Transform \n"\
"    float4 m0; \n"\
"    float4 m1; \n"\
"    float4 m2; \n"\
"    float4 m3; \n"\
"    // Motion blur params \n"\
"    float4 velocity_linear; \n"\
"    float4 velocity_angular; \n"\
"} Shape; \n"\
" \n"\
"typedef struct \n"\
"{ \n"\
"    // Vertex indices \n"\
"    int idx[3]; \n"\
"    // Shape ID \n"\
"    int shape_id; \n"\
"    // Primitive ID \n"\
"    int prim_id; \n"\
"} Face; \n"\
" \n"\
" \n"\
"INLINE float3 transform_point(float3 p, float4 m0, float4 m1, float4 m2, float4 m3) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = m0.s0 * p.x + m0.s1 * p.y + m0.s2 * p.z + m0.s3; \n"\
"    res.y = m1.s0 * p.x + m1.s1 * p.y + m1.s2 * p.z + m1.s3; \n"\
"    res.z = m2.s0 * p.x + m2.s1 * p.y + m2.s2 * p.z + m2.s3; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"INLINE float3 transform_vector(float3 p, float4 m0, float4 m1, float4 m2, float4 m3) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = m0.s0 * p.x + m0.s1 * p.y + m0.s2 * p.z; \n"\
"    res.y = m1.s0 * p.x + m1.s1 * p.y + m1.s2 * p.z; \n"\
"    res.z = m2.s0 * p.x + m2.s1 * p.y + m2.s2 * p.z; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"INLINE ray transform_ray(ray r, float4 m0, float4 m1, float4 m2, float4 m3) \n"\
"{ \n"\
"    ray res; \n"\
"    res.o.xyz = transform_point(r.o.xyz, m0, m1, m2, m3); \n"\
"    res.d.xyz = transform_vector(r.d.xyz, m0, m1, m2, m3); \n"\
"    res.o.w = r.o.w; \n"\
"    res.d.w = r.d.w; \n"\
"    return res; \n"\
"} \n"\
" \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"KERNEL void intersect_main( \n"\
"    // BVH nodes \n"\
"    GLOBAL bvh_node const* restrict nodes, \n"\
"    // Vertices \n"\
"    GLOBAL float3 const* restrict vertices, \n"\
"    // Faces \n"\
"    GLOBAL Face const* restrict faces, \n"\
"    // Shapes \n"\
"    GLOBAL Shape const* restrict shapes, \n"\
"    // BVH root index \n"\
"    int root_idx,               \n"\
"    // Rays \n"\
"    GLOBAL ray const* restrict rays, \n"\
"    // Number of rays in ray buffer \n"\
"    GLOBAL int const* restrict num_rays, \n"\
"    // Hits  \n"\
"    GLOBAL Intersection* hits \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    // Handle only working subset \n"\
"    if (global_id < *num_rays) \n"\
"    { \n"\
"        // Fetch ray \n"\
"        ray r = rays[global_id]; \n"\
" \n"\
"        if (ray_is_active(&r)) \n"\
"        { \n"\
"            // Precompute invdir for bbox testing \n"\
"            float3 invdir = safe_invdir(r); \n"\
"            float3 invdirtop = invdir; \n"\
"            float t_max = r.o.w; \n"\
" \n"\
"            // We need to keep original ray around for returns from bottom hierarchy \n"\
"            ray top_ray = r; \n"\
"            // Fetch top level BVH index \n"\
"            int addr = root_idx; \n"\
" \n"\
"            // Set top index \n"\
"            int top_addr = INVALID_IDX; \n"\
"            // Current shape ID \n"\
"            int shape_id = INVALID_IDX; \n"\
"            // Closest shape ID \n"\
"            int closest_shape_id = INVALID_IDX; \n"\
"            int closest_prim_id = INVALID_IDX; \n"\
"            float2 closest_barycentrics; \n"\
"            while (addr != INVALID_IDX) \n"\
"            { \n"\
"                // Fetch next node \n"\
"                bvh_node node = nodes[addr]; \n"\
" \n"\
"                // Intersect against bbox \n"\
"                float2 s = fast_intersect_bbox1(node, invdir, -r.o.xyz * invdir, t_max); \n"\
" \n"\
"                if (s.x <= s.y) \n"\
"                { \n"\
"                    if (LEAFNODE(node)) \n"\
"                    { \n"\
"                        // If this is the leaf it can be either a leaf containing primitives (bottom hierarchy) \n"\
"                        // or containing another BVH (top level hierarhcy) \n"\
"                        if (top_addr != INVALID_IDX) \n"\
"                        { \n"\
"                            // Intersect leaf here \n"\
"                            // \n"\
"                            int const face_idx = STARTIDX(node); \n"\
"                            Face const face = faces[face_idx]; \n"\
"                            float3 const v1 = vertices[face.idx[0]]; \n"\
"                            float3 const v2 = vertices[face.idx[1]]; \n"\
"                            float3 const v3 = vertices[face.idx[2]]; \n"\
" \n"\
"                            // Intersect triangle \n"\
"                            float const f = fast_intersect_triangle(r, v1, v2, v3, t_max); \n"\
"                            // If hit update closest hit distance and index \n"\
"                            if (f < t_max) \n"\
"                            { \n"\
"                                t_max = f; \n"\
"                                closest_prim_id = face.prim_id; \n"\
"                                closest_shape_id = shape_id; \n"\
" \n"\
"                                float3 const p = r.o.xyz + r.d.xyz * t_max; \n"\
"                                // Calculte barycentric coordinates \n"\
"                                closest_barycentrics = triangle_calculate_barycentrics(p, v1, v2, v3); \n"\
"                            } \n"\
" \n"\
"                            // And goto next node \n"\
"                            addr = NEXT(node); \n"\
"                        } \n"\
"                        else \n"\
"                        { \n"\
"                            // This is top level hierarchy leaf \n"\
"                            // Save top node index for return \n"\
"                            top_addr = addr; \n"\
"                            // Get shape descrition struct index \n"\
"                            int shape_idx = SHAPEIDX(node); \n"\
"                            // Get shape mask \n"\
"                            unsigned int const shapeDisabled = shapes[shape_idx].shapeDisabled; \n"\
"                            int const shapeId = shapes[shape_idx].id; \n"\
"                            // Drill into 2nd level BVH only if the geometry is not masked vs current ray \n"\
"                            // otherwise skip the subtree \n"\
"                            if (!shapeDisabled \n"\
"#ifdef RR_RAY_MASK \n"\
"                                && ray_get_mask(&r) != shapeId \n"\
"#endif // RR_RAY_MASK \n"\
"                                ) \n"\
"                            { \n"\
"                                // Fetch bottom level BVH index \n"\
"                                addr = shapes[shape_idx].bvh_idx; \n"\
"                                shape_id = shapeId; \n"\
" \n"\
"                                // Fetch BVH transform \n"\
"                                float4 wmi0 = shapes[shape_idx].m0; \n"\
"                                float4 wmi1 = shapes[shape_idx].m1; \n"\
"                                float4 wmi2 = shapes[shape_idx].m2; \n"\
"                                float4 wmi3 = shapes[shape_idx].m3; \n"\
" \n"\
"                                r = transform_ray(r, wmi0, wmi1, wmi2, wmi3); \n"\
"                                // Recalc invdir \n"\
"                                invdir = safe_invdir(r); \n"\
"                                // And continue traversal of the bottom level BVH \n"\
"                                continue; \n"\
"                            } \n"\
"                            else \n"\
"                            { \n"\
"                                addr = INVALID_IDX; \n"\
"                            } \n"\
"                        } \n"\
"                    } \n"\
"                    // Traverse child nodes otherwise. \n"\
"                    else \n"\
"                    { \n"\
"                        // This is an internal node, proceed to left child (it is at current + 1 index) \n"\
"                        addr = addr + 1; \n"\
"                    } \n"\
"                } \n"\
"                else \n"\
"                { \n"\
"                    // We missed the node, goto next one \n"\
"                    addr = NEXT(node); \n"\
"                } \n"\
" \n"\
"                // Here check if we ended up traversing bottom level BVH \n"\
"                // in this case idx = -1 and topidx has valid value \n"\
"                if (addr == INVALID_IDX && top_addr != INVALID_IDX) \n"\
"                { \n"\
"                    //  Proceed to next top level node \n"\
"                    addr = NEXT(nodes[top_addr]); \n"\
"                    // Set topidx \n"\
"                    top_addr = INVALID_IDX; \n"\
"                    // Restore ray here \n"\
"                    r = top_ray; \n"\
"                    // Restore invdir \n"\
"                    invdir = invdirtop; \n"\
"                } \n"\
"            } \n"\
" \n"\
"            // Check if we have found an intersection \n"\
"            if (closest_shape_id != INVALID_IDX) \n"\
"            { \n"\
"                // Update hit information \n"\
"                hits[global_id].shape_id = closest_shape_id; \n"\
"                hits[global_id].prim_id = closest_prim_id; \n"\
"                hits[global_id].uvwt = make_float4(closest_barycentrics.x, closest_barycentrics.y, 0.f, t_max); \n"\
"            } \n"\
"            else \n"\
"            { \n"\
"                // Miss here \n"\
"                hits[global_id].shape_id = MISS_MARKER; \n"\
"                hits[global_id].prim_id = MISS_MARKER; \n"\
"            } \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"KERNEL void occluded_main( \n"\
"    // BVH nodes \n"\
"    GLOBAL bvh_node const* restrict nodes, \n"\
"    // Vertices \n"\
"    GLOBAL float3 const* restrict vertices, \n"\
"    // Faces \n"\
"    GLOBAL Face const* restrict faces, \n"\
"    // Shapes \n"\
"    GLOBAL Shape const* restrict shapes, \n"\
"    // BVH root index \n"\
"    int root_idx, \n"\
"    // Rays \n"\
"    GLOBAL ray const* restrict rays, \n"\
"    // Number of rays in ray buffer \n"\
"    GLOBAL int const* restrict num_rays, \n"\
"    // Hits  \n"\
"    GLOBAL int* hits \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    // Handle only working subset \n"\
"    if (global_id < *num_rays) \n"\
"    { \n"\
"        // Fetch ray \n"\
"        ray r = rays[global_id]; \n"\
" \n"\
"        if (ray_is_active(&r)) \n"\
"        { \n"\
"            // Precompute invdir for bbox testing \n"\
"            float3 invdir = safe_invdir(r); \n"\
"            float3 invdirtop = invdir; \n"\
"            float const t_max = r.o.w; \n"\
" \n"\
"            // We need to keep original ray around for returns from bottom hierarchy \n"\
"            ray top_ray = r; \n"\
" \n"\
"            // Fetch top level BVH index \n"\
"            int addr = root_idx; \n"\
"            // Set top index \n"\
"            int top_addr = INVALID_IDX; \n"\
" \n"\
"            while (addr != INVALID_IDX) \n"\
"            { \n"\
"                // Fetch next node \n"\
"                bvh_node node = nodes[addr]; \n"\
"                // Intersect against bbox \n"\
"                float2 s = fast_intersect_bbox1(node, invdir, -r.o.xyz * invdir, t_max); \n"\
" \n"\
"                if (s.x <= s.y) \n"\
"                { \n"\
"                    if (LEAFNODE(node)) \n"\
"                    { \n"\
"                        // If this is the leaf it can be either a leaf containing primitives (bottom hierarchy) \n"\
"                        // or containing another BVH (top level hierarhcy) \n"\
"                        if (top_addr != INVALID_IDX) \n"\
"                        { \n"\
"                            // Intersect leaf here \n"\
"                            // \n"\
"                            int const face_idx = STARTIDX(node); \n"\
"                            Face const face = faces[face_idx]; \n"\
"                            float3 const v1 = vertices[face.idx[0]]; \n"\
"                            float3 const v2 = vertices[face.idx[1]]; \n"\
"                            float3 const v3 = vertices[face.idx[2]]; \n"\
" \n"\
"                            // Intersect triangle \n"\
"                            float const f = fast_intersect_triangle(r, v1, v2, v3, t_max); \n"\
"                            // If hit update closest hit distance and index \n"\
"                            if (f < t_max) \n"\
"                            { \n"\
"                                hits[global_id] = HIT_MARKER; \n"\
"                                return; \n"\
"                            } \n"\
" \n"\
"                            // And goto next node \n"\
"                            addr = NEXT(node); \n"\
"                        } \n"\
"                        else \n"\
"                        { \n"\
"                            // This is top level hierarchy leaf \n"\
"                            // Save top node index for return \n"\
"                            top_addr = addr; \n"\
"                            // Get shape descrition struct index \n"\
"                            int shape_idx = SHAPEIDX(node); \n"\
"                            // Get shape mask \n"\
"                            const unsigned int shapeDisabled = shapes[shape_idx].shapeDisabled; \n"\
"#ifdef RR_RAY_MASK \n"\
"                            const int shapeId = shapes[shape_idx].id; \n"\
"#endif // RR_RAY_MASK \n"\
"                            // Drill into 2nd level BVH only if the geometry is not masked vs current ray \n"\
"                            // otherwise skip the subtree \n"\
"                            if (!shapeDisabled  \n"\
"#ifdef RR_RAY_MASK \n"\
"                                && ray_get_mask(&r) != shapeId \n"\
"#endif // RR_RAY_MASK \n"\
"                                ) \n"\
"                            { \n"\
"                                // Fetch bottom level BVH index \n"\
"                                addr = shapes[shape_idx].bvh_idx; \n"\
" \n"\
"                                // Fetch BVH transform \n"\
"                                float4 wmi0 = shapes[shape_idx].m0; \n"\
"                                float4 wmi1 = shapes[shape_idx].m1; \n"\
"                                float4 wmi2 = shapes[shape_idx].m2; \n"\
"                                float4 wmi3 = shapes[shape_idx].m3; \n"\
" \n"\
"                                r = transform_ray(r, wmi0, wmi1, wmi2, wmi3); \n"\
"                                // Recalc invdir \n"\
"                                invdir = safe_invdir(r); \n"\
"                                // And continue traversal of the bottom level BVH \n"\
"                                continue; \n"\
"                            } \n"\
"                            else \n"\
"                            { \n"\
"                                addr = INVALID_IDX; \n"\
"                            } \n"\
"                        } \n"\
"                    } \n"\
"                    // Traverse child nodes otherwise. \n"\
"                    else \n"\
"                    { \n"\
"                        // This is an internal node, proceed to left child (it is at current + 1 index) \n"\
"                        addr = addr + 1; \n"\
"                    } \n"\
"                } \n"\
"                else \n"\
"                { \n"\
"                    // We missed the node, goto next one \n"\
"                    addr = NEXT(node); \n"\
"                } \n"\
" \n"\
"                // Here check if we ended up traversing bottom level BVH \n"\
"                // in this case idx = -1 and topidx has valid value \n"\
"                if (addr == INVALID_IDX && top_addr != INVALID_IDX) \n"\
"                { \n"\
"                    //  Proceed to next top level node \n"\
"                    addr = NEXT(nodes[top_addr]); \n"\
"                    // Set topidx \n"\
"                    top_addr = INVALID_IDX; \n"\
"                    // Restore ray here \n"\
"                    r = top_ray; \n"\
"                    // Restore invdir \n"\
"                    invdir = invdirtop; \n"\
"                } \n"\
"            } \n"\
" \n"\
"            hits[global_id] = MISS_MARKER; \n"\
"        } \n"\
"    } \n"\
"} \n"\
;
static const char g_intersect_bvh2_bittrail_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
"/** \n"\
"    \\file intersect_bvh2_bittrail.cl \n"\
"    \\author Dmitry Kozlov \n"\
"    \\version 1.0 \n"\
"    \\brief Intersector implementation based on BVH stackless traversal using bit trail and perfect hashing. \n"\
" \n"\
"    Intersector is using binary BVH with two bounding boxes per node. \n"\
"    Traversal is using bit trail and perfect hashing for backtracing and based on the following paper: \n"\
" \n"\
"    \"Efficient stackless hierarchy traversal on GPUs with backtracking in constant time\"\" \n"\
"    Nikolaus Binder, Alexander Keller \n"\
"    http://dl.acm.org/citation.cfm?id=2977343 \n"\
" \n"\
"    Traversal pseudocode: \n"\
" \n"\
"        while(addr is valid) \n"\
"        { \n"\
"            node <- fetch next node at addr \n"\
"            index <- 1 \n"\
"            trail <- 0 \n"\
"            if (node is leaf) \n"\
"                intersect leaf \n"\
"            else \n"\
"            { \n"\
"                intersect ray vs left child \n"\
"                intersect ray vs right child \n"\
"                if (intersect any of children) \n"\
"                { \n"\
"                    index <- index << 1 \n"\
"                    trail <- trail << 1 \n"\
"                    determine closer child \n"\
"                    if intersect both \n"\
"                    { \n"\
"                        trail <- trail ^ 1 \n"\
"                        addr = closer child \n"\
"                    } \n"\
"                    else \n"\
"                    { \n"\
"                        addr = intersected child \n"\
"                    } \n"\
"                    if addr is right \n"\
"                        index <- index ^ 1 \n"\
"                    continue \n"\
"                } \n"\
"            } \n"\
" \n"\
"            if (trail == 0) \n"\
"            { \n"\
"                break \n"\
"            } \n"\
" \n"\
"            num_levels = count trailing zeroes in trail \n"\
"            trail <- (trail << num_levels) & 1 \n"\
"            index <- (index << num_levels) & 1 \n"\
" \n"\
"            addr = hash[index] \n"\
"        } \n"\
" \n"\
"    Pros: \n"\
"        -Very fast traversal. \n"\
"        -Benefits from BVH quality optimization. \n"\
"        -Low VGPR pressure \n"\
"    Cons: \n"\
"        -Depth is limited. \n"\
"        -Generates global memory traffic. \n"\
" */ \n"\
" \n"\
"/************************************************************************* \n"\
"INCLUDES \n"\
"**************************************************************************/ \n"\
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
" \n"\
"/************************************************************************* \n"\
"DEFINES \n"\
"**************************************************************************/ \n"\
"#define PI 3.14159265358979323846f \n"\
"#define KERNEL __kernel \n"\
"#define GLOBAL __global \n"\
"#define INLINE __attribute__((always_inline)) \n"\
"#define HIT_MARKER 1 \n"\
"#define MISS_MARKER -1 \n"\
"#define INVALID_IDX -1 \n"\
" \n"\
"/************************************************************************* \n"\
"EXTENSIONS \n"\
"**************************************************************************/ \n"\
"#ifdef AMD_MEDIA_OPS \n"\
"#pragma OPENCL EXTENSION cl_amd_media_ops2 : enable \n"\
"#endif \n"\
" \n"\
"/************************************************************************* \n"\
"TYPES \n"\
"**************************************************************************/ \n"\
" \n"\
"// Axis aligned bounding box \n"\
"typedef struct \n"\
"{ \n"\
"    float4 pmin; \n"\
"    float4 pmax; \n"\
"} bbox; \n"\
" \n"\
"// Ray definition \n"\
"typedef struct \n"\
"{ \n"\
"    float4 o; \n"\
"    float4 d; \n"\
"    int2 extra; \n"\
"    int doBackfaceCulling; \n"\
"    int padding; \n"\
"} ray; \n"\
" \n"\
"// Intersection definition \n"\
"typedef struct \n"\
"{ \n"\
"    int shape_id; \n"\
"    int prim_id; \n"\
"    int2 padding; \n"\
" \n"\
"    float4 uvwt; \n"\
"} Intersection; \n"\
" \n"\
" \n"\
"/************************************************************************* \n"\
"HELPER FUNCTIONS \n"\
"**************************************************************************/ \n"\
"INLINE \n"\
"int ray_get_mask(ray const* r) \n"\
"{ \n"\
"    return r->extra.x; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"int ray_is_active(ray const* r) \n"\
"{ \n"\
"    return r->extra.y; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"float ray_get_maxt(ray const* r) \n"\
"{ \n"\
"    return r->o.w; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"float ray_get_time(ray const* r) \n"\
"{ \n"\
"    return r->d.w; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"int ray_get_doBackfaceCull(ray const* r) \n"\
"{ \n"\
"    return r->doBackfaceCulling; \n"\
"} \n"\
" \n"\
"/************************************************************************* \n"\
"FUNCTIONS \n"\
"**************************************************************************/ \n"\
"#ifndef APPLE \n"\
"INLINE \n"\
"float4 make_float4(float x, float y, float z, float w) \n"\
"{ \n"\
"    float4 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    res.w = w; \n"\
"    return res; \n"\
"} \n"\
"INLINE \n"\
"float3 make_float3(float x, float y, float z) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    return res; \n"\
"} \n"\
"INLINE \n"\
"float2 make_float2(float x, float y) \n"\
"{ \n"\
"    float2 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    return res; \n"\
"} \n"\
"INLINE \n"\
"int2 make_int2(int x, int y) \n"\
"{ \n"\
"    int2 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    return res; \n"\
"} \n"\
"INLINE \n"\
"int3 make_int3(int x, int y, int z) \n"\
"{ \n"\
"    int3 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    return res; \n"\
"} \n"\
"#endif \n"\
" \n"\
"INLINE float min3(float a, float b, float c) \n"\
"{ \n"\
"#ifdef AMD_MEDIA_OPS \n"\
"    return amd_min3(a, b, c); \n"\
"#else \n"\
"    return min(min(a,b), c); \n"\
"#endif \n"\
"} \n"\
" \n"\
"INLINE float max3(float a, float b, float c) \n"\
"{ \n"\
"#ifdef AMD_MEDIA_OPS \n"\
"    return amd_max3(a, b, c); \n"\
"#else \n"\
"    return max(max(a,b), c); \n"\
"#endif \n"\
"} \n"\
" \n"\
" \n"\
"// Intersect ray against a triangle and return intersection interval value if it is in \n"\
"// (0, t_max], return t_max otherwise. \n"\
"INLINE \n"\
"float fast_intersect_triangle(ray r, float3 v1, float3 v2, float3 v3, float t_max) \n"\
"{ \n"\
"    float3 const e1 = v2 - v1; \n"\
"    float3 const e2 = v3 - v1; \n"\
" \n"\
"#ifdef RR_BACKFACE_CULL \n"\
"    if (ray_get_doBackfaceCull(&r) && dot(cross(e1, e2), r.d.xyz) > 0.f) \n"\
"    { \n"\
"        return t_max; \n"\
"    } \n"\
"#endif // RR_BACKFACE_CULL \n"\
" \n"\
"    float3 const s1 = cross(r.d.xyz, e2); \n"\
" \n"\
"    float denom = dot(s1, e1); \n"\
"    if (denom == 0.f) \n"\
"    { \n"\
"        return t_max; \n"\
"    } \n"\
"     \n"\
"#ifdef USE_SAFE_MATH \n"\
"    float const invd = 1.f / denom; \n"\
"#else \n"\
"    float const invd = native_recip(denom); \n"\
"#endif \n"\
" \n"\
"    float3 const d = r.o.xyz - v1; \n"\
"    float const b1 = dot(d, s1) * invd; \n"\
"    float3 const s2 = cross(d, e1); \n"\
"    float const b2 = dot(r.d.xyz, s2) * invd; \n"\
"    float const temp = dot(e2, s2) * invd; \n"\
" \n"\
"    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f || temp < 0.f || temp > t_max) \n"\
"    { \n"\
"        return t_max; \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        return temp; \n"\
"    } \n"\
"} \n"\
" \n"\
"INLINE \n"\
"float3 safe_invdir(ray r) \n"\
"{ \n"\
"    float const dirx = r.d.x; \n"\
"    float const diry = r.d.y; \n"\
"    float const dirz = r.d.z; \n"\
"    float const ooeps = 1e-8; \n"\
"    float3 invdir; \n"\
"    invdir.x = 1.0f / (fabs(dirx) > ooeps ? dirx : copysign(ooeps, dirx)); \n"\
"    invdir.y = 1.0f / (fabs(diry) > ooeps ? diry : copysign(ooeps, diry)); \n"\
"    invdir.z = 1.0f / (fabs(dirz) > ooeps ? dirz : copysign(ooeps, dirz)); \n"\
"    return invdir; \n"\
"} \n"\
" \n"\
"// Intersect rays vs bbox and return intersection span.  \n"\
"// Intersection criteria is ret.x <= ret.y \n"\
"INLINE \n"\
"float2 fast_intersect_bbox1(bbox box, float3 invdir, float3 oxinvdir, float t_max) \n"\
"{ \n"\
"    float3 const f = mad(box.pmax.xyz, invdir, oxinvdir); \n"\
"    float3 const n = mad(box.pmin.xyz, invdir, oxinvdir); \n"\
"    float3 const tmax = max(f, n); \n"\
"    float3 const tmin = min(f, n); \n"\
"    float const t1 = min(min3(tmax.x, tmax.y, tmax.z), t_max); \n"\
"    float const t0 = max(max3(tmin.x, tmin.y, tmin.z), 0.f); \n"\
"    return make_float2(t0, t1); \n"\
"} \n"\
" \n"\
"// Given a point in triangle plane, calculate its barycentrics \n"\
"INLINE \n"\
"float2 triangle_calculate_barycentrics(float3 p, float3 v1, float3 v2, float3 v3) \n"\
"{ \n"\
"    float3 const e1 = v2 - v1; \n"\
"    float3 const e2 = v3 - v1; \n"\
"    float3 const e = p - v1; \n"\
"    float const d00 = dot(e1, e1); \n"\
"    float const d01 = dot(e1, e2); \n"\
"    float const d11 = dot(e2, e2); \n"\
"    float const d20 = dot(e, e1); \n"\
"    float const d21 = dot(e, e2); \n"\
" \n"\
"    float denom = (d00 * d11 - d01 * d01); \n"\
"     \n"\
"    if (denom == 0.f) \n"\
"    { \n"\
"        return make_float2(0.f, 0.f); \n"\
"    } \n"\
"     \n"\
"#ifdef USE_SAFE_MATH \n"\
"    float const invdenom = 1.f / denom; \n"\
"#else \n"\
"    float const invdenom = native_recip(denom); \n"\
"#endif \n"\
" \n"\
"    float const b1 = (d11 * d20 - d01 * d21) * invdenom; \n"\
"    float const b2 = (d00 * d21 - d01 * d20) * invdenom; \n"\
"    return make_float2(b1, b2); \n"\
"} \n"\
" \n"\
" \n"\
"/************************************************************************* \n"\
"TYPE DEFINITIONS \n"\
"**************************************************************************/ \n"\
" \n"\
"#define LEAFNODE(x) (((x).child0) == -1) \n"\
" \n"\
"// BVH node \n"\
"typedef struct \n"\
"{ \n"\
"    union  \n"\
"    { \n"\
"        struct \n"\
"        { \n"\
"            // Child bounds \n"\
"            bbox bounds[2]; \n"\
"        }; \n"\
" \n"\
"        struct \n"\
"        { \n"\
"            // If node is a leaf we keep vertex indices here \n"\
"            int i0, i1, i2; \n"\
"            // Address of a left child \n"\
"            int child0; \n"\
"            // Shape ID \n"\
"            int shape_id; \n"\
"            // Primitive ID \n"\
"            int prim_id; \n"\
"            // Address of a right child \n"\
"            int child1; \n"\
"        }; \n"\
"    }; \n"\
" \n"\
"} bvh_node; \n"\
" \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"KERNEL void \n"\
"occluded_main( \n"\
"    // Bvh nodes \n"\
"    GLOBAL bvh_node const * restrict nodes, \n"\
"    // Triangles vertices \n"\
"    GLOBAL float3 const * restrict vertices, \n"\
"    // Rays \n"\
"    GLOBAL ray const * restrict rays, \n"\
"    // Number of rays in rays buffer \n"\
"    GLOBAL int const * restrict num_rays, \n"\
"    // Displacement table for perfect hashing \n"\
"    GLOBAL int const * restrict displacement_table, \n"\
"    // Hash table for perfect hashing \n"\
"    GLOBAL int const * restrict hash_table, \n"\
"    // Displacement table size \n"\
"    int const displacement_table_size, \n"\
"    // Hit results: 1 for hit and -1 for miss \n"\
"    GLOBAL int* hits \n"\
"    ) \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
"    int local_id = get_local_id(0); \n"\
"    int group_id = get_group_id(0); \n"\
" \n"\
"    // Handle only working set \n"\
"    if (global_id < *num_rays) \n"\
"    { \n"\
"        ray const r = rays[global_id]; \n"\
" \n"\
"        if (ray_is_active(&r)) \n"\
"        { \n"\
"            // Precompute inverse direction and origin / dir for bbox testing \n"\
"            float3 const invdir = safe_invdir(r); \n"\
"            float3 const oxinvdir = -r.o.xyz * invdir; \n"\
"            // Intersection parametric distance \n"\
"            float const t_max = r.o.w; \n"\
" \n"\
"            // Bit tail to track traversal \n"\
"            int bit_trail = 0; \n"\
"            // Current node index (complete tree enumeration) \n"\
"            int node_idx = 1; \n"\
"            // Current node address \n"\
"            int addr = 0; \n"\
" \n"\
"            // Start from 0 node (root) \n"\
"            while (addr != INVALID_IDX) \n"\
"            { \n"\
"                // Fetch next node \n"\
"                bvh_node const node = nodes[addr]; \n"\
" \n"\
"                // Check if it is a leaf \n"\
"                if (LEAFNODE(node)) \n"\
"                { \n"\
"#ifdef RR_RAY_MASK \n"\
"                    if (ray_get_mask(&r) != node.shape_id) \n"\
"                    { \n"\
"#endif // RR_RAY_MASK \n"\
"                        // Leafs directly store vertex indices \n"\
"                        // so we load vertices directly \n"\
"                        float3 const v1 = vertices[node.i0]; \n"\
"                        float3 const v2 = vertices[node.i1]; \n"\
"                        float3 const v3 = vertices[node.i2]; \n"\
"                        // Intersect triangle \n"\
"                        float const f = fast_intersect_triangle(r, v1, v2, v3, t_max); \n"\
"                        // If hit store the result and bail out \n"\
"                        if (f < t_max) \n"\
"                        { \n"\
"                            hits[global_id] = HIT_MARKER; \n"\
"                            return; \n"\
"                        } \n"\
"#ifdef RR_RAY_MASK \n"\
"                    } \n"\
"#endif // RR_RAY_MASK \n"\
"                } \n"\
"                else \n"\
"                { \n"\
"                    // It is internal node, so intersect vs both children bounds \n"\
"                    float2 const s0 = fast_intersect_bbox1(node.bounds[0], invdir, oxinvdir, t_max); \n"\
"                    float2 const s1 = fast_intersect_bbox1(node.bounds[1], invdir, oxinvdir, t_max); \n"\
" \n"\
"                    // Determine which one to traverse \n"\
"                    bool const traverse_c0 = (s0.x <= s0.y); \n"\
"                    bool const traverse_c1 = (s1.x <= s1.y); \n"\
"                    bool const c1first = traverse_c1 && (s0.x > s1.x); \n"\
" \n"\
"                    if (traverse_c0 || traverse_c1) \n"\
"                    { \n"\
"                        // Go one level down => shift bit trail \n"\
"                        bit_trail = bit_trail << 1; \n"\
"                        // idx = idx * 2 (this is for left child) \n"\
"                        node_idx = node_idx << 1; \n"\
" \n"\
"                        // If we postpone one node here we  \n"\
"                        // set last bit in bit trail \n"\
"                        if (traverse_c0 && traverse_c1) \n"\
"                        { \n"\
"                            bit_trail = bit_trail ^ 0x1; \n"\
"                        } \n"\
" \n"\
"                        // Determine which one to traverse first \n"\
"                        if (c1first || !traverse_c0) \n"\
"                        { \n"\
"                            // Right one is closer or left one not travesed \n"\
"                            addr = node.child1; \n"\
"                            // Fix index \n"\
"                            // idx = 2 * idx + 1 for right one \n"\
"                            node_idx = node_idx ^ 0x1; \n"\
"                        } \n"\
"                        else \n"\
"                        { \n"\
"                            // Traverse left node otherwise \n"\
"                            addr = node.child0; \n"\
"                        } \n"\
" \n"\
"                        // Continue traversal \n"\
"                        continue; \n"\
"                    } \n"\
"                } \n"\
" \n"\
"                // Here we need to either backtrack or \n"\
"                // stop traversal. \n"\
"                // If bit trail is zero, there is nothing  \n"\
"                // to traverse. \n"\
"                if (bit_trail == 0) \n"\
"                { \n"\
"                    addr = INVALID_IDX; \n"\
"                    continue; \n"\
"                } \n"\
"                 \n"\
"                // Backtrack \n"\
"                // Calculate where we postponed the last node. \n"\
"                // = number of trailing zeroes in bit_trail \n"\
"                int const num_levels = 31 - clz(bit_trail & -bit_trail); \n"\
"                // Update bit trail (shift and unset last bit) \n"\
"                bit_trail = (bit_trail >> num_levels) ^ 0x1; \n"\
"                // Calculate postponed index \n"\
"                node_idx = (node_idx >> num_levels) ^ 0x1; \n"\
" \n"\
"                // Calculate node address using perfect hasing of node indices \n"\
"                int const displacement = displacement_table[node_idx / displacement_table_size]; \n"\
"                addr = hash_table[displacement + (node_idx & (displacement_table_size - 1))]; \n"\
"            } \n"\
" \n"\
"            // Finished traversal, but no intersection found \n"\
"            hits[global_id] = MISS_MARKER; \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"KERNEL void intersect_main( \n"\
"    // Bvh nodes \n"\
"    GLOBAL bvh_node const * restrict nodes, \n"\
"    // Triangles vertices \n"\
"    GLOBAL float3 const * restrict vertices, \n"\
"    // Rays \n"\
"    GLOBAL ray const * restrict rays, \n"\
"    // Number of rays in rays buffer \n"\
"    GLOBAL int const * restrict num_rays, \n"\
"    // Displacement table for perfect hashing \n"\
"    GLOBAL int const * restrict displacement_table, \n"\
"    // Hash table for perfect hashing \n"\
"    GLOBAL int const * restrict hash_table, \n"\
"    // Displacement table size \n"\
"    int const displacement_table_size, \n"\
"    // Hit results: 1 for hit and -1 for miss \n"\
"    GLOBAL Intersection* hits) \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
"    int local_id = get_local_id(0); \n"\
"    int group_id = get_group_id(0); \n"\
" \n"\
"    // Handle only working subset \n"\
"    if (global_id < *num_rays) \n"\
"    { \n"\
"        ray const r = rays[global_id]; \n"\
" \n"\
"        if (ray_is_active(&r)) \n"\
"        { \n"\
"            // Precompute inverse direction and origin / dir for bbox testing \n"\
"            float3 const invdir = safe_invdir(r); \n"\
"            float3 const oxinvdir = -r.o.xyz * invdir; \n"\
"            // Intersection parametric distance \n"\
"            float t_max = r.o.w; \n"\
" \n"\
"            // Bit tail to track traversal \n"\
"            int bit_trail = 0; \n"\
"            // Current node index (complete tree enumeration) \n"\
"            int node_idx = 1; \n"\
"            // Current node address \n"\
"            int addr = 0; \n"\
"            // Current closest intersection leaf index \n"\
"            int isect_idx = INVALID_IDX; \n"\
" \n"\
"            // Start from 0 node (root) \n"\
"            while (addr != INVALID_IDX) \n"\
"            { \n"\
"                // Fetch next node \n"\
"                bvh_node const node = nodes[addr]; \n"\
" \n"\
"                // Check if it is a leaf \n"\
"                if (LEAFNODE(node)) \n"\
"                { \n"\
"#ifdef RR_RAY_MASK \n"\
"                    if (ray_get_mask(&r) != node.shape_id) \n"\
"                    { \n"\
"#endif // RR_RAY_MASK \n"\
"                        // Leafs directly store vertex indices \n"\
"                        // so we load vertices directly \n"\
"                        float3 const v1 = vertices[node.i0]; \n"\
"                        float3 const v2 = vertices[node.i1]; \n"\
"                        float3 const v3 = vertices[node.i2]; \n"\
"                        // Intersect triangle \n"\
"                        float const f = fast_intersect_triangle(r, v1, v2, v3, t_max); \n"\
"                        // If hit update closest hit distance and index \n"\
"                        if (f < t_max) \n"\
"                        { \n"\
"                            t_max = f; \n"\
"                            isect_idx = addr; \n"\
"                        } \n"\
"#ifdef RR_RAY_MASK \n"\
"                    } \n"\
"#endif // RR_RAY_MASK \n"\
"                } \n"\
"                else \n"\
"                { \n"\
"                    // It is internal node, so intersect vs both children bounds \n"\
"                    float2 const s0 = fast_intersect_bbox1(node.bounds[0], invdir, oxinvdir, t_max); \n"\
"                    float2 const s1 = fast_intersect_bbox1(node.bounds[1], invdir, oxinvdir, t_max); \n"\
" \n"\
"                    // Determine which one to traverse \n"\
"                    bool const traverse_c0 = (s0.x <= s0.y); \n"\
"                    bool const traverse_c1 = (s1.x <= s1.y); \n"\
"                    bool const c1first = traverse_c1 && (s0.x > s1.x); \n"\
" \n"\
"                    if (traverse_c0 || traverse_c1) \n"\
"                    { \n"\
"                        // Go one level down => shift bit trail \n"\
"                        bit_trail = bit_trail << 1; \n"\
"                        // idx = idx * 2 (this is for left child) \n"\
"                        node_idx = node_idx << 1; \n"\
" \n"\
"                        // If we postpone one node here we  \n"\
"                        // set last bit in bit trail \n"\
"                        if (traverse_c0 && traverse_c1) \n"\
"                        { \n"\
"                            bit_trail = bit_trail ^ 0x1; \n"\
"                        } \n"\
" \n"\
"                        // Determine which one to traverse first \n"\
"                        if (c1first || !traverse_c0) \n"\
"                        { \n"\
"                            // Right one is closer or left one not travesed \n"\
"                            addr = node.child1; \n"\
"                            // Fix index \n"\
"                            // idx = 2 * idx + 1 for right one \n"\
"                            node_idx = node_idx ^ 0x1; \n"\
"                        } \n"\
"                        else \n"\
"                        { \n"\
"                            // Traverse left node otherwise \n"\
"                            addr = node.child0; \n"\
"                        } \n"\
" \n"\
"                        // Continue traversal \n"\
"                        continue; \n"\
"                    } \n"\
"                } \n"\
" \n"\
"                // Here we need to either backtrack or \n"\
"                // stop traversal. \n"\
"                // If bit trail is zero, there is nothing  \n"\
"                // to traverse. \n"\
"                if (bit_trail == 0) \n"\
"                { \n"\
"                    addr = INVALID_IDX; \n"\
"                    continue; \n"\
"                } \n"\
" \n"\
"                // Backtrack \n"\
"                // Calculate where we postponed the last node. \n"\
"                // = number of trailing zeroes in bit_trail \n"\
"                int num_levels = 31 - clz(bit_trail & -bit_trail); \n"\
"                // Update bit trail (shift and unset last bit) \n"\
"                bit_trail = (bit_trail >> num_levels) ^ 0x1; \n"\
"                // Calculate postponed index \n"\
"                node_idx = (node_idx >> num_levels) ^ 0x1; \n"\
" \n"\
"                // Calculate node address using perfect hasing of node indices \n"\
"                int displacement = displacement_table[node_idx / displacement_table_size]; \n"\
"                addr = hash_table[displacement + (node_idx & (displacement_table_size - 1))]; \n"\
"            } \n"\
" \n"\
"            // Check if we have found an intersection \n"\
"            if (isect_idx != INVALID_IDX) \n"\
"            { \n"\
"                // Fetch the node & vertices \n"\
"                bvh_node const node = nodes[isect_idx]; \n"\
"                float3 const v1 = vertices[node.i0]; \n"\
"                float3 const v2 = vertices[node.i1]; \n"\
"                float3 const v3 = vertices[node.i2]; \n"\
"                // Calculate hit position \n"\
"                float3 const p = r.o.xyz + r.d.xyz * t_max; \n"\
"                // Calculate barycentric coordinates \n"\
"                float2 const uv = triangle_calculate_barycentrics(p, v1, v2, v3); \n"\
"                // Update hit information \n"\
"                hits[global_id].shape_id = node.shape_id; \n"\
"                hits[global_id].prim_id = node.prim_id; \n"\
"                hits[global_id].uvwt = make_float4(uv.x, uv.y, 0.f, t_max); \n"\
"            } \n"\
"            else \n"\
"            { \n"\
"                // Miss here \n"\
"                hits[global_id].shape_id = MISS_MARKER; \n"\
"                hits[global_id].prim_id = MISS_MARKER; \n"\
"            } \n"\
"        } \n"\
"    } \n"\
"} \n"\
;
static const char g_intersect_bvh2_lds_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
" \n"\
"/************************************************************************* \n"\
"INCLUDES \n"\
"**************************************************************************/ \n"\
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
" \n"\
"/************************************************************************* \n"\
"DEFINES \n"\
"**************************************************************************/ \n"\
"#define PI 3.14159265358979323846f \n"\
"#define KERNEL __kernel \n"\
"#define GLOBAL __global \n"\
"#define INLINE __attribute__((always_inline)) \n"\
"#define HIT_MARKER 1 \n"\
"#define MISS_MARKER -1 \n"\
"#define INVALID_IDX -1 \n"\
" \n"\
"/************************************************************************* \n"\
"EXTENSIONS \n"\
"**************************************************************************/ \n"\
"#ifdef AMD_MEDIA_OPS \n"\
"#pragma OPENCL EXTENSION cl_amd_media_ops2 : enable \n"\
"#endif \n"\
" \n"\
"/************************************************************************* \n"\
"TYPES \n"\
"**************************************************************************/ \n"\
" \n"\
"// Axis aligned bounding box \n"\
"typedef struct \n"\
"{ \n"\
"    float4 pmin; \n"\
"    float4 pmax; \n"\
"} bbox; \n"\
" \n"\
"// Ray definition \n"\
"typedef struct \n"\
"{ \n"\
"    float4 o; \n"\
"    float4 d; \n"\
"    int2 extra; \n"\
"    int doBackfaceCulling; \n"\
"    int padding; \n"\
"} ray; \n"\
" \n"\
"// Intersection definition \n"\
"typedef struct \n"\
"{ \n"\
"    int shape_id; \n"\
"    int prim_id; \n"\
"    int2 padding; \n"\
" \n"\
"    float4 uvwt; \n"\
"} Intersection; \n"\
" \n"\
" \n"\
"/************************************************************************* \n"\
"HELPER FUNCTIONS \n"\
"**************************************************************************/ \n"\
"INLINE \n"\
"int ray_get_mask(ray const* r) \n"\
"{ \n"\
"    return r->extra.x; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"int ray_is_active(ray const* r) \n"\
"{ \n"\
"    return r->extra.y; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"float ray_get_maxt(ray const* r) \n"\
"{ \n"\
"    return r->o.w; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"float ray_get_time(ray const* r) \n"\
"{ \n"\
"    return r->d.w; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"int ray_get_doBackfaceCull(ray const* r) \n"\
"{ \n"\
"    return r->doBackfaceCulling; \n"\
"} \n"\
" \n"\
"/************************************************************************* \n"\
"FUNCTIONS \n"\
"**************************************************************************/ \n"\
"#ifndef APPLE \n"\
"INLINE \n"\
"float4 make_float4(float x, float y, float z, float w) \n"\
"{ \n"\
"    float4 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    res.w = w; \n"\
"    return res; \n"\
"} \n"\
"INLINE \n"\
"float3 make_float3(float x, float y, float z) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    return res; \n"\
"} \n"\
"INLINE \n"\
"float2 make_float2(float x, float y) \n"\
"{ \n"\
"    float2 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    return res; \n"\
"} \n"\
"INLINE \n"\
"int2 make_int2(int x, int y) \n"\
"{ \n"\
"    int2 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    return res; \n"\
"} \n"\
"INLINE \n"\
"int3 make_int3(int x, int y, int z) \n"\
"{ \n"\
"    int3 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    return res; \n"\
"} \n"\
"#endif \n"\
" \n"\
"INLINE float min3(float a, float b, float c) \n"\
"{ \n"\
"#ifdef AMD_MEDIA_OPS \n"\
"    return amd_min3(a, b, c); \n"\
"#else \n"\
"    return min(min(a,b), c); \n"\
"#endif \n"\
"} \n"\
" \n"\
"INLINE float max3(float a, float b, float c) \n"\
"{ \n"\
"#ifdef AMD_MEDIA_OPS \n"\
"    return amd_max3(a, b, c); \n"\
"#else \n"\
"    return max(max(a,b), c); \n"\
"#endif \n"\
"} \n"\
" \n"\
" \n"\
"// Intersect ray against a triangle and return intersection interval value if it is in \n"\
"// (0, t_max], return t_max otherwise. \n"\
"INLINE \n"\
"float fast_intersect_triangle(ray r, float3 v1, float3 v2, float3 v3, float t_max) \n"\
"{ \n"\
"    float3 const e1 = v2 - v1; \n"\
"    float3 const e2 = v3 - v1; \n"\
" \n"\
"#ifdef RR_BACKFACE_CULL \n"\
"    if (ray_get_doBackfaceCull(&r) && dot(cross(e1, e2), r.d.xyz) > 0.f) \n"\
"    { \n"\
"        return t_max; \n"\
"    } \n"\
"#endif // RR_BACKFACE_CULL \n"\
" \n"\
"    float3 const s1 = cross(r.d.xyz, e2); \n"\
" \n"\
"    float denom = dot(s1, e1); \n"\
"    if (denom == 0.f) \n"\
"    { \n"\
"        return t_max; \n"\
"    } \n"\
"     \n"\
"#ifdef USE_SAFE_MATH \n"\
"    float const invd = 1.f / denom; \n"\
"#else \n"\
"    float const invd = native_recip(denom); \n"\
"#endif \n"\
" \n"\
"    float3 const d = r.o.xyz - v1; \n"\
"    float const b1 = dot(d, s1) * invd; \n"\
"    float3 const s2 = cross(d, e1); \n"\
"    float const b2 = dot(r.d.xyz, s2) * invd; \n"\
"    float const temp = dot(e2, s2) * invd; \n"\
" \n"\
"    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f || temp < 0.f || temp > t_max) \n"\
"    { \n"\
"        return t_max; \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        return temp; \n"\
"    } \n"\
"} \n"\
" \n"\
"INLINE \n"\
"float3 safe_invdir(ray r) \n"\
"{ \n"\
"    float const dirx = r.d.x; \n"\
"    float const diry = r.d.y; \n"\
"    float const dirz = r.d.z; \n"\
"    float const ooeps = 1e-8; \n"\
"    float3 invdir; \n"\
"    invdir.x = 1.0f / (fabs(dirx) > ooeps ? dirx : copysign(ooeps, dirx)); \n"\
"    invdir.y = 1.0f / (fabs(diry) > ooeps ? diry : copysign(ooeps, diry)); \n"\
"    invdir.z = 1.0f / (fabs(dirz) > ooeps ? dirz : copysign(ooeps, dirz)); \n"\
"    return invdir; \n"\
"} \n"\
" \n"\
"// Intersect rays vs bbox and return intersection span.  \n"\
"// Intersection criteria is ret.x <= ret.y \n"\
"INLINE \n"\
"float2 fast_intersect_bbox1(bbox box, float3 invdir, float3 oxinvdir, float t_max) \n"\
"{ \n"\
"    float3 const f = mad(box.pmax.xyz, invdir, oxinvdir); \n"\
"    float3 const n = mad(box.pmin.xyz, invdir, oxinvdir); \n"\
"    float3 const tmax = max(f, n); \n"\
"    float3 const tmin = min(f, n); \n"\
"    float const t1 = min(min3(tmax.x, tmax.y, tmax.z), t_max); \n"\
"    float const t0 = max(max3(tmin.x, tmin.y, tmin.z), 0.f); \n"\
"    return make_float2(t0, t1); \n"\
"} \n"\
" \n"\
"// Given a point in triangle plane, calculate its barycentrics \n"\
"INLINE \n"\
"float2 triangle_calculate_barycentrics(float3 p, float3 v1, float3 v2, float3 v3) \n"\
"{ \n"\
"    float3 const e1 = v2 - v1; \n"\
"    float3 const e2 = v3 - v1; \n"\
"    float3 const e = p - v1; \n"\
"    float const d00 = dot(e1, e1); \n"\
"    float const d01 = dot(e1, e2); \n"\
"    float const d11 = dot(e2, e2); \n"\
"    float const d20 = dot(e, e1); \n"\
"    float const d21 = dot(e, e2); \n"\
" \n"\
"    float denom = (d00 * d11 - d01 * d01); \n"\
"     \n"\
"    if (denom == 0.f) \n"\
"    { \n"\
"        return make_float2(0.f, 0.f); \n"\
"    } \n"\
"     \n"\
"#ifdef USE_SAFE_MATH \n"\
"    float const invdenom = 1.f / denom; \n"\
"#else \n"\
"    float const invdenom = native_recip(denom); \n"\
"#endif \n"\
" \n"\
"    float const b1 = (d11 * d20 - d01 * d21) * invdenom; \n"\
"    float const b2 = (d00 * d21 - d01 * d20) * invdenom; \n"\
"    return make_float2(b1, b2); \n"\
"} \n"\
" \n"\
"/************************************************************************* \n"\
"TYPE DEFINITIONS \n"\
"**************************************************************************/ \n"\
" \n"\
"#define INVALID_ADDR 0xffffffffu \n"\
"#define INTERNAL_NODE(node) (GetAddrLeft(node) != INVALID_ADDR) \n"\
" \n"\
"#define GROUP_SIZE 64 \n"\
"#define STACK_SIZE 32 \n"\
"#define LDS_STACK_SIZE 16 \n"\
" \n"\
"// BVH node \n"\
"typedef struct \n"\
"{ \n"\
"    float4 aabb_left_min_or_v0_and_addr_left; \n"\
"    float4 aabb_left_max_or_v1_and_mesh_id; \n"\
"    float4 aabb_right_min_or_v2_and_addr_right; \n"\
"    float4 aabb_right_max_and_prim_id; \n"\
" \n"\
"} bvh_node; \n"\
" \n"\
"#define GetAddrLeft(node)   as_uint((node).aabb_left_min_or_v0_and_addr_left.w) \n"\
"#define GetAddrRight(node)  as_uint((node).aabb_right_min_or_v2_and_addr_right.w) \n"\
"#define GetMeshId(node)     as_uint((node).aabb_left_max_or_v1_and_mesh_id.w) \n"\
"#define GetPrimId(node)     as_uint((node).aabb_right_max_and_prim_id.w) \n"\
" \n"\
"INLINE float2 fast_intersect_bbox2(float3 pmin, float3 pmax, float3 invdir, float3 oxinvdir, float t_max) \n"\
"{ \n"\
"    const float3 f = mad(pmax.xyz, invdir, oxinvdir); \n"\
"    const float3 n = mad(pmin.xyz, invdir, oxinvdir); \n"\
"    const float3 tmax = max(f, n); \n"\
"    const float3 tmin = min(f, n); \n"\
"    const float t1 = min(min3(tmax.x, tmax.y, tmax.z), t_max); \n"\
"    const float t0 = max(max3(tmin.x, tmin.y, tmin.z), 0.f); \n"\
"    return (float2)(t0, t1); \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"KERNEL void intersect_main( \n"\
"    // Bvh nodes \n"\
"    GLOBAL const bvh_node *restrict nodes, \n"\
"    // Rays \n"\
"    GLOBAL const ray *restrict rays, \n"\
"    // Number of rays in rays buffer \n"\
"    GLOBAL const int *restrict num_rays, \n"\
"    // Stack memory \n"\
"    GLOBAL uint *stack, \n"\
"    // Hit data \n"\
"    GLOBAL Intersection *hits) \n"\
"{ \n"\
"    __local uint lds_stack[GROUP_SIZE * LDS_STACK_SIZE]; \n"\
" \n"\
"    uint index = get_global_id(0); \n"\
"    uint local_index = get_local_id(0); \n"\
" \n"\
"    // Handle only working subset \n"\
"    if (index < *num_rays) \n"\
"    { \n"\
"        const ray my_ray = rays[index]; \n"\
" \n"\
"        if (ray_is_active(&my_ray)) \n"\
"        { \n"\
"            const float3 invDir = safe_invdir(my_ray); \n"\
"            const float3 oxInvDir = -my_ray.o.xyz * invDir; \n"\
" \n"\
"            // Intersection parametric distance \n"\
"            float closest_t = my_ray.o.w; \n"\
" \n"\
"            // Current node address \n"\
"            uint addr = 0; \n"\
"            // Current closest address \n"\
"            uint closest_addr = INVALID_ADDR; \n"\
" \n"\
"            uint stack_bottom = STACK_SIZE * index; \n"\
"            uint sptr = stack_bottom; \n"\
"            uint lds_stack_bottom = local_index * LDS_STACK_SIZE; \n"\
"            uint lds_sptr = lds_stack_bottom; \n"\
" \n"\
"            lds_stack[lds_sptr++] = INVALID_ADDR; \n"\
" \n"\
"            while (addr != INVALID_ADDR) \n"\
"            { \n"\
"                const bvh_node node = nodes[addr]; \n"\
" \n"\
"                if (INTERNAL_NODE(node)) \n"\
"                { \n"\
"                    float2 s0 = fast_intersect_bbox2( \n"\
"                        node.aabb_left_min_or_v0_and_addr_left.xyz, \n"\
"                        node.aabb_left_max_or_v1_and_mesh_id.xyz, \n"\
"                        invDir, oxInvDir, closest_t); \n"\
"                    float2 s1 = fast_intersect_bbox2( \n"\
"                        node.aabb_right_min_or_v2_and_addr_right.xyz, \n"\
"                        node.aabb_right_max_and_prim_id.xyz, \n"\
"                        invDir, oxInvDir, closest_t); \n"\
" \n"\
"                    bool traverse_c0 = (s0.x <= s0.y); \n"\
"                    bool traverse_c1 = (s1.x <= s1.y); \n"\
"                    bool c1first = traverse_c1 && (s0.x > s1.x); \n"\
" \n"\
"                    if (traverse_c0 || traverse_c1) \n"\
"                    { \n"\
"                        uint deferred = INVALID_ADDR; \n"\
" \n"\
"                        if (c1first || !traverse_c0)  \n"\
"                        { \n"\
"                            addr = GetAddrRight(node); \n"\
"                            deferred = GetAddrLeft(node); \n"\
"                        } \n"\
"                        else \n"\
"                        { \n"\
"                            addr = GetAddrLeft(node); \n"\
"                            deferred = GetAddrRight(node); \n"\
"                        } \n"\
" \n"\
"                        if (traverse_c0 && traverse_c1) \n"\
"                        { \n"\
"                            if (lds_sptr - lds_stack_bottom >= LDS_STACK_SIZE) \n"\
"                            { \n"\
"                                for (int i = 1; i < LDS_STACK_SIZE; ++i) \n"\
"                                { \n"\
"                                    stack[sptr + i] = lds_stack[lds_stack_bottom + i]; \n"\
"                                } \n"\
" \n"\
"                                sptr += LDS_STACK_SIZE; \n"\
"                                lds_sptr = lds_stack_bottom + 1; \n"\
"                            } \n"\
" \n"\
"                            lds_stack[lds_sptr++] = deferred; \n"\
"                        } \n"\
" \n"\
"                        continue; \n"\
"                    } \n"\
"                } \n"\
"                else \n"\
"                { \n"\
"#ifdef RR_RAY_MASK \n"\
"                    if (ray_get_mask(&my_ray) != convert_int(GetMeshId(node))) \n"\
"                    { \n"\
"#endif // RR_RAY_MASK \n"\
"                        float t = fast_intersect_triangle( \n"\
"                            my_ray, \n"\
"                            node.aabb_left_min_or_v0_and_addr_left.xyz, \n"\
"                            node.aabb_left_max_or_v1_and_mesh_id.xyz, \n"\
"                            node.aabb_right_min_or_v2_and_addr_right.xyz, \n"\
"                            closest_t); \n"\
" \n"\
"                        if (t < closest_t) \n"\
"                        { \n"\
"                            closest_t = t; \n"\
"                            closest_addr = addr; \n"\
"                        } \n"\
"#ifdef RR_RAY_MASK \n"\
"                    } \n"\
"#endif // RR_RAY_MASK \n"\
"                } \n"\
" \n"\
"                addr = lds_stack[--lds_sptr]; \n"\
" \n"\
"                if (addr == INVALID_ADDR && sptr > stack_bottom) \n"\
"                { \n"\
"                    sptr -= LDS_STACK_SIZE; \n"\
"                    for (int i = 1; i < LDS_STACK_SIZE; ++i) \n"\
"                    { \n"\
"                        lds_stack[lds_stack_bottom + i] = stack[sptr + i]; \n"\
"                    } \n"\
" \n"\
"                    lds_sptr = lds_stack_bottom + LDS_STACK_SIZE - 1; \n"\
"                    addr = lds_stack[lds_sptr]; \n"\
"                } \n"\
"            } \n"\
" \n"\
"            // Check if we have found an intersection \n"\
"            if (closest_addr != INVALID_ADDR) \n"\
"            { \n"\
"                // Calculate hit position \n"\
"                const bvh_node node = nodes[closest_addr]; \n"\
"                const float3 p = my_ray.o.xyz + closest_t * my_ray.d.xyz; \n"\
" \n"\
"                // Calculate barycentric coordinates \n"\
"                const float2 uv = triangle_calculate_barycentrics( \n"\
"                    p, \n"\
"                    node.aabb_left_min_or_v0_and_addr_left.xyz, \n"\
"                    node.aabb_left_max_or_v1_and_mesh_id.xyz, \n"\
"                    node.aabb_right_min_or_v2_and_addr_right.xyz); \n"\
" \n"\
"                // Update hit information \n"\
"                hits[index].prim_id = GetPrimId(node); \n"\
"                hits[index].shape_id = GetMeshId(node); \n"\
"                hits[index].uvwt = (float4)(uv.x, uv.y, 0.0f, closest_t); \n"\
"            } \n"\
"            else \n"\
"            { \n"\
"                // Miss here \n"\
"                hits[index].prim_id = MISS_MARKER; \n"\
"                hits[index].shape_id = MISS_MARKER; \n"\
"            } \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"KERNEL void occluded_main( \n"\
"    // Bvh nodes \n"\
"    GLOBAL const bvh_node *restrict nodes, \n"\
"    // Rays \n"\
"    GLOBAL const ray *restrict rays, \n"\
"    // Number of rays in rays buffer \n"\
"    GLOBAL const int *restrict num_rays, \n"\
"    // Stack memory \n"\
"    GLOBAL uint *stack, \n"\
"    // Hit results: 1 for hit and -1 for miss \n"\
"    GLOBAL int *hits) \n"\
"{ \n"\
"    __local uint lds_stack[GROUP_SIZE * LDS_STACK_SIZE]; \n"\
" \n"\
"    uint index = get_global_id(0); \n"\
"    uint local_index = get_local_id(0); \n"\
" \n"\
"    // Handle only working subset \n"\
"    if (index < *num_rays) \n"\
"    { \n"\
"        const ray my_ray = rays[index]; \n"\
" \n"\
"        if (ray_is_active(&my_ray)) \n"\
"        { \n"\
"            const float3 invDir = safe_invdir(my_ray); \n"\
"            const float3 oxInvDir = -my_ray.o.xyz * invDir; \n"\
" \n"\
"            // Current node address \n"\
"            uint addr = 0; \n"\
"            // Intersection parametric distance \n"\
"            const float closest_t = my_ray.o.w; \n"\
" \n"\
"            uint stack_bottom = STACK_SIZE * index; \n"\
"            uint sptr = stack_bottom; \n"\
"            uint lds_stack_bottom = local_index * LDS_STACK_SIZE; \n"\
"            uint lds_sptr = lds_stack_bottom; \n"\
" \n"\
"            lds_stack[lds_sptr++] = INVALID_ADDR; \n"\
" \n"\
"            while (addr != INVALID_ADDR) \n"\
"            { \n"\
"                const bvh_node node = nodes[addr]; \n"\
" \n"\
"                if (INTERNAL_NODE(node)) \n"\
"                { \n"\
"                    float2 s0 = fast_intersect_bbox2( \n"\
"                        node.aabb_left_min_or_v0_and_addr_left.xyz, \n"\
"                        node.aabb_left_max_or_v1_and_mesh_id.xyz, \n"\
"                        invDir, oxInvDir, closest_t); \n"\
"                    float2 s1 = fast_intersect_bbox2( \n"\
"                        node.aabb_right_min_or_v2_and_addr_right.xyz, \n"\
"                        node.aabb_right_max_and_prim_id.xyz, \n"\
"                        invDir, oxInvDir, closest_t); \n"\
" \n"\
"                    bool traverse_c0 = (s0.x <= s0.y); \n"\
"                    bool traverse_c1 = (s1.x <= s1.y); \n"\
"                    bool c1first = traverse_c1 && (s0.x > s1.x); \n"\
" \n"\
"                    if (traverse_c0 || traverse_c1) \n"\
"                    { \n"\
"                        uint deferred = INVALID_ADDR; \n"\
" \n"\
"                        if (c1first || !traverse_c0) \n"\
"                        { \n"\
"                            addr = GetAddrRight(node); \n"\
"                            deferred = GetAddrLeft(node); \n"\
"                        } \n"\
"                        else \n"\
"                        { \n"\
"                            addr = GetAddrLeft(node); \n"\
"                            deferred = GetAddrRight(node); \n"\
"                        } \n"\
" \n"\
"                        if (traverse_c0 && traverse_c1) \n"\
"                        { \n"\
"                            if (lds_sptr - lds_stack_bottom >= LDS_STACK_SIZE) \n"\
"                            { \n"\
"                                for (int i = 1; i < LDS_STACK_SIZE; ++i) \n"\
"                                { \n"\
"                                    stack[sptr + i] = lds_stack[lds_stack_bottom + i]; \n"\
"                                } \n"\
" \n"\
"                                sptr += LDS_STACK_SIZE; \n"\
"                                lds_sptr = lds_stack_bottom + 1; \n"\
"                            } \n"\
" \n"\
"                            lds_stack[lds_sptr++] = deferred; \n"\
"                        } \n"\
" \n"\
"                        continue; \n"\
"                    } \n"\
"                } \n"\
"                else \n"\
"                { \n"\
"#ifdef RR_RAY_MASK \n"\
"                    if (ray_get_mask(&my_ray) != convert_int(GetMeshId(node))) \n"\
"                    { \n"\
"#endif // RR_RAY_MASK \n"\
"                        float t = fast_intersect_triangle( \n"\
"                            my_ray, \n"\
"                            node.aabb_left_min_or_v0_and_addr_left.xyz, \n"\
"                            node.aabb_left_max_or_v1_and_mesh_id.xyz, \n"\
"                            node.aabb_right_min_or_v2_and_addr_right.xyz, \n"\
"                            closest_t); \n"\
" \n"\
"                        if (t < closest_t) \n"\
"                        { \n"\
"                            hits[index] = HIT_MARKER; \n"\
"                            return; \n"\
"                        } \n"\
"#ifdef RR_RAY_MASK \n"\
"                    } \n"\
"#endif // RR_RAY_MASK \n"\
"                } \n"\
" \n"\
"                addr = lds_stack[--lds_sptr]; \n"\
" \n"\
"                if (addr == INVALID_ADDR && sptr > stack_bottom) \n"\
"                { \n"\
"                    sptr -= LDS_STACK_SIZE; \n"\
"                    for (int i = 1; i < LDS_STACK_SIZE; ++i) \n"\
"                    { \n"\
"                        lds_stack[lds_stack_bottom + i] = stack[sptr + i]; \n"\
"                    } \n"\
" \n"\
"                    lds_sptr = lds_stack_bottom + LDS_STACK_SIZE - 1; \n"\
"                    addr = lds_stack[lds_sptr]; \n"\
"                } \n"\
"            } \n"\
" \n"\
"            // Finished traversal, but no intersection found \n"\
"            hits[index] = MISS_MARKER; \n"\
"        } \n"\
"    } \n"\
"} \n"\
;
static const char g_intersect_bvh2_lds_fp16_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
" \n"\
"#pragma OPENCL EXTENSION cl_khr_fp16 : enable \n"\
" \n"\
"/************************************************************************* \n"\
"INCLUDES \n"\
"**************************************************************************/ \n"\
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
" \n"\
"/************************************************************************* \n"\
"DEFINES \n"\
"**************************************************************************/ \n"\
"#define PI 3.14159265358979323846f \n"\
"#define KERNEL __kernel \n"\
"#define GLOBAL __global \n"\
"#define INLINE __attribute__((always_inline)) \n"\
"#define HIT_MARKER 1 \n"\
"#define MISS_MARKER -1 \n"\
"#define INVALID_IDX -1 \n"\
" \n"\
"/************************************************************************* \n"\
"EXTENSIONS \n"\
"**************************************************************************/ \n"\
"#ifdef AMD_MEDIA_OPS \n"\
"#pragma OPENCL EXTENSION cl_amd_media_ops2 : enable \n"\
"#endif \n"\
" \n"\
"/************************************************************************* \n"\
"TYPES \n"\
"**************************************************************************/ \n"\
" \n"\
"// Axis aligned bounding box \n"\
"typedef struct \n"\
"{ \n"\
"    float4 pmin; \n"\
"    float4 pmax; \n"\
"} bbox; \n"\
" \n"\
"// Ray definition \n"\
"typedef struct \n"\
"{ \n"\
"    float4 o; \n"\
"    float4 d; \n"\
"    int2 extra; \n"\
"    int doBackfaceCulling; \n"\
"    int padding; \n"\
"} ray; \n"\
" \n"\
"// Intersection definition \n"\
"typedef struct \n"\
"{ \n"\
"    int shape_id; \n"\
"    int prim_id; \n"\
"    int2 padding; \n"\
" \n"\
"    float4 uvwt; \n"\
"} Intersection; \n"\
" \n"\
" \n"\
"/************************************************************************* \n"\
"HELPER FUNCTIONS \n"\
"**************************************************************************/ \n"\
"INLINE \n"\
"int ray_get_mask(ray const* r) \n"\
"{ \n"\
"    return r->extra.x; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"int ray_is_active(ray const* r) \n"\
"{ \n"\
"    return r->extra.y; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"float ray_get_maxt(ray const* r) \n"\
"{ \n"\
"    return r->o.w; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"float ray_get_time(ray const* r) \n"\
"{ \n"\
"    return r->d.w; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"int ray_get_doBackfaceCull(ray const* r) \n"\
"{ \n"\
"    return r->doBackfaceCulling; \n"\
"} \n"\
" \n"\
"/************************************************************************* \n"\
"FUNCTIONS \n"\
"**************************************************************************/ \n"\
"#ifndef APPLE \n"\
"INLINE \n"\
"float4 make_float4(float x, float y, float z, float w) \n"\
"{ \n"\
"    float4 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    res.w = w; \n"\
"    return res; \n"\
"} \n"\
"INLINE \n"\
"float3 make_float3(float x, float y, float z) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    return res; \n"\
"} \n"\
"INLINE \n"\
"float2 make_float2(float x, float y) \n"\
"{ \n"\
"    float2 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    return res; \n"\
"} \n"\
"INLINE \n"\
"int2 make_int2(int x, int y) \n"\
"{ \n"\
"    int2 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    return res; \n"\
"} \n"\
"INLINE \n"\
"int3 make_int3(int x, int y, int z) \n"\
"{ \n"\
"    int3 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    return res; \n"\
"} \n"\
"#endif \n"\
" \n"\
"INLINE float min3(float a, float b, float c) \n"\
"{ \n"\
"#ifdef AMD_MEDIA_OPS \n"\
"    return amd_min3(a, b, c); \n"\
"#else \n"\
"    return min(min(a,b), c); \n"\
"#endif \n"\
"} \n"\
" \n"\
"INLINE float max3(float a, float b, float c) \n"\
"{ \n"\
"#ifdef AMD_MEDIA_OPS \n"\
"    return amd_max3(a, b, c); \n"\
"#else \n"\
"    return max(max(a,b), c); \n"\
"#endif \n"\
"} \n"\
" \n"\
" \n"\
"// Intersect ray against a triangle and return intersection interval value if it is in \n"\
"// (0, t_max], return t_max otherwise. \n"\
"INLINE \n"\
"float fast_intersect_triangle(ray r, float3 v1, float3 v2, float3 v3, float t_max) \n"\
"{ \n"\
"    float3 const e1 = v2 - v1; \n"\
"    float3 const e2 = v3 - v1; \n"\
" \n"\
"#ifdef RR_BACKFACE_CULL \n"\
"    if (ray_get_doBackfaceCull(&r) && dot(cross(e1, e2), r.d.xyz) > 0.f) \n"\
"    { \n"\
"        return t_max; \n"\
"    } \n"\
"#endif // RR_BACKFACE_CULL \n"\
" \n"\
"    float3 const s1 = cross(r.d.xyz, e2); \n"\
" \n"\
"    float denom = dot(s1, e1); \n"\
"    if (denom == 0.f) \n"\
"    { \n"\
"        return t_max; \n"\
"    } \n"\
"     \n"\
"#ifdef USE_SAFE_MATH \n"\
"    float const invd = 1.f / denom; \n"\
"#else \n"\
"    float const invd = native_recip(denom); \n"\
"#endif \n"\
" \n"\
"    float3 const d = r.o.xyz - v1; \n"\
"    float const b1 = dot(d, s1) * invd; \n"\
"    float3 const s2 = cross(d, e1); \n"\
"    float const b2 = dot(r.d.xyz, s2) * invd; \n"\
"    float const temp = dot(e2, s2) * invd; \n"\
" \n"\
"    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f || temp < 0.f || temp > t_max) \n"\
"    { \n"\
"        return t_max; \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        return temp; \n"\
"    } \n"\
"} \n"\
" \n"\
"INLINE \n"\
"float3 safe_invdir(ray r) \n"\
"{ \n"\
"    float const dirx = r.d.x; \n"\
"    float const diry = r.d.y; \n"\
"    float const dirz = r.d.z; \n"\
"    float const ooeps = 1e-8; \n"\
"    float3 invdir; \n"\
"    invdir.x = 1.0f / (fabs(dirx) > ooeps ? dirx : copysign(ooeps, dirx)); \n"\
"    invdir.y = 1.0f / (fabs(diry) > ooeps ? diry : copysign(ooeps, diry)); \n"\
"    invdir.z = 1.0f / (fabs(dirz) > ooeps ? dirz : copysign(ooeps, dirz)); \n"\
"    return invdir; \n"\
"} \n"\
" \n"\
"// Intersect rays vs bbox and return intersection span.  \n"\
"// Intersection criteria is ret.x <= ret.y \n"\
"INLINE \n"\
"float2 fast_intersect_bbox1(bbox box, float3 invdir, float3 oxinvdir, float t_max) \n"\
"{ \n"\
"    float3 const f = mad(box.pmax.xyz, invdir, oxinvdir); \n"\
"    float3 const n = mad(box.pmin.xyz, invdir, oxinvdir); \n"\
"    float3 const tmax = max(f, n); \n"\
"    float3 const tmin = min(f, n); \n"\
"    float const t1 = min(min3(tmax.x, tmax.y, tmax.z), t_max); \n"\
"    float const t0 = max(max3(tmin.x, tmin.y, tmin.z), 0.f); \n"\
"    return make_float2(t0, t1); \n"\
"} \n"\
" \n"\
"// Given a point in triangle plane, calculate its barycentrics \n"\
"INLINE \n"\
"float2 triangle_calculate_barycentrics(float3 p, float3 v1, float3 v2, float3 v3) \n"\
"{ \n"\
"    float3 const e1 = v2 - v1; \n"\
"    float3 const e2 = v3 - v1; \n"\
"    float3 const e = p - v1; \n"\
"    float const d00 = dot(e1, e1); \n"\
"    float const d01 = dot(e1, e2); \n"\
"    float const d11 = dot(e2, e2); \n"\
"    float const d20 = dot(e, e1); \n"\
"    float const d21 = dot(e, e2); \n"\
" \n"\
"    float denom = (d00 * d11 - d01 * d01); \n"\
"     \n"\
"    if (denom == 0.f) \n"\
"    { \n"\
"        return make_float2(0.f, 0.f); \n"\
"    } \n"\
"     \n"\
"#ifdef USE_SAFE_MATH \n"\
"    float const invdenom = 1.f / denom; \n"\
"#else \n"\
"    float const invdenom = native_recip(denom); \n"\
"#endif \n"\
" \n"\
"    float const b1 = (d11 * d20 - d01 * d21) * invdenom; \n"\
"    float const b2 = (d00 * d21 - d01 * d20) * invdenom; \n"\
"    return make_float2(b1, b2); \n"\
"} \n"\
" \n"\
"/************************************************************************* \n"\
"TYPE DEFINITIONS \n"\
"**************************************************************************/ \n"\
" \n"\
"#define INVALID_ADDR 0xffffffffu \n"\
"#define INTERNAL_NODE(node) ((node).aabb01_min_or_v0_and_addr0.w != INVALID_ADDR) \n"\
" \n"\
"#define GROUP_SIZE 64 \n"\
"#define STACK_SIZE 32 \n"\
"#define LDS_STACK_SIZE 16 \n"\
" \n"\
"// BVH node \n"\
"typedef struct \n"\
"{ \n"\
"    uint4 aabb01_min_or_v0_and_addr0; \n"\
"    uint4 aabb01_max_or_v1_and_addr1_or_mesh_id; \n"\
"    uint4 aabb23_min_or_v2_and_addr2_or_prim_id; \n"\
"    uint4 aabb23_max_and_addr3; \n"\
" \n"\
"} bvh_node; \n"\
" \n"\
"#define mymin3(a, b, c) min(min((a), (b)), (c)) \n"\
"#define mymax3(a, b, c) max(max((a), (b)), (c)) \n"\
" \n"\
"INLINE half2 unpackFloat2x16(uint v) \n"\
"{ \n"\
"    return (half2) \n"\
"        (as_half(convert_ushort(v & 0xffffu)), \n"\
"         as_half(convert_ushort(v >> 16))); \n"\
"} \n"\
" \n"\
"INLINE half4 fast_intersect_bbox2(uint3 pmin, uint3 pmax, half3 invdir, half3 oxinvdir, float t_max) \n"\
"{ \n"\
"    half2 pmin_x = unpackFloat2x16(pmin.x); \n"\
"    half2 pmin_y = unpackFloat2x16(pmin.y); \n"\
"    half2 pmin_z = unpackFloat2x16(pmin.z); \n"\
"    half2 pmax_x = unpackFloat2x16(pmax.x); \n"\
"    half2 pmax_y = unpackFloat2x16(pmax.y); \n"\
"    half2 pmax_z = unpackFloat2x16(pmax.z); \n"\
" \n"\
"    half2 f_x = fma(pmax_x, invdir.xx, oxinvdir.xx); \n"\
"    half2 f_y = fma(pmax_y, invdir.yy, oxinvdir.yy); \n"\
"    half2 f_z = fma(pmax_z, invdir.zz, oxinvdir.zz); \n"\
" \n"\
"    half2 n_x = fma(pmin_x, invdir.xx, oxinvdir.xx); \n"\
"    half2 n_y = fma(pmin_y, invdir.yy, oxinvdir.yy); \n"\
"    half2 n_z = fma(pmin_z, invdir.zz, oxinvdir.zz); \n"\
" \n"\
"    half2 t_max_x = max(f_x, n_x); \n"\
"    half2 t_max_y = max(f_y, n_y); \n"\
"    half2 t_max_z = max(f_z, n_z); \n"\
" \n"\
"    half2 t_min_x = min(f_x, n_x); \n"\
"    half2 t_min_y = min(f_y, n_y); \n"\
"    half2 t_min_z = min(f_z, n_z); \n"\
" \n"\
"    half2 t_zero = (half2)(0.0f, 0.0f); \n"\
"    half2 t_max2 = (half2)(t_max, t_max); \n"\
"    half2 t1 = min(mymin3(t_max_x, t_max_y, t_max_z), t_max2); \n"\
"    half2 t0 = max(mymax3(t_min_x, t_min_y, t_min_z), t_zero); \n"\
" \n"\
"    return (half4)(t0, t1); \n"\
"} \n"\
" \n"\
"INLINE float3 safe_invdir2(ray r) \n"\
"{ \n"\
"    float const dirx = r.d.x; \n"\
"    float const diry = r.d.y; \n"\
"    float const dirz = r.d.z; \n"\
"    float const ooeps = 1e-5; \n"\
"    float3 invdir; \n"\
"    invdir.x = 1.0f / (fabs(dirx) > ooeps ? dirx : copysign(ooeps, dirx)); \n"\
"    invdir.y = 1.0f / (fabs(diry) > ooeps ? diry : copysign(ooeps, diry)); \n"\
"    invdir.z = 1.0f / (fabs(dirz) > ooeps ? dirz : copysign(ooeps, dirz)); \n"\
"    return invdir; \n"\
"} \n"\
" \n"\
"INLINE void stack_push( \n"\
"    __local uint *lds_stack, \n"\
"    __private uint *lds_sptr, \n"\
"    uint lds_stack_bottom, \n"\
"    __global uint *stack, \n"\
"    __private uint *sptr, \n"\
"    uint idx) \n"\
"{ \n"\
"    if (*lds_sptr - lds_stack_bottom >= LDS_STACK_SIZE) \n"\
"    { \n"\
"        for (int i = 1; i < LDS_STACK_SIZE; ++i) \n"\
"        { \n"\
"            stack[*sptr + i] = lds_stack[lds_stack_bottom + i]; \n"\
"        } \n"\
" \n"\
"        *sptr = *sptr + LDS_STACK_SIZE; \n"\
"        *lds_sptr = lds_stack_bottom + 1; \n"\
"    } \n"\
" \n"\
"    lds_stack[*lds_sptr] = idx; \n"\
"    *lds_sptr = *lds_sptr + 1; \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"KERNEL void intersect_main( \n"\
"    // Bvh nodes \n"\
"    GLOBAL const bvh_node *restrict nodes, \n"\
"    // Rays \n"\
"    GLOBAL const ray *restrict rays, \n"\
"    // Number of rays in rays buffer \n"\
"    GLOBAL const int *restrict num_rays, \n"\
"    // Stack memory \n"\
"    GLOBAL uint *stack, \n"\
"    // Hit data \n"\
"    GLOBAL Intersection *hits) \n"\
"{ \n"\
"    __local uint lds_stack[GROUP_SIZE * LDS_STACK_SIZE]; \n"\
" \n"\
"    uint index = get_global_id(0); \n"\
"    uint local_index = get_local_id(0); \n"\
" \n"\
"    // Handle only working subset \n"\
"    if (index < *num_rays) \n"\
"    { \n"\
"        const ray my_ray = rays[index]; \n"\
" \n"\
"        if (ray_is_active(&my_ray)) \n"\
"        { \n"\
"            // Precompute inverse direction and origin / dir for bbox testing \n"\
"            const float3 invDir32 = safe_invdir2(my_ray); \n"\
"            const half3 invDir = convert_half3(invDir32); \n"\
"            const half3 oxInvDir = convert_half3(-my_ray.o.xyz * invDir32); \n"\
" \n"\
"            // Intersection parametric distance \n"\
"            float closest_t = my_ray.o.w; \n"\
" \n"\
"            // Current node address \n"\
"            uint addr = 0; \n"\
"            // Current closest address \n"\
"            uint closest_addr = INVALID_ADDR; \n"\
" \n"\
"            uint stack_bottom = STACK_SIZE * index; \n"\
"            uint sptr = stack_bottom; \n"\
"            uint lds_stack_bottom = local_index * LDS_STACK_SIZE; \n"\
"            uint lds_sptr = lds_stack_bottom; \n"\
" \n"\
"            lds_stack[lds_sptr++] = INVALID_ADDR; \n"\
" \n"\
"            while (addr != INVALID_ADDR) \n"\
"            { \n"\
"                const bvh_node node = nodes[addr]; \n"\
" \n"\
"                if (INTERNAL_NODE(node)) \n"\
"                { \n"\
"                    half4 s01 = fast_intersect_bbox2( \n"\
"                        node.aabb01_min_or_v0_and_addr0.xyz, \n"\
"                        node.aabb01_max_or_v1_and_addr1_or_mesh_id.xyz, \n"\
"                        invDir, oxInvDir, closest_t); \n"\
"                    half4 s23 = fast_intersect_bbox2( \n"\
"                        node.aabb23_min_or_v2_and_addr2_or_prim_id.xyz, \n"\
"                        node.aabb23_max_and_addr3.xyz, \n"\
"                        invDir, oxInvDir, closest_t); \n"\
" \n"\
"                    bool traverse_c0 = (s01.x <= s01.z); \n"\
"                    bool traverse_c1 = (s01.y <= s01.w) && (node.aabb01_max_or_v1_and_addr1_or_mesh_id.w != INVALID_ADDR); \n"\
"                    bool traverse_c2 = (s23.x <= s23.z); \n"\
"                    bool traverse_c3 = (s23.y <= s23.w) && (node.aabb23_max_and_addr3.w != INVALID_ADDR); \n"\
" \n"\
"                    if (traverse_c0 || traverse_c1 || traverse_c2 || traverse_c3) \n"\
"                    { \n"\
"                        uint a = INVALID_ADDR; \n"\
"                        half d = 100000000.0f; \n"\
" \n"\
"                        if (traverse_c0) \n"\
"                        { \n"\
"                            a = node.aabb01_min_or_v0_and_addr0.w; \n"\
"                            d = s01.x; \n"\
"                        } \n"\
" \n"\
"                        if (traverse_c1) \n"\
"                        { \n"\
"                            if (a == INVALID_ADDR) \n"\
"                                a = node.aabb01_max_or_v1_and_addr1_or_mesh_id.w; \n"\
"                            else \n"\
"                            { \n"\
"                                uint topush = s01.y < d ? a : node.aabb01_max_or_v1_and_addr1_or_mesh_id.w; \n"\
"                                d = min(s01.y, d); \n"\
"                                a = topush == a ? node.aabb01_max_or_v1_and_addr1_or_mesh_id.w : a; \n"\
"                                stack_push(lds_stack, &lds_sptr, lds_stack_bottom, stack, &sptr, topush); \n"\
"                            } \n"\
"                        } \n"\
" \n"\
"                        if (traverse_c2) \n"\
"                        { \n"\
"                            if (a == INVALID_ADDR) \n"\
"                                a = node.aabb23_min_or_v2_and_addr2_or_prim_id.w; \n"\
"                            else \n"\
"                            { \n"\
"                                uint topush = s23.x < d ? a : node.aabb23_min_or_v2_and_addr2_or_prim_id.w; \n"\
"                                d = min(s23.x, d); \n"\
"                                a = topush == a ? node.aabb23_min_or_v2_and_addr2_or_prim_id.w : a; \n"\
"                                stack_push(lds_stack, &lds_sptr, lds_stack_bottom, stack, &sptr, topush); \n"\
"                            } \n"\
"                        } \n"\
" \n"\
"                        if (traverse_c3) \n"\
"                        { \n"\
"                            if (a == INVALID_ADDR) \n"\
"                                a = node.aabb23_max_and_addr3.w; \n"\
"                            else \n"\
"                            { \n"\
"                                uint topush = s23.y < d ? a : node.aabb23_max_and_addr3.w; \n"\
"                                d = min(s23.y, d); \n"\
"                                a = topush == a ? node.aabb23_max_and_addr3.w : a; \n"\
"                                stack_push(lds_stack, &lds_sptr, lds_stack_bottom, stack, &sptr, topush); \n"\
"                            } \n"\
"                        } \n"\
" \n"\
"                        addr = a; \n"\
"                        continue; \n"\
"                    } \n"\
"                } \n"\
"                else \n"\
"                { \n"\
"#ifdef RR_RAY_MASK \n"\
"                    if (ray_get_mask(&my_ray) != convert_int(GetMeshId(node))) \n"\
"                    { \n"\
"#endif // RR_RAY_MASK \n"\
"                        float t = fast_intersect_triangle( \n"\
"                            my_ray, \n"\
"                            as_float3(node.aabb01_min_or_v0_and_addr0.xyz), \n"\
"                            as_float3(node.aabb01_max_or_v1_and_addr1_or_mesh_id.xyz), \n"\
"                            as_float3(node.aabb23_min_or_v2_and_addr2_or_prim_id.xyz), \n"\
"                            closest_t); \n"\
" \n"\
"                        if (t < closest_t) \n"\
"                        { \n"\
"                            closest_t = t; \n"\
"                            closest_addr = addr; \n"\
"                        } \n"\
"#ifdef RR_RAY_MASK \n"\
"                    } \n"\
"#endif // RR_RAY_MASK \n"\
"                } \n"\
" \n"\
"                addr = lds_stack[--lds_sptr]; \n"\
" \n"\
"                if (addr == INVALID_ADDR && sptr > stack_bottom) \n"\
"                { \n"\
"                    sptr -= LDS_STACK_SIZE; \n"\
"                    for (int i = 1; i < LDS_STACK_SIZE; ++i) \n"\
"                    { \n"\
"                        lds_stack[lds_stack_bottom + i] = stack[sptr + i]; \n"\
"                    } \n"\
" \n"\
"                    lds_sptr = lds_stack_bottom + LDS_STACK_SIZE - 1; \n"\
"                    addr = lds_stack[lds_sptr]; \n"\
"                } \n"\
"            } \n"\
" \n"\
"            // Check if we have found an intersection \n"\
"            if (closest_addr != INVALID_ADDR) \n"\
"            { \n"\
"                // Calculate hit position \n"\
"                const bvh_node node = nodes[closest_addr]; \n"\
"                const float3 p = my_ray.o.xyz + closest_t * my_ray.d.xyz; \n"\
" \n"\
"                // Calculate barycentric coordinates \n"\
"                const float2 uv = triangle_calculate_barycentrics( \n"\
"                    p, \n"\
"                    as_float3(node.aabb01_min_or_v0_and_addr0.xyz), \n"\
"                    as_float3(node.aabb01_max_or_v1_and_addr1_or_mesh_id.xyz), \n"\
"                    as_float3(node.aabb23_min_or_v2_and_addr2_or_prim_id.xyz)); \n"\
" \n"\
"                // Update hit information \n"\
"                hits[index].prim_id = node.aabb23_min_or_v2_and_addr2_or_prim_id.w; \n"\
"                hits[index].shape_id = node.aabb01_max_or_v1_and_addr1_or_mesh_id.w; \n"\
"                hits[index].uvwt = (float4)(uv.x, uv.y, 0.0f, closest_t); \n"\
"            } \n"\
"            else \n"\
"            { \n"\
"                // Miss here \n"\
"                hits[index].prim_id = MISS_MARKER; \n"\
"                hits[index].shape_id = MISS_MARKER; \n"\
"            } \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"KERNEL void occluded_main( \n"\
"    // Bvh nodes \n"\
"    GLOBAL const bvh_node *restrict nodes, \n"\
"    // Rays \n"\
"    GLOBAL const ray *restrict rays, \n"\
"    // Number of rays in rays buffer \n"\
"    GLOBAL const int *restrict num_rays, \n"\
"    // Stack memory \n"\
"    GLOBAL uint *stack, \n"\
"    // Hit results: 1 for hit and -1 for miss \n"\
"    GLOBAL int *hits) \n"\
"{ \n"\
"    __local uint lds_stack[GROUP_SIZE * LDS_STACK_SIZE]; \n"\
" \n"\
"    uint index = get_global_id(0); \n"\
"    uint local_index = get_local_id(0); \n"\
" \n"\
"    // Handle only working subset \n"\
"    if (index < *num_rays) \n"\
"    { \n"\
"        const ray my_ray = rays[index]; \n"\
" \n"\
"        if (ray_is_active(&my_ray)) \n"\
"        { \n"\
"            // Precompute inverse direction and origin / dir for bbox testing \n"\
"            const float3 invDir32 = safe_invdir2(my_ray); \n"\
"            const half3 invDir = convert_half3(invDir32); \n"\
"            const half3 oxInvDir = convert_half3(-my_ray.o.xyz * invDir32); \n"\
" \n"\
"            // Intersection parametric distance \n"\
"            float closest_t = my_ray.o.w; \n"\
" \n"\
"            // Current node address \n"\
"            uint addr = 0; \n"\
"            // Current closest address \n"\
"            uint closest_addr = INVALID_ADDR; \n"\
" \n"\
"            uint stack_bottom = STACK_SIZE * index; \n"\
"            uint sptr = stack_bottom; \n"\
"            uint lds_stack_bottom = local_index * LDS_STACK_SIZE; \n"\
"            uint lds_sptr = lds_stack_bottom; \n"\
" \n"\
"            lds_stack[lds_sptr++] = INVALID_ADDR; \n"\
" \n"\
"            while (addr != INVALID_ADDR) \n"\
"            { \n"\
"                const bvh_node node = nodes[addr]; \n"\
" \n"\
"                if (INTERNAL_NODE(node)) \n"\
"                { \n"\
"                    half4 s01 = fast_intersect_bbox2( \n"\
"                        node.aabb01_min_or_v0_and_addr0.xyz, \n"\
"                        node.aabb01_max_or_v1_and_addr1_or_mesh_id.xyz, \n"\
"                        invDir, oxInvDir, closest_t); \n"\
"                    half4 s23 = fast_intersect_bbox2( \n"\
"                        node.aabb23_min_or_v2_and_addr2_or_prim_id.xyz, \n"\
"                        node.aabb23_max_and_addr3.xyz, \n"\
"                        invDir, oxInvDir, closest_t); \n"\
" \n"\
"                    bool traverse_c0 = (s01.x <= s01.z); \n"\
"                    bool traverse_c1 = (s01.y <= s01.w) && (node.aabb01_max_or_v1_and_addr1_or_mesh_id.w != INVALID_ADDR); \n"\
"                    bool traverse_c2 = (s23.x <= s23.z); \n"\
"                    bool traverse_c3 = (s23.y <= s23.w) && (node.aabb23_max_and_addr3.w != INVALID_ADDR); \n"\
" \n"\
"                    if (traverse_c0 || traverse_c1 || traverse_c2 || traverse_c3) \n"\
"                    { \n"\
"                        uint a = INVALID_ADDR; \n"\
"                        half d = 100000000.0f; \n"\
" \n"\
"                        if (traverse_c0) \n"\
"                        { \n"\
"                            a = node.aabb01_min_or_v0_and_addr0.w; \n"\
"                            d = s01.x; \n"\
"                        } \n"\
" \n"\
"                        if (traverse_c1) \n"\
"                        { \n"\
"                            if (a == INVALID_ADDR) \n"\
"                                a = node.aabb01_max_or_v1_and_addr1_or_mesh_id.w; \n"\
"                            else \n"\
"                            { \n"\
"                                uint topush = s01.y < d ? a : node.aabb01_max_or_v1_and_addr1_or_mesh_id.w; \n"\
"                                d = min(s01.y, d); \n"\
"                                a = topush == a ? node.aabb01_max_or_v1_and_addr1_or_mesh_id.w : a; \n"\
"                                stack_push(lds_stack, &lds_sptr, lds_stack_bottom, stack, &sptr, topush); \n"\
"                            } \n"\
"                        } \n"\
" \n"\
"                        if (traverse_c2) \n"\
"                        { \n"\
"                            if (a == INVALID_ADDR) \n"\
"                                a = node.aabb23_min_or_v2_and_addr2_or_prim_id.w; \n"\
"                            else \n"\
"                            { \n"\
"                                uint topush = s23.x < d ? a : node.aabb23_min_or_v2_and_addr2_or_prim_id.w; \n"\
"                                d = min(s23.x, d); \n"\
"                                a = topush == a ? node.aabb23_min_or_v2_and_addr2_or_prim_id.w : a; \n"\
"                                stack_push(lds_stack, &lds_sptr, lds_stack_bottom, stack, &sptr, topush); \n"\
"                            } \n"\
"                        } \n"\
" \n"\
"                        if (traverse_c3) \n"\
"                        { \n"\
"                            if (a == INVALID_ADDR) \n"\
"                                a = node.aabb23_max_and_addr3.w; \n"\
"                            else \n"\
"                            { \n"\
"                                uint topush = s23.y < d ? a : node.aabb23_max_and_addr3.w; \n"\
"                                d = min(s23.y, d); \n"\
"                                a = topush == a ? node.aabb23_max_and_addr3.w : a; \n"\
"                                stack_push(lds_stack, &lds_sptr, lds_stack_bottom, stack, &sptr, topush); \n"\
"                            } \n"\
"                        } \n"\
" \n"\
"                        addr = a; \n"\
"                        continue; \n"\
"                    } \n"\
"                } \n"\
"                else \n"\
"                { \n"\
"#ifdef RR_RAY_MASK \n"\
"                    if (ray_get_mask(&my_ray) != convert_int(GetMeshId(node))) \n"\
"                    { \n"\
"#endif // RR_RAY_MASK \n"\
"                        float t = fast_intersect_triangle( \n"\
"                            my_ray, \n"\
"                            as_float3(node.aabb01_min_or_v0_and_addr0.xyz), \n"\
"                            as_float3(node.aabb01_max_or_v1_and_addr1_or_mesh_id.xyz), \n"\
"                            as_float3(node.aabb23_min_or_v2_and_addr2_or_prim_id.xyz), \n"\
"                            closest_t); \n"\
" \n"\
"                        if (t < closest_t) \n"\
"                        { \n"\
"                            hits[index] = HIT_MARKER; \n"\
"                            return; \n"\
"                        } \n"\
"#ifdef RR_RAY_MASK \n"\
"                } \n"\
"#endif // RR_RAY_MASK \n"\
"                } \n"\
" \n"\
"                addr = lds_stack[--lds_sptr]; \n"\
" \n"\
"                if (addr == INVALID_ADDR && sptr > stack_bottom) \n"\
"                { \n"\
"                    sptr -= LDS_STACK_SIZE; \n"\
"                    for (int i = 1; i < LDS_STACK_SIZE; ++i) \n"\
"                    { \n"\
"                        lds_stack[lds_stack_bottom + i] = stack[sptr + i]; \n"\
"                    } \n"\
" \n"\
"                    lds_sptr = lds_stack_bottom + LDS_STACK_SIZE - 1; \n"\
"                    addr = lds_stack[lds_sptr]; \n"\
"                } \n"\
"            } \n"\
" \n"\
"            // Finished traversal, but no intersection found \n"\
"            hits[index] = MISS_MARKER; \n"\
"        } \n"\
"    } \n"\
"} \n"\
;
static const char g_intersect_bvh2_short_stack_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
"/** \n"\
"    \\file intersect_bvh2_short_stack.cl \n"\
"    \\author Dmitry Kozlov \n"\
"    \\version 1.0 \n"\
"    \\brief Intersector implementation based on BVH stacked travesal. \n"\
" \n"\
"    Intersector is using binary BVH with two bounding boxes per node. \n"\
"    Traversal is using a stack which is split into two parts: \n"\
"        -Top part in fast LDS memory \n"\
"        -Bottom part in slow global memory. \n"\
"    Push operations first check for top part overflow and offload top \n"\
"    part into slow global memory if necessary. \n"\
"    Pop operations first check for top part emptiness and try to offload \n"\
"    from bottom part if necessary.  \n"\
" \n"\
"    Traversal pseudocode: \n"\
" \n"\
"        while(addr is valid) \n"\
"        { \n"\
"            node <- fetch next node at addr \n"\
" \n"\
"            if (node is leaf) \n"\
"                intersect leaf \n"\
"            else \n"\
"            { \n"\
"                intersect ray vs left child \n"\
"                intersect ray vs right child \n"\
"                if (intersect any of children) \n"\
"                { \n"\
"                    determine closer child \n"\
"                    if intersect both \n"\
"                    { \n"\
"                        addr = closer child \n"\
"                        check top stack and offload if necesary \n"\
"                        push farther child into the stack \n"\
"                    } \n"\
"                    else \n"\
"                    { \n"\
"                        addr = intersected child \n"\
"                    } \n"\
"                    continue \n"\
"                } \n"\
"            } \n"\
" \n"\
"            addr <- pop from top stack \n"\
"            if (addr is not valid) \n"\
"            { \n"\
"                try loading data from bottom stack to top stack \n"\
"                addr <- pop from top stack \n"\
"            } \n"\
"        } \n"\
" \n"\
"    Pros: \n"\
"        -Very fast traversal. \n"\
"        -Benefits from BVH quality optimization. \n"\
"    Cons: \n"\
"        -Depth is limited. \n"\
"        -Generates LDS traffic. \n"\
" */ \n"\
" \n"\
"/************************************************************************* \n"\
"INCLUDES \n"\
"**************************************************************************/ \n"\
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
" \n"\
"/************************************************************************* \n"\
"DEFINES \n"\
"**************************************************************************/ \n"\
"#define PI 3.14159265358979323846f \n"\
"#define KERNEL __kernel \n"\
"#define GLOBAL __global \n"\
"#define INLINE __attribute__((always_inline)) \n"\
"#define HIT_MARKER 1 \n"\
"#define MISS_MARKER -1 \n"\
"#define INVALID_IDX -1 \n"\
" \n"\
"/************************************************************************* \n"\
"EXTENSIONS \n"\
"**************************************************************************/ \n"\
"#ifdef AMD_MEDIA_OPS \n"\
"#pragma OPENCL EXTENSION cl_amd_media_ops2 : enable \n"\
"#endif \n"\
" \n"\
"/************************************************************************* \n"\
"TYPES \n"\
"**************************************************************************/ \n"\
" \n"\
"// Axis aligned bounding box \n"\
"typedef struct \n"\
"{ \n"\
"    float4 pmin; \n"\
"    float4 pmax; \n"\
"} bbox; \n"\
" \n"\
"// Ray definition \n"\
"typedef struct \n"\
"{ \n"\
"    float4 o; \n"\
"    float4 d; \n"\
"    int2 extra; \n"\
"    int doBackfaceCulling; \n"\
"    int padding; \n"\
"} ray; \n"\
" \n"\
"// Intersection definition \n"\
"typedef struct \n"\
"{ \n"\
"    int shape_id; \n"\
"    int prim_id; \n"\
"    int2 padding; \n"\
" \n"\
"    float4 uvwt; \n"\
"} Intersection; \n"\
" \n"\
" \n"\
"/************************************************************************* \n"\
"HELPER FUNCTIONS \n"\
"**************************************************************************/ \n"\
"INLINE \n"\
"int ray_get_mask(ray const* r) \n"\
"{ \n"\
"    return r->extra.x; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"int ray_is_active(ray const* r) \n"\
"{ \n"\
"    return r->extra.y; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"float ray_get_maxt(ray const* r) \n"\
"{ \n"\
"    return r->o.w; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"float ray_get_time(ray const* r) \n"\
"{ \n"\
"    return r->d.w; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"int ray_get_doBackfaceCull(ray const* r) \n"\
"{ \n"\
"    return r->doBackfaceCulling; \n"\
"} \n"\
" \n"\
"/************************************************************************* \n"\
"FUNCTIONS \n"\
"**************************************************************************/ \n"\
"#ifndef APPLE \n"\
"INLINE \n"\
"float4 make_float4(float x, float y, float z, float w) \n"\
"{ \n"\
"    float4 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    res.w = w; \n"\
"    return res; \n"\
"} \n"\
"INLINE \n"\
"float3 make_float3(float x, float y, float z) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    return res; \n"\
"} \n"\
"INLINE \n"\
"float2 make_float2(float x, float y) \n"\
"{ \n"\
"    float2 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    return res; \n"\
"} \n"\
"INLINE \n"\
"int2 make_int2(int x, int y) \n"\
"{ \n"\
"    int2 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    return res; \n"\
"} \n"\
"INLINE \n"\
"int3 make_int3(int x, int y, int z) \n"\
"{ \n"\
"    int3 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    return res; \n"\
"} \n"\
"#endif \n"\
" \n"\
"INLINE float min3(float a, float b, float c) \n"\
"{ \n"\
"#ifdef AMD_MEDIA_OPS \n"\
"    return amd_min3(a, b, c); \n"\
"#else \n"\
"    return min(min(a,b), c); \n"\
"#endif \n"\
"} \n"\
" \n"\
"INLINE float max3(float a, float b, float c) \n"\
"{ \n"\
"#ifdef AMD_MEDIA_OPS \n"\
"    return amd_max3(a, b, c); \n"\
"#else \n"\
"    return max(max(a,b), c); \n"\
"#endif \n"\
"} \n"\
" \n"\
" \n"\
"// Intersect ray against a triangle and return intersection interval value if it is in \n"\
"// (0, t_max], return t_max otherwise. \n"\
"INLINE \n"\
"float fast_intersect_triangle(ray r, float3 v1, float3 v2, float3 v3, float t_max) \n"\
"{ \n"\
"    float3 const e1 = v2 - v1; \n"\
"    float3 const e2 = v3 - v1; \n"\
" \n"\
"#ifdef RR_BACKFACE_CULL \n"\
"    if (ray_get_doBackfaceCull(&r) && dot(cross(e1, e2), r.d.xyz) > 0.f) \n"\
"    { \n"\
"        return t_max; \n"\
"    } \n"\
"#endif // RR_BACKFACE_CULL \n"\
" \n"\
"    float3 const s1 = cross(r.d.xyz, e2); \n"\
" \n"\
"    float denom = dot(s1, e1); \n"\
"    if (denom == 0.f) \n"\
"    { \n"\
"        return t_max; \n"\
"    } \n"\
"     \n"\
"#ifdef USE_SAFE_MATH \n"\
"    float const invd = 1.f / denom; \n"\
"#else \n"\
"    float const invd = native_recip(denom); \n"\
"#endif \n"\
" \n"\
"    float3 const d = r.o.xyz - v1; \n"\
"    float const b1 = dot(d, s1) * invd; \n"\
"    float3 const s2 = cross(d, e1); \n"\
"    float const b2 = dot(r.d.xyz, s2) * invd; \n"\
"    float const temp = dot(e2, s2) * invd; \n"\
" \n"\
"    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f || temp < 0.f || temp > t_max) \n"\
"    { \n"\
"        return t_max; \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        return temp; \n"\
"    } \n"\
"} \n"\
" \n"\
"INLINE \n"\
"float3 safe_invdir(ray r) \n"\
"{ \n"\
"    float const dirx = r.d.x; \n"\
"    float const diry = r.d.y; \n"\
"    float const dirz = r.d.z; \n"\
"    float const ooeps = 1e-8; \n"\
"    float3 invdir; \n"\
"    invdir.x = 1.0f / (fabs(dirx) > ooeps ? dirx : copysign(ooeps, dirx)); \n"\
"    invdir.y = 1.0f / (fabs(diry) > ooeps ? diry : copysign(ooeps, diry)); \n"\
"    invdir.z = 1.0f / (fabs(dirz) > ooeps ? dirz : copysign(ooeps, dirz)); \n"\
"    return invdir; \n"\
"} \n"\
" \n"\
"// Intersect rays vs bbox and return intersection span.  \n"\
"// Intersection criteria is ret.x <= ret.y \n"\
"INLINE \n"\
"float2 fast_intersect_bbox1(bbox box, float3 invdir, float3 oxinvdir, float t_max) \n"\
"{ \n"\
"    float3 const f = mad(box.pmax.xyz, invdir, oxinvdir); \n"\
"    float3 const n = mad(box.pmin.xyz, invdir, oxinvdir); \n"\
"    float3 const tmax = max(f, n); \n"\
"    float3 const tmin = min(f, n); \n"\
"    float const t1 = min(min3(tmax.x, tmax.y, tmax.z), t_max); \n"\
"    float const t0 = max(max3(tmin.x, tmin.y, tmin.z), 0.f); \n"\
"    return make_float2(t0, t1); \n"\
"} \n"\
" \n"\
"// Given a point in triangle plane, calculate its barycentrics \n"\
"INLINE \n"\
"float2 triangle_calculate_barycentrics(float3 p, float3 v1, float3 v2, float3 v3) \n"\
"{ \n"\
"    float3 const e1 = v2 - v1; \n"\
"    float3 const e2 = v3 - v1; \n"\
"    float3 const e = p - v1; \n"\
"    float const d00 = dot(e1, e1); \n"\
"    float const d01 = dot(e1, e2); \n"\
"    float const d11 = dot(e2, e2); \n"\
"    float const d20 = dot(e, e1); \n"\
"    float const d21 = dot(e, e2); \n"\
" \n"\
"    float denom = (d00 * d11 - d01 * d01); \n"\
"     \n"\
"    if (denom == 0.f) \n"\
"    { \n"\
"        return make_float2(0.f, 0.f); \n"\
"    } \n"\
"     \n"\
"#ifdef USE_SAFE_MATH \n"\
"    float const invdenom = 1.f / denom; \n"\
"#else \n"\
"    float const invdenom = native_recip(denom); \n"\
"#endif \n"\
" \n"\
"    float const b1 = (d11 * d20 - d01 * d21) * invdenom; \n"\
"    float const b2 = (d00 * d21 - d01 * d20) * invdenom; \n"\
"    return make_float2(b1, b2); \n"\
"} \n"\
" \n"\
" \n"\
"/************************************************************************* \n"\
"TYPE DEFINITIONS \n"\
"**************************************************************************/ \n"\
" \n"\
"#define LEAFNODE(x) (((x).child0) == -1) \n"\
"#define GLOBAL_STACK_SIZE 32 \n"\
"#define SHORT_STACK_SIZE 16 \n"\
"#define WAVEFRONT_SIZE 64 \n"\
" \n"\
"// BVH node \n"\
"typedef struct \n"\
"{ \n"\
"    union  \n"\
"    { \n"\
"        struct \n"\
"        { \n"\
"            // Child bounds \n"\
"            bbox bounds[2]; \n"\
"        }; \n"\
" \n"\
"        struct \n"\
"        { \n"\
"            // If node is a leaf we keep vertex indices here \n"\
"            int i0, i1, i2; \n"\
"            // Address of a left child \n"\
"            int child0; \n"\
"            // Shape ID \n"\
"            int shape_id; \n"\
"            // Primitive ID \n"\
"            int prim_id; \n"\
"            // Address of a right child \n"\
"            int child1; \n"\
"        }; \n"\
"    }; \n"\
" \n"\
"} bvh_node; \n"\
" \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"KERNEL void \n"\
"occluded_main( \n"\
"    // Bvh nodes \n"\
"    GLOBAL bvh_node const * restrict nodes, \n"\
"    // Triangles vertices \n"\
"    GLOBAL float3 const * restrict vertices, \n"\
"    // Rays \n"\
"    GLOBAL ray const * restrict rays, \n"\
"    // Number of rays in rays buffer \n"\
"    GLOBAL int const * restrict num_rays, \n"\
"    // Stack memory \n"\
"    GLOBAL int* stack, \n"\
"    // Hit results: 1 for hit and -1 for miss \n"\
"    GLOBAL int* hits \n"\
"    ) \n"\
"{ \n"\
"    // Allocate stack in LDS \n"\
"    __local int lds[SHORT_STACK_SIZE * WAVEFRONT_SIZE]; \n"\
" \n"\
"    int global_id = get_global_id(0); \n"\
"    int local_id = get_local_id(0); \n"\
"    int group_id = get_group_id(0); \n"\
" \n"\
"    // Handle only working set \n"\
"    if (global_id < *num_rays) \n"\
"    { \n"\
"        ray const r = rays[global_id]; \n"\
" \n"\
"        if (ray_is_active(&r)) \n"\
"        { \n"\
"            // Allocate stack in global memory  \n"\
"            __global int* gm_stack_base = stack + (group_id * WAVEFRONT_SIZE + local_id) * GLOBAL_STACK_SIZE; \n"\
"            __global int* gm_stack = gm_stack_base; \n"\
" \n"\
"            __local int* lm_stack_base = lds + local_id; \n"\
"            __local int* lm_stack = lm_stack_base; \n"\
" \n"\
"            // Precompute inverse direction and origin / dir for bbox testing \n"\
"            float3 const invdir = safe_invdir(r); \n"\
"            float3 const oxinvdir = -r.o.xyz * invdir; \n"\
"            // Intersection parametric distance \n"\
"            float const t_max = r.o.w; \n"\
" \n"\
"            // Current node address \n"\
"            int addr = 0; \n"\
"            // Current closest intersection leaf index \n"\
"            int isect_idx = INVALID_IDX; \n"\
" \n"\
"            //  Initalize local stack \n"\
"            *lm_stack = INVALID_IDX; \n"\
"            lm_stack += WAVEFRONT_SIZE; \n"\
" \n"\
"            // Start from 0 node (root) \n"\
"            while (addr != INVALID_IDX) \n"\
"            { \n"\
"                // Fetch next node \n"\
"                bvh_node const node = nodes[addr]; \n"\
" \n"\
"                // Check if it is a leaf \n"\
"                if (LEAFNODE(node)) \n"\
"                { \n"\
"#ifdef RR_RAY_MASK \n"\
"                    if (ray_get_mask(&r) != node.shape_id) \n"\
"                    { \n"\
"#endif // RR_RAY_MASK \n"\
"                        // Leafs directly store vertex indices \n"\
"                        // so we load vertices directly \n"\
"                        float3 const v1 = vertices[node.i0]; \n"\
"                        float3 const v2 = vertices[node.i1]; \n"\
"                        float3 const v3 = vertices[node.i2]; \n"\
"                        // Intersect triangle \n"\
"                        float const f = fast_intersect_triangle(r, v1, v2, v3, t_max); \n"\
"                        // If hit update closest hit distance and index \n"\
"                        if (f < t_max) \n"\
"                        { \n"\
"                            hits[global_id] = HIT_MARKER; \n"\
"                            return; \n"\
"                        } \n"\
"#ifdef RR_RAY_MASK \n"\
"                    } \n"\
"#endif // RR_RAY_MASK \n"\
"                } \n"\
"                else \n"\
"                { \n"\
"                    // It is internal node, so intersect vs both children bounds \n"\
"                    float2 const s0 = fast_intersect_bbox1(node.bounds[0], invdir, oxinvdir, t_max); \n"\
"                    float2 const s1 = fast_intersect_bbox1(node.bounds[1], invdir, oxinvdir, t_max); \n"\
" \n"\
"                    // Determine which one to traverse \n"\
"                    bool const traverse_c0 = (s0.x <= s0.y); \n"\
"                    bool const traverse_c1 = (s1.x <= s1.y); \n"\
"                    bool const c1first = traverse_c1 && (s0.x > s1.x); \n"\
" \n"\
"                    if (traverse_c0 || traverse_c1) \n"\
"                    { \n"\
"                        int deferred = -1; \n"\
" \n"\
"                        // Determine which one to traverse first \n"\
"                        if (c1first || !traverse_c0) \n"\
"                        { \n"\
"                            // Right one is closer or left one not travesed \n"\
"                            addr = node.child1; \n"\
"                            deferred = node.child0; \n"\
"                        } \n"\
"                        else \n"\
"                        { \n"\
"                            // Traverse left node otherwise \n"\
"                            addr = node.child0; \n"\
"                            deferred = node.child1; \n"\
"                        } \n"\
" \n"\
"                        // If we traverse both children we need to postpone the node \n"\
"                        if (traverse_c0 && traverse_c1) \n"\
"                        { \n"\
"                            // If short stack is full, we offload it into global memory \n"\
"                            if (lm_stack - lm_stack_base >= SHORT_STACK_SIZE * WAVEFRONT_SIZE) \n"\
"                            { \n"\
"                                for (int i = 1; i < SHORT_STACK_SIZE; ++i) \n"\
"                                { \n"\
"                                    gm_stack[i] = lm_stack_base[i * WAVEFRONT_SIZE]; \n"\
"                                } \n"\
" \n"\
"                                gm_stack += SHORT_STACK_SIZE; \n"\
"                                lm_stack = lm_stack_base + WAVEFRONT_SIZE; \n"\
"                            } \n"\
" \n"\
"                            *lm_stack = deferred; \n"\
"                            lm_stack += WAVEFRONT_SIZE; \n"\
"                        } \n"\
" \n"\
"                        // Continue traversal \n"\
"                        continue; \n"\
"                    } \n"\
"                } \n"\
" \n"\
"                // Try popping from local stack \n"\
"                lm_stack -= WAVEFRONT_SIZE; \n"\
"                addr = *(lm_stack); \n"\
" \n"\
"                // If we popped INVALID_IDX then check global stack \n"\
"                if (addr == INVALID_IDX && gm_stack > gm_stack_base) \n"\
"                { \n"\
"                    // Adjust stack pointer \n"\
"                    gm_stack -= SHORT_STACK_SIZE; \n"\
"                    // Copy data from global memory to LDS \n"\
"                    for (int i = 1; i < SHORT_STACK_SIZE; ++i) \n"\
"                    { \n"\
"                        lm_stack_base[i * WAVEFRONT_SIZE] = gm_stack[i]; \n"\
"                    } \n"\
"                    // Point local stack pointer to the end \n"\
"                    lm_stack = lm_stack_base + (SHORT_STACK_SIZE - 1) * WAVEFRONT_SIZE; \n"\
"                    addr = lm_stack_base[WAVEFRONT_SIZE * (SHORT_STACK_SIZE - 1)]; \n"\
"                } \n"\
"            } \n"\
" \n"\
"            // Finished traversal, but no intersection found \n"\
"            hits[global_id] = MISS_MARKER; \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"KERNEL void intersect_main( \n"\
"    // Bvh nodes \n"\
"    GLOBAL bvh_node const* restrict nodes, \n"\
"    // Triangles vertices \n"\
"    GLOBAL float3 const* restrict vertices, \n"\
"    // Rays \n"\
"    GLOBAL ray const* restrict rays, \n"\
"    // Number of rays in rays buffer \n"\
"    GLOBAL int const* restrict num_rays, \n"\
"    // Stack memory \n"\
"    GLOBAL int* stack, \n"\
"    // Hit data \n"\
"    GLOBAL Intersection* hits) \n"\
"{ \n"\
"    // Allocate stack in LDS \n"\
"    __local int lds[SHORT_STACK_SIZE * WAVEFRONT_SIZE]; \n"\
" \n"\
"    int global_id = get_global_id(0); \n"\
"    int local_id = get_local_id(0); \n"\
"    int group_id = get_group_id(0); \n"\
" \n"\
"    // Handle only working subset \n"\
"    if (global_id < *num_rays) \n"\
"    { \n"\
"        ray const r = rays[global_id]; \n"\
" \n"\
"        if (ray_is_active(&r)) \n"\
"        { \n"\
"            // Allocate stack in global memory  \n"\
"            __global int* gm_stack_base = stack + (group_id * WAVEFRONT_SIZE + local_id) * GLOBAL_STACK_SIZE; \n"\
"            __global int* gm_stack = gm_stack_base; \n"\
"            __local int* lm_stack_base = lds + local_id; \n"\
"            __local int* lm_stack = lm_stack_base; \n"\
" \n"\
"            // Precompute inverse direction and origin / dir for bbox testing \n"\
"            float3 const invdir = safe_invdir(r); \n"\
"            float3 const oxinvdir = -r.o.xyz * invdir; \n"\
"            // Intersection parametric distance \n"\
"            float t_max = r.o.w; \n"\
" \n"\
"            // Current node address \n"\
"            int addr = 0; \n"\
"            // Current closest intersection leaf index \n"\
"            int isect_idx = INVALID_IDX; \n"\
" \n"\
"            //  Initalize local stack \n"\
"            *lm_stack = INVALID_IDX; \n"\
"            lm_stack += WAVEFRONT_SIZE; \n"\
" \n"\
"            // Start from 0 node (root) \n"\
"            while (addr != INVALID_IDX) \n"\
"            { \n"\
"                // Fetch next node \n"\
"                bvh_node const node = nodes[addr]; \n"\
" \n"\
"                // Check if it is a leaf \n"\
"                if (LEAFNODE(node)) \n"\
"                { \n"\
"#ifdef RR_RAY_MASK \n"\
"                    if (ray_get_mask(&r) != node.shape_id) \n"\
"                    { \n"\
"#endif // RR_RAY_MASK \n"\
"                        // Leafs directly store vertex indices \n"\
"                        // so we load vertices directly \n"\
"                        float3 const v1 = vertices[node.i0]; \n"\
"                        float3 const v2 = vertices[node.i1]; \n"\
"                        float3 const v3 = vertices[node.i2]; \n"\
"                        // Intersect triangle \n"\
"                        float const f = fast_intersect_triangle(r, v1, v2, v3, t_max); \n"\
"                        // If hit update closest hit distance and index \n"\
"                        if (f < t_max) \n"\
"                        { \n"\
"                            t_max = f; \n"\
"                            isect_idx = addr; \n"\
"                        } \n"\
"#ifdef RR_RAY_MASK \n"\
"                    } \n"\
"#endif // RR_RAY_MASK \n"\
"                } \n"\
"                else \n"\
"                { \n"\
"                    // It is internal node, so intersect vs both children bounds \n"\
"                    float2 const s0 = fast_intersect_bbox1(node.bounds[0], invdir, oxinvdir, t_max); \n"\
"                    float2 const s1 = fast_intersect_bbox1(node.bounds[1], invdir, oxinvdir, t_max); \n"\
" \n"\
"                    // Determine which one to traverse \n"\
"                    bool const traverse_c0 = (s0.x <= s0.y); \n"\
"                    bool const traverse_c1 = (s1.x <= s1.y); \n"\
"                    bool const c1first = traverse_c1 && (s0.x > s1.x); \n"\
" \n"\
"                    if (traverse_c0 || traverse_c1) \n"\
"                    { \n"\
"                        int deferred = -1; \n"\
" \n"\
"                        // Determine which one to traverse first \n"\
"                        if (c1first || !traverse_c0) \n"\
"                        { \n"\
"                            // Right one is closer or left one not travesed \n"\
"                            addr = node.child1; \n"\
"                            deferred = node.child0; \n"\
"                        } \n"\
"                        else \n"\
"                        { \n"\
"                            // Traverse left node otherwise \n"\
"                            addr = node.child0; \n"\
"                            deferred = node.child1; \n"\
"                        } \n"\
" \n"\
"                        // If we traverse both children we need to postpone the node \n"\
"                        if (traverse_c0 && traverse_c1) \n"\
"                        { \n"\
"                            // If short stack is full, we offload it into global memory \n"\
"                            if ( lm_stack - lm_stack_base >= SHORT_STACK_SIZE * WAVEFRONT_SIZE) \n"\
"                            { \n"\
"                                for (int i = 1; i < SHORT_STACK_SIZE; ++i) \n"\
"                                { \n"\
"                                    gm_stack[i] = lm_stack_base[i * WAVEFRONT_SIZE]; \n"\
"                                } \n"\
" \n"\
"                                gm_stack += SHORT_STACK_SIZE; \n"\
"                                lm_stack = lm_stack_base + WAVEFRONT_SIZE; \n"\
"                            } \n"\
" \n"\
"                            *lm_stack = deferred; \n"\
"                            lm_stack += WAVEFRONT_SIZE; \n"\
"                        } \n"\
" \n"\
"                        // Continue traversal \n"\
"                        continue; \n"\
"                    } \n"\
"                } \n"\
" \n"\
"                // Try popping from local stack \n"\
"                lm_stack -= WAVEFRONT_SIZE; \n"\
"                addr = *(lm_stack); \n"\
" \n"\
"                // If we popped INVALID_IDX then check global stack \n"\
"                if (addr == INVALID_IDX && gm_stack > gm_stack_base) \n"\
"                { \n"\
"                    // Adjust stack pointer \n"\
"                    gm_stack -= SHORT_STACK_SIZE; \n"\
"                    // Copy data from global memory to LDS \n"\
"                    for (int i = 1; i < SHORT_STACK_SIZE; ++i) \n"\
"                    { \n"\
"                        lm_stack_base[i * WAVEFRONT_SIZE] = gm_stack[i]; \n"\
"                    } \n"\
"                    // Point local stack pointer to the end \n"\
"                    lm_stack = lm_stack_base + (SHORT_STACK_SIZE - 1) * WAVEFRONT_SIZE; \n"\
"                    addr = lm_stack_base[WAVEFRONT_SIZE * (SHORT_STACK_SIZE - 1)]; \n"\
"                } \n"\
"            } \n"\
" \n"\
"            // Check if we have found an intersection \n"\
"            if (isect_idx != INVALID_IDX) \n"\
"            { \n"\
"                // Fetch the node & vertices \n"\
"                bvh_node const node = nodes[isect_idx]; \n"\
"                float3 const v1 = vertices[node.i0]; \n"\
"                float3 const v2 = vertices[node.i1]; \n"\
"                float3 const v3 = vertices[node.i2]; \n"\
"                // Calculate hit position \n"\
"                float3 const p = r.o.xyz + r.d.xyz * t_max; \n"\
"                // Calculate barycentric coordinates \n"\
"                float2 const uv = triangle_calculate_barycentrics(p, v1, v2, v3); \n"\
"                // Update hit information \n"\
"                hits[global_id].shape_id = node.shape_id; \n"\
"                hits[global_id].prim_id = node.prim_id; \n"\
"                hits[global_id].uvwt = make_float4(uv.x, uv.y, 0.f, t_max); \n"\
"            } \n"\
"            else \n"\
"            { \n"\
"                // Miss here \n"\
"                hits[global_id].shape_id = MISS_MARKER; \n"\
"                hits[global_id].prim_id = MISS_MARKER; \n"\
"            } \n"\
"        } \n"\
"    } \n"\
"} \n"\
;
static const char g_intersect_bvh2_skiplinks_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
"/** \n"\
"    \\file intersect_bvh2_skiplinks.cl \n"\
"    \\author Dmitry Kozlov \n"\
"    \\version 1.0 \n"\
"    \\brief Intersector implementation based on BVH with skip links. \n"\
" \n"\
"    IntersectorSkipLinks implementation is based on the following paper: \n"\
"    \"Efficiency Issues for Ray Tracing\" Brian Smits \n"\
"    http://www.cse.chalmers.se/edu/year/2016/course/course/TDA361/EfficiencyIssuesForRayTracing.pdf \n"\
" \n"\
"    Intersector is using binary BVH with a single bounding box per node. BVH layout guarantees \n"\
"    that left child of an internal node lies right next to it in memory. Each BVH node has a  \n"\
"    skip link to the node traversed next. The traversal pseude code is \n"\
" \n"\
"        while(addr is valid) \n"\
"        { \n"\
"            node <- fetch next node at addr \n"\
"            if (rays intersects with node bbox) \n"\
"            { \n"\
"                if (node is leaf) \n"\
"                    intersect leaf \n"\
"                else \n"\
"                { \n"\
"                    addr <- addr + 1 (follow left child) \n"\
"                    continue \n"\
"                } \n"\
"            } \n"\
" \n"\
"            addr <- skiplink at node (follow next) \n"\
"        } \n"\
" \n"\
"    Pros: \n"\
"        -Simple and efficient kernel with low VGPR pressure. \n"\
"        -Can traverse trees of arbitrary depth. \n"\
"    Cons: \n"\
"        -Travesal order is fixed, so poor algorithmic characteristics. \n"\
"        -Does not benefit from BVH quality optimizations. \n"\
" */ \n"\
" \n"\
"/************************************************************************* \n"\
" INCLUDES \n"\
" **************************************************************************/ \n"\
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
" \n"\
"/************************************************************************* \n"\
"DEFINES \n"\
"**************************************************************************/ \n"\
"#define PI 3.14159265358979323846f \n"\
"#define KERNEL __kernel \n"\
"#define GLOBAL __global \n"\
"#define INLINE __attribute__((always_inline)) \n"\
"#define HIT_MARKER 1 \n"\
"#define MISS_MARKER -1 \n"\
"#define INVALID_IDX -1 \n"\
" \n"\
"/************************************************************************* \n"\
"EXTENSIONS \n"\
"**************************************************************************/ \n"\
"#ifdef AMD_MEDIA_OPS \n"\
"#pragma OPENCL EXTENSION cl_amd_media_ops2 : enable \n"\
"#endif \n"\
" \n"\
"/************************************************************************* \n"\
"TYPES \n"\
"**************************************************************************/ \n"\
" \n"\
"// Axis aligned bounding box \n"\
"typedef struct \n"\
"{ \n"\
"    float4 pmin; \n"\
"    float4 pmax; \n"\
"} bbox; \n"\
" \n"\
"// Ray definition \n"\
"typedef struct \n"\
"{ \n"\
"    float4 o; \n"\
"    float4 d; \n"\
"    int2 extra; \n"\
"    int doBackfaceCulling; \n"\
"    int padding; \n"\
"} ray; \n"\
" \n"\
"// Intersection definition \n"\
"typedef struct \n"\
"{ \n"\
"    int shape_id; \n"\
"    int prim_id; \n"\
"    int2 padding; \n"\
" \n"\
"    float4 uvwt; \n"\
"} Intersection; \n"\
" \n"\
" \n"\
"/************************************************************************* \n"\
"HELPER FUNCTIONS \n"\
"**************************************************************************/ \n"\
"INLINE \n"\
"int ray_get_mask(ray const* r) \n"\
"{ \n"\
"    return r->extra.x; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"int ray_is_active(ray const* r) \n"\
"{ \n"\
"    return r->extra.y; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"float ray_get_maxt(ray const* r) \n"\
"{ \n"\
"    return r->o.w; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"float ray_get_time(ray const* r) \n"\
"{ \n"\
"    return r->d.w; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"int ray_get_doBackfaceCull(ray const* r) \n"\
"{ \n"\
"    return r->doBackfaceCulling; \n"\
"} \n"\
" \n"\
"/************************************************************************* \n"\
"FUNCTIONS \n"\
"**************************************************************************/ \n"\
"#ifndef APPLE \n"\
"INLINE \n"\
"float4 make_float4(float x, float y, float z, float w) \n"\
"{ \n"\
"    float4 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    res.w = w; \n"\
"    return res; \n"\
"} \n"\
"INLINE \n"\
"float3 make_float3(float x, float y, float z) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    return res; \n"\
"} \n"\
"INLINE \n"\
"float2 make_float2(float x, float y) \n"\
"{ \n"\
"    float2 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    return res; \n"\
"} \n"\
"INLINE \n"\
"int2 make_int2(int x, int y) \n"\
"{ \n"\
"    int2 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    return res; \n"\
"} \n"\
"INLINE \n"\
"int3 make_int3(int x, int y, int z) \n"\
"{ \n"\
"    int3 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    return res; \n"\
"} \n"\
"#endif \n"\
" \n"\
"INLINE float min3(float a, float b, float c) \n"\
"{ \n"\
"#ifdef AMD_MEDIA_OPS \n"\
"    return amd_min3(a, b, c); \n"\
"#else \n"\
"    return min(min(a,b), c); \n"\
"#endif \n"\
"} \n"\
" \n"\
"INLINE float max3(float a, float b, float c) \n"\
"{ \n"\
"#ifdef AMD_MEDIA_OPS \n"\
"    return amd_max3(a, b, c); \n"\
"#else \n"\
"    return max(max(a,b), c); \n"\
"#endif \n"\
"} \n"\
" \n"\
" \n"\
"// Intersect ray against a triangle and return intersection interval value if it is in \n"\
"// (0, t_max], return t_max otherwise. \n"\
"INLINE \n"\
"float fast_intersect_triangle(ray r, float3 v1, float3 v2, float3 v3, float t_max) \n"\
"{ \n"\
"    float3 const e1 = v2 - v1; \n"\
"    float3 const e2 = v3 - v1; \n"\
" \n"\
"#ifdef RR_BACKFACE_CULL \n"\
"    if (ray_get_doBackfaceCull(&r) && dot(cross(e1, e2), r.d.xyz) > 0.f) \n"\
"    { \n"\
"        return t_max; \n"\
"    } \n"\
"#endif // RR_BACKFACE_CULL \n"\
" \n"\
"    float3 const s1 = cross(r.d.xyz, e2); \n"\
" \n"\
"    float denom = dot(s1, e1); \n"\
"    if (denom == 0.f) \n"\
"    { \n"\
"        return t_max; \n"\
"    } \n"\
"     \n"\
"#ifdef USE_SAFE_MATH \n"\
"    float const invd = 1.f / denom; \n"\
"#else \n"\
"    float const invd = native_recip(denom); \n"\
"#endif \n"\
" \n"\
"    float3 const d = r.o.xyz - v1; \n"\
"    float const b1 = dot(d, s1) * invd; \n"\
"    float3 const s2 = cross(d, e1); \n"\
"    float const b2 = dot(r.d.xyz, s2) * invd; \n"\
"    float const temp = dot(e2, s2) * invd; \n"\
" \n"\
"    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f || temp < 0.f || temp > t_max) \n"\
"    { \n"\
"        return t_max; \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        return temp; \n"\
"    } \n"\
"} \n"\
" \n"\
"INLINE \n"\
"float3 safe_invdir(ray r) \n"\
"{ \n"\
"    float const dirx = r.d.x; \n"\
"    float const diry = r.d.y; \n"\
"    float const dirz = r.d.z; \n"\
"    float const ooeps = 1e-8; \n"\
"    float3 invdir; \n"\
"    invdir.x = 1.0f / (fabs(dirx) > ooeps ? dirx : copysign(ooeps, dirx)); \n"\
"    invdir.y = 1.0f / (fabs(diry) > ooeps ? diry : copysign(ooeps, diry)); \n"\
"    invdir.z = 1.0f / (fabs(dirz) > ooeps ? dirz : copysign(ooeps, dirz)); \n"\
"    return invdir; \n"\
"} \n"\
" \n"\
"// Intersect rays vs bbox and return intersection span.  \n"\
"// Intersection criteria is ret.x <= ret.y \n"\
"INLINE \n"\
"float2 fast_intersect_bbox1(bbox box, float3 invdir, float3 oxinvdir, float t_max) \n"\
"{ \n"\
"    float3 const f = mad(box.pmax.xyz, invdir, oxinvdir); \n"\
"    float3 const n = mad(box.pmin.xyz, invdir, oxinvdir); \n"\
"    float3 const tmax = max(f, n); \n"\
"    float3 const tmin = min(f, n); \n"\
"    float const t1 = min(min3(tmax.x, tmax.y, tmax.z), t_max); \n"\
"    float const t0 = max(max3(tmin.x, tmin.y, tmin.z), 0.f); \n"\
"    return make_float2(t0, t1); \n"\
"} \n"\
" \n"\
"// Given a point in triangle plane, calculate its barycentrics \n"\
"INLINE \n"\
"float2 triangle_calculate_barycentrics(float3 p, float3 v1, float3 v2, float3 v3) \n"\
"{ \n"\
"    float3 const e1 = v2 - v1; \n"\
"    float3 const e2 = v3 - v1; \n"\
"    float3 const e = p - v1; \n"\
"    float const d00 = dot(e1, e1); \n"\
"    float const d01 = dot(e1, e2); \n"\
"    float const d11 = dot(e2, e2); \n"\
"    float const d20 = dot(e, e1); \n"\
"    float const d21 = dot(e, e2); \n"\
" \n"\
"    float denom = (d00 * d11 - d01 * d01); \n"\
"     \n"\
"    if (denom == 0.f) \n"\
"    { \n"\
"        return make_float2(0.f, 0.f); \n"\
"    } \n"\
"     \n"\
"#ifdef USE_SAFE_MATH \n"\
"    float const invdenom = 1.f / denom; \n"\
"#else \n"\
"    float const invdenom = native_recip(denom); \n"\
"#endif \n"\
" \n"\
"    float const b1 = (d11 * d20 - d01 * d21) * invdenom; \n"\
"    float const b2 = (d00 * d21 - d01 * d20) * invdenom; \n"\
"    return make_float2(b1, b2); \n"\
"} \n"\
"/************************************************************************* \n"\
"EXTENSIONS \n"\
"**************************************************************************/ \n"\
" \n"\
"/************************************************************************* \n"\
"DEFINES \n"\
"**************************************************************************/ \n"\
"#define PI 3.14159265358979323846f \n"\
"#define STARTIDX(x)     (((int)(x.pmin.w)) >> 4) \n"\
"#define NUMPRIMS(x)     (((int)(x.pmin.w)) & 0xF) \n"\
"#define LEAFNODE(x)     (((x).pmin.w) != -1.f) \n"\
"#define NEXT(x)     ((int)((x).pmax.w)) \n"\
" \n"\
" \n"\
" \n"\
"/************************************************************************* \n"\
" TYPE DEFINITIONS \n"\
" **************************************************************************/ \n"\
"typedef bbox bvh_node; \n"\
" \n"\
"typedef struct \n"\
"{ \n"\
"    // Vertex indices \n"\
"    int idx[3]; \n"\
"    // Shape ID \n"\
"    int shape_id; \n"\
"    // Primitive ID \n"\
"    int prim_id; \n"\
"} Face; \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"KERNEL  \n"\
"void intersect_main( \n"\
"    // BVH nodes \n"\
"    GLOBAL bvh_node const* restrict nodes, \n"\
"    // Triangle vertices \n"\
"    GLOBAL float3 const* restrict vertices, \n"\
"    // Triangle indices \n"\
"    GLOBAL Face const* restrict faces, \n"\
"    // Rays  \n"\
"    GLOBAL ray const* restrict rays, \n"\
"    // Number of rays \n"\
"    GLOBAL int const* restrict num_rays, \n"\
"    // Hit data \n"\
"    GLOBAL Intersection* hits \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    if (global_id < *num_rays) \n"\
"    { \n"\
"        // Fetch ray \n"\
"        ray const r = rays[global_id]; \n"\
" \n"\
"        if (ray_is_active(&r)) \n"\
"        { \n"\
"            // Precompute inverse direction and origin / dir for bbox testing \n"\
"            float3 const invdir = safe_invdir(r); \n"\
"            float3 const oxinvdir = -r.o.xyz * invdir; \n"\
"            // Intersection parametric distance \n"\
"            float t_max = r.o.w; \n"\
" \n"\
"            // Current node address \n"\
"            int addr = 0; \n"\
"            // Current closest face index \n"\
"            int isect_idx = INVALID_IDX; \n"\
" \n"\
"            while (addr != INVALID_IDX) \n"\
"            { \n"\
"                // Fetch next node \n"\
"                bvh_node node = nodes[addr]; \n"\
"                // Intersect against bbox \n"\
"                float2 s = fast_intersect_bbox1(node, invdir, oxinvdir, t_max); \n"\
" \n"\
"                if (s.x <= s.y) \n"\
"                { \n"\
"                    // Check if the node is a leaf \n"\
"                    if (LEAFNODE(node)) \n"\
"                    { \n"\
"                        int const face_idx = STARTIDX(node); \n"\
"                        Face const face = faces[face_idx]; \n"\
"#ifdef RR_RAY_MASK \n"\
"                        if (ray_get_mask(&r) != face.shape_id) \n"\
"                        { \n"\
"#endif // RR_RAY_MASK \n"\
"                            float3 const v1 = vertices[face.idx[0]]; \n"\
"                            float3 const v2 = vertices[face.idx[1]]; \n"\
"                            float3 const v3 = vertices[face.idx[2]]; \n"\
" \n"\
"                            // Intersect triangle \n"\
"                            float const f = fast_intersect_triangle(r, v1, v2, v3, t_max); \n"\
"                            // If hit update closest hit distance and index \n"\
"                            if (f < t_max) \n"\
"                            { \n"\
"                                t_max = f; \n"\
"                                isect_idx = face_idx; \n"\
"                            } \n"\
"#ifdef RR_RAY_MASK \n"\
"                        } \n"\
"#endif // RR_RAY_MASK \n"\
"                    } \n"\
"                    else \n"\
"                    { \n"\
"                        // Move to next node otherwise. \n"\
"                        // Left child is always at addr + 1 \n"\
"                        ++addr; \n"\
"                        continue; \n"\
"                    } \n"\
"                } \n"\
" \n"\
"                addr = NEXT(node); \n"\
"            } \n"\
" \n"\
"            // Check if we have found an intersection \n"\
"            if (isect_idx != INVALID_IDX) \n"\
"            { \n"\
"                // Fetch the node & vertices \n"\
"                Face const face = faces[isect_idx]; \n"\
"                float3 const v1 = vertices[face.idx[0]]; \n"\
"                float3 const v2 = vertices[face.idx[1]]; \n"\
"                float3 const v3 = vertices[face.idx[2]]; \n"\
"                // Calculate hit position \n"\
"                float3 const p = r.o.xyz + r.d.xyz * t_max; \n"\
"                // Calculte barycentric coordinates \n"\
"                float2 const uv = triangle_calculate_barycentrics(p, v1, v2, v3); \n"\
"                // Update hit information \n"\
"                hits[global_id].shape_id = face.shape_id; \n"\
"                hits[global_id].prim_id = face.prim_id; \n"\
"                hits[global_id].uvwt = make_float4(uv.x, uv.y, 0.f, t_max); \n"\
"            } \n"\
"            else \n"\
"            { \n"\
"                // Miss here \n"\
"                hits[global_id].shape_id = MISS_MARKER; \n"\
"                hits[global_id].prim_id = MISS_MARKER; \n"\
"            } \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"KERNEL  \n"\
"void occluded_main( \n"\
"    // BVH nodes \n"\
"    GLOBAL bvh_node const* restrict nodes, \n"\
"    // Triangle vertices \n"\
"    GLOBAL float3 const* restrict vertices, \n"\
"    // Triangle indices \n"\
"    GLOBAL Face const* restrict faces, \n"\
"    // Rays  \n"\
"    GLOBAL ray const* restrict rays, \n"\
"    // Number of rays \n"\
"    GLOBAL int const* restrict num_rays, \n"\
"    // Hit data \n"\
"    GLOBAL int* hits \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    // Handle only working subset \n"\
"    if (global_id < *num_rays) \n"\
"    { \n"\
"        // Fetch ray \n"\
"        ray const r = rays[global_id]; \n"\
" \n"\
"        if (ray_is_active(&r)) \n"\
"        { \n"\
"            // Precompute inverse direction and origin / dir for bbox testing \n"\
"            float3 const invdir = safe_invdir(r); \n"\
"            float3 const oxinvdir = -r.o.xyz * invdir; \n"\
"            // Intersection parametric distance \n"\
"            float t_max = r.o.w; \n"\
" \n"\
"            // Current node address \n"\
"            int addr = 0; \n"\
" \n"\
"            while (addr != INVALID_IDX) \n"\
"            { \n"\
"                // Fetch next node \n"\
"                bvh_node node = nodes[addr]; \n"\
"                // Intersect against bbox \n"\
"                float2 s = fast_intersect_bbox1(node, invdir, oxinvdir, t_max); \n"\
" \n"\
"                if (s.x <= s.y) \n"\
"                { \n"\
"                    // Check if the node is a leaf \n"\
"                    if (LEAFNODE(node)) \n"\
"                    { \n"\
"                        int const face_idx = STARTIDX(node); \n"\
"                        Face const face = faces[face_idx]; \n"\
"#ifdef RR_RAY_MASK \n"\
"                        if (ray_get_mask(&r) != face.shape_id) \n"\
"                        { \n"\
"#endif // RR_RAY_MASK \n"\
"                            float3 const v1 = vertices[face.idx[0]]; \n"\
"                            float3 const v2 = vertices[face.idx[1]]; \n"\
"                            float3 const v3 = vertices[face.idx[2]]; \n"\
" \n"\
"                            // Intersect triangle \n"\
"                            float const f = fast_intersect_triangle(r, v1, v2, v3, t_max); \n"\
"                            // If hit store the result and bail out \n"\
"                            if (f < t_max) \n"\
"                            { \n"\
"                                hits[global_id] = HIT_MARKER; \n"\
"                                return; \n"\
"                            } \n"\
"#ifdef RR_RAY_MASK \n"\
"                        } \n"\
"#endif // RR_RAY_MASK \n"\
"                    } \n"\
"                    else \n"\
"                    { \n"\
"                        // Move to next node otherwise. \n"\
"                        // Left child is always at addr + 1 \n"\
"                        ++addr; \n"\
"                        continue; \n"\
"                    } \n"\
"                } \n"\
" \n"\
"                addr = NEXT(node); \n"\
"            } \n"\
" \n"\
"            // Finished traversal, but no intersection found \n"\
"            hits[global_id] = MISS_MARKER; \n"\
"        } \n"\
"    } \n"\
"} \n"\
;
static const char g_intersect_hlbvh_stack_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
"/** \n"\
"    \\file intersect_hlbvh_stack.cl \n"\
"    \\author Dmitry Kozlov \n"\
"    \\version 1.0 \n"\
"    \\brief HLBVH build implementation \n"\
" \n"\
"    IntersectorHlbvh implementation is based on the following paper: \n"\
"    \"HLBVH: Hierarchical LBVH Construction for Real-Time Ray Tracing\" \n"\
"    Jacopo Pantaleoni (NVIDIA), David Luebke (NVIDIA), in High Performance Graphics 2010, June 2010 \n"\
"    https://research.nvidia.com/sites/default/files/publications/HLBVH-final.pdf \n"\
" \n"\
"    Pros: \n"\
"        -Very fast to build and update. \n"\
"    Cons: \n"\
"        -Poor BVH quality, slow traversal. \n"\
" */ \n"\
" \n"\
" /************************************************************************* \n"\
"  INCLUDES \n"\
"  **************************************************************************/ \n"\
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
" \n"\
"/************************************************************************* \n"\
"DEFINES \n"\
"**************************************************************************/ \n"\
"#define PI 3.14159265358979323846f \n"\
"#define KERNEL __kernel \n"\
"#define GLOBAL __global \n"\
"#define INLINE __attribute__((always_inline)) \n"\
"#define HIT_MARKER 1 \n"\
"#define MISS_MARKER -1 \n"\
"#define INVALID_IDX -1 \n"\
" \n"\
"/************************************************************************* \n"\
"EXTENSIONS \n"\
"**************************************************************************/ \n"\
"#ifdef AMD_MEDIA_OPS \n"\
"#pragma OPENCL EXTENSION cl_amd_media_ops2 : enable \n"\
"#endif \n"\
" \n"\
"/************************************************************************* \n"\
"TYPES \n"\
"**************************************************************************/ \n"\
" \n"\
"// Axis aligned bounding box \n"\
"typedef struct \n"\
"{ \n"\
"    float4 pmin; \n"\
"    float4 pmax; \n"\
"} bbox; \n"\
" \n"\
"// Ray definition \n"\
"typedef struct \n"\
"{ \n"\
"    float4 o; \n"\
"    float4 d; \n"\
"    int2 extra; \n"\
"    int doBackfaceCulling; \n"\
"    int padding; \n"\
"} ray; \n"\
" \n"\
"// Intersection definition \n"\
"typedef struct \n"\
"{ \n"\
"    int shape_id; \n"\
"    int prim_id; \n"\
"    int2 padding; \n"\
" \n"\
"    float4 uvwt; \n"\
"} Intersection; \n"\
" \n"\
" \n"\
"/************************************************************************* \n"\
"HELPER FUNCTIONS \n"\
"**************************************************************************/ \n"\
"INLINE \n"\
"int ray_get_mask(ray const* r) \n"\
"{ \n"\
"    return r->extra.x; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"int ray_is_active(ray const* r) \n"\
"{ \n"\
"    return r->extra.y; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"float ray_get_maxt(ray const* r) \n"\
"{ \n"\
"    return r->o.w; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"float ray_get_time(ray const* r) \n"\
"{ \n"\
"    return r->d.w; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"int ray_get_doBackfaceCull(ray const* r) \n"\
"{ \n"\
"    return r->doBackfaceCulling; \n"\
"} \n"\
" \n"\
"/************************************************************************* \n"\
"FUNCTIONS \n"\
"**************************************************************************/ \n"\
"#ifndef APPLE \n"\
"INLINE \n"\
"float4 make_float4(float x, float y, float z, float w) \n"\
"{ \n"\
"    float4 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    res.w = w; \n"\
"    return res; \n"\
"} \n"\
"INLINE \n"\
"float3 make_float3(float x, float y, float z) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    return res; \n"\
"} \n"\
"INLINE \n"\
"float2 make_float2(float x, float y) \n"\
"{ \n"\
"    float2 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    return res; \n"\
"} \n"\
"INLINE \n"\
"int2 make_int2(int x, int y) \n"\
"{ \n"\
"    int2 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    return res; \n"\
"} \n"\
"INLINE \n"\
"int3 make_int3(int x, int y, int z) \n"\
"{ \n"\
"    int3 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    return res; \n"\
"} \n"\
"#endif \n"\
" \n"\
"INLINE float min3(float a, float b, float c) \n"\
"{ \n"\
"#ifdef AMD_MEDIA_OPS \n"\
"    return amd_min3(a, b, c); \n"\
"#else \n"\
"    return min(min(a,b), c); \n"\
"#endif \n"\
"} \n"\
" \n"\
"INLINE float max3(float a, float b, float c) \n"\
"{ \n"\
"#ifdef AMD_MEDIA_OPS \n"\
"    return amd_max3(a, b, c); \n"\
"#else \n"\
"    return max(max(a,b), c); \n"\
"#endif \n"\
"} \n"\
" \n"\
" \n"\
"// Intersect ray against a triangle and return intersection interval value if it is in \n"\
"// (0, t_max], return t_max otherwise. \n"\
"INLINE \n"\
"float fast_intersect_triangle(ray r, float3 v1, float3 v2, float3 v3, float t_max) \n"\
"{ \n"\
"    float3 const e1 = v2 - v1; \n"\
"    float3 const e2 = v3 - v1; \n"\
" \n"\
"#ifdef RR_BACKFACE_CULL \n"\
"    if (ray_get_doBackfaceCull(&r) && dot(cross(e1, e2), r.d.xyz) > 0.f) \n"\
"    { \n"\
"        return t_max; \n"\
"    } \n"\
"#endif // RR_BACKFACE_CULL \n"\
" \n"\
"    float3 const s1 = cross(r.d.xyz, e2); \n"\
" \n"\
"    float denom = dot(s1, e1); \n"\
"    if (denom == 0.f) \n"\
"    { \n"\
"        return t_max; \n"\
"    } \n"\
"     \n"\
"#ifdef USE_SAFE_MATH \n"\
"    float const invd = 1.f / denom; \n"\
"#else \n"\
"    float const invd = native_recip(denom); \n"\
"#endif \n"\
" \n"\
"    float3 const d = r.o.xyz - v1; \n"\
"    float const b1 = dot(d, s1) * invd; \n"\
"    float3 const s2 = cross(d, e1); \n"\
"    float const b2 = dot(r.d.xyz, s2) * invd; \n"\
"    float const temp = dot(e2, s2) * invd; \n"\
" \n"\
"    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f || temp < 0.f || temp > t_max) \n"\
"    { \n"\
"        return t_max; \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        return temp; \n"\
"    } \n"\
"} \n"\
" \n"\
"INLINE \n"\
"float3 safe_invdir(ray r) \n"\
"{ \n"\
"    float const dirx = r.d.x; \n"\
"    float const diry = r.d.y; \n"\
"    float const dirz = r.d.z; \n"\
"    float const ooeps = 1e-8; \n"\
"    float3 invdir; \n"\
"    invdir.x = 1.0f / (fabs(dirx) > ooeps ? dirx : copysign(ooeps, dirx)); \n"\
"    invdir.y = 1.0f / (fabs(diry) > ooeps ? diry : copysign(ooeps, diry)); \n"\
"    invdir.z = 1.0f / (fabs(dirz) > ooeps ? dirz : copysign(ooeps, dirz)); \n"\
"    return invdir; \n"\
"} \n"\
" \n"\
"// Intersect rays vs bbox and return intersection span.  \n"\
"// Intersection criteria is ret.x <= ret.y \n"\
"INLINE \n"\
"float2 fast_intersect_bbox1(bbox box, float3 invdir, float3 oxinvdir, float t_max) \n"\
"{ \n"\
"    float3 const f = mad(box.pmax.xyz, invdir, oxinvdir); \n"\
"    float3 const n = mad(box.pmin.xyz, invdir, oxinvdir); \n"\
"    float3 const tmax = max(f, n); \n"\
"    float3 const tmin = min(f, n); \n"\
"    float const t1 = min(min3(tmax.x, tmax.y, tmax.z), t_max); \n"\
"    float const t0 = max(max3(tmin.x, tmin.y, tmin.z), 0.f); \n"\
"    return make_float2(t0, t1); \n"\
"} \n"\
" \n"\
"// Given a point in triangle plane, calculate its barycentrics \n"\
"INLINE \n"\
"float2 triangle_calculate_barycentrics(float3 p, float3 v1, float3 v2, float3 v3) \n"\
"{ \n"\
"    float3 const e1 = v2 - v1; \n"\
"    float3 const e2 = v3 - v1; \n"\
"    float3 const e = p - v1; \n"\
"    float const d00 = dot(e1, e1); \n"\
"    float const d01 = dot(e1, e2); \n"\
"    float const d11 = dot(e2, e2); \n"\
"    float const d20 = dot(e, e1); \n"\
"    float const d21 = dot(e, e2); \n"\
" \n"\
"    float denom = (d00 * d11 - d01 * d01); \n"\
"     \n"\
"    if (denom == 0.f) \n"\
"    { \n"\
"        return make_float2(0.f, 0.f); \n"\
"    } \n"\
"     \n"\
"#ifdef USE_SAFE_MATH \n"\
"    float const invdenom = 1.f / denom; \n"\
"#else \n"\
"    float const invdenom = native_recip(denom); \n"\
"#endif \n"\
" \n"\
"    float const b1 = (d11 * d20 - d01 * d21) * invdenom; \n"\
"    float const b2 = (d00 * d21 - d01 * d20) * invdenom; \n"\
"    return make_float2(b1, b2); \n"\
"} \n"\
" \n"\
" /************************************************************************* \n"\
"   EXTENSIONS \n"\
"**************************************************************************/ \n"\
" \n"\
" \n"\
" \n"\
"/************************************************************************* \n"\
"TYPE DEFINITIONS \n"\
"**************************************************************************/ \n"\
"#define STARTIDX(x)     (((int)((x).child0))) \n"\
"#define LEAFNODE(x)     (((x).child0) == ((x).child1)) \n"\
"#define GLOBAL_STACK_SIZE 32 \n"\
"#define SHORT_STACK_SIZE 16 \n"\
"#define WAVEFRONT_SIZE 64 \n"\
" \n"\
"typedef struct \n"\
"{ \n"\
"    int parent; \n"\
"    int child0; \n"\
"    int child1; \n"\
"    int next; \n"\
"} bvh_node; \n"\
" \n"\
"typedef struct \n"\
"{ \n"\
"    // Vertex indices \n"\
"    int idx[3]; \n"\
"    // Shape ID \n"\
"    int shape_id; \n"\
"    // Primitive ID \n"\
"    int prim_id; \n"\
"} Face; \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"KERNEL void \n"\
"occluded_main( \n"\
"    // Bvh nodes \n"\
"    GLOBAL bvh_node const * restrict nodes, \n"\
"    // Bounding boxes \n"\
"    GLOBAL bbox const* restrict bounds, \n"\
"    // Triangles vertices \n"\
"    GLOBAL float3 const * restrict vertices, \n"\
"    // Triangle indices \n"\
"    GLOBAL Face const* faces, \n"\
"    // Rays \n"\
"    GLOBAL ray const * restrict rays, \n"\
"    // Number of rays in rays buffer \n"\
"    GLOBAL int const * restrict num_rays, \n"\
"    // Stack memory \n"\
"    GLOBAL int* stack, \n"\
"    // Hit results: 1 for hit and -1 for miss \n"\
"    GLOBAL int* hits \n"\
"    ) \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
"    int local_id = get_local_id(0); \n"\
"    int group_id = get_group_id(0); \n"\
" \n"\
"    // Handle only working set \n"\
"    if (global_id < *num_rays) \n"\
"    { \n"\
"        ray const r = rays[global_id]; \n"\
" \n"\
"        if (ray_is_active(&r)) \n"\
"        { \n"\
"            // Allocate stack in global memory  \n"\
"            __global int* gm_stack_base = stack + (group_id * WAVEFRONT_SIZE + local_id) * GLOBAL_STACK_SIZE; \n"\
"            __global int* gm_stack = gm_stack_base; \n"\
"            // Allocate stack in LDS \n"\
"            __local int lds[SHORT_STACK_SIZE * WAVEFRONT_SIZE]; \n"\
"            __local int* lm_stack_base = lds + local_id; \n"\
"            __local int* lm_stack = lm_stack_base; \n"\
" \n"\
"            // Precompute inverse direction and origin / dir for bbox testing \n"\
"            float3 const invdir = safe_invdir(r); \n"\
"            float3 const oxinvdir = -r.o.xyz * invdir; \n"\
"            // Intersection parametric distance \n"\
"            float const t_max = r.o.w; \n"\
" \n"\
"            // Current node address \n"\
"            int addr = 0; \n"\
"            // Current closest intersection leaf index \n"\
"            int isect_idx = INVALID_IDX; \n"\
" \n"\
"            //  Initalize local stack \n"\
"            *lm_stack = INVALID_IDX; \n"\
"            lm_stack += WAVEFRONT_SIZE; \n"\
" \n"\
"            // Start from 0 node (root) \n"\
"            while (addr != INVALID_IDX) \n"\
"            { \n"\
"                // Fetch next node \n"\
"                bvh_node const node = nodes[addr]; \n"\
" \n"\
"                // Check if it is a leaf \n"\
"                if (LEAFNODE(node)) \n"\
"                { \n"\
"                    Face face = faces[STARTIDX(node)]; \n"\
"#ifdef RR_RAY_MASK \n"\
"                    if (ray_get_mask(&r) != face.shape_id) \n"\
"                    { \n"\
"#endif // RR_RAY_MASK \n"\
"                        // Leafs directly store vertex indices \n"\
"                        // so we load vertices directly \n"\
"                        float3 const v1 = vertices[face.idx[0]]; \n"\
"                        float3 const v2 = vertices[face.idx[1]]; \n"\
"                        float3 const v3 = vertices[face.idx[2]]; \n"\
"                        // Intersect triangle \n"\
"                        float const f = fast_intersect_triangle(r, v1, v2, v3, t_max); \n"\
"                        // If hit update closest hit distance and index \n"\
"                        if (f < t_max) \n"\
"                        { \n"\
"                            hits[global_id] = HIT_MARKER; \n"\
"                            return; \n"\
"                        } \n"\
"#ifdef RR_RAY_MASK \n"\
"                    } \n"\
"#endif // RR_RAY_MASK \n"\
"                } \n"\
"                else \n"\
"                { \n"\
"                    // It is internal node, so intersect vs both children bounds \n"\
"                    float2 const s0 = fast_intersect_bbox1(bounds[node.child0], invdir, oxinvdir, t_max); \n"\
"                    float2 const s1 = fast_intersect_bbox1(bounds[node.child1], invdir, oxinvdir, t_max); \n"\
" \n"\
"                    // Determine which one to traverse \n"\
"                    bool const traverse_c0 = (s0.x <= s0.y); \n"\
"                    bool const traverse_c1 = (s1.x <= s1.y); \n"\
"                    bool const c1first = traverse_c1 && (s0.x > s1.x); \n"\
" \n"\
"                    if (traverse_c0 || traverse_c1) \n"\
"                    { \n"\
"                        int deferred = -1; \n"\
" \n"\
"                        // Determine which one to traverse first \n"\
"                        if (c1first || !traverse_c0) \n"\
"                        { \n"\
"                            // Right one is closer or left one not travesed \n"\
"                            addr = node.child1; \n"\
"                            deferred = node.child0; \n"\
"                        } \n"\
"                        else \n"\
"                        { \n"\
"                            // Traverse left node otherwise \n"\
"                            addr = node.child0; \n"\
"                            deferred = node.child1; \n"\
"                        } \n"\
" \n"\
"                        // If we traverse both children we need to postpone the node \n"\
"                        if (traverse_c0 && traverse_c1) \n"\
"                        { \n"\
"                            // If short stack is full, we offload it into global memory \n"\
"                            if (lm_stack - lm_stack_base >= SHORT_STACK_SIZE * WAVEFRONT_SIZE) \n"\
"                            { \n"\
"                                for (int i = 1; i < SHORT_STACK_SIZE; ++i) \n"\
"                                { \n"\
"                                    gm_stack[i] = lm_stack_base[i * WAVEFRONT_SIZE]; \n"\
"                                } \n"\
" \n"\
"                                gm_stack += SHORT_STACK_SIZE; \n"\
"                                lm_stack = lm_stack_base + WAVEFRONT_SIZE; \n"\
"                            } \n"\
" \n"\
"                            *lm_stack = deferred; \n"\
"                            lm_stack += WAVEFRONT_SIZE; \n"\
"                        } \n"\
" \n"\
"                        // Continue traversal \n"\
"                        continue; \n"\
"                    } \n"\
"                } \n"\
" \n"\
"                // Try popping from local stack \n"\
"                lm_stack -= WAVEFRONT_SIZE; \n"\
"                addr = *(lm_stack); \n"\
" \n"\
"                // If we popped INVALID_IDX then check global stack \n"\
"                if (addr == INVALID_IDX && gm_stack > gm_stack_base) \n"\
"                { \n"\
"                    // Adjust stack pointer \n"\
"                    gm_stack -= SHORT_STACK_SIZE; \n"\
"                    // Copy data from global memory to LDS \n"\
"                    for (int i = 1; i < SHORT_STACK_SIZE; ++i) \n"\
"                    { \n"\
"                        lm_stack_base[i * WAVEFRONT_SIZE] = gm_stack[i]; \n"\
"                    } \n"\
"                    // Point local stack pointer to the end \n"\
"                    lm_stack = lm_stack_base + (SHORT_STACK_SIZE - 1) * WAVEFRONT_SIZE; \n"\
"                    addr = lm_stack_base[WAVEFRONT_SIZE * (SHORT_STACK_SIZE - 1)]; \n"\
"                } \n"\
"            } \n"\
" \n"\
"            // Finished traversal, but no intersection found \n"\
"            hits[global_id] = MISS_MARKER; \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"KERNEL void intersect_main( \n"\
"    // Bvh nodes \n"\
"    GLOBAL bvh_node const* restrict nodes, \n"\
"    // Bounding boxes \n"\
"    GLOBAL bbox const* restrict bounds, \n"\
"    // Triangles vertices \n"\
"    GLOBAL float3 const* restrict vertices, \n"\
"    // Faces \n"\
"    GLOBAL Face const* restrict faces, \n"\
"    // Rays \n"\
"    GLOBAL ray const* restrict rays, \n"\
"    // Number of rays in rays buffer \n"\
"    GLOBAL int const* restrict num_rays, \n"\
"    // Stack memory \n"\
"    GLOBAL int* stack, \n"\
"    // Hit data \n"\
"    GLOBAL Intersection* hits) \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
"    int local_id = get_local_id(0); \n"\
"    int group_id = get_group_id(0); \n"\
" \n"\
"    // Handle only working subset \n"\
"    if (global_id < *num_rays) \n"\
"    { \n"\
"        ray const r = rays[global_id]; \n"\
" \n"\
"        if (ray_is_active(&r)) \n"\
"        { \n"\
"            // Allocate stack in global memory  \n"\
"            __global int* gm_stack_base = stack + (group_id * WAVEFRONT_SIZE + local_id) * GLOBAL_STACK_SIZE; \n"\
"            __global int* gm_stack = gm_stack_base; \n"\
"            // Allocate stack in LDS \n"\
"            __local int lds[SHORT_STACK_SIZE * WAVEFRONT_SIZE]; \n"\
"            __local int* lm_stack_base = lds + local_id; \n"\
"            __local int* lm_stack = lm_stack_base; \n"\
" \n"\
"            // Precompute inverse direction and origin / dir for bbox testing \n"\
"            float3 const invdir = safe_invdir(r); \n"\
"            float3 const oxinvdir = -r.o.xyz * invdir; \n"\
"            // Intersection parametric distance \n"\
"            float t_max = r.o.w; \n"\
" \n"\
"            // Current node address \n"\
"            int addr = 0; \n"\
"            // Current closest intersection leaf index \n"\
"            int isect_idx = INVALID_IDX; \n"\
" \n"\
"            //  Initalize local stack \n"\
"            *lm_stack = INVALID_IDX; \n"\
"            lm_stack += WAVEFRONT_SIZE; \n"\
" \n"\
"            // Start from 0 node (root) \n"\
"            while (addr != INVALID_IDX) \n"\
"            { \n"\
"                // Fetch next node \n"\
"                bvh_node const node = nodes[addr]; \n"\
" \n"\
"                // Check if it is a leaf \n"\
"                if (LEAFNODE(node)) \n"\
"                { \n"\
"                    Face face = faces[STARTIDX(node)]; \n"\
"#ifdef RR_RAY_MASK \n"\
"                    if (ray_get_mask(&r) != face.shape_id) \n"\
"                    { \n"\
"#endif // RR_RAY_MASK \n"\
"                        // Leafs directly store vertex indices \n"\
"                        // so we load vertices directly \n"\
"                        float3 const v1 = vertices[face.idx[0]]; \n"\
"                        float3 const v2 = vertices[face.idx[1]]; \n"\
"                        float3 const v3 = vertices[face.idx[2]]; \n"\
"                        // Intersect triangle \n"\
"                        float const f = fast_intersect_triangle(r, v1, v2, v3, t_max); \n"\
"                        // If hit update closest hit distance and index \n"\
"                        if (f < t_max) \n"\
"                        { \n"\
"                            t_max = f; \n"\
"                            isect_idx = STARTIDX(node); \n"\
"                        } \n"\
"#ifdef RR_RAY_MASK \n"\
"                    } \n"\
"#endif // RR_RAY_MASK \n"\
"                } \n"\
"                else \n"\
"                { \n"\
"                    // It is internal node, so intersect vs both children bounds \n"\
"                    float2 const s0 = fast_intersect_bbox1(bounds[node.child0], invdir, oxinvdir, t_max); \n"\
"                    float2 const s1 = fast_intersect_bbox1(bounds[node.child1], invdir, oxinvdir, t_max); \n"\
" \n"\
"                    // Determine which one to traverse \n"\
"                    bool const traverse_c0 = (s0.x <= s0.y); \n"\
"                    bool const traverse_c1 = (s1.x <= s1.y); \n"\
"                    bool const c1first = traverse_c1 && (s0.x > s1.x); \n"\
" \n"\
"                    if (traverse_c0 || traverse_c1) \n"\
"                    { \n"\
"                        int deferred = -1; \n"\
" \n"\
"                        // Determine which one to traverse first \n"\
"                        if (c1first || !traverse_c0) \n"\
"                        { \n"\
"                            // Right one is closer or left one not travesed \n"\
"                            addr = node.child1; \n"\
"                            deferred = node.child0; \n"\
"                        } \n"\
"                        else \n"\
"                        { \n"\
"                            // Traverse left node otherwise \n"\
"                            addr = node.child0; \n"\
"                            deferred = node.child1; \n"\
"                        } \n"\
" \n"\
"                        // If we traverse both children we need to postpone the node \n"\
"                        if (traverse_c0 && traverse_c1) \n"\
"                        { \n"\
"                            // If short stack is full, we offload it into global memory \n"\
"                            if ( lm_stack - lm_stack_base >= SHORT_STACK_SIZE * WAVEFRONT_SIZE) \n"\
"                            { \n"\
"                                for (int i = 1; i < SHORT_STACK_SIZE; ++i) \n"\
"                                { \n"\
"                                    gm_stack[i] = lm_stack_base[i * WAVEFRONT_SIZE]; \n"\
"                                } \n"\
" \n"\
"                                gm_stack += SHORT_STACK_SIZE; \n"\
"                                lm_stack = lm_stack_base + WAVEFRONT_SIZE; \n"\
"                            } \n"\
" \n"\
"                            *lm_stack = deferred; \n"\
"                            lm_stack += WAVEFRONT_SIZE; \n"\
"                        } \n"\
" \n"\
"                        // Continue traversal \n"\
"                        continue; \n"\
"                    } \n"\
"                } \n"\
" \n"\
"                // Try popping from local stack \n"\
"                lm_stack -= WAVEFRONT_SIZE; \n"\
"                addr = *(lm_stack); \n"\
" \n"\
"                // If we popped INVALID_IDX then check global stack \n"\
"                if (addr == INVALID_IDX && gm_stack > gm_stack_base) \n"\
"                { \n"\
"                    // Adjust stack pointer \n"\
"                    gm_stack -= SHORT_STACK_SIZE; \n"\
"                    // Copy data from global memory to LDS \n"\
"                    for (int i = 1; i < SHORT_STACK_SIZE; ++i) \n"\
"                    { \n"\
"                        lm_stack_base[i * WAVEFRONT_SIZE] = gm_stack[i]; \n"\
"                    } \n"\
"                    // Point local stack pointer to the end \n"\
"                    lm_stack = lm_stack_base + (SHORT_STACK_SIZE - 1) * WAVEFRONT_SIZE; \n"\
"                    addr = lm_stack_base[WAVEFRONT_SIZE * (SHORT_STACK_SIZE - 1)]; \n"\
"                } \n"\
"            } \n"\
" \n"\
"            // Check if we have found an intersection \n"\
"            if (isect_idx != INVALID_IDX) \n"\
"            { \n"\
"                // Fetch the node & vertices \n"\
"                Face const face = faces[isect_idx]; \n"\
"                float3 const v1 = vertices[face.idx[0]]; \n"\
"                float3 const v2 = vertices[face.idx[1]]; \n"\
"                float3 const v3 = vertices[face.idx[2]]; \n"\
"                // Calculate hit position \n"\
"                float3 const p = r.o.xyz + r.d.xyz * t_max; \n"\
"                // Calculte barycentric coordinates \n"\
"                float2 const uv = triangle_calculate_barycentrics(p, v1, v2, v3); \n"\
"                // Update hit information \n"\
"                hits[global_id].shape_id = face.shape_id; \n"\
"                hits[global_id].prim_id = face.prim_id; \n"\
"                hits[global_id].uvwt = make_float4(uv.x, uv.y, 0.f, t_max); \n"\
"            } \n"\
"            else \n"\
"            { \n"\
"                // Miss here \n"\
"                hits[global_id].shape_id = MISS_MARKER; \n"\
"                hits[global_id].prim_id = MISS_MARKER; \n"\
"            } \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
" \n"\
" \n"\
;
