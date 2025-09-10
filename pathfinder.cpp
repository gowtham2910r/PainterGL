using namespace std;

std::string models_dir = "C:/Users/rgowt/source/repos/RTR_FInal_Project/Models/";
std::string vertex_shader_dir = "C:/Users/rgowt/source/repos/RTR_Final_Project/Shaders/VertexShaders/";
std::string fragment_shader_dir = "C:/Users/rgowt/source/repos/RTR_Final_Project/Shaders/FragmentShaders/";
std::string texture_dir = "C:/Users/rgowt/source/repos/RTR_FInal_Project/Textures/";
std::string images_dir = "C:/Users/rgowt/source/repos/RTR_FInal_Project/Images/";

std::string get_model_full_path(const char* filename) {
	std::string model_name_str = models_dir + (string)(filename);
	return model_name_str;
}

std::string get_vertex_shader_full_path(const char* filename) {
	std::string vertex_shader_name_str = vertex_shader_dir + (string)(filename);
	return vertex_shader_name_str;
}

std::string get_fragment_shader_full_path(const char* filename) {
	std::string fragment_shader_name_str = fragment_shader_dir + (string)(filename);
	return fragment_shader_name_str;
}

std::string get_texture_full_path(const char* filename) {
	std::string texture_name_str = texture_dir + (string)(filename);
	return texture_name_str;
}

std::string get_image_full_path(const char* filename) {
	std::string image_name_str = images_dir + (string)(filename);
	return image_name_str;
}