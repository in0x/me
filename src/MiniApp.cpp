#include "MiniApp.h"

#include "GpuDeviceDX12.h"
#include "WindowConfig.h"
#include "InputMessageQueue.h"
#include "GeoUtils.h"

void MiniApp::Init()
{
	__super::Init();

#ifdef _DEBUG
	u32 gfx_flags = Gfx::InitFlags::Enable_Debug_Layer | Gfx::InitFlags::Allow_Tearing;
#else
	u32 gfx_flags = Gfx::InitFlags::Allow_Tearing;
#endif

	Gfx::CreateGpuDevice(GetNativeHandle(), m_window_cfg->width, m_window_cfg->height, gfx_flags);

	// Create command lists
	{
		m_upload_cmds = Gfx::CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, L"geo_upload_cmds");
		m_draw_cmds = Gfx::CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, L"draw_cmds");
		m_present_cmds = Gfx::CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, L"present_cmds");
	}

	// Generate cube geometry buffers
	{
		Gfx::OpenCommandList(m_upload_cmds);

		GeoUtils::CubeGeometry cube;
		GeoUtils::CreateBox(1.5f, 1.5f, 1.5f, &cube);

		u32 const vertex_size = sizeof(GeoUtils::Vertex);
		u32 const index_size = sizeof(GeoUtils::Index);
		m_cube_mesh.vertex_buffer_gpu = Gfx::CreateVertexBuffer(m_upload_cmds, cube.vertices, vertex_size * GeoUtils::CubeGeometry::num_vertices, vertex_size);
		m_cube_mesh.index_buffer_gpu = Gfx::CreateIndexBuffer(m_upload_cmds, cube.indices, index_size * GeoUtils::CubeGeometry::num_indices);

		Gfx::SubMesh* submesh = m_cube_mesh.submeshes.PushBack();
		submesh->num_indices = cube.num_indices;
		submesh->base_vertex_location = 0;
		submesh->first_index_location = 0;
	}

	// Generate per-frame and per-object constant buffers
	{
		Gfx::GpuBufferDesc frame;
		frame.bind_flags = Gfx::BindFlags::ConstantBuffer;
		frame.usage = Gfx::BufferUsage::Default;
		frame.cpu_access_flags = 0;
		frame.sizes_bytes = sizeof(PerFrameData);
		m_frame_constants = Gfx::CreateBuffer(m_upload_cmds, frame, L"FrameConstants");

		Gfx::GpuBufferDesc obj;
		obj.bind_flags = Gfx::BindFlags::ConstantBuffer;
		obj.usage = Gfx::BufferUsage::Dynamic;
		obj.cpu_access_flags = 0;
		obj.sizes_bytes = sizeof(PerObjectData);
		m_obj_constants = Gfx::CreateBuffer(m_upload_cmds, obj, L"ObjectConstants");
	}

	u64 upload_fence = Gfx::SubmitCommandList(m_upload_cmds);

	Gfx::CompileBasicPSOs();
	Gfx::WaitForFenceValueCpuBlocking(upload_fence);
}

void MiniApp::render()
{
	Gfx::BeginPresent(m_present_cmds);
	Gfx::OpenCommandList(m_draw_cmds);

	PerFrameData frame_constants;
	frame_constants.view_proj = m_proj * m_view;
	Gfx::UpdateBuffer(m_draw_cmds, &m_frame_constants, &frame_constants, sizeof(frame_constants));

	// TODO(): Surely I should be able to record this into upload_cmds, then submit and make draw_cmds wait on the fence.
	PerObjectData obj_constants;
	obj_constants.model = m_world;
	Gfx::UpdateBuffer(m_draw_cmds, &m_obj_constants, &obj_constants, sizeof(obj_constants));

	Gfx::BindPSO(m_draw_cmds, Gfx::BasicPSO::VertexColorSolid);
	Gfx::BindConstantBuffer(&m_frame_constants, Gfx::ShaderStage::Vertex, 0);
	Gfx::BindConstantBuffer(&m_obj_constants, Gfx::ShaderStage::Vertex, 1);

	Gfx::DrawMesh(m_draw_cmds, &m_cube_mesh);
	Gfx::SubmitCommandList(m_draw_cmds);

	Gfx::EndPresent(m_present_cmds);
}

bool IsKeyDown(InputMessages const* msg, KeyCode::Enum key)
{
	for (u32 i = msg->m_keys.Size(); i--;)
	{
		if (msg->m_keys[i].m_key == key)
		{
			return true;
		}
	}

	return false;
}

struct ArcBallCamera
{
	f32 m_phi = Math::Pi / 4.0f;
	f32 m_zoom = 5.0f;
	f32 m_theta =  Math::Pi * 1.5f;

	vec3 m_eye_pos;
};

void UpdateCamera(InputMessages const* input, ArcBallCamera* camera)
{
	f32 const rot_speed = 0.2f;

	if (IsKeyDown(input, KeyCode::S))
	{
		camera->m_phi += rot_speed;
	}
	
	if (IsKeyDown(input, KeyCode::W))
	{
		camera->m_phi -= rot_speed;
	}
	
	if (IsKeyDown(input, KeyCode::D))
	{
		camera->m_theta += rot_speed;
	}

	if (IsKeyDown(input, KeyCode::A))
	{
		camera->m_theta -= rot_speed;
	}

	camera->m_phi = Clamp(camera->m_phi, 0.1f, Math::Pi - 0.1f); // NOTE(): Restrict to ~+-180�

	camera->m_eye_pos.x = camera->m_zoom * sinf(camera->m_phi) * cosf(camera->m_theta);
	camera->m_eye_pos.z = camera->m_zoom * sinf(camera->m_phi) * sinf(camera->m_theta);
	camera->m_eye_pos.y = camera->m_zoom * cosf(camera->m_phi);
}

bool MiniApp::Update()
{
	__super::Update();

	InputMessages input = m_msg_queue->PumpMessages();
	
	if (input.m_wants_to_quit)
	{
		Gfx::Flush();

		// Don't start another frame if we want to quit 
		// and have stopped pumping the window thread.
		return false;
	}

	f32 total_time = GetTotalTimeS(m_timer);

	// Calc single object world matrix
	{
		f32 angle = sin(total_time);

		mat44 translate = Math::Translation<mat44>(0.0f, 0.0f, 0.0f);
		mat44 rotation = Math::RotationX<mat44>(Math::Rad(angle));

		//m_world = translate * rotation;
		m_world = translate;
	}

	static ArcBallCamera s_camera;
	UpdateCamera(&input, &s_camera);

	// Calc view matrix
	{
		vec3 eye_pos = s_camera.m_eye_pos;
		vec3 look_at = vec3(0.0f, 0.0f, 0.0f);
		vec3 up = Math::UpDir();

		m_view = Math::MatrixLookAtLH(eye_pos, look_at, up);
	}

	// Calc proj mat
	{
		f32 aspect_ratio = (f32)m_window_cfg->width / (f32)m_window_cfg->height;
		f32 fov_y = Math::DegreeToRad(70.0f);

		m_proj = Math::MatrixPerspectiveFovLH(fov_y, aspect_ratio, 0.01f, 1000.0f);
	}

	render();
	
	return true;
}

void MiniApp::Exit()
{
	__super::Exit();
	Gfx::DestroyGpuDevice();
}