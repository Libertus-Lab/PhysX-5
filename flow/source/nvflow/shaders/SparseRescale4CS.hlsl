// SPDX-FileCopyrightText: Copyright (c) 2014-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "NvFlowShader.hlsli"

#include "SparseParams.h"

ConstantBuffer<SparseRescaleGlobalParams> globalParamsIn;
StructuredBuffer<SparseRescaleLayerParams> layerParamsIn;

StructuredBuffer<uint> gTable;
StructuredBuffer<uint> gTableOld;

SamplerState valueSampler;

Texture3D<float4> valueIn;
RWTexture3D<float4> valueOut;

[numthreads(128, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupID : SV_GroupID)
{
    int3 threadIdx = NvFlowComputeThreadIdx(globalParamsIn.tableParams, dispatchThreadID.x);
    uint blockIdx = groupID.y + globalParamsIn.blockIdxOffset;

    uint layerParamIdx = NvFlowGetLayerParamIdx(gTable, globalParamsIn.tableParams, blockIdx);

    int4 vidx;
    NvFlowRealToVirtual(gTable, globalParamsIn.tableParams, blockIdx, threadIdx, vidx);

    float3 vidxf = float3(vidx.xyz) + float3(0.5f, 0.5f, 0.5f);

    float3 oldVidxf = vidxf.xyz * layerParamsIn[layerParamIdx].blockSizeWorld / layerParamsIn[layerParamIdx].blockSizeWorldOld;

    float4 copyValue;
    if (bool(layerParamsIn[layerParamIdx].shouldClear))
    {
        copyValue = float4(0.f, 0.f, 0.f, 0.f);
    }
    else
    {
        copyValue = NvFlowGlobalReadLinear4f(valueIn, valueSampler, gTableOld, globalParamsIn.tableParamsOld, oldVidxf, vidx.w);
    }

    NvFlowLocalWrite4f(valueOut, gTable, globalParamsIn.tableParams, blockIdx, threadIdx, copyValue);
}
