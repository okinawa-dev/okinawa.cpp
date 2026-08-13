#ifndef OK_MCP_SERVER_HPP
#define OK_MCP_SERVER_HPP

/**
 * @brief In-engine MCP (Model Context Protocol) server.
 *
 * Lets an external agent connect to a running okinawa app over local HTTP
 * and drive it through MCP tools such as view_frame, which returns the
 * rendered frame as an image. The public surface deliberately exposes no HTTP
 * or JSON types: all of that lives behind the Impl pimpl in the .cpp, so
 * consumers do not inherit those dependencies.
 *
 * Compiled only when OKINAWA_WITH_MCP is defined (xmake option "mcp").
 * Threading: the HTTP server runs on its own thread; tool calls that touch
 * OpenGL are marshalled onto the engine loop thread via drainCommands(),
 * which must be called once per frame from OkCore::loop.
 */
class OkMcpServer {
public:
  explicit OkMcpServer(int port);
  ~OkMcpServer();

  // Non-copyable (owns a server thread and a command queue).
  OkMcpServer(const OkMcpServer &)            = delete;
  OkMcpServer &operator=(const OkMcpServer &) = delete;

  // Start the HTTP server thread (binds 127.0.0.1:port).
  void start();
  // Stop the server and join its thread.
  void stop();

  // Run any queued tool commands on the calling thread. MUST be called from
  // the engine loop thread (the one holding the OpenGL context), once per
  // frame, after the scene is rendered and before the buffers are swapped.
  void drainCommands();

private:
  struct Impl;
  Impl *_impl;
};

#endif  // OK_MCP_SERVER_HPP
