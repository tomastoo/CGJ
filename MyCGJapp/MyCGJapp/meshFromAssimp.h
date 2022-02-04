bool Import3DFromFile(const std::string& pFile, const aiScene** scene, Assimp::Importer &importer);
std::vector<struct MyMesh> createMeshFromAssimp(const aiScene* sc, char model_dir[]);