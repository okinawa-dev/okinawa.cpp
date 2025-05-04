#include "wavefront.hpp"
#include "../utils/logger.hpp"
#include <fstream>
#include <sstream>

/**
 * @brief Method to check if the Wavefront file has texture coordinates.
 *       It reads the file line by line and looks for "vt" lines.
 * @param filename The name of the Wavefront file.
 * @return True if texture coordinates are found, false otherwise.
 */
bool OkWavefrontImporter::hasTextureCoordinates(const std::string &filename) {
  std::ifstream file(filename);
  if (!file.is_open())
    return false;

  std::string line;
  while (std::getline(file, line)) {
    if (line.substr(0, 3) == "vt ") {
      return true;
    }
  }
  return false;
}

/**
 * @brief Method to parse the geometry from the Wavefront file.
 *        It reads vertices and faces, and triangulates the faces.
 * @param filename The name of the Wavefront file.
 * @param vertices The vector to store the vertex positions.
 * @param indices  The vector to store the indices of the vertices.
 * @return True if parsing was successful, false otherwise.
 */
bool OkWavefrontImporter::parseGeometry(const std::string         &filename,
                                        std::vector<float>        &vertices,
                                        std::vector<unsigned int> &indices) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    OkLogger::error("Wavefront :: Error opening file: " + filename);
    return false;
  }

  std::string line;
  while (std::getline(file, line)) {
    std::istringstream iss(line);
    std::string        type;
    iss >> type;

    if (type == "v") {
      float x, y, z;
      if (iss >> x >> y >> z) {
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(z);
      }
    } else if (type == "f") {
      std::vector<int> face;
      int              idx;
      while (iss >> idx) {
        face.push_back(idx - 1);  // OBJ indices start at 1
      }

      // Triangulate face
      for (size_t i = 2; i < face.size(); ++i) {
        indices.push_back(face[0]);
        indices.push_back(face[i - 1]);
        indices.push_back(face[i]);
      }
    }
  }

  return true;
}

/**
 * @brief Method to parse the geometry with texture coordinates from the
 * Wavefront file. It reads vertices, texture coordinates, and faces, and
 * triangulates the faces.
 * @param filename The name of the Wavefront file.
 * @param mesh     The TempMesh structure to store the parsed data.
 * @return True if parsing was successful, false otherwise.
 */
bool OkWavefrontImporter::parseGeometryWithUV(const std::string &filename,
                                              TempMesh          &mesh) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    OkLogger::error("Wavefront :: Error opening file: " + filename);
    return false;
  }

  std::string line;
  // First pass: read vertices and texture coordinates
  while (std::getline(file, line)) {
    std::istringstream iss(line);
    std::string        type;
    iss >> type;

    if (type == "v") {
      float x, y, z;
      if (iss >> x >> y >> z) {
        mesh.positions.push_back(x);
        mesh.positions.push_back(y);
        mesh.positions.push_back(z);
      }
    } else if (type == "vt") {
      float u, v;
      if (iss >> u >> v) {
        mesh.texcoords.push_back(u);
        mesh.texcoords.push_back(v);
      }
    }
  }

  // Second pass: read faces
  file.clear();
  file.seekg(0);

  while (std::getline(file, line)) {
    if (line[0] == 'f') {
      std::istringstream               iss(line.substr(2));
      std::vector<std::pair<int, int>> face;

      std::string vertex;
      while (iss >> vertex) {
        size_t slash = vertex.find('/');
        if (slash != std::string::npos) {
          int v = std::stoi(vertex.substr(0, slash)) - 1;
          int t = std::stoi(vertex.substr(slash + 1)) - 1;
          face.emplace_back(v, t);
        }
      }

      // Triangulate face
      for (size_t i = 2; i < face.size(); ++i) {
        for (size_t j = 0; j < 3; ++j) {
          size_t idx = (j == 0) ? 0 : (j == 1) ? i - 1 : i;
          int    v   = face[idx].first;   // vertex index
          int    t   = face[idx].second;  // texture index

          TempVertex vertex;
          std::copy_n(&mesh.positions[v * 3], 3, vertex.position);
          std::copy_n(&mesh.texcoords[t * 2], 2, vertex.texcoord);

          mesh.vertices.push_back(vertex);
          mesh.indices.push_back(mesh.vertices.size() - 1);
        }
      }
    }
  }

  return true;
}

/**
 * @brief Method to extract the item name from the filename.
 *        It removes the path and file extension.
 * @param filename The name of the Wavefront file.
 * @return The item name without path and extension.
 */
std::string OkWavefrontImporter::getItemName(const std::string &filename) {
  size_t      lastSlash = filename.find_last_of("/\\");
  std::string baseName  = (lastSlash == std::string::npos)
                              ? filename
                              : filename.substr(lastSlash + 1);
  size_t      dot       = baseName.find_last_of('.');
  return (dot == std::string::npos) ? baseName : baseName.substr(0, dot);
}

/**
 * @brief Method to import a Wavefront file and create an OkItem.
 *        It checks for texture coordinates and parses the geometry accordingly.
 * @param filename The name of the Wavefront file.
 * @return A pointer to the created OkItem, or nullptr on failure.
 */
OkItem *OkWavefrontImporter::importFile(const std::string &filename) {
  bool hasUV = hasTextureCoordinates(filename);
  OkLogger::info("Wavefront :: File " + filename +
                 (hasUV ? " has" : " does not have") + " texture coordinates");

  if (!hasUV) {
    std::vector<float>        vertices;
    std::vector<unsigned int> indices;

    if (!parseGeometry(filename, vertices, indices)) {
      OkLogger::error("Wavefront :: Failed to parse geometry from " + filename);
      return nullptr;
    }

    return new OkItem(getItemName(filename), vertices.data(), vertices.size(),
                      indices.data(), indices.size());
  } else {
    TempMesh mesh;
    if (!parseGeometryWithUV(filename, mesh)) {
      OkLogger::error("Wavefront :: Failed to parse geometry with UV from " +
                      filename);
      return nullptr;
    }

    // Create combined vertex data (3 pos + 2 tex = 5 floats per vertex)
    std::vector<float> vertexData;
    vertexData.reserve(mesh.vertices.size() * 5);

    for (const auto &vertex : mesh.vertices) {
      vertexData.insert(vertexData.end(), std::begin(vertex.position),
                        std::end(vertex.position));
      vertexData.insert(vertexData.end(), std::begin(vertex.texcoord),
                        std::end(vertex.texcoord));
    }

    return new OkItem(getItemName(filename), vertexData.data(),
                      vertexData.size(), mesh.indices.data(),
                      mesh.indices.size());
  }
}
