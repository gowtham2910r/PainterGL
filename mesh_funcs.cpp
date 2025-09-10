/*----------------------------------------------------------------------------
MESH LOADING FUNCTIONS
----------------------------------------------------------------------------*/
struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoords;
	glm::vec3 tangent;
};

struct Mesh {
	unsigned int VAO, VBO, EBO; // OpenGL buffers
	std::vector<Vertex> vertices; // Vertex data (positions, normals, textures)
	std::vector<unsigned int> indices; // Index data
};

Mesh processMesh(const aiMesh* mesh) {
	Mesh result;

	// Extract vertices
	for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
		// Position
		Vertex v;
		v.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

		// Normal (optional, if available)
		if (mesh->HasNormals()) {
			v.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
		}

		if (mesh->HasTextureCoords(0)) {
			v.texCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
		}
		else {
			// Add default texture coordinates if none exist
			v.texCoords = glm::vec2(0.0f);
		}

		if (mesh->mTangents) {
			v.tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
		}
		else {
			//compute later
			v.tangent = glm::vec3(0.0f);
		}
		result.vertices.push_back(v);
	}

	// Extract indices
	for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++) {
			result.indices.push_back(face.mIndices[j]);
		}
	}

	// Create OpenGL buffers
	glGenVertexArrays(1, &result.VAO);
	glGenBuffers(1, &result.VBO);
	glGenBuffers(1, &result.EBO);

	glBindVertexArray(result.VAO);

	// Vertex buffer
	glBindBuffer(GL_ARRAY_BUFFER, result.VBO);
	glBufferData(GL_ARRAY_BUFFER, result.vertices.size() * sizeof(Vertex), result.vertices.data(), GL_STATIC_DRAW);

	// Element buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, result.EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, result.indices.size() * sizeof(unsigned int), result.indices.data(), GL_STATIC_DRAW);

	// Vertex attributes (position, normal, texture, tangent)
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0); // Position
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float))); // Normal
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float))); //Texture
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float))); //Tangent


	glBindVertexArray(0);

	return result;
}


std::vector <Mesh> load_meshes(const char* file_name) {

	std::vector<Mesh> meshes;

	/* Use assimp to read the model file, forcing it to be read as    */
	/* triangles. The second flag (aiProcess_PreTransformVertices) is */
	/* relevant if there are multiple meshes in the model file that   */
	/* are offset from the origin. This is pre-transform them so      */
	/* they're in the right position.                                 */
	const aiScene* scene = aiImportFile(
		file_name,
		aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace
	);

	if (!scene) {
		fprintf(stderr, "ERROR: reading mesh %s\n", file_name);
		return meshes;
	}

	printf("  %i materials\n", scene->mNumMaterials);
	printf("  %i meshes\n", scene->mNumMeshes);
	printf("  %i textures\n", scene->mNumTextures);


	for (unsigned int m_i = 0; m_i < scene->mNumMeshes; m_i++) {
		const aiMesh* mesh = scene->mMeshes[m_i];
		printf("    %i vertices in mesh\n", mesh->mNumVertices);
		meshes.push_back(processMesh(mesh));

	}

	aiReleaseImport(scene);
	return meshes;
}
