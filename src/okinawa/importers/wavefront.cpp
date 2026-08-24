#include "wavefront.hpp"
#include "../utils/logger.hpp"
#include "item/item.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib>

// Position and texture coordinate: what a textured vertex carries.
static const size_t kFloatsPerVertex = 5;

// With the normals the file carries as well: x, y, z, u, v, nx, ny, nz,
// which is the layout OkItem keeps internally.
static const size_t kFloatsPerVertexN = 8;

// Floats per position, texture coordinate and normal in the file.
static const size_t POSITION_FLOATS = 3;
static const size_t TEXCOORD_FLOATS = 2;
static const size_t NORMAL_FLOATS   = 3;
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {

  // What a corner names, when it names nothing.
  const long CORNER_NONE = -1;

  /**
   * @brief One index of a face corner, resolved against what is read.
   *
   * The format counts from 1, and counts BACKWARDS from the end when the
   * number is negative -- so -1 is the last vertex read so far, which is
   * how a file written piece by piece refers to its own most recent
   * geometry.
   */
  long resolveIndex(const std::string &text, long available) {
    if (text.empty()) {
      return CORNER_NONE;
    }
    long value = std::strtol(text.c_str(), nullptr, 10);
    if (value > 0) {
      value = value - 1;
    } else if (value < 0) {
      value = available + value;
    } else {
      return CORNER_NONE;  // 0 is not an index in this format
    }
    if (value < 0 || value >= available) {
      return CORNER_NONE;  // out of range: named, but not there
    }
    return value;
  }

}  // namespace

bool OkWavefrontImporter::parseCorner(const std::string &token, long nPositions,
                                      long nTexcoords, long nNormals,
                                      long *outPosition, long *outTexcoord,
                                      long *outNormal) {
  *outPosition = CORNER_NONE;
  *outTexcoord = CORNER_NONE;
  *outNormal   = CORNER_NONE;
  if (token.empty()) {
    return false;
  }
  // Split on the slashes rather than searching for them: the empty
  // middle field of `v//vn` is the case that used to be handed to
  // std::stoi, which throws where a loader has to shrug.
  std::array<std::string, 3> fields;
  size_t                     at = 0;
  std::istringstream         parts(token);
  std::string                field;
  while (at < 3 && std::getline(parts, field, '/')) {
    fields[at++] = field;
  }
  *outPosition = resolveIndex(fields[0], nPositions);
  if (*outPosition == CORNER_NONE) {
    return false;
  }
  *outTexcoord = resolveIndex(fields[1], nTexcoords);
  *outNormal   = resolveIndex(fields[2], nNormals);
  return true;
}

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
    OkLogger::error("Wavefront", "Error opening file: " + filename);
    return false;
  }

  std::string line;
  while (std::getline(file, line)) {
    std::istringstream iss(line);
    std::string        type;
    iss >> type;

    if (type == "v") {
      float x;
      float y;
      float z;
      if (iss >> x >> y >> z) {
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(z);
      }
    } else if (type == "f") {
      std::vector<long> face;
      std::string       corner;
      long nPositions = static_cast<long>(vertices.size() / POSITION_FLOATS);
      while (iss >> corner) {
        // A corner is `v`, `v/vt`, `v//vn` or `v/vt/vn`, and its index
        // may count backwards from the end. Read as a bare integer --
        // which is what this did -- the first slash stops the stream and
        // a triangle comes out with one corner, so a file written by
        // anything that exports texture coordinates loaded as no
        // geometry at all and said nothing about it.
        long position = 0;
        long texcoord = 0;
        long normal   = 0;
        if (parseCorner(corner, nPositions, nPositions, nPositions, &position,
                        &texcoord, &normal)) {
          face.push_back(position);
        }
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
bool OkWavefrontImporter::parseMesh(const std::string         &filename,
                                    std::vector<float>        &vertices,
                                    std::vector<unsigned int> &indices) {
  TempMesh mesh;
  if (!parseGeometryWithUV(filename, mesh) || mesh.indices.empty()) {
    return false;
  }
  vertices.clear();
  vertices.reserve(mesh.vertices.size() * kFloatsPerVertex);
  for (const auto &vertex : mesh.vertices) {
    vertices.insert(vertices.end(), std::begin(vertex.position),
                    std::end(vertex.position));
    vertices.insert(vertices.end(), std::begin(vertex.texcoord),
                    std::end(vertex.texcoord));
  }
  indices = mesh.indices;
  return true;
}

bool OkWavefrontImporter::parseMeshWithNormals(
    const std::string &filename, std::vector<float> &vertices,
    std::vector<unsigned int> &indices) {
  TempMesh mesh;
  if (!parseGeometryWithUV(filename, mesh) || mesh.indices.empty() ||
      !mesh.hasNormals) {
    return false;
  }
  vertices.clear();
  vertices.reserve(mesh.vertices.size() * kFloatsPerVertexN);
  for (const auto &vertex : mesh.vertices) {
    vertices.insert(vertices.end(), std::begin(vertex.position),
                    std::end(vertex.position));
    vertices.insert(vertices.end(), std::begin(vertex.texcoord),
                    std::end(vertex.texcoord));
    vertices.insert(vertices.end(), std::begin(vertex.normal),
                    std::end(vertex.normal));
  }
  indices = mesh.indices;
  return true;
}

bool OkWavefrontImporter::parseGeometryWithUV(const std::string &filename,
                                              TempMesh          &mesh) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    OkLogger::error("Wavefront", "Error opening file: " + filename);
    return false;
  }

  std::string line;
  // First pass: read vertices and texture coordinates
  while (std::getline(file, line)) {
    std::istringstream iss(line);
    std::string        type;
    iss >> type;

    if (type == "v") {
      float x;
      float y;
      float z;
      if (iss >> x >> y >> z) {
        mesh.positions.push_back(x);
        mesh.positions.push_back(y);
        mesh.positions.push_back(z);
      }
    } else if (type == "vt") {
      float u;
      float v;
      if (iss >> u >> v) {
        mesh.texcoords.push_back(u);
        mesh.texcoords.push_back(v);
      }
    } else if (type == "vn") {
      float nx;
      float ny;
      float nz;
      if (iss >> nx >> ny >> nz) {
        mesh.normals.push_back(nx);
        mesh.normals.push_back(ny);
        mesh.normals.push_back(nz);
        mesh.hasNormals = true;
      }
    }
  }

  // Second pass: read faces
  file.clear();
  file.seekg(0);

  long nPositions = static_cast<long>(mesh.positions.size() / POSITION_FLOATS);
  long nTexcoords = static_cast<long>(mesh.texcoords.size() / TEXCOORD_FLOATS);
  long nNormals   = static_cast<long>(mesh.normals.size() / NORMAL_FLOATS);

  while (std::getline(file, line)) {
    std::istringstream head(line);
    std::string        type;
    head >> type;
    if (type != "f") {
      continue;  // `f` and nothing else: not every line beginning with it
    }
    // A corner names a position and MAY name a texture coordinate and a
    // normal. Each is looked up only if the corner named it and the file
    // actually holds it -- an index past the end used to be read anyway,
    // off the end of the vector.
    std::vector<std::array<long, 3>> face;
    std::string                      corner;
    while (head >> corner) {
      std::array<long, 3> at = {CORNER_NONE, CORNER_NONE, CORNER_NONE};
      if (parseCorner(corner, nPositions, nTexcoords, nNormals, at.data(),
                      at.data() + 1, at.data() + 2)) {
        face.push_back(at);
      }
    }

    // Triangulate face
    for (size_t i = 2; i < face.size(); ++i) {
      std::array<size_t, 3> pick = {0, i - 1, i};
      for (size_t j = 0; j < 3; ++j) {
        const std::array<long, 3> &at = face[pick[j]];

        TempVertex vertex;
        vertex.position = {0.0f, 0.0f, 0.0f};
        vertex.texcoord = {0.0f, 0.0f};
        // A vertex with no normal of its own is left at zero rather than
        // pointed anywhere: the item computes normals from the winding
        // when the file gave none, and a made-up direction here would
        // quietly replace that with something worse.
        vertex.normal = {0.0f, 0.0f, 0.0f};
        std::copy_n(
            &mesh.positions[static_cast<size_t>(at[0]) * POSITION_FLOATS],
            POSITION_FLOATS, vertex.position.begin());
        if (at[1] != CORNER_NONE) {
          std::copy_n(
              &mesh.texcoords[static_cast<size_t>(at[1]) * TEXCOORD_FLOATS],
              TEXCOORD_FLOATS, vertex.texcoord.begin());
        }
        if (at[2] != CORNER_NONE) {
          std::copy_n(mesh.normals.data() +
                          static_cast<size_t>(at[2]) * NORMAL_FLOATS,
                      NORMAL_FLOATS, vertex.normal.begin());
        }

        mesh.vertices.push_back(vertex);
        mesh.indices.push_back(
            static_cast<unsigned int>(mesh.vertices.size() - 1));
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
 */// Interleaved vertex layout: three position floats then two texture
// coordinates.

OkItem *OkWavefrontImporter::importFile(const std::string &filename) {
  bool hasUV = hasTextureCoordinates(filename);
  OkLogger::info("Wavefront", "File " + filename +
                                  (hasUV ? " has" : " does not have") +
                                  " texture coordinates");

  if (!hasUV) {
    std::vector<float>        vertices;
    std::vector<unsigned int> indices;

    if (!parseGeometry(filename, vertices, indices)) {
      OkLogger::error("Wavefront", "Failed to parse geometry from " + filename);
      return nullptr;
    }

    return new OkItem(getItemName(filename), vertices.data(),
                      static_cast<int>(vertices.size()), indices.data(),
                      static_cast<int>(indices.size()));
  }
  // The normals the file carries, when it carries any: a model authored
  // with them is lit the way its author meant, and not by the winding of
  // its triangles -- which is a different thing that happens to agree
  // most of the time.
  {
    std::vector<float>        withNormals;
    std::vector<unsigned int> normalIndices;
    if (parseMeshWithNormals(filename, withNormals, normalIndices)) {
      return new OkItem(getItemName(filename), withNormals.data(),
                        static_cast<long>(withNormals.size()),
                        normalIndices.data(),
                        static_cast<long>(normalIndices.size()),
                        static_cast<int>(kFloatsPerVertexN));
    }
  }

  // else {
  TempMesh mesh;
  if (!parseGeometryWithUV(filename, mesh)) {
    OkLogger::error("Wavefront",
                    "Failed to parse geometry with UV from " + filename);
    return nullptr;
  }

  // Create combined vertex data (3 pos + 2 tex = 5 floats per vertex)
  std::vector<float> vertexData;
  vertexData.reserve(mesh.vertices.size() * kFloatsPerVertex);

  for (const auto &vertex : mesh.vertices) {
    vertexData.insert(vertexData.end(), std::begin(vertex.position),
                      std::end(vertex.position));
    vertexData.insert(vertexData.end(), std::begin(vertex.texcoord),
                      std::end(vertex.texcoord));
  }

  return new OkItem(getItemName(filename), vertexData.data(),
                    static_cast<int>(vertexData.size()), mesh.indices.data(),
                    static_cast<int>(mesh.indices.size()));
}
