#pragma once

#include "Win32.h"
#include "d3dx12.h"
#include "Array.h"
#include "Math.h"

using Microsoft::WRL::ComPtr;

namespace Gfx
{
	struct ResourceState
	{
		enum Enum
		{
			Common = 0,
			Vertex_And_Constant_Buffer = 0x1,
			Index_Buffer = 0x2,
			Render_Target = 0x4,
			Unordered_Acces = 0x8,
			Depth_Write = 0x10,
			Depth_Read = 0x20,
			Non_Pixel_Shader_Resource = 0x40,
			Pixel_Shader_Resource = 0x80,
			Stream_Out = 0x100,
			Indirect_Argument = 0x200,
			Copy_Dest = 0x400,
			Copy_Source = 0x800,
			Resolve_Dest = 0x1000,
			Resolve_Source = 0x2000,
			Generic_Read = (((((0x1 | 0x2) | 0x40) | 0x80) | 0x200) | 0x800),
			Present = 0,
			Predication = 0x200,
			Video_Decode_Read = 0x10000,
			Video_Decode_Write = 0x20000,
			Video_Proecss_Read = 0x40000,
			Video_Process_Write = 0x80000
		};
	};

	struct ShaderStage
	{
		enum Enum
		{
			Vertex,
			Pixel,
			Geometry,
			Hull,
			Domain,
			Compute,
			Invalid,
			Count = Invalid
		};
	};

	static_assert(ShaderStage::Vertex == 0, "Assuming ShaderStage 0 is Vertex!");

	struct BufferUsage
	{
		enum Enum
		{
			Default,
			Immutable,
			Dynamic,
			Staging
		};
	};

	struct BindFlags
	{
		enum Enum
		{
			VertexBuffer = 1 << 0,
			IndexBuffer = 1 << 2,
			ConstantBuffer = 1 << 3,
			ShaderResource = 1 << 4,
			StreamOutput = 1 << 5,
			RenderTarget = 1 << 6,
			DepthStencil = 1 << 7,
			UnorderedAccess = 1 << 8,
		};
	};

	struct ResourceFlags
	{
		enum Enum
		{
			AllowRawViews = 1 << 0,
			StructuredBuffer = 1 << 1,
			GenerateMips = 1 << 2,
			Shared = 1 << 3,
			TextureCube = 1 << 4,
			DrawIndirectArgs = 1 << 5,
			Tiled = 1 << 6
		};
	};

	struct Shader
	{
		ID3DBlob* blob = nullptr;
		ShaderStage::Enum stage;
	};

	struct GraphicsPSO
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;
		ID3D12PipelineState* pso = nullptr;
	};

	struct PSO
	{
		static const s32 INVALID_HANDLE = -1;

		s32 handle = INVALID_HANDLE;

		inline bool IsValid() const
		{
			return handle != INVALID_HANDLE;
		}
	};

	struct GpuBufferDesc
	{
		u32 sizes_bytes = 0;
		u32 bind_flags = 0;
		u32 cpu_access_flags = 0;
		u32 misc_flags = 0;
		u32 stride_in_bytes = 0;
		BufferUsage::Enum usage = BufferUsage::Default;
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
	};

	struct GpuBuffer
	{
		GpuBufferDesc desc;
		ID3D12Resource* resource = nullptr;
		D3D12_CPU_DESCRIPTOR_HANDLE srv;
		D3D12_CPU_DESCRIPTOR_HANDLE uav;
		D3D12_CPU_DESCRIPTOR_HANDLE cbv;
	};

	struct CpuBuffer
	{
		ComPtr<ID3DBlob> blob;
	};

	struct Commandlist
	{
		static const s32 INVALID_HANDLE = -1;

		s32 handle = INVALID_HANDLE;

		inline bool IsValid() const
		{
			return handle != INVALID_HANDLE;
		}
	};

	struct SubMesh
	{
		u32 num_indices = 0;
		u32 first_index_location = 0;
		u32 base_vertex_location = 0;
	};

	using Position_t = vec3;
	using Normal_t = vec3;
	using Tangent_t = vec3;
	using TexCoord_t = vec2;
	using Index_t = u16;

	struct VertexAttribType
	{
		enum Enum
		{
			Position,
			Normal,
			TexCoord,
			Tangent,

			EnumCount,
			EnumFirst = Position
		};
	};

	struct Mesh
	{
		GpuBuffer vertex_attribs_gpu[VertexAttribType::EnumCount];
		GpuBuffer index_buffer_gpu;

		Array<SubMesh, 8> submeshes;
	};
}
