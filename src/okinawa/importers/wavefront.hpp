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

  /**
   * @brief Read a file's geometry WITH its texture coordinates.
   *
   * The same as `parseGeometry` and for the same reasons, but giving
   * back what a textured mesh needs: five floats a vertex, position
   * then texture coordinate, in the layout `OkItem` and
   * `OkInstancedItem` take. Use this when the model is to be drawn
   * rather than measured.
   *
   * @param filename the Wavefront file to read.
   * @param vertices filled with x, y, z, u, v per vertex.
   * @param indices  filled with one index per corner.
   * @return false when the file cannot be read or holds no faces.
   */
  static bool parseMesh(const std::string         &filename,
                        std::vector<float>        &vertices,
                        std::vector<unsigned int> &indices);

  /**
   * @brief Read a file's geometry with its texture coordinates AND the
   *        normals the file itself carries.
   *
   * Eight floats a vertex -- x, y, z, u, v, nx, ny, nz -- which is the
   * layout `OkItem` keeps internally, so an item built from this is
   * drawn with the normals the model was authored with.
   *
   * That matters more than it sounds. Without normals of its own a mesh
   * is lit from the WINDING of its triangles, which is a second thing
   * the exporter has to get right and which nothing complains about
   * when it is wrong: a model whose faces are wound inward is not
   * invisible, it is lit from the wrong side -- undersides bright,
   * everything facing the sky dark.
   *
   * @param filename the Wavefront file to read.
   * @param vertices filled with x, y, z, u, v, nx, ny, nz per vertex.
   * @param indices  filled with one index per corner.
   * @return false when the file cannot be read, holds no faces, or has
   *         no `vn` lines at all -- in which case there is nothing here
   *         that `parseMesh` does not give, and the item will work the
   *         normals out from the winding.
   */
  static bool parseMeshWithNormals(const std::string         &filename,
                                   std::vector<float>        &vertices,
                                   std::vector<unsigned int> &indices);

private:
  /**
   * @brief Temporary vertex structure to hold position and texture coordinates.
   */
  struct TempVertex {
    std::array<float, 3> position;
    std::array<float, 2> texcoord;
    std::array<float, 3> normal;
  };

  /**
   * @brief Temporary mesh structure to hold raw positions, texture coordinates,
   *       combined vertices, and indices.
   */
  struct TempMesh {
    std::vector<float>        positions;  // Raw positions from file
    std::vector<float>        texcoords;  // Raw texture coordinates
    std::vector<float>        normals;    // Raw normals, when the file has any
    std::vector<TempVertex>   vertices;   // Final combined vertices
    std::vector<unsigned int> indices;
    bool                      hasNormals = false;
  };

  static bool hasTextureCoordinates(const std::string &filename);

  /**
   * @brief One corner of a face: `v`, `v/vt`, `v//vn` or `v/vt/vn`.
   *
   * All four forms, and negative indices, which the format allows and
   * which count backwards from the end of what has been read so far.
   * Written as one place because the two readers below disagreed about
   * it: one took the vertex and dropped the rest, the other insisted on
   * a slash and threw an uncaught exception on `v//vn` -- a valid file
   * that crashed the application rather than failing to load.
   *
   * @param nPositions how many positions have been read, for resolving a
   *        negative index. Same for the other two counts.
   * @return false when the token holds no vertex index at all. The
   *         outputs are -1 where the corner does not name one.
   */
  static bool parseCorner(const std::string &token, long nPositions,
                          long nTexcoords, long nNormals, long *outPosition,
                          long *outTexcoord, long *outNormal);
  static bool parseGeometryWithUV(const std::string &filename, TempMesh &mesh);
  static std::string getItemName(const std::string &filename);
};

#endif
