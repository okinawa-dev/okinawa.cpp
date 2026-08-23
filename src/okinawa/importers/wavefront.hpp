#ifndef OK_WAVEFRONT_HPP
#define OK_WAVEFRONT_HPP

#include "../item/item.hpp"
#include <array>
#include <string>
#include <vector>

/**
 * @brief Class for importing Wavefront OBJ files.
 *        It handles the parsing of geometry and texture coordinates.
 */
class OkWavefrontImporter {
public:
  static OkItem *importFile(const std::string &filename);

  /**
   * @brief Read a file's geometry without building anything from it.
   *
   * `importFile` gives a finished item, which is what an application
   * usually wants and is also a GPU object it did not ask for. A tool
   * that wants to measure a model, show it in a panel it draws itself,
   * or hand its triangles to something else needs the numbers and
   * nothing more.
   *
   * @param filename the Wavefront file to read.
   * @param vertices filled with three floats per vertex: x, y, z.
   * @param indices  filled with one index per corner, three to a face.
   * @return false when the file cannot be read or holds no faces.
   */
  static bool parseGeometry(const std::string         &filename,
                            std::vector<float>        &vertices,
                            std::vector<unsigned int> &indices);

private:
  /**
   * @brief Temporary vertex structure to hold position and texture coordinates.
   */
  struct TempVertex {
    std::array<float, 3> position;
    std::array<float, 2> texcoord;
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
  static bool parseGeometryWithUV(const std::string &filename, TempMesh &mesh);
  static std::string getItemName(const std::string &filename);
};

#endif
