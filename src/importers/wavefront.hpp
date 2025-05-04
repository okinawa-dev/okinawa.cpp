#ifndef OK_WAVEFRONT_HPP
#define OK_WAVEFRONT_HPP

#include "../item/item.hpp"
#include <string>
#include <vector>

/**
 * @brief Class for importing Wavefront OBJ files.
 *        It handles the parsing of geometry and texture coordinates.
 */
class OkWavefrontImporter {
public:
  static OkItem *importFile(const std::string &filename);

private:
  /**
   * @brief Temporary vertex structure to hold position and texture coordinates.
   */
  struct TempVertex {
    float position[3];
    float texcoord[2];
  };

  /**
   * @brief Temporary mesh structure to hold raw positions, texture coordinates,
   *       combined vertices, and indices.
   */
  struct TempMesh {
    std::vector<float>        positions;  // Raw positions from file
    std::vector<float>        texcoords;  // Raw texture coordinates
    std::vector<TempVertex>   vertices;   // Final combined vertices
    std::vector<unsigned int> indices;
  };

  static bool hasTextureCoordinates(const std::string &filename);
  static bool parseGeometry(const std::string         &filename,
                            std::vector<float>        &vertices,
                            std::vector<unsigned int> &indices);
  static bool parseGeometryWithUV(const std::string &filename, TempMesh &mesh);
  static std::string getItemName(const std::string &filename);
};

#endif
