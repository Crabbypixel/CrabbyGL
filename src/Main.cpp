#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "core/Camera.h"
#include "core/OpenGL_3D.h"

#include "models/VertexData.h"
#include "debug/ChunkDebug.h"

#include "world/Raycast.h"
#include "world/World.h"
#include "world/Chunk.h"
#include "world/BlockRegistry.h"

#include "player/Player.h"

#include "physics/WorldPhysics.h"

#include "rendering/VertexArray.h"
#include "rendering/VertexBuffer.h"
#include "rendering/BufferLayout.h"

#include "rendering/UIRenderer.h"

#include "imgui/imgui_includes.h"
#include "imgui/imgui_internal.h"

#include <iostream>
#include <iomanip>
#include <string>
#include <chrono>

class Window : public OpenGL_3D
{
private:
	// Axes
	VertexArray axesVAO;
	VertexBuffer<float> axesVBO;
	BufferLayout axesLayout;

	// Quad
	VertexArray quadVAO;
	VertexBuffer<float> quadVBO;
	BufferLayout quadLayout;

	// Crosshair
	VertexArray crosshairVAO;
	VertexBuffer<float> crosshairVBO;
	BufferLayout crosshairLayout;

	// World
	World world;
	Player player;
	ChunkDebug chunkDebug;

	// Shaders
	Shader axesShader;
	Shader framebufferShader;
	Shader crosshairShader;
	Shader chunkMeshShader;

	// Framebuffer variables
	unsigned int framebuffer;
	unsigned int textureColorBuffer;
	unsigned int rbo;

	// Constants
	const float tickSpeed = 0.05f;

	// Other variables
	float fDebugTimer = 0.0f;

	// Physics
	WorldPhysics worldPhysics;

	// Hotbar
	UIRenderer UIRenderer;
	int hotbarIndex = 0;

	// Fly
	const float DOUBLE_TAP_WINDOW = 0.3f;
	float spaceTimer = 0.0f;
	bool waitingForSecondTap = false;

	BlockType selectedBlock = BlockType::AIR;

	bool isAOEnabled = true;

public:
	bool Setup() override
	{
		player.pos = glm::vec3(20.0f, 39.0f, 78.0f);
		camera.Init(glm::vec3(24.0f, 37.0f, 56.0f), glm::vec3(0.0f, 0.0f, -1.0f), ScreenWidth(), ScreenHeight());

		// Axes
		axesVAO.generate();
		axesVBO.generate(3);		// 3 floats per vertex
		axesVBO.setBuffer(sizeof(line_vertices), (const void*)line_vertices);
		axesLayout.setBufferLayout(axesVAO, axesVBO, 3, BufferType::FLOAT);
		axesShader.load("assets/shaders/Line.glsl");

		// Quad
		quadVAO.generate();
		quadVBO.generate(4);		// 4 floats per vertex
		quadVBO.setBuffer(sizeof(quadVertices), (const void*)quadVertices);
		quadLayout.setBufferLayout(quadVAO, quadVBO, 2, BufferType::FLOAT);			// 2 for NDC vertices
		quadLayout.setBufferLayout(quadVAO, quadVBO, 2, BufferType::FLOAT);			// 2 for texture coords

		// Crosshair
		crosshairVAO.generate();
		crosshairVBO.generate(2);	// 2 floats per vertex
		crosshairVBO.setBuffer(sizeof(crosshairVertices), (const void*)crosshairVertices);
		crosshairLayout.setBufferLayout(crosshairVAO, crosshairVBO, 2, BufferType::FLOAT);	// 2 for positions (NDC)
		crosshairShader.load("assets/shaders/Crosshair.glsl");

		// Chunk boundaries
		chunkDebug.Init("assets/shaders/ChunkDebug.glsl");
		
		// ───── World ──────────────────────────────────────────────────
		auto dt1 = std::chrono::system_clock::now();

		chunkMeshShader.load("assets/shaders/ChunkMesh.glsl");

		world.SetChunkShader(chunkMeshShader);
		world.LoadAtlasTexture("assets/textures/textures.png");

		// Spawn 4 threads for loading chunks and 4 threads for generating meshes
		// This is the optimal spot for good performance without overly increasing number of threads
		world.StartChunkLoadWorkers(4);
		world.StartMeshWorkers(4);

		auto dt2 = std::chrono::system_clock::now();
		float fTimeTaken = std::chrono::duration_cast<std::chrono::milliseconds>(dt2 - dt1).count();
		std::cout << "Time taken to generate world: " << std::fixed << std::setprecision(2) << fTimeTaken / 1000.0f << " seconds" << std::endl;

		// ───── Shaders ──────────────────────────────────────────────────
		InitShaders();

		{
			// ───── Framebuffers ──────────────────────────────────────────────────
			// Generate and bind the framebuffer
			framebufferShader.load("assets/shaders/FrameBuffer.glsl");

			// Generate and bind the framebuffer
			glGenFramebuffers(1, &framebuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

			// Create a texture attachment for color attachment
			glGenTextures(1, &textureColorBuffer);
			glBindTexture(GL_TEXTURE_2D, textureColorBuffer);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ScreenWidth(), ScreenHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glBindTexture(GL_TEXTURE_2D, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorBuffer, 0);			// Finally attach the texture attachment to the currently bound framebuffer as color attachment

			// Create a renderbuffer object for depth and stencil attachments (as we won't be sampling these, renderbuffer is a better choice)
			glGenRenderbuffers(1, &rbo);
			glBindRenderbuffer(GL_RENDERBUFFER, rbo);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, ScreenWidth(), ScreenHeight());
			glBindRenderbuffer(GL_RENDERBUFFER, 0);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);				// Finally attach the renderbuffer to the currently bound framebuffer as depth & stencil attachment

			// Check if the custom framebuffer is complete
			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

			// Bind back to the default framebuffer
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			// ──────────────────────────────────────────────────────────────

			// ───── UBOs ──────────────────────────────────────────────────
			// UBOs to unnecessarily avoid settings uniforms in shaders repeatedly
			// Bind "Matrices" uniform to binding index 0 in every shader
			glUniformBlockBinding(axesShader.getID(), glGetUniformBlockIndex(axesShader.getID(), "Matrices"), 0);
			glUniformBlockBinding(chunkMeshShader.getID(), glGetUniformBlockIndex(chunkMeshShader.getID(), "Matrices"), 0);

			ErrorLog();

			// Same for the other side
			glGenBuffers(1, &uboMatrices);
			glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
			glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), NULL, GL_STATIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			glBindBufferBase(GL_UNIFORM_BUFFER, 0, uboMatrices);
			// ──────────────────────────────────────────────────────────────
		}

		// Initalize hotbar UI
		UIRenderer.Init(ScreenWidth(), ScreenHeight());

		// Initialize ImGui
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		ImGui::StyleColorsDark();
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 330");

		// Enable transparency
		glEnable(GL_BLEND);

		return true;
	}

	bool Update(float dt) override
	{
		// ImGui - new frame settings
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Bind to the custom framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

		// Clear colorbuffer, depthbuffer and stencilbuffer
		glClearColor(0.227f, 0.757f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		// Render scene to depth cubemap
		glEnable(GL_DEPTH_TEST);

		// Get user controls
		UserControls(dt);

		if (GetKey('U').bPressed)
		{
			for (int i = 200; i < 220; i++)
			{
				for (int j = 200; j < 220; j++)
				{
					world.SetBlock(i, 150, j, BlockType::SAND);
				}
			}
		}

		// ───── Physics ───────────────────────────────────────────────
		if (!bIsPaused)
		{
			player.Update(
				dt,
				camera.front,
				GetKey('W').bHeld,
				GetKey('S').bHeld,
				GetKey('A').bHeld,
				GetKey('D').bHeld,
				GetKey(GLFW_KEY_SPACE).bHeld,
				GetKey(GLFW_KEY_LEFT_SHIFT).bHeld,
				GetKey(GLFW_KEY_LEFT_CONTROL).bHeld,
				world
			);

			// Camera tracks player head
			camera.position = player.EyePos();
		}

		// Toggle rendering chunk borders
		if (GetKey('G').bPressed)
			chunkDebug.visible = !chunkDebug.visible;

		RaycastHit m_currentHit = RaycastDDA(camera.position, camera.front, world);
		glm::ivec3 raycastPlacePos = m_currentHit.blockPos + m_currentHit.normal;
		glm::ivec3 playerPos = { (int)floor(player.pos.x), (int)floor(player.pos.y), (int)floor(player.pos.z) };

		// Select block
		if (!bIsPaused && GetMouseButton(Mouse::MIDDLE).bPressed && m_currentHit.hit)
			selectedBlock = world.GetBlock(m_currentHit.blockPos.x, m_currentHit.blockPos.y, m_currentHit.blockPos.z);

		// Break block
		if (!bIsPaused && GetMouseButton(Mouse::LEFT).bPressed && m_currentHit.hit)
		{
			bool isBlockBreakValid = world.BreakBlock(m_currentHit);
			
			if(isBlockBreakValid)
				worldPhysics.NotifyBlockChanged(m_currentHit.blockPos.x, m_currentHit.blockPos.y, m_currentHit.blockPos.z);
		}

		// Place block
		if (!bIsPaused && GetMouseButton(Mouse::RIGHT).bPressed && (playerPos != raycastPlacePos) && (glm::ivec3(playerPos.x, playerPos.y + 1, playerPos.z) != raycastPlacePos) && m_currentHit.hit)
		{
			bool isBlockPlaceValid = world.PlaceBlock(m_currentHit, selectedBlock);

			if(isBlockPlaceValid)
				worldPhysics.NotifyBlockChanged(raycastPlacePos.x, raycastPlacePos.y, raycastPlacePos.z);
		}

		worldPhysics.Update(dt, world);

		// ───── Rendering ───────────────────────────────────────────────
		// Update chunk streaming state based on player position:
		// - Enqueue new chunks for generation within view distance
		// - Identify chunks outside unload distance for removal
		// - Maintains streaming window around the player
		world.UpdateChunkStreaming(player.pos);
		
		// Promote fully generated chunks from staging into the main world: 
		// - Transfers ownership into `chunks` map (main thread)
		// - Ensures chunks become visible/usable only after complete generation
		world.CommitGeneratedChunks();
		
		// Synchronize CPU-side world state with GPU rendering:
		// - Enqueue dirty chunks for meshing
		// - Upload completed mesh data to GPU buffers
		// - Remove meshes for unloaded chunks
		world.SyncRenderer();

		// Check for any errors
		Debug(dt, world);

		// Highlight targeted block
		chunkMeshShader.use();
		if (m_currentHit.hit)
		{
			chunkMeshShader.setBool("u_isSelected", true);
			chunkMeshShader.setIvec3("u_selectedBlock", m_currentHit.blockPos);
		}
		else
			chunkMeshShader.setBool("u_isSelected", false);

		chunkMeshShader.setBool("u_isAOEnabled", isAOEnabled);

		world.DrawAll(matProjection, camera.getLookAt());

		chunkDebug.DrawChunkBoundary(camera.position);

		// Coordinate axis
		RenderAxis();

		//glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);

		// Crosshair
		RenderCrosshair();

		// Hotbar
		UIRenderer.DrawHotbar();

		// Hotbar selector
		UIRenderer.DrawHotbarSelector(hotbarIndex);

		//glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);

		// Bind back to the default framebuffer & draw quad keeping the texture rendered in the custom framebuffer bounded
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDisable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

		quadVAO.bind();
		framebufferShader.use();
		framebufferShader.setInt("screenTexture", 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureColorBuffer);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		// ───── ImGui ───────────────────────────────────────────────
		glm::ivec2 playerChunk = World::ChunkCoord(player.pos.x, player.pos.z);
		ImGui::Begin("Debug Console");
		ImGui::Text("Hello World!");
		ImGui::Text("Player Position: %d %d %d", (int)camera.position.x, (int)camera.position.y, (int)camera.position.z);
		ImGui::Text("Currently at chunk: %d %d", playerChunk.x, playerChunk.y);

		ImGui::Text("Selected Block: %s", GetDef(selectedBlock).name);
		ImGui::Text("Raycast place position: %d %d %d", raycastPlacePos.x, raycastPlacePos.y, raycastPlacePos.z);
		ImGui::Text("AO (H to toggle): %s", isAOEnabled ? "Yes" : "No");
		ImGui::Text("Chunk borders (G to toggle): %s", chunkDebug.visible ? "Enabled" : "Disabled");

		static int teleportX = 0;
		static int teleportY = 0;
		static int teleportZ = 0;

		ImGui::InputInt("X", &teleportX);
		ImGui::InputInt("Y", &teleportY);
		ImGui::InputInt("Z", &teleportZ);

		if (ImGui::Button("Teleport"))
		{
			player.pos = glm::vec3(teleportX, teleportY, teleportZ);
		}

		ImGui::End();
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		return true;
	}

	void UserControls(float dt)
	{
		// Fly toggle
		if (GetKey(GLFW_KEY_SPACE).bPressed)
		{
			if (waitingForSecondTap)
			{
				if (spaceTimer <= DOUBLE_TAP_WINDOW)
				{
					// DOUBLE TAP
					player.canFly = !player.canFly;

					waitingForSecondTap = false;
					spaceTimer = DOUBLE_TAP_WINDOW + 1.0f; // invalidate
				}
				else
				{
					// Too late -> restart as first tap
					spaceTimer = 0.0f;
				}
			}
			else
			{
				// First tap
				waitingForSecondTap = true;
				spaceTimer = 0.0f;
			}
		}

		// Fly toggle - timer update
		if (waitingForSecondTap)
		{
			spaceTimer += dt;

			if (spaceTimer > DOUBLE_TAP_WINDOW)
			{
				waitingForSecondTap = false;
			}
		}

		// Toggle Ambient Occlusion
		if (GetKey('H').bPressed)
			isAOEnabled = !isAOEnabled;

		// Toggle player inventory
		if (GetKey('E').bPressed)
		{
		}

		if (GetMouseScroll() == Mouse::SCROLL_DOWN)
		{
			hotbarIndex = (hotbarIndex + 1) % 9;
		}
		else if (GetMouseScroll() == Mouse::SCROLL_UP)
		{
			hotbarIndex = (hotbarIndex - 1 + 9) % 9;
		}
	}

	void InitShaders()
	{
		chunkMeshShader.use();
		chunkMeshShader.setVec3("u_lightDir", glm::vec3(0.0f, -1.0f, 0.0f));
		chunkMeshShader.setVec3("u_ambient", glm::vec3(0.4f));
		chunkMeshShader.setVec3("u_diffuse", glm::vec3(0.7f));
	}

	void Debug(float dt, const World& world)
	{
		// Prints on the screen every 500ms
		// Nothing as of now - be happy!
		fDebugTimer += dt;

		if (fDebugTimer >= 0.5f)
		{
			fDebugTimer = 0.0f;
		}
	}

	void RenderAxis()
	{
		/*
		* Rendering process
		*	1) Bind shader
		*   2) Bind vertex array
		*   3) Activate and use shaders
		*	4) Set uniforms in shaders
		*	5) Compute model matrix
		*	6) Call glDrawElements() or glDrawArrays() to draw
		*/

		axesShader.use();
		axesVAO.bind();

		glm::mat4 matModel = glm::mat4(1.0f);
		matModel = glm::scale(matModel, glm::vec3(5.0f, 5.0f, 5.0f));
		axesShader.setMat4("matModel", matModel);

		// Increase line width
		glLineWidth(2.0f);

		// Draw axes lines
		axesShader.setVec3("vColor", 1.0f, 0.0f, 0.0f);
		glDrawArrays(GL_LINES, 0, 2);
		axesShader.setVec3("vColor", 0.0f, 1.0f, 0.0f);
		glDrawArrays(GL_LINES, 2, 2);
		axesShader.setVec3("vColor", 0.0f, 0.0f, 1.0f);
		glDrawArrays(GL_LINES, 4, 2);

		// Set line width back to normal
		glLineWidth(1.0f);
	}

	void RenderCrosshair()
	{
		glDisable(GL_DEPTH_TEST);
		glLineWidth(2.0f);

		crosshairShader.use();
		crosshairVAO.bind();
		crosshairShader.setFloat("aspect", static_cast<float>(ScreenWidth()) / static_cast<float>(ScreenHeight()));

		glDrawArrays(GL_LINES, 0, 4);	
		glEnable(GL_DEPTH_TEST);

		glLineWidth(1.0f);
	}

	void Destroy() override
	{
		world.StopAllWorkers();			// Stop all threads
		world.UnloadChunks();			// Unload all chunks

		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		glDeleteBuffers(1, &uboMatrices);
		glDeleteFramebuffers(1, &framebuffer);
		glDeleteRenderbuffers(1, &rbo);
		glDeleteTextures(1, &textureColorBuffer);

		// Check for any errors - debug
		ErrorLog("Destroy()");

		std::cout << "\nDuration: " << std::fixed << std::setprecision(2) << fTimeSinceStart << 's' << std::endl;
	}
};

int main()
{
	Window window;
	//window.ConstructWindow(800, 450, "OpenGL");
	//window.ConstructWindow(1600, 900, "OpenGL");
	window.ConstructWindow(1200, 675, "OpenGL");
	window.Start();

	std::cout << "Goodbye!" << std::endl;

	return 0;
}