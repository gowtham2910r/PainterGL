// Windows includes (For Time, IO, etc.)
#include <windows.h>
#include <mmsystem.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <math.h>
#include <commdlg.h>
#include <chrono>

//GlM includes
#include <glm/common.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "glm/ext/matrix_transform.hpp"

//Vector
#include <vector>
#include <random>


// OpenGL includes
#include <GL/glew.h>
#include <GL/freeglut.h>

// Assimp includes
#include <assimp/cimport.h> // scene importer
#include <assimp/scene.h> // collects data
#include <assimp/postprocess.h> // various extra operations

//Camera
//#include "Camera.cpp"
#include "shader_funcs.cpp"
#include "pathfinder.cpp"
#include "mesh_funcs.cpp"

////ImGUI
#include <imgui.h>
#include <imgui_impl_glut.h>
#include <imgui_impl_opengl3.h>

//Painting
#include "painting.h"
#include "image.h"

int width = 1600;
int height = 900;

using namespace std;

//Camera camera(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f, 45.0f);

GLuint imageTexture;
GLuint quadVAO, quadVBO;

GLuint simpleShaderProgramID = 0;

bool isPainting = false;

float strokeLengths[2] = { 5.0f, 10.0f };
float minRadius = 3.0f, maxRadius = 25.0f;
float fc = 0.5f;

std::wstring selectedFilePath;

int windowWidth, windowHeight;
bool fileNotFound = false;


std::wstring OpenFileDialog() {
	// Initialize the OPENFILENAME structure
	OPENFILENAME ofn;       // common dialog box structure
	wchar_t szFile[260];    // buffer for file name (wide-character)

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = GetActiveWindow();
	ofn.lpstrFile = szFile;  // Direct assignment of wide-char array
	ofn.lpstrFile[0] = L'\0'; // Ensure the buffer is empty initially
	ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);  // Size in wchar_t units
	ofn.lpstrFilter = L"All\0*.*\0Text\0*.TXT\0";  // File filter (wide-char)
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrTitle = L"Open File";
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	// Display the file dialog
	if (GetOpenFileName(&ofn) == TRUE) {
		// Convert the result to std::wstring and return it
		return std::wstring(ofn.lpstrFile);
	}

	return L"";  // Return an empty wide string if the dialog is canceled
}

void display_quad() {

	glUseProgram(simpleShaderProgramID);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, imageTexture);
	glUniform1i(glGetUniformLocation(simpleShaderProgramID, "image"), 0);

	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}


void bind_quad_vao_vbo() {

	float quadVertices[] = {
		// Positions   // TexCoords (Y flipped)
		-1.0f,  1.0f,  0.0f, 0.0f,
		-1.0f, -1.0f,  0.0f, 1.0f,
		 1.0f, -1.0f,  1.0f, 1.0f,

		-1.0f,  1.0f,  0.0f, 0.0f,
		 1.0f, -1.0f,  1.0f, 1.0f,
		 1.0f,  1.0f,  1.0f, 0.0f
	};

	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}




void LoadTextureFromImage(const Image& img) {
	glGenTextures(1, &imageTexture);
	glBindTexture(GL_TEXTURE_2D, imageTexture);
	printf("Channels: %d\n", img.channels);
	GLenum format = (img.channels == 3) ? GL_RGB : GL_RGBA; // Ensure correct format
	glTexImage2D(GL_TEXTURE_2D, 0, format, img.width, img.height, 0, format, GL_UNSIGNED_BYTE, img.data.data());

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenerateMipmap(GL_TEXTURE_2D);
}

void set_view_projection(GLuint shaderProgramId, glm::mat4 view, glm::mat4 persp_proj) {

	int view_mat_location = glGetUniformLocation(shaderProgramId, "view");
	int proj_mat_location = glGetUniformLocation(shaderProgramId, "proj");
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, glm::value_ptr(persp_proj));
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, glm::value_ptr(view));
}


void ComputeImageToRender() {
	std::string path = std::string(selectedFilePath.begin(), selectedFilePath.end());
	Image original_image = LoadImageFromFile(path.c_str());
	float imgAspect = (float)original_image.width / (float)original_image.height;
	float screenAspect = (float)width / (float)height;

	

	if (imgAspect > screenAspect) {
		// Image is wider than screen — scale based on width
		windowWidth = width;
		windowHeight = (int)(width / imgAspect);
	}
	else {
		// Image is taller than screen — scale based on height
		windowHeight = height;
		windowWidth = (int)(height * imgAspect);
	}

	Image canvas = CreateConstantColorImage(original_image.width, original_image.height);
	float radii[] = {0.0f, 0.0f, 0.0f};
	if (maxRadius - minRadius <= 1.0f) {
		radii[0] = (maxRadius + minRadius) / 2;
	}
	else if (maxRadius - minRadius <= 5.0f) {
		radii[0] = maxRadius;
		radii[1] = minRadius;
	}
	else {
		radii[0] = maxRadius;
		radii[1] = (maxRadius + minRadius) / 2;
		radii[2] = minRadius;
	}
	for (int i = 0; i < 3; i++) {
		if (radii[i] != 0) {
			Image blurredImage = ApplyGaussianBlur(original_image, radii[i], radii[i] / 2);
			PaintLayer(canvas, blurredImage, radii[i], fc);
		}
		
	}

	LoadTextureFromImage(canvas);
}

void display() {

	glClearColor(163 / 255.0f, 227 / 255.0f, 255 / 255.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	if (!isPainting) {
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGLUT_NewFrame();
		ImGui::NewFrame();

		// GUI Window: Bottom-Right Corner
		ImGuiIO& io = ImGui::GetIO();

		// Center the window on the screen
		ImVec2 windowSize(550, 330);
		ImVec2 windowPos = ImVec2((io.DisplaySize.x - windowSize.x) * 0.5f, (io.DisplaySize.y - windowSize.y) * 0.5f);
		ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(windowSize);

		// Optional: Set style for more polished look
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 6));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.13f, 0.14f, 0.17f, 1.0f)); // Dark background
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));         // Light text

		if (ImGui::Begin("Painterly Parameters", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
		{
			ImGui::Spacing();
			ImGui::SeparatorText("File Selection");

			if (ImGui::Button("Open File", ImVec2(-1, 0))) {
				selectedFilePath = OpenFileDialog();
			}

			if (!selectedFilePath.empty()) {
				ImGui::TextWrapped("Selected file: %S", selectedFilePath.c_str());
			}
			else {
				if (fileNotFound) {
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f)); // RGBA: bright red
					ImGui::TextWrapped("Please select a file");
					ImGui::PopStyleColor();
				}
			}

			ImGui::Spacing();
			ImGui::SeparatorText("Brush Settings");

			ImGui::DragFloatRange2("Radius Range", &minRadius, &maxRadius, 0.25f, 3.0f, 25.0f, "%.1f");
			ImGui::DragFloatRange2("Stroke Lengths", &minStrokeLength, &maxStrokeLength, 0.25f, 3.0f, 30.0f, "%.1f");

			ImGui::Spacing();
			ImGui::SeparatorText("Rendering Parameters");

			ImGui::SliderFloat("Curvature Filter", &fc, 0.1f, 1.0f, "%.2f");
			ImGui::SliderFloat("Color Threshold", &thresholdColorDiff, 5.0f, 40.0f, "%.1f");
			ImGui::SliderFloat("Jitter Amount", &jitterAmount, 0.0f, 50.0f, "%.1f");

			ImGui::Spacing();
			ImGui::Separator();

			if (ImGui::Button("Generate Image", ImVec2(-1, 0))) {
				if (!selectedFilePath.empty()) {
					auto start = std::chrono::high_resolution_clock::now();
					ComputeImageToRender();
					auto end = std::chrono::high_resolution_clock::now();
					auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
					std::cout << "Time taken to generate image: " << duration << " ms\n";
					glutReshapeWindow(windowWidth, windowHeight);
					glViewport(0, 0, windowWidth, windowHeight);
					isPainting = true;
				}
				else {
					fileNotFound = true;
				}
				
			}
		}
		ImGui::End();

		// Pop styling
		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar(3);

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}
	else {
		display_quad();
	}

	glutSwapBuffers();
}




void updateScene() {

	static DWORD last_time = 0;
	DWORD curr_time = timeGetTime();
	if (last_time == 0)
		last_time = curr_time;
	float delta = (curr_time - last_time);
	last_time = curr_time;

	// Draw the next frame
	glutPostRedisplay();
}







void init()
{
	
	bind_quad_vao_vbo();
	simpleShaderProgramID = CompileShaders(get_vertex_shader_full_path("simpleVertexShader.txt").c_str(), get_fragment_shader_full_path("simpleTextureFragmentShader.txt").c_str());

}

bool mousePressed = false;
float lastX, lastY;

void processNormalKeys(unsigned char key, int x, int y) {

	ImGui_ImplGLUT_KeyboardFunc(key, x, y);
	ImGuiIO& io = ImGui::GetIO();
	float deltaTime = 0.1f;
	if (key == 'b') {
		isPainting = false;
		glutReshapeWindow(width, height);
		glViewport(0, 0, width, height);
	}
	glutPostRedisplay();
}

void mouseButtonCallback(int button, int state, int x, int y) {
	ImGui_ImplGLUT_MouseFunc(button, state, x, y);
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureMouse) return;
	if (button == GLUT_LEFT_BUTTON) {
		if (state == GLUT_DOWN) {
			mousePressed = true;
			lastX = x;
			lastY = y;
		}
		else if (state == GLUT_UP) {
			mousePressed = false;
		}
	}
}


void mouseMotionCallback(int x, int y) {
	ImGui_ImplGLUT_MotionFunc(x, y);
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureMouse) return;
	if (mousePressed) {
		float xOffset = x - lastX;
		float yOffset = lastY - y;

		lastX = x;
		lastY = y;

		// adapt the perpective by the movement
		//camera.processMouseMovement(xOffset, yOffset);
		glutPostRedisplay();
	}
}

void OnExit() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGLUT_Shutdown();
	ImGui::DestroyContext();
}

void SetupImGui() {


	ImGui::CreateContext();
	ImGui_ImplGLUT_Init();
	ImGui_ImplOpenGL3_Init();

	ImGui_ImplGLUT_InstallFuncs();
	glutKeyboardFunc(processNormalKeys);
	glutMouseFunc(mouseButtonCallback);
	glutMotionFunc(mouseMotionCallback);


}


int main(int argc, char** argv) {

	// Set up the window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(width, height);
	glutCreateWindow("Painterly Rendering");

	SetupImGui();

	// Tell glut where the display function is
	glutDisplayFunc(display);
	glutIdleFunc(updateScene);


	// A call to glewInit() must be done after glut is initialized!
	GLenum res = glewInit();
	// Check for any errors
	if (res != GLEW_OK) {
		fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
		return 1;
	}
	// Set up your objects and shaders
	init();
	// Begin infinite event loop
	glutMainLoop();
	atexit(OnExit);
	return 0;
}
