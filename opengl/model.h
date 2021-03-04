#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h> 

#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/SceneCombiner.h>

#include "mesh.h"
#include "shader.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

unsigned int TextureFromFile(const char* path, const string& directory, bool gamma = false);

class Model
{
public:
    // model data 
    vector<Texture> textures_loaded;	// stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
    vector<Mesh>    meshes;
    string directory;
    aiScene* pscene;
    int wingCalibCoefLen;
    float* wingCalibCoefFront;
    float* wingCalibCoefBack;
    float wingCalibG;

    // constructor, expects a filepath to a 3D model.
    Model(string const& path, int len, float* coefFront, float* coefBack, float G): wingCalibCoefLen(len), wingCalibG(G)
    {
        pscene = new aiScene;
        wingCalibCoefFront = new float[len];
        wingCalibCoefBack = new float[len];
        for (int i = 0; i < len; i++) {
            wingCalibCoefFront[i] = coefFront[i];
            wingCalibCoefBack[i] = coefBack[i];
        }
        loadModel(path);
    }

    Model(string const& path)
    {
        pscene = new aiScene;
        loadModel(path);
    }

    void saveModel(string const& path = "./model/output.obj") {
        Assimp::Exporter exporter;
        exporter.Export(pscene, "obj", path);
    }

    // draws the model, and thus all its meshes
    void Draw(Shader& shader)
    {
        shader.use();
        for (unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);
    }

    // this function one only changes the data inside the self-defined class Model, data in aiScene is not changed.
    void wingTransform(float* coefficient, int length) {
        float x_mm, z_calib_mm, z_calib_inch;
        for (int i = 0; i < this->meshes.size(); i++) {
            for (int j = 0; j < this->meshes[i].vertices.size(); j++) {
                V3f* position = &(this->meshes[i].vertices[j].Position);
                x_mm = position->x() * 25.4;
                z_calib_mm = 0;
                for (int a = 0; a < length; a++) {
                    z_calib_mm = z_calib_mm * x_mm;
                    z_calib_mm = z_calib_mm + coefficient[a];
                }
                z_calib_inch = z_calib_mm /254;
                position->z() = position->z() + z_calib_inch;
            }
            this->meshes[i].setup();
        }
    }

    aiScene* combineModels(Model* model2, bool isOutput=false) {
        aiScene* output = NULL;
        aiScene* output1 = new aiScene();
        aiScene* output2 = new aiScene();
        Assimp::SceneCombiner::CopyScene(&output1, pscene);
        Assimp::SceneCombiner::CopyScene(&output2, model2->pscene);
        vector<aiScene*>input;
        input.push_back(output1);
        input.push_back(output2);
        Assimp::SceneCombiner::MergeScenes(&output, input);
        if (isOutput) {
            Assimp::Exporter exporter;
            exporter.Export(output, "obj", "./model/output.obj");
        }
        return output;
    }

private:
    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(string const& path)
    {
        // read file via ASSIMP
        Assimp::Importer importer;
        const aiScene* const_pscene = (importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace));
        pscene = importer.GetOrphanedScene();

        // check for errors
        if (!pscene || pscene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !pscene->mRootNode) // if is Not Zero
        {
            cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
            return;
        }
        // retrieve the directory path of the filepath
        directory = path.substr(0, path.find_last_of('/'));

        // process ASSIMP's root node recursively
        processNode(pscene->mRootNode, pscene);
    }

    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void processNode(aiNode* node, const aiScene* scene)
    {
        // process each mesh located at the current node
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            // the node object only contains indices to index the actual objects in the scene. 
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }

    }

    Mesh processMesh(aiMesh* mesh, const aiScene* scene)
    {
        // data to fill
        vector<Vertex> vertices;
        vector<unsigned int> indices;
        vector<Texture> textures;
        aiColor4D colors;

        // walk through each of the mesh's vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            Eigen::Vector3f vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder Vector3f first.
            // positions
            vector.x() = mesh->mVertices[i].x;
            vector.y() = mesh->mVertices[i].y;
            vector.z() = mesh->mVertices[i].z;
            if (wingCalibCoefLen) {
                vector = wingTransform(vector);
                mesh->mVertices[i].z = vector.z();
            }
            vertex.Position = vector;
            // normals
            if (mesh->HasNormals())
            {
                vector.x() = mesh->mNormals[i].x;
                vector.y() = mesh->mNormals[i].y;
                vector.z() = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }
            // texture coordinates
            if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
            {
                Eigen::Vector2f vec;
                // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
                // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
                vec.x() = mesh->mTextureCoords[0][i].x;
                vec.y() = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
                // tangent
                vector.x() = mesh->mTangents[i].x;
                vector.y() = mesh->mTangents[i].y;
                vector.z() = mesh->mTangents[i].z;
                vertex.Tangent = vector;
                // bitangent
                vector.x() = mesh->mBitangents[i].x;
                vector.y() = mesh->mBitangents[i].y;
                vector.z() = mesh->mBitangents[i].z;
                vertex.Bitangent = vector;
            }
            else
                vertex.TexCoords = Eigen::Vector2f::Zero();

            vertices.push_back(vertex);
        }
        // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            // retrieve all indices of the face and store them in the indices vector
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }
        // process materials
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        aiColor4D diffuse;
        if (AI_SUCCESS != material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse)) {
            // handle epic failure here
        }
        // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
        // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER. 
        // Same applies to other texture as the following list summarizes:
        // diffuse: texture_diffuseN
        // specular: texture_specularN
        // normal: texture_normalN

        // 1. diffuse maps
        vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        // 2. specular maps
        vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        // 3. normal maps
        std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        // 4. height maps
        std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

        // return a mesh object created from the extracted mesh data
        return Mesh(vertices, indices, textures, diffuse);
    }

    Eigen::Vector3f wingTransform(Eigen::Vector3f vec) {
        float x_mm, z_calib_mm_front, z_calib_mm_back, z_calib_mm_total, z_calib_inch, y_front, y_back;
        x_mm = vec.x() * 25.4;
        z_calib_mm_front = calculatePolynomial(wingCalibCoefFront, wingCalibCoefLen, x_mm);
        z_calib_mm_back = calculatePolynomial(wingCalibCoefBack, wingCalibCoefLen, x_mm);
        float linePoseFront[] = { -0.5192, 243.9 };
        float linePoseBack[] = { -2.907e-16, 6.746e-16, 1.491e-10, -4.031e-10, -0.000323, 3.6e-05, -31.02 };
        y_front = calculatePolynomial(linePoseFront, 2, (vec.x() > 0 ? vec.x() : -vec.x()));
        y_back = calculatePolynomial(linePoseBack, 7, vec.x());
        float ratio = (y_front - vec.y()) / (y_front - y_back);
        z_calib_mm_total = z_calib_mm_front * ratio + z_calib_mm_back * (1 - ratio);
        z_calib_inch = z_calib_mm_total / 25.4;
        vec.z() = vec.z() + z_calib_inch * (wingCalibG / 2.5);
        return vec;
    }

    float calculatePolynomial(float* coef, int len, float x) {
        float output = 0;
        for (int a = 0; a < len; a++) {
            output = output * x;
            output = output + coef[a];
        }
        return output;
    }

    // checks all material textures of a given type and loads the textures if they're not loaded yet.
    // the required info is returned as a Texture struct.
    vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName)
    {
        vector<Texture> textures;
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
            bool skip = false;
            for (unsigned int j = 0; j < textures_loaded.size(); j++)
            {
                if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
                    break;
                }
            }
            if (!skip)
            {   // if texture hasn't been loaded already, load it
                Texture texture;
                texture.id = TextureFromFile(str.C_Str(), this->directory);
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
            }
        }
        return textures;
    }
};


inline unsigned int TextureFromFile(const char* path, const string& directory, bool gamma)
{
    string filename = string(path);
    filename = directory + '/' + filename;

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;
        else return 0;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
#endif